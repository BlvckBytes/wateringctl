#include "web_server.h"

AsyncWebServer wsrv(WEB_SERVER_PORT);
scheduler_t *sched;

/*
============================================================================
                               Error routine                                
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
  web_server_error_resp(request, 404, "The requested resource was not found (" QUOTSTR ")!", request->url());
}

/*
============================================================================
                              Common routines                               
============================================================================
*/

static dynarr_t *generate_day_schedule_json(scheduler_weekday_t day)
{
  scptr dynarr_t *weekday_schedule = dynarr_make(SCHEDULER_MAX_INTERVALS_PER_DAY, SCHEDULER_MAX_INTERVALS_PER_DAY, mman_dealloc_nr);

  for (int j = 0; j < SCHEDULER_MAX_INTERVALS_PER_DAY; j++)
  {
    scheduler_interval_t interval = sched->daily_schedules[day][j];
    scptr char *start_str = scheduler_time_stringify(&(interval.start));
    scptr char *end_str = scheduler_time_stringify(&(interval.end));

    scptr htable_t *int_jsn = jsonh_make();
    jsonh_insert_arr_obj(weekday_schedule, (htable_t *) mman_ref(int_jsn));

    jsonh_set_str(int_jsn, "start", (char *) mman_ref(start_str));
    jsonh_set_str(int_jsn, "end", (char *) mman_ref(end_str));
    jsonh_set_int(int_jsn, "identifier", interval.identifier);
    jsonh_set_bool(int_jsn, "active", interval.active);
  }

  return (dynarr_t *) mman_ref(weekday_schedule);
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
  const char *day_param = request->pathArg(0).c_str();
  if (scheduler_weekday_value(day_param, &day) != ENUMLUT_SUCCESS)
  {
    web_server_error_resp(request, 400, "Invalid weekday specified (%s)!", day_param);
    return;
  }

  scptr htable_t *jsn = jsonh_make();
  scptr dynarr_t *weekday_schedule = generate_day_schedule_json(day);
  jsonh_set_arr(jsn, "items", weekday_schedule);

  scptr char *jsn_str = jsonh_stringify(jsn, 2);
  request->send(200, "text/json", jsn_str);
}

void web_server_init(scheduler_t *scheduler)
{
  // /scheduler
  wsrv.on("^/scheduler$", HTTP_GET, web_server_route_scheduler);

  // /scheduler/{day}
  wsrv.on("^\\/scheduler\\/([A-Za-z0-9_]+)$", HTTP_GET, web_server_route_scheduler_day);

  // All remaining paths
  wsrv.onNotFound(web_server_route_not_found);

  sched = scheduler;
  wsrv.begin();
}