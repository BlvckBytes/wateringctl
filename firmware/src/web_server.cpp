#include "web_server.h"

AsyncWebServer wsrv(WEB_SERVER_PORT);
scheduler_t *sched;

void web_server_route_not_found(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}

void web_server_route_scheduler(AsyncWebServerRequest *request)
{
  scptr htable_t *jsn = jsonh_make();
  
  for (int i = 0; i < 7; i++)
  {
    const char *weekday_name = scheduler_weekday_name((scheduler_weekday_t) i);
    scptr dynarr_t *weekday_schedule = dynarr_make(SCHEDULER_MAX_INTERVALS_PER_DAY, SCHEDULER_MAX_INTERVALS_PER_DAY, mman_dealloc_nr);
    jsonh_set_arr(jsn, weekday_name, (dynarr_t *) mman_ref(weekday_schedule));

    for (int j = 0; j < SCHEDULER_MAX_INTERVALS_PER_DAY; j++)
    {
      scheduler_interval_t interval = sched->daily_schedules[i][j];
      scptr char *start_str = scheduler_time_stringify(&(interval.start));
      scptr char *end_str = scheduler_time_stringify(&(interval.end));

      scptr htable_t *int_jsn = jsonh_make();
      jsonh_insert_arr_obj(weekday_schedule, (htable_t *) mman_ref(int_jsn));

      jsonh_set_str(int_jsn, "start", (char *) mman_ref(start_str));
      jsonh_set_str(int_jsn, "end", (char *) mman_ref(end_str));
      jsonh_set_int(int_jsn, "identifier", interval.identifier);
      jsonh_set_bool(int_jsn, "active", interval.active);
    }
  }

  scptr char *stringified = jsonh_stringify(jsn, 2);
  request->send(200, "text/json", stringified);
}

void web_server_init(scheduler_t *scheduler)
{
  wsrv.on("/scheduler", HTTP_GET, web_server_route_scheduler);
  wsrv.onNotFound(web_server_route_not_found);

  sched = scheduler;
  wsrv.begin();
}