#include "web_server.h"

static AsyncWebServer wsrv(WEB_SERVER_PORT);
static scheduler_t *sched;

/*
============================================================================
                               Success routines                               
============================================================================
*/

INLINED static void web_server_empty_ok(AsyncWebServerRequest *request)
{
  request->send(204);
}

INLINED static void web_server_json_resp(AsyncWebServerRequest *request, int status, htable_t *json)
{
  scptr char *stringified = jsonh_stringify(json, 2);
  request->send(status, "application/json", stringified);
}

/*
============================================================================
                               Error routines                               
============================================================================
*/

static void web_server_error_resp(AsyncWebServerRequest *request, int status, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);

  scptr char *error_msg = vstrfmt_direct(fmt, ap);

  va_end(ap);

  scptr htable_t *resp = jsonh_make();

  jsonh_set_bool(resp, "error", true);
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
  web_server_error_resp(request, 404, "The requested resource was not found (" QUOTSTR ")!", request->url().c_str());
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
      web_server_error_resp(request, 500, "Body too long, not enough space!");
      return false;
    }

    *output = (char *) body->content;
    return true;
  }

  // No body collected
  web_server_error_resp(request, 400, "No body content provided!");
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
    web_server_error_resp(request, 400, "This endpoint only accepts JSON bodies!");
    return false;
  }

  // Parse json
  scptr char *err = NULL;
  scptr htable_t *body_jsn = jsonh_parse(body, &err);
  if (!body_jsn)
  {
    web_server_error_resp(request, 400, "Could not parse the JSON body: %s", err);
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

INLINED static bool scheduler_parse_day(AsyncWebServerRequest *request, const char *day_str, scheduler_weekday_t *day)
{
  // Parse weekday from path arg
  if (scheduler_weekday_value(day_str, day) != ENUMLUT_SUCCESS)
  {
    web_server_error_resp(request, 400, "Invalid weekday specified (%s)!", day_str);
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
    web_server_error_resp(request, 400, "Invalid non-numeric index (%s)!", index_str);
    return false;
  }

  // Check index validity
  if (*index < 0 || *index >= SCHEDULER_MAX_INTERVALS_PER_DAY)
  {
    web_server_error_resp(request, 400, "Invalid out-of-range index (%s)!", index_str);
    return false;
  }

  return true;
}

/*
============================================================================
                                 /scheduler                                 
============================================================================
*/

void web_server_route_scheduler(AsyncWebServerRequest *request)
{
  scptr htable_t *jsn = jsonh_make();
  
  for (int i = 0; i < 7; i++)
  {
    scheduler_weekday_t day = (scheduler_weekday_t) i;
    const char *weekday_name = scheduler_weekday_name(day);
    scptr dynarr_t *weekday_schedule = scheduler_weekday_jsonify(sched, day);
    jsonh_set_arr(jsn, weekday_name, (dynarr_t *) mman_ref(weekday_schedule));
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

  scptr htable_t *jsn = jsonh_make();
  scptr dynarr_t *weekday_schedule = scheduler_weekday_jsonify(sched, day);
  jsonh_set_arr(jsn, "items", weekday_schedule);

  web_server_json_resp(request, 200, jsn);
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

  scptr htable_t *int_jsn = scheduler_interval_jsonify(index, sched->daily_schedules[day][index]);
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
    web_server_error_resp(request, 400, "Body data malformed: %s", err);
    return;
  }

  // Print interval
  scptr char *resp = strfmt_direct(
    "interval { start=%02u:%02u:%02u, end=%02u:%02u:%02u, id=%03u }",
    interval.start.hours, interval.start.minutes, interval.start.seconds,
    interval.end.hours, interval.end.minutes, interval.end.seconds,
    interval.identifier
  );

  // Update the entry and save it persistently
  sched->daily_schedules[day][index] = interval;
  scheduler_eeprom_save(sched);

  // Respond with the updated entry
  scptr htable_t *int_jsn = scheduler_interval_jsonify(index, sched->daily_schedules[day][index]);
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

  scheduler_interval_t *targ = &(sched->daily_schedules[day][index]);

  // Already an empty slot
  if (scheduler_interval_empty(*targ))
  {
    web_server_error_resp(request, 404, "This index is already empty");
    return;
  }

  // Clear slot and save persistently
  *targ = SCHEDULER_INTERVAL_EMPTY;
  scheduler_eeprom_save(sched);
  web_server_empty_ok(request);
}

/*
============================================================================
                          Webserver Configuration                           
============================================================================
*/

void web_server_init(scheduler_t *scheduler)
{
  // /scheduler
  wsrv.on("^/scheduler$", HTTP_GET, web_server_route_scheduler);

  // /scheduler/{day}
  wsrv.on("^\\/scheduler\\/([A-Za-z0-9_]+)$", HTTP_GET, web_server_route_scheduler_day);

  // /scheduler/{day}/index
  const char *sched_day_index = "^\\/scheduler\\/([A-Za-z0-9_]+)\\/([0-9]+)$";
  wsrv.on(sched_day_index, HTTP_GET, web_server_route_scheduler_day_index);
  wsrv.on(sched_day_index, HTTP_PUT, web_server_route_scheduler_day_index_edit, NULL, web_server_str_body_handler);
  wsrv.on(sched_day_index, HTTP_DELETE, web_server_route_scheduler_day_index_delete);

  // All remaining paths
  wsrv.onNotFound(web_server_route_not_found);

  sched = scheduler;
  wsrv.begin();
}