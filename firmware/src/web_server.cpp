#include "web_server.h"

ENUM_LUT_FULL_IMPL(web_server_error, _EVALS_WEB_SERVER_ERROR);

static AsyncWebServer wsrv(WEB_SERVER_PORT);
static scheduler_t *sched;
static valve_control_t *valvectl;

/*
============================================================================
                               Success routines                               
============================================================================
*/

INLINED static void web_server_append_cors_headers(AsyncWebServerResponse *resp)
{
  // Allow CORS requests
  resp->addHeader("Access-Control-Allow-Origin", "*");
  resp->addHeader("Access-Control-Max-Age", "600");
  resp->addHeader("Access-Control-Allow-Methods", "PUT,POST,GET,DELETE,OPTIONS");
  resp->addHeader("Access-Control-Allow-Headers", "*");
}

INLINED static void web_server_empty_ok(AsyncWebServerRequest *request)
{
  AsyncWebServerResponse *resp = request->beginResponse(204);
  web_server_append_cors_headers(resp);
  request->send(resp);
}

INLINED static void web_server_json_resp(AsyncWebServerRequest *request, int status, htable_t *json)
{
  scptr char *stringified = jsonh_stringify(json, 2);

  AsyncWebServerResponse *resp = request->beginResponse(status, "application/json", stringified);
  web_server_append_cors_headers(resp);

  request->send(resp);
}

/*
============================================================================
                               Error routines                               
============================================================================
*/

static void web_server_error_resp(AsyncWebServerRequest *request, int status, web_server_error_t code, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);

  scptr char *error_msg = vstrfmt_direct(fmt, ap);

  va_end(ap);

  scptr htable_t *resp = jsonh_make();

  scptr char *error_code_name = strclone(web_server_error_name(code));

  jsonh_set_bool(resp, "error", true);
  jsonh_set_str(resp, "code", error_code_name);
  jsonh_set_str(resp, "message", (char *) mman_ref(error_msg));

  web_server_json_resp(request, status, resp);
}

/*
============================================================================
                              Not Found Routes                              
============================================================================
*/

void web_server_route_not_found(AsyncWebServerRequest *request)
{
  // API route requested, show 404 json errors
  if (request->url().startsWith("/api"))
  {
    web_server_error_resp(
      request,
      404, RESOURCE_NOT_FOUND,
      "The requested resource was not found (%s " QUOTSTR ")!",
      request->methodToString(),
      request->url().c_str()
    );
    return;
  }

  // File requested, fall back to index.html (used for Angular)
  File index = SD.open("/index.html");
  request->send(200, "text/html", index.readString());
  index.close();
}

/*
============================================================================
                                Body Handling                               
============================================================================
*/

void web_server_str_body_handler(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
{
  web_server_request_body_t *body = (web_server_request_body_t *) request->_tempObject;

  // Create buffer on the first segment of the message
  if(index == 0)
  {
    // Allocate using malloc since the API will automatically call free after the request's lifetime
    request->_tempObject = malloc(sizeof(web_server_request_body_t));
    body = (web_server_request_body_t *) request->_tempObject;

    // Not enough space available
    if (!body)
      return;

    // Allocate as much memory as the whole body will require
    body->content = (uint8_t *) mman_alloc(sizeof(uint8_t), total, NULL);
    request->_tempObject = body;
  }

  // Not enough space for the whole body
  if (!body->content)
    return;

  // Write the segment into the buffer
  memcpy(&body->content[index], data, len);
}

INLINED static bool web_server_ensure_str_body(AsyncWebServerRequest *request, char **output)
{
  // Body segment collector has been called at least once
  if (request->_tempObject)
  {
    web_server_request_body_t *body = (web_server_request_body_t *) request->_tempObject;

    // Check if we had enough space
    if (!body->content)
    {
      web_server_error_resp(request, 500, BODY_TOO_LONG, "Body too long, not enough space!");
      return false;
    }

    *output = (char *) body->content;
    return true;
  }

  // No body collected
  web_server_error_resp(request, 400, NO_CONTENT, "No body content provided!");
  return false;
}

INLINED static bool web_server_ensure_json_body(AsyncWebServerRequest *request, htable_t **output)
{
  // Get the string body
  scptr char *body = NULL;
  if (!web_server_ensure_str_body(request, &body))
    return false;

  // Check that the content-type actually matches
  if (request->contentType() != "application/json")
  {
    web_server_error_resp(request, 400, NOT_JSON, "This endpoint only accepts JSON bodies!");
    return false;
  }

  // Parse json
  scptr char *err = NULL;
  scptr htable_t *body_jsn = jsonh_parse(body, &err);
  if (!body_jsn)
  {
    web_server_error_resp(request, 400, INVALID_JSON, "Could not parse the JSON body: %s", err);
    return false;
  }

  // Write output
  *output = (htable_t *) mman_ref(body_jsn);
  return true;
}

/*
============================================================================
                              Common routines                               
============================================================================
*/

INLINED static bool valves_parse_id(AsyncWebServerRequest *request, size_t *valve_id)
{
  // Parse numeric identifier
  const char *id_str = request->pathArg(0).c_str();
  long valve_id_l;
  if (longp(&valve_id_l, id_str, 10) != LONGP_SUCCESS)
  {
    web_server_error_resp(request, 400, NON_NUM_ID, "Invalid non-numeric identifier (%s)!", id_str);
    return false;
  }

  // Check identifier validity
  if (valve_id_l < 0 || valve_id_l >= VALVE_CONTROL_NUM_VALVES)
  {
    web_server_error_resp(request, 400, OUT_OF_RANGE_ID, "Invalid out-of-range identifier (%s)!", id_str);
    return false;
  }

  *valve_id = (size_t) valve_id_l;
  return true;
}

INLINED static bool scheduler_parse_day(AsyncWebServerRequest *request, const char *day_str, scheduler_weekday_t *day)
{
  // Parse weekday from path arg
  if (scheduler_weekday_value(day_str, day) != ENUMLUT_SUCCESS)
  {
    web_server_error_resp(request, 400, INVALID_WEEKDAY, "Invalid weekday specified (%s)!", day_str);
    return false;
  }

  return true;
}

INLINED static bool web_server_route_scheduler_day_index_parse(
  AsyncWebServerRequest *request,
  scheduler_weekday_t *day,
  long *index
)
{
  // Parse weekday from path arg
  if (!scheduler_parse_day(request, request->pathArg(0).c_str(), day))
    return false;

  // Parse numeric index
  const char *index_str = request->pathArg(1).c_str();
  if (longp(index, index_str, 10) != LONGP_SUCCESS)
  {
    web_server_error_resp(request, 400, NON_NUM_ID, "Invalid non-numeric index (%s)!", index_str);
    return false;
  }

  // Check index validity
  if (*index < 0 || *index >= SCHEDULER_MAX_INTERVALS_PER_DAY)
  {
    web_server_error_resp(request, 400, OUT_OF_RANGE_ID, "Invalid out-of-range index (%s)!", index_str);
    return false;
  }

  return true;
}

/*
============================================================================
                               GET /scheduler                               
============================================================================
*/

void web_server_route_scheduler(AsyncWebServerRequest *request)
{
  scptr htable_t *jsn = jsonh_make();
  
  for (int i = 0; i < 7; i++)
  {
    scheduler_weekday_t day = (scheduler_weekday_t) i;
    const char *weekday_name = scheduler_weekday_name(day);
    scptr htable_t *weekday= scheduler_weekday_jsonify(sched, day);
    jsonh_set_obj(jsn, weekday_name, (htable_t *) mman_ref(weekday));
  }

  web_server_json_resp(request, 200, jsn);
}

/*
============================================================================
                            GET /scheduler/{day}                            
============================================================================
*/

void web_server_route_scheduler_day(AsyncWebServerRequest *request)
{
  // Parse weekday from path arg
  scheduler_weekday_t day;
  if (!scheduler_parse_day(request, request->pathArg(0).c_str(), &day))
    return;

  scptr htable_t *weekday_schedule = scheduler_weekday_jsonify(sched, day);
  web_server_json_resp(request, 200, weekday_schedule);
}

/*
============================================================================
                            PUT /scheduler/{day}                            
============================================================================
*/

void web_server_route_scheduler_day_edit(AsyncWebServerRequest *request)
{
  // Parse weekday from path arg
  scheduler_weekday_t day;
  if (!scheduler_parse_day(request, request->pathArg(0).c_str(), &day))
    return;

  scptr htable_t *body = NULL;
  if (!web_server_ensure_json_body(request, &body))
    return;

  // Parse day from json
  scptr char *err = NULL;
  scheduler_day_t sched_day;
  if (!scheduler_day_parse(body, &err, &sched_day))
  {
    web_server_error_resp(request, 400, BODY_MALFORMED, "Body data malformed: %s", err);
    return;
  }

  // Update the day and save it persistently
  scheduler_day_t *targ_day = &(sched->daily_schedules[day]);

  // Check for deltas and disabled state
  if (targ_day->disabled != sched_day.disabled)
  {
    targ_day->disabled = sched_day.disabled;

    scptr char *ev_args = strfmt_direct("%s", scheduler_weekday_name(day));
    web_socket_broadcast_event(sched_day.disabled ? WSE_DAY_DISABLE_ON : WSE_DAY_DISABLE_OFF, ev_args);
  }

  scheduler_eeprom_save(sched);

  // Respond with the updated day
  scptr htable_t *day_jsn = scheduler_weekday_jsonify(sched, day);
  web_server_json_resp(request, 200, day_jsn);
}

/*
============================================================================
                        GET /scheduler/{day}/{index}                        
============================================================================
*/

void web_server_route_scheduler_day_index(AsyncWebServerRequest *request)
{
  scheduler_weekday_t day;
  long index;

  if (!web_server_route_scheduler_day_index_parse(request, &day, &index))
    return;

  scptr htable_t *int_jsn = scheduler_interval_jsonify(index, sched->daily_schedules[day].intervals[index]);
  web_server_json_resp(request, 200, int_jsn);
}

/*
============================================================================
                        PUT /scheduler/{day}/{index}                        
============================================================================
*/

void web_server_route_scheduler_day_index_edit(AsyncWebServerRequest *request)
{
  scheduler_weekday_t day;
  long index;

  if (!web_server_route_scheduler_day_index_parse(request, &day, &index))
    return;

  scptr htable_t *body = NULL;
  if (!web_server_ensure_json_body(request, &body))
    return;

  // Parse interval from json
  scptr char *err = NULL;
  scheduler_interval_t interval;
  if (!scheduler_interval_parse(body, &err, &interval))
  {
    web_server_error_resp(request, 400, BODY_MALFORMED, "Body data malformed: %s", err);
    return;
  }

  // Update the entry and save it persistently
  scheduler_interval_t *targ_interval = &(sched->daily_schedules[day].intervals[index]);

  // Check for deltas and patch disabled
  if (targ_interval->disabled != interval.disabled)
  {
    targ_interval->disabled = interval.disabled;

    scptr char *ev_args = strfmt_direct("%s;%ld", scheduler_weekday_name(day), index);
    web_socket_broadcast_event(interval.disabled ? WSE_INTERVAL_DISABLE_ON : WSE_INTERVAL_DISABLE_OFF, ev_args);
  }

  // Check for deltas and patch end
  if (scheduler_time_compare(targ_interval->end, interval.end) != 0)
  {
    targ_interval->end = interval.end;

    scptr char *ev_args = strfmt_direct("%s;%ld;%s", scheduler_weekday_name(day), index, scheduler_time_stringify(&(interval.end)));
    web_socket_broadcast_event(WSE_INTERVAL_END_CHANGE, ev_args);
  }

  // Check for deltas and patch start
  if (scheduler_time_compare(targ_interval->start, interval.start) != 0)
  {
    targ_interval->start = interval.start;

    scptr char *ev_args = strfmt_direct("%s;%ld;%s", scheduler_weekday_name(day), index, scheduler_time_stringify(&(interval.start)));
    web_socket_broadcast_event(WSE_INTERVAL_START_CHANGE, ev_args);
  }

  // Check for deltas and patch identifier
  if (targ_interval->identifier != interval.identifier)
  {
    targ_interval->identifier = interval.identifier;
    scptr char *ev_args = strfmt_direct("%s;%ld;%u", scheduler_weekday_name(day), index, interval.identifier);
    web_socket_broadcast_event(WSE_INTERVAL_IDENTIFIER_CHANGE, ev_args);
  }

  scheduler_eeprom_save(sched);

  // Respond with the updated entry
  scptr htable_t *int_jsn = scheduler_interval_jsonify(index, *targ_interval);
  web_server_json_resp(request, 200, int_jsn);
}

/*
============================================================================
                      DELETE /scheduler/{day}/{index}                       
============================================================================
*/

void web_server_route_scheduler_day_index_delete(AsyncWebServerRequest *request)
{
  scheduler_weekday_t day;
  long index;

  if (!web_server_route_scheduler_day_index_parse(request, &day, &index))
    return;

  scheduler_interval_t *targ = &(sched->daily_schedules[day].intervals[index]);

  // Already an empty slot
  if (scheduler_interval_empty(*targ))
  {
    web_server_error_resp(request, 404, INDEX_EMPTY, "This index is already empty");
    return;
  }

  // Clear slot and save persistently
  *targ = SCHEDULER_INTERVAL_EMPTY;
  scheduler_eeprom_save(sched);

  scptr char *ev_args = strfmt_direct("%s;%ld", scheduler_weekday_name(day), index);
  web_socket_broadcast_event(WSE_INTERVAL_DELETED, ev_args);

  web_server_empty_ok(request);
}

/*
============================================================================
                                 GET /valves                                
============================================================================
*/

void web_server_route_valves(AsyncWebServerRequest *request)
{
  scptr htable_t *resp = jsonh_make();

  // Create a list of all available valves
  scptr dynarr_t *valves = dynarr_make(VALVE_CONTROL_NUM_VALVES, VALVE_CONTROL_NUM_VALVES, mman_dealloc_nr);
  for (size_t i = 0; i < VALVE_CONTROL_NUM_VALVES; i++)
  {
    scptr htable_t *valve = valve_control_valve_jsonify(valvectl, i);

    if (!valve)
      continue;

    if (jsonh_insert_arr_obj(valves, (htable_t *) mman_ref(valve)) != JOPRES_SUCCESS)
      mman_dealloc(valve);
  }

  jsonh_set_arr(resp, "items", valves);
  web_server_json_resp(request, 200, resp);
}

/*
============================================================================
                               PUT /valves/{id}                             
============================================================================
*/

void web_server_route_valves_edit(AsyncWebServerRequest *request)
{
  size_t valve_id = 0;
  if (!valves_parse_id(request, &valve_id))
    return;

  scptr htable_t *body = NULL;
  if (!web_server_ensure_json_body(request, &body))
    return;

  // Parse valve from json
  scptr char *err = NULL;
  valve_t valve;
  if (!valve_control_valve_parse(body, &err, &valve))
  {
    web_server_error_resp(request, 400, BODY_MALFORMED, "Body data malformed: %s", err);
    return;
  }

  // Check if that name is already in use, ignore casing
  for (size_t i = 0; i < VALVE_CONTROL_NUM_VALVES; i++)
  {
    // Skip self
    if (i == valve_id)
      continue;

    // Name collision
    if (strncasecmp(valvectl->valves[i].alias, valve.alias, VALVE_CONTROL_ALIAS_MAXLEN) == 0)
    {
      web_server_error_resp(request, 409, VALVE_ALIAS_DUP, "The alias " QUOTSTR " is already in use", valve.alias);
      return;
    }
  }

  // Get the target valve and patch it, then save
  valve_t *targ_valve = &(valvectl->valves[valve_id]);

  // Check for deltas and patch name
  if (strncmp(targ_valve->alias, valve.alias, VALVE_CONTROL_ALIAS_MAXLEN) != 0)
  {
    strncpy(targ_valve->alias, valve.alias, VALVE_CONTROL_ALIAS_MAXLEN);

    scptr char *ev_arg = strfmt_direct("%d;%s", valve_id, valve.alias);
    web_socket_broadcast_event(WSE_VALVE_RENAME, ev_arg);
  }

  // Check for deltas and patch disabled state
  if (targ_valve->disabled != valve.disabled)
  {
    targ_valve->disabled = valve.disabled;

    scptr char *ev_arg = strfmt_direct("%d", valve_id);
    web_socket_broadcast_event(valve.disabled ? WSE_VALVE_DISABLE_ON : WSE_VALVE_DISABLE_OFF, ev_arg);
  }

  valve_control_eeprom_save(valvectl);

  // Respond with the updated valve
  scptr htable_t *valve_jsn = valve_control_valve_jsonify(valvectl, valve_id);
  web_server_json_resp(request, 200, valve_jsn);
}

/*
============================================================================
                               POST /valves/{id}                            
============================================================================
*/

void web_server_route_valves_activate(AsyncWebServerRequest *request)
{
  size_t valve_id;
  if (!valves_parse_id(request, &valve_id))
    return;

  // Check the target valve
  if(valvectl->valves[valve_id].state)
  {
    web_server_error_resp(request, 409, VALVE_ALREADY_ACTIVE, "This valve is already active");
    return;
  }

  // Toggle valve on
  valve_control_toggle(valvectl, valve_id, true);
  web_server_empty_ok(request);
}

/*
============================================================================
                             DELETE /valves/{id}                            
============================================================================
*/

void web_server_route_valves_deactivate(AsyncWebServerRequest *request)
{
  size_t valve_id;
  if (!valves_parse_id(request, &valve_id))
    return;

  valve_t *targ_valve = &(valvectl->valves[valve_id]);

  // Check the target valve
  if (!targ_valve->state)
  {
    web_server_error_resp(request, 409, VALVE_NOT_ACTIVE, "This valve is not active");
    return;
  }

  // There's a timer active, forbid action
  if (scheduler_time_compare(targ_valve->timer, SCHEDULER_TIME_MIDNIGHT) != 0)
  {
    web_server_error_resp(request, 409, VALVE_TIMER_IN_CONTROL, "There is an active timer in control of this valve");
    return;
  }

  // Toggle valve off
  valve_control_toggle(valvectl, valve_id, false);
  web_server_empty_ok(request);
}

/*
============================================================================
                            POST /valves/{id}/timer                         
============================================================================
*/

void web_server_route_valves_timer_set(AsyncWebServerRequest *request)
{
  size_t valve_id = 0;
  if (!valves_parse_id(request, &valve_id))
    return;

  scptr htable_t *body = NULL;
  if (!web_server_ensure_json_body(request, &body))
    return;

  // Get timer value from json
  jsonh_opres_t jopr;
  char *timer_str = NULL;
  if ((jopr = jsonh_get_str(body, "duration", &timer_str)) != JOPRES_SUCCESS)
  {
    scptr char *err = jsonh_getter_errstr("duration", jopr);
    web_server_error_resp(request, 400, BODY_MALFORMED, "Body data malformed: %s", err);
    return;
  }

  // Parse timer string
  scptr char *err = NULL;
  scheduler_time_t timer;
  if (!scheduler_time_parse(timer_str, &err, &timer))
  {
    web_server_error_resp(request, 400, BODY_MALFORMED, "Body data malformed: %s", err);
    return;
  }

  // Ensure the timer is not empty
  if (scheduler_time_compare(timer, SCHEDULER_TIME_MIDNIGHT) == 0)
  {
    web_server_error_resp(request, 400, VALVE_TIMER_ZERO, "Body data malformed: \"duration\" cannot be zero");
    return;
  }

  valve_t *targ_valve = &(valvectl->valves[valve_id]);

  // Check the target valve
  if(scheduler_time_compare(targ_valve->timer, SCHEDULER_TIME_MIDNIGHT) != 0)
  {
    web_server_error_resp(request, 409, VALVE_TIMER_NOT_ACTIVE, "This valve already has an active timer");
    return;
  }

  // Set timer and turn on valve
  targ_valve->timer = timer;
  targ_valve->has_timer = true;
  valve_control_toggle(valvectl, valve_id, true);

  scptr char *timer_strval = scheduler_time_stringify(&timer);
  scptr char *ev_args_timer = strfmt_direct("%lu;%s", valve_id, timer_strval);
  web_socket_broadcast_event(WSE_VALVE_TIMER_UPDATED, ev_args_timer);

  scptr char *ev_args_off = strfmt_direct("%lu", valve_id);
  web_socket_broadcast_event(WSE_VALVE_ON, ev_args_off);

  web_server_empty_ok(request);
}

/*
============================================================================
                          DELETE /valves/{id}/timer                         
============================================================================
*/

void web_server_route_valves_timer_clear(AsyncWebServerRequest *request)
{
  size_t valve_id;
  if (!valves_parse_id(request, &valve_id))
    return;

  valve_t *targ_valve = &(valvectl->valves[valve_id]);

  // Check the target valve
  if(scheduler_time_compare(targ_valve->timer, SCHEDULER_TIME_MIDNIGHT) == 0)
  {
    web_server_error_resp(request, 409, VALVE_TIMER_NOT_ACTIVE, "This valve has no active timer");
    return;
  }

  // Clear timer and turn off valve
  targ_valve->timer = SCHEDULER_TIME_MIDNIGHT;
  targ_valve->has_timer = false;
  valve_control_toggle(valvectl, valve_id, false);

  scptr char *timer_strval = scheduler_time_stringify(&SCHEDULER_TIME_MIDNIGHT);
  scptr char *ev_args = strfmt_direct("%lu;%s", valve_id, timer_strval);
  web_socket_broadcast_event(WSE_VALVE_TIMER_UPDATED, ev_args);
  web_socket_broadcast_event(WSE_VALVE_OFF, ev_args);

  web_server_empty_ok(request);
}

/*
============================================================================
                                GET /memstat                                
============================================================================
*/

void web_server_route_memstat(AsyncWebServerRequest *request)
{
  size_t mac = mman_get_alloc_count(), mdeac = mman_get_dealloc_count();
  scptr char *resp = strfmt_direct("%lu %lu %lu\n", esp_get_free_heap_size(), mac, mdeac);
  request->send(200, "text/plain", resp);
}

/*
============================================================================
                                OPTIONS ...                                 
============================================================================
*/

void web_server_route_any_options(AsyncWebServerRequest *request)
{
  AsyncWebServerResponse *resp = request->beginResponse(204);
  web_server_append_cors_headers(resp);
  request->send(resp);
}

/*
============================================================================
                          Webserver Configuration                           
============================================================================
*/

void web_server_init(scheduler_t *scheduler, valve_control_t *valve_control)
{
  // /memstat, Memory statistics for debugging purposes
  wsrv.on("/api/memstat", HTTP_GET, web_server_route_memstat);
  wsrv.on("/api/memstat", HTTP_OPTIONS, web_server_route_any_options);

  // /scheduler
  const char *p_sched = "^\\/api\\/scheduler$";
  wsrv.on(p_sched, HTTP_GET, web_server_route_scheduler);
  wsrv.on(p_sched, HTTP_OPTIONS, web_server_route_any_options);

  // /scheduler/{day}
  const char *p_sched_day = "^\\/api\\/scheduler\\/([A-Za-z0-9_]+)$";
  wsrv.on(p_sched_day, HTTP_GET, web_server_route_scheduler_day);
  wsrv.on(p_sched_day, HTTP_PUT, web_server_route_scheduler_day_edit, NULL, web_server_str_body_handler);
  wsrv.on(p_sched_day, HTTP_OPTIONS, web_server_route_any_options);

  // /scheduler/{day}/index
  const char *p_sched_day_index = "^\\/api\\/scheduler\\/([A-Za-z0-9_]+)\\/([0-9]+)$";
  wsrv.on(p_sched_day_index, HTTP_GET, web_server_route_scheduler_day_index);
  wsrv.on(p_sched_day_index, HTTP_PUT, web_server_route_scheduler_day_index_edit, NULL, web_server_str_body_handler);
  wsrv.on(p_sched_day_index, HTTP_DELETE, web_server_route_scheduler_day_index_delete);
  wsrv.on(p_sched_day_index, HTTP_OPTIONS, web_server_route_any_options);

  // /valves
  const char *p_valves = "^\\/api\\/valves$";
  wsrv.on(p_valves, HTTP_GET, web_server_route_valves);
  wsrv.on(p_valves, HTTP_OPTIONS, web_server_route_any_options);

  // /valves/{id}
  const char *p_valves_id = "^\\/api\\/valves\\/([0-9]+)$";
  wsrv.on(p_valves_id, HTTP_PUT, web_server_route_valves_edit, NULL, web_server_str_body_handler);
  wsrv.on(p_valves_id, HTTP_POST, web_server_route_valves_activate);
  wsrv.on(p_valves_id, HTTP_DELETE, web_server_route_valves_deactivate);
  wsrv.on(p_valves_id, HTTP_OPTIONS, web_server_route_any_options);

  // /valves/id/timer
  const char *p_valves_id_timer = "^\\/api\\/valves\\/([0-9]+)\\/timer$";
  wsrv.on(p_valves_id_timer, HTTP_POST, web_server_route_valves_timer_set, NULL, web_server_str_body_handler);
  wsrv.on(p_valves_id_timer, HTTP_DELETE, web_server_route_valves_timer_clear);
  wsrv.on(p_valves_id_timer, HTTP_OPTIONS, web_server_route_any_options);

  // Serve static files from SD, using index.html as a default file for / requests
  wsrv.serveStatic("/", SD, WEB_SERVER_SD_ROOT).setDefaultFile("index.html");

  // All remaining paths
  wsrv.onNotFound(web_server_route_not_found);

  // Set dependency pointers
  sched = scheduler;
  valvectl = valve_control;

  // Initialize the websocket
  web_socket_init(&wsrv);

  // Start listening
  wsrv.begin();
}