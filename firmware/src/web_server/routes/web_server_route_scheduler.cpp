#include "web_server/routes/web_server_route_scheduler.h"

static scheduler_t *sched = NULL;

/*
============================================================================
                                 Routines                                   
============================================================================
*/

INLINED static bool web_server_parse_scheduler_day(AsyncWebServerRequest *request, const char *day_str, scheduler_weekday_t *day)
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
  if (!web_server_parse_scheduler_day(request, request->pathArg(0).c_str(), day))
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
                            GET /scheduler/{day}                            
============================================================================
*/

static void web_server_route_scheduler_day(AsyncWebServerRequest *request)
{
  // Parse weekday from path arg
  scheduler_weekday_t day;
  if (!web_server_parse_scheduler_day(request, request->pathArg(0).c_str(), &day))
    return;

  scptr htable_t *weekday_schedule = scheduler_weekday_jsonify(sched, day);
  web_server_json_resp(request, 200, weekday_schedule);
}

/*
============================================================================
                            PUT /scheduler/{day}                            
============================================================================
*/

static void web_server_route_scheduler_day_edit(AsyncWebServerRequest *request)
{
  // Parse weekday from path arg
  scheduler_weekday_t day;
  if (!web_server_parse_scheduler_day(request, request->pathArg(0).c_str(), &day))
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
    web_server_socket_events_broadcast(sched_day.disabled ? WSE_DAY_DISABLE_ON : WSE_DAY_DISABLE_OFF, ev_args);
  }

  scheduler_file_save(sched);

  // Respond with the updated day
  scptr htable_t *day_jsn = scheduler_weekday_jsonify(sched, day);
  web_server_json_resp(request, 200, day_jsn);
}

/*
============================================================================
                        GET /scheduler/{day}/{index}                        
============================================================================
*/

static void web_server_route_scheduler_day_index(AsyncWebServerRequest *request)
{
  scheduler_weekday_t day;
  long index;

  if (!web_server_route_scheduler_day_index_parse(request, &day, &index))
    return;

  scptr htable_t *int_jsn = scheduler_interval_jsonify(index, &(sched->daily_schedules[day].intervals[index]));
  web_server_json_resp(request, 200, int_jsn);
}

/*
============================================================================
                        PUT /scheduler/{day}/{index}                        
============================================================================
*/

static void web_server_route_scheduler_day_index_edit(AsyncWebServerRequest *request)
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
    web_server_socket_events_broadcast(interval.disabled ? WSE_INTERVAL_DISABLE_ON : WSE_INTERVAL_DISABLE_OFF, ev_args);
  }

  // Check for deltas and patch end
  if (scheduler_time_compare(targ_interval->end, interval.end) != 0)
  {
    targ_interval->end = interval.end;

    scptr char *ev_args = strfmt_direct("%s;%ld;%s", scheduler_weekday_name(day), index, scheduler_time_stringify(&(interval.end)));
    web_server_socket_events_broadcast(WSE_INTERVAL_END_CHANGE, ev_args);
  }

  // Check for deltas and patch start
  if (scheduler_time_compare(targ_interval->start, interval.start) != 0)
  {
    targ_interval->start = interval.start;

    scptr char *ev_args = strfmt_direct("%s;%ld;%s", scheduler_weekday_name(day), index, scheduler_time_stringify(&(interval.start)));
    web_server_socket_events_broadcast(WSE_INTERVAL_START_CHANGE, ev_args);
  }

  // Check for deltas and patch identifier
  if (targ_interval->identifier != interval.identifier)
  {
    targ_interval->identifier = interval.identifier;
    scptr char *ev_args = strfmt_direct("%s;%ld;%u", scheduler_weekday_name(day), index, interval.identifier);
    web_server_socket_events_broadcast(WSE_INTERVAL_IDENTIFIER_CHANGE, ev_args);
  }

  scheduler_file_save(sched);

  // Respond with the updated entry
  scptr htable_t *int_jsn = scheduler_interval_jsonify(index, targ_interval);
  web_server_json_resp(request, 200, int_jsn);
}

/*
============================================================================
                      DELETE /scheduler/{day}/{index}                       
============================================================================
*/

static void web_server_route_scheduler_day_index_delete(AsyncWebServerRequest *request)
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
  scheduler_file_save(sched);

  scptr char *ev_args = strfmt_direct("%s;%ld", scheduler_weekday_name(day), index);
  web_server_socket_events_broadcast(WSE_INTERVAL_DELETED, ev_args);

  web_server_empty_ok(request);
}

/*
============================================================================
                              Initialization                                
============================================================================
*/

void web_server_route_scheduler_init(scheduler_t *scheduler_ref, AsyncWebServer *wsrv)
{
  sched = scheduler_ref;

  // /scheduler/{day}
  const char *p_sched_day = "^\\/api\\/scheduler\\/([A-Za-z0-9_]+)$";
  wsrv->on(p_sched_day, HTTP_GET, web_server_route_scheduler_day);
  wsrv->on(p_sched_day, HTTP_PUT, web_server_route_scheduler_day_edit, NULL, web_server_str_body_handler);
  wsrv->on(p_sched_day, HTTP_OPTIONS, web_server_route_any_options);

  // /scheduler/{day}/index
  const char *p_sched_day_index = "^\\/api\\/scheduler\\/([A-Za-z0-9_]+)\\/([0-9]+)$";
  wsrv->on(p_sched_day_index, HTTP_GET, web_server_route_scheduler_day_index);
  wsrv->on(p_sched_day_index, HTTP_PUT, web_server_route_scheduler_day_index_edit, NULL, web_server_str_body_handler);
  wsrv->on(p_sched_day_index, HTTP_DELETE, web_server_route_scheduler_day_index_delete);
  wsrv->on(p_sched_day_index, HTTP_OPTIONS, web_server_route_any_options);
}