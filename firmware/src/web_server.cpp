#include "web_server.h"

AsyncWebServer wsrv(WEB_SERVER_PORT);
scheduler_t *sched;

void web_server_route_not_found(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}

void web_server_route_scheduler(AsyncWebServerRequest *request)
{
  scptr char *resp = (char *) mman_alloc(sizeof(char), 2048, NULL);
  size_t resp_offs = 0;

  strfmt(&resp, &resp_offs, "{\n");

  for (int i = 0; i < 7; i++)
  {
    strfmt(&resp, &resp_offs, "  \"%s\": [\n", scheduler_weekday_name((scheduler_weekday_t) i));
    for (int j = 0; j < SCHEDULER_MAX_INTERVALS_PER_DAY; j++)
    {
      scheduler_interval_t interval = sched->daily_schedules[i][j];
      scptr char *start_str = scheduler_time_stringify(&(interval.start));
      scptr char *end_str = scheduler_time_stringify(&(interval.end));

      strfmt(&resp, &resp_offs, "    {\n");
      strfmt(&resp, &resp_offs, "      \"start\": \"%s\",\n", start_str);
      strfmt(&resp, &resp_offs, "      \"end\": \"%s\",\n", end_str);
      strfmt(&resp, &resp_offs, "      \"identifier\": %" PRIu8 ",\n", interval.identifier);
      strfmt(&resp, &resp_offs, "      \"active\": %s\n", interval.active ? "true" : "false");
      strfmt(&resp, &resp_offs, "    }%s\n", j == SCHEDULER_MAX_INTERVALS_PER_DAY - 1 ? "" : ",");
    }

    strfmt(&resp, &resp_offs, "  ]%s\n", i == 6 ? "" : ",");
  }

  strfmt(&resp, &resp_offs, "}");

  request->send(200, "text/plain", resp);
}

void web_server_init(scheduler_t *scheduler)
{
  wsrv.on("/scheduler", HTTP_GET, web_server_route_scheduler);
  wsrv.onNotFound(web_server_route_not_found);

  sched = scheduler;
  wsrv.begin();
}