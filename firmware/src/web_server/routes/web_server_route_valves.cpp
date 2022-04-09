#include "web_server/routes/web_server_route_valves.h"

static valve_control_t *valvectl = NULL;

/*
============================================================================
                                  Routines                                  
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

/*
============================================================================
                                 GET /valves                                
============================================================================
*/

static void web_server_route_valves(AsyncWebServerRequest *request)
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

static void web_server_route_valves_edit(AsyncWebServerRequest *request)
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
    web_server_socket_events_broadcast(WSE_VALVE_RENAME, ev_arg);
  }

  // Check for deltas and patch disabled state
  if (targ_valve->disabled != valve.disabled)
  {
    targ_valve->disabled = valve.disabled;

    scptr char *ev_arg = strfmt_direct("%d", valve_id);
    web_server_socket_events_broadcast(valve.disabled ? WSE_VALVE_DISABLE_ON : WSE_VALVE_DISABLE_OFF, ev_arg);
  }

  valve_control_file_save(valvectl);

  // Respond with the updated valve
  scptr htable_t *valve_jsn = valve_control_valve_jsonify(valvectl, valve_id);
  web_server_json_resp(request, 200, valve_jsn);
}

/*
============================================================================
                               POST /valves/{id}                            
============================================================================
*/

static void web_server_route_valves_activate(AsyncWebServerRequest *request)
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

static void web_server_route_valves_deactivate(AsyncWebServerRequest *request)
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

static void web_server_route_valves_timer_set(AsyncWebServerRequest *request)
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
  web_server_socket_events_broadcast(WSE_VALVE_TIMER_UPDATED, ev_args_timer);

  scptr char *ev_args_off = strfmt_direct("%lu", valve_id);
  web_server_socket_events_broadcast(WSE_VALVE_ON, ev_args_off);

  web_server_empty_ok(request);
}

/*
============================================================================
                          DELETE /valves/{id}/timer                         
============================================================================
*/

static void web_server_route_valves_timer_clear(AsyncWebServerRequest *request)
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
  web_server_socket_events_broadcast(WSE_VALVE_TIMER_UPDATED, ev_args);
  web_server_socket_events_broadcast(WSE_VALVE_OFF, ev_args);

  web_server_empty_ok(request);
}

/*
============================================================================
                              Initialization                                
============================================================================
*/

void web_server_route_valves_init(valve_control_t *valvectl_ref, AsyncWebServer *wsrv)
{
  valvectl = valvectl_ref;

  // /valves
  const char *p_valves = "^\\/api\\/valves$";
  wsrv->on(p_valves, HTTP_GET, web_server_route_valves);
  wsrv->on(p_valves, HTTP_OPTIONS, web_server_route_any_options);

  // /valves/{id}
  const char *p_valves_id = "^\\/api\\/valves\\/([0-9]+)$";
  wsrv->on(p_valves_id, HTTP_PUT, web_server_route_valves_edit, NULL, web_server_str_body_handler);
  wsrv->on(p_valves_id, HTTP_POST, web_server_route_valves_activate);
  wsrv->on(p_valves_id, HTTP_DELETE, web_server_route_valves_deactivate);
  wsrv->on(p_valves_id, HTTP_OPTIONS, web_server_route_any_options);

  // /valves/id/timer
  const char *p_valves_id_timer = "^\\/api\\/valves\\/([0-9]+)\\/timer$";
  wsrv->on(p_valves_id_timer, HTTP_POST, web_server_route_valves_timer_set, NULL, web_server_str_body_handler);
  wsrv->on(p_valves_id_timer, HTTP_DELETE, web_server_route_valves_timer_clear);
  wsrv->on(p_valves_id_timer, HTTP_OPTIONS, web_server_route_any_options);
}