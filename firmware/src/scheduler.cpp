#include "scheduler.h"

ENUM_LUT_FULL_IMPL(scheduler_weekday, _EVALS_SCHEDULER_WEEKDAY);
ENUM_LUT_FULL_IMPL(scheduler_edge, _EVALS_SCHEDULER_EDGE);

scheduler_t scheduler_make(scheduler_callback_t callback, scheduler_day_and_time_provider_t dt_provider)
{
  return (scheduler_t) {
    .daily_schedules = { { SCHEDULER_INTERVAL_EMPTY } },
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

htable_t *scheduler_interval_jsonify(int index, scheduler_interval_t interval)
{
  scptr char *start_str = scheduler_time_stringify(&(interval.start));
  scptr char *end_str = scheduler_time_stringify(&(interval.end));

  scptr htable_t *int_jsn = jsonh_make();

  jsonh_set_str(int_jsn, "start", (char *) mman_ref(start_str));
  jsonh_set_str(int_jsn, "end", (char *) mman_ref(end_str));
  jsonh_set_int(int_jsn, "identifier", interval.identifier);
  jsonh_set_int(int_jsn, "index", index);
  jsonh_set_bool(int_jsn, "active", interval.active);
  jsonh_set_bool(int_jsn, "disabled", interval.disabled);

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
  scptr dynarr_t *weekday_intervals = dynarr_make(SCHEDULER_MAX_INTERVALS_PER_DAY, SCHEDULER_MAX_INTERVALS_PER_DAY, mman_dealloc_nr);

  if (jsonh_set_arr(weekday, "intervals", (dynarr_t *) mman_ref(weekday_intervals)) != JOPRES_SUCCESS)
    mman_dealloc(weekday_intervals);

  scheduler_day_t targ_day = scheduler->daily_schedules[day];
  for (int j = 0; j < SCHEDULER_MAX_INTERVALS_PER_DAY; j++)
  {
    scptr htable_t *int_jsn = scheduler_interval_jsonify(j, targ_day.intervals[j]);
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
      web_socket_broadcast_event(WSE_INTERVAL_SCHED_ON, ev_arg);

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
      web_socket_broadcast_event(WSE_INTERVAL_SCHED_OFF, ev_arg);

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
    web_socket_broadcast_event(WSE_VALVE_TIMER_UPDATED, ev_args);
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

static void scheduler_eeprom_write_time(int *addr_ind, scheduler_time_t time)
{
  EEPROM.write((*addr_ind)++, time.hours);
  EEPROM.write((*addr_ind)++, time.minutes);
  EEPROM.write((*addr_ind)++, time.seconds);
}

static void scheduler_eeprom_read_time(int *addr_ind, scheduler_time_t *time)
{
  time->hours = EEPROM.read((*addr_ind)++);
  time->minutes = EEPROM.read((*addr_ind)++);
  time->seconds = EEPROM.read((*addr_ind)++);
}

void scheduler_eeprom_save(scheduler_t *scheduler)
{
  // Keep track of the current EEPROM address
  int addr_ind = 0;

  // Bytepacked disable state buffer
  static uint8_t disabled_states[SCHEDULER_NUM_DISABLED_BYTES];

  // Loop all days
  for (int i = 0; i < 7; i++)
  {
    // Loop all their slots
    for (int j = 0; j < SCHEDULER_MAX_INTERVALS_PER_DAY; j++)
    {
      scheduler_interval_t *slot = &(scheduler->daily_schedules[i].intervals[j]);

      // Save disabled state to buffer
      int int_index = i * SCHEDULER_MAX_INTERVALS_PER_DAY + j;
      uint8_t *tarb = &disabled_states[int_index / 8];
      if (slot->disabled)
        *tarb |= (1 << (int_index % 8));
      else
        *tarb &= ~(1 << (int_index % 8));

      // Write start, end and the identifier of this slot
      scheduler_eeprom_write_time(&addr_ind, slot->start);
      scheduler_eeprom_write_time(&addr_ind, slot->end);
      EEPROM.write(addr_ind++, slot->identifier);
    }
  }

  // Write disabled bytes
  for (int i = 0; i < SCHEDULER_NUM_DISABLED_BYTES; i++)
    EEPROM.write(addr_ind++, disabled_states[i]);

  // Write days disable byte
  uint8_t days_disableb = 0x00;
  for (int i = 0; i < 7; i++)
  {
    if (scheduler->daily_schedules[i].disabled)
      days_disableb |= (1 << (i % 8));
    else
      days_disableb &= ~(1 << (i % 8));
  }
  EEPROM.write(addr_ind++, days_disableb);

  EEPROM.commit();
}

void scheduler_eeprom_load(scheduler_t *scheduler)
{
  // Keep track of the current EEPROM address
  int addr_ind = 0;

  // Loop all days
  for (int i = 0; i < 7; i++)
  {
    // Loop all their slots
    for (int j = 0; j < SCHEDULER_MAX_INTERVALS_PER_DAY; j++)
    {
      scheduler_interval_t *slot = &(scheduler->daily_schedules[i].intervals[j]);

      // Read start, end and the identifier into this slot
      scheduler_eeprom_read_time(&addr_ind, &(slot->start));
      scheduler_eeprom_read_time(&addr_ind, &(slot->end));
      slot->identifier = EEPROM.read(addr_ind++);
    }
  }

  // Read disabled bytes
  for (int i = 0; i < SCHEDULER_NUM_DISABLED_BYTES; i++)
  {
    // Read state bits
    uint8_t dbyte = EEPROM.read(addr_ind++);
    for (int j = 0; j < 8; j++)
    {
      // Account for unused "padding bits"
      int int_index = i * 8 + j;
      if (int_index + 1 >= SCHEDULER_TOTAL_INTERVALS)
        break;

      // Apply bit to valve's disabled state
      int targ_day = int_index / SCHEDULER_MAX_INTERVALS_PER_DAY;
      int targ_int = int_index % SCHEDULER_MAX_INTERVALS_PER_DAY;
      scheduler->daily_schedules[targ_day].intervals[targ_int].disabled = (dbyte >> j) & 0x01;
    }
  }

  // Read days disable byte
  uint8_t day_disableb = EEPROM.read(addr_ind++);
  for (int i = 0; i < 7; i++)
    scheduler->daily_schedules[i].disabled = (day_disableb >> i) & 0x01;
}