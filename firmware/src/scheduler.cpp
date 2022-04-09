#include "scheduler.h"

ENUM_LUT_FULL_IMPL(scheduler_weekday, _EVALS_SCHEDULER_WEEKDAY);
ENUM_LUT_FULL_IMPL(scheduler_edge, _EVALS_SCHEDULER_EDGE);

scheduler_t scheduler_make(scheduler_callback_t callback, scheduler_day_and_time_provider_t dt_provider)
{
  return (scheduler_t) {
    .daily_schedules = { { SCHEDULER_INTERVAL_EMPTY }, false },
    .callback = callback,                             // Set the user-provided callback
    .dt_provider = dt_provider,                       // Set the user-provided provider
    .last_tick_time = SCHEDULER_TIME_MIDNIGHT,        // Start out with an arbitrary last tick time
    .last_tick_day = WEEKDAY_SU                       // Start out with an arbitrary last tick day
  };
}

/**
 * @brief Parse an interval's time-property from json by a specific key
 * 
 * @param json JSONH json node
 * @param key Key to fetch from
 * @param err Error output buffer
 * @param out Value output buffer
 * 
 * @return true Parsing success
 * @return false Parsing error, see err
 */
INLINED static bool scheduler_interval_parse_time(
  htable_t *json,
  const char *key,
  char **err,
  scheduler_time_t *out
)
{
  // Get value
  jsonh_opres_t jopr;
  char *start_s = NULL;
  if ((jopr = jsonh_get_str(json, key, &start_s)) != JOPRES_SUCCESS)
  {
    *err = jsonh_getter_errstr(key, jopr);
    return false;
  }

  // Parse value
  scheduler_time_t res;
  if (!scheduler_time_parse(start_s, err, &res))
    return false;

  *out = res;
  return true;
}

bool scheduler_day_parse(htable_t *json, char **err, scheduler_day_t *out)
{
  // Get disabled state
  bool disabled;
  jsonh_opres_t jopr;
  if ((jopr = jsonh_get_bool(json, "disabled", &disabled)) != JOPRES_SUCCESS)
  {
    *err = jsonh_getter_errstr("disabled", jopr);
    return false;
  }

  out->disabled = disabled;
  return true;
}

bool scheduler_interval_parse(htable_t *json, char **err, scheduler_interval_t *out)
{
  // Parse start
  scheduler_time_t start;
  if (!scheduler_interval_parse_time(json, "start", err, &start))
    return false;

  // Parse end
  scheduler_time_t end;
  if (!scheduler_interval_parse_time(json, "end", err, &end))
    return false;

  // Make sure that the end is greater than the start
  if (scheduler_time_compare(end, start) != 1)
  {
    *err = strfmt_direct("\"end\" has to be greater than \"start\"");
    return false;
  }

  // Get identifier
  int identifier;
  jsonh_opres_t jopr;
  if ((jopr = jsonh_get_int(json, "identifier", &identifier)) != JOPRES_SUCCESS)
  {
    *err = jsonh_getter_errstr("identifier", jopr);
    return false;
  }

  // Get disabled state
  bool disabled;
  if ((jopr = jsonh_get_bool(json, "disabled", &disabled)) != JOPRES_SUCCESS)
  {
    *err = jsonh_getter_errstr("disabled", jopr);
    return false;
  }

  // Build result
  *out = scheduler_interval_make(start, end, identifier, disabled);
  return true;
}

htable_t *scheduler_interval_jsonify(int index, scheduler_interval_t *interval)
{
  scptr char *start_str = scheduler_time_stringify(&(interval->start));
  scptr char *end_str = scheduler_time_stringify(&(interval->end));

  scptr htable_t *int_jsn = htable_make(6, mman_dealloc_nr);

  jsonh_set_str(int_jsn, "start", (char *) mman_ref(start_str));
  jsonh_set_str(int_jsn, "end", (char *) mman_ref(end_str));
  jsonh_set_int(int_jsn, "identifier", interval->identifier);
  jsonh_set_int(int_jsn, "index", index);
  jsonh_set_bool(int_jsn, "active", interval->active);
  jsonh_set_bool(int_jsn, "disabled", interval->disabled);

  return (htable_t *) mman_ref(int_jsn);
}

/**
 * @brief Check if a given interval is equal to the empty interval constant
 */
bool scheduler_interval_empty(scheduler_interval_t interval)
{
  return (
    scheduler_time_compare(interval.start, SCHEDULER_INTERVAL_EMPTY.start) == 0   // Start is equal to empty
    && scheduler_time_compare(interval.end, SCHEDULER_INTERVAL_EMPTY.end) == 0    // End is equal to empty
    && interval.identifier == SCHEDULER_INTERVAL_EMPTY.identifier                 // Identifier is equal to empty
    && interval.active == SCHEDULER_INTERVAL_EMPTY.active                         // Active state is equal to empty
  );
}

bool scheduler_interval_equals(scheduler_interval_t a, scheduler_interval_t b)
{
  // Compare start time
  if (scheduler_time_compare(a.start, b.start) != 0) return false;

  // Compare end time
  if (scheduler_time_compare(a.end, b.end) != 0) return false;

  // Compare identifier
  return a.identifier == b.identifier;
}

scheduler_interval_t scheduler_interval_make(scheduler_time_t start, scheduler_time_t end, uint8_t identifier, bool disabled)
{
  return (scheduler_interval_t) {
    .start = start,
    .end = end,
    .identifier = identifier,
    .active = false,
    .disabled = disabled
  };
}

static int scheduler_find_interval_slot(scheduler_t *scheduler, scheduler_weekday_t day, scheduler_interval_t interval)
{
  // Loop all interval slots for the target day
  for (size_t i = 0; i < SCHEDULER_MAX_INTERVALS_PER_DAY; i++)
  {
    scheduler_interval_t slot = scheduler->daily_schedules[day].intervals[i];

    // Not the target interval
    if (!scheduler_interval_equals(slot, interval)) continue;

    return i;
  }

  // Target interval does not exist
  return -1;
}

htable_t *scheduler_weekday_jsonify(scheduler_t *scheduler, scheduler_weekday_t day)
{
  scptr htable_t *weekday = htable_make(2, mman_dealloc_nr);
  scptr dynarr_t *weekday_intervals = dynarr_make_mmf(SCHEDULER_MAX_INTERVALS_PER_DAY);

  if (jsonh_set_arr(weekday, "intervals", (dynarr_t *) mman_ref(weekday_intervals)) != JOPRES_SUCCESS)
    mman_dealloc(weekday_intervals);

  scheduler_day_t targ_day = scheduler->daily_schedules[day];
  for (int j = 0; j < SCHEDULER_MAX_INTERVALS_PER_DAY; j++)
  {
    scptr htable_t *int_jsn = scheduler_interval_jsonify(j, &(targ_day.intervals[j]));
    if (jsonh_insert_arr_obj(weekday_intervals, (htable_t *) mman_ref(int_jsn)) != JOPRES_SUCCESS)
      mman_dealloc(int_jsn);
  }

  jsonh_set_bool(weekday, "disabled", targ_day.disabled);
  return (htable_t *) mman_ref(weekday);
}

bool scheduler_register_interval(scheduler_t *scheduler, scheduler_weekday_t day, scheduler_interval_t interval)
{
  // Find an empty slot
  int slot = scheduler_find_interval_slot(scheduler, day, SCHEDULER_INTERVAL_EMPTY);

  // No more empty slots remaining
  if (slot < 0) return false;

  // Set the slot
  scheduler->daily_schedules[day].intervals[slot] = interval;
  return true;
}

bool scheduler_unregister_interval(scheduler_t *scheduler, scheduler_weekday_t day, scheduler_interval_t interval)
{
  int slot = scheduler_find_interval_slot(scheduler, day, interval);

  // This interval is not registered
  if (slot < 0) return false;

  // Unregister by setting the slot to an empty value
  scheduler->daily_schedules[day].intervals[slot] = SCHEDULER_INTERVAL_EMPTY;
  return true;
}

bool scheduler_change_interval(scheduler_t *scheduler, scheduler_weekday_t day, scheduler_interval_t from, scheduler_interval_t to)
{
  int slot = scheduler_find_interval_slot(scheduler, day, from);

  // This interval is not registered
  if (slot < 0) return false;

  // Change the interval value
  scheduler->daily_schedules[day].intervals[slot] = to;
  return true;
}

/**
 * @brief Tick all intervals of the current day
 */
INLINED static void scheduler_tick_intervals(scheduler_t *scheduler, scheduler_weekday_t day, scheduler_time_t time)
{
  // Loop all intervals of the day
  scheduler_day_t *curr_day = &(scheduler->daily_schedules[day]);
  for (size_t i = 0; i < SCHEDULER_MAX_INTERVALS_PER_DAY; i++)
  {
    scheduler_interval_t *interval = &(curr_day->intervals[i]);

    // Skip empty slots
    if (scheduler_interval_empty(*interval)) continue;

    // Interval turned on
    if (
      scheduler_time_compare(time, interval->start) == 1      // Time is after start
      && scheduler_time_compare(time, interval->end) == -1    // And time is before end
      && !interval->active                                    // And interval is not already active
      && !interval->disabled                                  // And interval is not disabled
      && !curr_day->disabled                                   // And current day is not disabled
    )
    {
      interval->active = true;
      scheduler->callback(EDGE_OFF_TO_ON, interval->identifier, day, time);

      // Broadcast scheduler on event
      scptr char *ev_arg = strfmt_direct("%d", i);
      web_server_socket_events_broadcast(WSE_INTERVAL_SCHED_ON, ev_arg);

      continue;
    }

    // Interval turned off
    if (
      (
        scheduler_time_compare(time, interval->end) == 1        // Time is after end
        && interval->active                                     // And interval is active
      ) ||
      (
        interval->active && interval->disabled                  // Interval has been disabled during active state
      )
    )
    {
      interval->active = false;
      scheduler->callback(EDGE_ON_TO_OFF, interval->identifier, day, time);

      // Broadcast scheduler off event
      scptr char *ev_arg = strfmt_direct("%d", i);
      web_server_socket_events_broadcast(WSE_INTERVAL_SCHED_OFF, ev_arg);

      continue;
    }
  }
}

/**
 * @brief Tick all available valves and their corresponding timer fields
 */
INLINED static void scheduler_tick_valve_timers(valve_control_t *valve_ctl, scheduler_weekday_t day, scheduler_time_t time)
{
  for (size_t i = 0; i < VALVE_CONTROL_NUM_VALVES; i++)
  {
    valve_t *targ_valve = &(valve_ctl->valves[i]);

    // This valve has no timer
    if (!targ_valve->has_timer)
      continue;  

    int time_comparison = scheduler_time_compare(targ_valve->timer, SCHEDULER_TIME_MIDNIGHT);

    // Timer just ended
    if (time_comparison == 0)
    {
      valve_control_toggle(valve_ctl, i, false);
      targ_valve->has_timer = false;
    }

    // Timer still active, decrement
    else if (time_comparison > 0)
    {
      scheduler_time_decrement_bound(&(targ_valve->timer), 1);
    }

    scptr char *time_strval = scheduler_time_stringify(&(targ_valve->timer));
    scptr char *ev_args = strfmt_direct("%lu;%s", i, time_strval);
    web_server_socket_events_broadcast(WSE_VALVE_TIMER_UPDATED, ev_args);
  }
}

void scheduler_tick(scheduler_t *scheduler, valve_control_t *valve_ctl)
{
  // Fetch the current day and time
  scheduler_weekday_t day;
  scheduler_time_t time;
  scheduler->dt_provider(&day, &time);

  // Skip duplicate ticks
  if (
    scheduler_time_compare(time, scheduler->last_tick_time) == 0  // Same time as last tick
    && day == scheduler->last_tick_day                            // And same day as last tick
  )
    return;

  scheduler_tick_valve_timers(valve_ctl, day, time);
  scheduler_tick_intervals(scheduler, day, time);

  // Update last tick day and time
  scheduler->last_tick_day = day;
  scheduler->last_tick_time = time;
}

void scheduler_file_save(scheduler_t *scheduler)
{
  scptr htable_t *jsn = htable_make(7, mman_dealloc_nr);

  // Loop all weekdays and append them with their weekday name
  for (int i = 0; i < 7; i++)
  {
    scheduler_weekday_t day = (scheduler_weekday_t) i;
    scptr htable_t *day_jsn = scheduler_weekday_jsonify(scheduler, day);
    jsonh_set_obj_ref(jsn, scheduler_weekday_name(day), day_jsn);
  }

  // Write json to the file
  if (!sdh_write_json_file(jsn, SCHEDULER_FILE))
  {
    dbgerr("Could not write the scheduler file");
    return;
  }
}

/**
 * @brief Parse a scheduler time field from a JSON object, identified by it's key
 * 
 * @param jsn Json to parse
 * @param key Key of target string field
 * @return scheduler_time_t Parsed scheduler time
 */
static scheduler_time_t scheduler_json_parse_time(htable_t *jsn, const char *key)
{
  scheduler_time_t res;

  char *time_str = NULL;
  if (jsonh_get_str(jsn, key, &time_str) == JOPRES_SUCCESS)
  {
    scptr char *err = NULL;
    if (!scheduler_time_parse(time_str, &err, &res))
      dbgerr("Could not parse scheduler time from file: %s", err);
  }

  return res;
}

/**
 * @brief Parse a scheduler interval from a JSON object
 * 
 * @param scheduler Scheduler instance to modify
 * @param jsn Json to parse
 * @param day Day to modify of the scheduler
 */
static void scheduler_json_parse_interval(scheduler_t *scheduler, htable_t *jsn, scheduler_day_t *day)
{
  // Try to get the interval's index
  int index = 0;
  if (jsonh_get_int(jsn, "index", &index) != JOPRES_SUCCESS)
    return;

  scheduler_interval_t *curr_int = &(day->intervals[index]);

  // Load disabled state and target identifier
  jsonh_get_bool(jsn, "disabled", &(curr_int->disabled));
  jsonh_get_int(jsn, "identifier", (int *) &(curr_int->identifier));

  // Load start and end times
  curr_int->start = scheduler_json_parse_time(jsn, "start");
  curr_int->end = scheduler_json_parse_time(jsn, "end");
}

/**
 * @brief Parse a scheduler day from a JSON object
 * 
 * @param scheduler Scheduler to modify
 * @param jsn Json to parse
 * @param day Day of the scheduler to modify
 */
static void scheduler_json_parse_day(scheduler_t *scheduler, htable_t *jsn, scheduler_weekday_t day)
{
  scheduler_day_t *sched_day = &(scheduler->daily_schedules[day]);

  // Try to get the current day's disabled state
  jsonh_get_bool(jsn, "disabled", &(sched_day->disabled));

  // Try to get the stored intervals from this day
  dynarr_t *ints_jsn = NULL;
  if (jsonh_get_arr(jsn, "intervals", &ints_jsn) != JOPRES_SUCCESS)
    return;

  // Loop all stored intervals
  scptr size_t *active = NULL;
  size_t num_active = 0;
  dynarr_indices(ints_jsn, &active, &num_active);
  for (size_t i = 0; i < num_active; i++)
  {
    // Try to get the current interval json
    htable_t *int_jsn = NULL;
    if (jsonh_get_arr_obj(ints_jsn, active[i], &int_jsn) != JOPRES_SUCCESS)
      continue;

    scheduler_json_parse_interval(scheduler, int_jsn, sched_day);
  }
}

void scheduler_file_load(scheduler_t *scheduler)
{
  // Read the json file
  scptr htable_t *jsn = sdh_read_json_file(SCHEDULER_FILE);
  if (!jsn)
    return;

  // Loop all weekdays and get them by their name
  for (int i = 0; i < 7; i++)
  {
    // Get the current day
    scheduler_weekday_t day = (scheduler_weekday_t) i;
    htable_t *day_jsn = NULL;
    if (jsonh_get_obj(jsn, scheduler_weekday_name(day), &day_jsn) != JOPRES_SUCCESS)
      continue;

    scheduler_json_parse_day(scheduler, day_jsn, day);
  }
}