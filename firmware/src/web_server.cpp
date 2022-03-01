#include "web_server.h"

AsyncWebServer wsrv(WEB_SERVER_PORT);
scheduler_t *sched;

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

  scptr char *resp_str = jsonh_stringify(resp, 2);
  request->send(status, "text/json", resp_str);
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
                                Body Handler                                
============================================================================
*/

void web_server_str_body_handler(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
{
  // Create buffer on first segment of the message
  if(index == 0)
  {
    // Since the framework calls free on _tempObject if it's non-null, just allocate a
    // pointer to the mman resource we'll dealloc later ourselves
    request->_tempObject = malloc(sizeof(char **));

    // Allocate actual string buffer
    *((char **) request->_tempObject) = (char *) mman_alloc(sizeof(char), 128, NULL);
  }

  // Collect segments into buffer
  scptr char *str = strclone_s((char *) data, len);
  strfmt((char **) request->_tempObject, &index, "%s", str);
}

/*
============================================================================
                              Common routines                               
============================================================================
*/

static htable_t *generate_day_ind_interval_json(scheduler_weekday_t day, int index)
{
  scheduler_interval_t interval = sched->daily_schedules[day][index];
  scptr char *start_str = scheduler_time_stringify(&(interval.start));
  scptr char *end_str = scheduler_time_stringify(&(interval.end));

  scptr htable_t *int_jsn = jsonh_make();

  jsonh_set_str(int_jsn, "start", (char *) mman_ref(start_str));
  jsonh_set_str(int_jsn, "end", (char *) mman_ref(end_str));
  jsonh_set_int(int_jsn, "identifier", interval.identifier);
  jsonh_set_int(int_jsn, "index", index);
  jsonh_set_bool(int_jsn, "active", interval.active);

  return (htable_t *) mman_ref(int_jsn);
}

static dynarr_t *generate_day_schedule_json(scheduler_weekday_t day)
{
  scptr dynarr_t *weekday_schedule = dynarr_make(SCHEDULER_MAX_INTERVALS_PER_DAY, SCHEDULER_MAX_INTERVALS_PER_DAY, mman_dealloc_nr);

  for (int j = 0; j < SCHEDULER_MAX_INTERVALS_PER_DAY; j++)
  {
    scptr htable_t *int_jsn = generate_day_ind_interval_json(day, j);
    jsonh_insert_arr_obj(weekday_schedule, (htable_t *) mman_ref(int_jsn));
  }

  return (dynarr_t *) mman_ref(weekday_schedule);
}

static bool scheduler_parse_day(AsyncWebServerRequest *request, const char *day_str, scheduler_weekday_t *day)
{
  // Parse weekday from path arg
  if (scheduler_weekday_value(day_str, day) != ENUMLUT_SUCCESS)
  {
    web_server_error_resp(request, 400, "Invalid weekday specified (%s)!", day_str);
    return false;
  }

  return true;
}

static bool web_server_ensure_body(AsyncWebServerRequest *request, char **output)
{
  // Body segment collector has been called at least once
  if (request->_tempObject)
  {
    *output = (char *) *((void **) request->_tempObject);
    return true;
  }

  web_server_error_resp(request, 400, "No body content provided!");
  return false;
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
    scptr dynarr_t *weekday_schedule = generate_day_schedule_json(day);
    jsonh_set_arr(jsn, weekday_name, (dynarr_t *) mman_ref(weekday_schedule));
  }

  scptr char *stringified = jsonh_stringify(jsn, 2);
  request->send(200, "text/json", stringified);
}

/*
============================================================================
                              /scheduler/{day}                              
============================================================================
*/

void web_server_route_scheduler_day(AsyncWebServerRequest *request)
{
  // Parse weekday from path arg
  scheduler_weekday_t day;
  if (!scheduler_parse_day(request, request->pathArg(0).c_str(), &day))
    return;

  scptr htable_t *jsn = jsonh_make();
  scptr dynarr_t *weekday_schedule = generate_day_schedule_json(day);
  jsonh_set_arr(jsn, "items", weekday_schedule);

  scptr char *jsn_str = jsonh_stringify(jsn, 2);
  request->send(200, "text/json", jsn_str);
}

/*
============================================================================
                          /scheduler/{day}/{index}                          
============================================================================
*/

static bool web_server_route_scheduler_day_index_parse(
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

void web_server_route_scheduler_day_index(AsyncWebServerRequest *request)
{
  scheduler_weekday_t day;
  long index;

  if (!web_server_route_scheduler_day_index_parse(request, &day, &index))
    return;

  scptr htable_t *int_jsn = generate_day_ind_interval_json(day, index);
  scptr char *int_jsn_str = jsonh_stringify(int_jsn, 2);
  request->send(200, "text/json", int_jsn_str);
}

void web_server_route_scheduler_day_index_edit(AsyncWebServerRequest *request)
{
  scheduler_weekday_t day;
  long index;

  if (!web_server_route_scheduler_day_index_parse(request, &day, &index))
    return;

  scptr char *body = NULL;
  if (!web_server_ensure_body(request, &body))
    return;

  // Parse json
  scptr char *err = NULL;
  scptr htable_t *body_jsn = jsonh_parse(body, &err);
  if (!body_jsn)
  {
    web_server_error_resp(request, 400, "Could not parse the JSON body: %s", err);
    return;
  }

  // Parse interval from json
  scheduler_interval_t interval;
  if (!scheduler_interval_parse(body_jsn, &err, &interval))
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

  // TODO: Actually use the data instead of just printing it

  request->send(200, "text/plain", resp);
}

void web_server_init(scheduler_t *scheduler)
{
  // /scheduler
  wsrv.on("^/scheduler$", HTTP_GET, web_server_route_scheduler);

  // /scheduler/{day}
  wsrv.on("^\\/scheduler\\/([A-Za-z0-9_]+)$", HTTP_GET, web_server_route_scheduler_day);

  // /scheduler/{day}/index
  wsrv.on("^\\/scheduler\\/([A-Za-z0-9_]+)\\/([0-9]+)$", HTTP_GET, web_server_route_scheduler_day_index);

  // /scheduler/{day}/index
  wsrv.on("^\\/scheduler\\/([A-Za-z0-9_]+)\\/([0-9]+)$", HTTP_PUT, web_server_route_scheduler_day_index_edit, NULL, web_server_str_body_handler);

  // All remaining paths
  wsrv.onNotFound(web_server_route_not_found);

  sched = scheduler;
  wsrv.begin();
}