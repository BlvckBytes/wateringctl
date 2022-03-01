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

char *scheduler_time_stringify(scheduler_time_t *time)
{
  return strfmt_direct("%02d:%02d:%02d", time->hours, time->minutes, time->seconds);
}

/**
 * @brief Subroutine to parse a time-part: 00-max
 * 
 * @param part Time-part string to parse
 * @param part_name Name of this part, used for error messages
 * @param err Error output buffer
 * @param max Maximum value this part can take
 * @param out Output value buffer
 * 
 * @return true Parsing successful
 * @return false Parsing error, see err
 */
INLINED static bool scheduler_time_parse_part(
  char *part,
  const char *part_name,
  char **err,
  long max,
  uint8_t *out
)
{
  long result;
  if (
    longp(&result, part, 10) != LONGP_SUCCESS  // No number
    || !(result >= 0 && result <= max)         // Or not between 0-max
  )
  {
    *err = strfmt_direct(QUOTSTR " of " QUOTSTR " is not valid (0-%ld)", part_name, part, max);
    return false;
  }

  *out = (uint8_t) result;
  return true;
}

bool scheduler_time_parse(const char *str, char **err, scheduler_time_t *out)
{
  size_t str_offs = 0;
  scptr char *hours_s = partial_strdup(str, &str_offs, ":", false);
  scptr char *minutes_s = partial_strdup(str, &str_offs, ":", false);
  scptr char *seconds_s = partial_strdup(str, &str_offs, "\0", false);

  // Missing any time-part
  if (!hours_s || !minutes_s || !seconds_s)
  {
    *err = strfmt_direct("Invalid format, use HH:MM:SS");
    return false;
  }

  scheduler_time_t result = { 00, 00, 00 };

  // Parse hours
  if (!scheduler_time_parse_part(hours_s, "Hours", err, 23, &(result.hours)))
    return false;

  // Parse minutes
  if (!scheduler_time_parse_part(minutes_s, "Minutes", err, 59, &(result.minutes)))
    return false;

  // Parse seconds
  if (!scheduler_time_parse_part(seconds_s, "Seconds", err, 59, &(result.seconds)))
    return false;

  *out = result;
  return true;
}

int scheduler_time_compare(scheduler_time_t a, scheduler_time_t b)
{
  // Hours differ, compare hours
  if (a.hours > b.hours) return 1;
  if (a.hours < b.hours) return -1;

  // Hours equal, minutes differ, compare minutes
  if (a.minutes > b.minutes) return 1;
  if (a.minutes < b.minutes) return -1;

  // Minutes equal, seconds differ, compare seconds
  if (a.seconds > b.seconds) return 1;
  if (a.seconds < b.seconds) return -1;

  // All values equal, times are equal
  return 0;
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

  // Get identifier
  int identifier;
  jsonh_opres_t jopr;
  if ((jopr = jsonh_get_int(json, "identifier", &identifier)) != JOPRES_SUCCESS)
  {
    *err = jsonh_getter_errstr("identifier", jopr);
    return false;
  }

  // Build result
  *out = scheduler_interval_make(start, end, identifier);
  return true;
}

/**
 * @brief Check if a given interval is equal to the empty interval constant
 */
static bool scheduler_interval_empty(scheduler_interval_t interval)
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

scheduler_interval_t scheduler_interval_make(scheduler_time_t start, scheduler_time_t end, uint8_t identifier)
{
  return (scheduler_interval_t) {
    .start = start,
    .end = end,
    .identifier = identifier,
    false
  };
}

static int scheduler_find_interval_slot(scheduler_t *scheduler, scheduler_weekday_t day, scheduler_interval_t interval)
{
  // Loop all interval slots for the target day
  for (size_t i = 0; i < SCHEDULER_MAX_INTERVALS_PER_DAY; i++)
  {
    scheduler_interval_t slot = scheduler->daily_schedules[day][i];

    // Not the target interval
    if (!scheduler_interval_equals(slot, interval)) continue;

    return i;
  }

  // Target interval does not exist
  return -1;
}

bool scheduler_register_interval(scheduler_t *scheduler, scheduler_weekday_t day, scheduler_interval_t interval)
{
  // Find an empty slot
  int slot = scheduler_find_interval_slot(scheduler, day, SCHEDULER_INTERVAL_EMPTY);

  // No more empty slots remaining
  if (slot < 0) return false;

  // Set the slot
  scheduler->daily_schedules[day][slot] = interval;
  return true;
}

bool scheduler_unregister_interval(scheduler_t *scheduler, scheduler_weekday_t day, scheduler_interval_t interval)
{
  int slot = scheduler_find_interval_slot(scheduler, day, interval);

  // This interval is not registered
  if (slot < 0) return false;

  // Unregister by setting the slot to an empty value
  scheduler->daily_schedules[day][slot] = SCHEDULER_INTERVAL_EMPTY;
  return true;
}

bool scheduler_change_interval(scheduler_t *scheduler, scheduler_weekday_t day, scheduler_interval_t from, scheduler_interval_t to)
{
  int slot = scheduler_find_interval_slot(scheduler, day, from);

  // This interval is not registered
  if (slot < 0) return false;

  // Change the interval value
  scheduler->daily_schedules[day][slot] = to;
  return true;
}

void scheduler_tick(scheduler_t *scheduler)
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

  // Loop all intervals of the day
  for (size_t i = 0; i < SCHEDULER_MAX_INTERVALS_PER_DAY; i++)
  {
    scheduler_interval_t *interval = &(scheduler->daily_schedules[day][i]);

    // Skip empty slots
    if (scheduler_interval_empty(*interval)) continue;

    // Interval turned on
    if (
      scheduler_time_compare(time, interval->start) == 1      // Time is after start
      && scheduler_time_compare(time, interval->end) == -1    // And time is before end
      && !interval->active                                    // And interval is not already active
    )
    {
      interval->active = true;
      scheduler->callback(EDGE_OFF_TO_ON, interval->identifier, day, time);
      continue;
    }

    // Interval turned off
    if (
      scheduler_time_compare(time, interval->end) == 1        // Time is after end
      && interval->active                                     // And interval is active
    )
    {
      interval->active = false;
      scheduler->callback(EDGE_ON_TO_OFF, interval->identifier, day, time);
      continue;
    }
  }

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
  // Keep track of the current EEPROM writing-address
  int addr_ind = 0;

  // Loop all days
  for (int i = 0; i < 7; i++)
  {
    // Loop all their slots
    for (int j = 0; j < SCHEDULER_MAX_INTERVALS_PER_DAY; j++)
    {
      scheduler_interval_t *slot = &(scheduler->daily_schedules[i][j]);

      // Write start, end and the identifier of this slot
      scheduler_eeprom_write_time(&addr_ind, slot->start);
      scheduler_eeprom_write_time(&addr_ind, slot->end);
      EEPROM.write(addr_ind++, slot->identifier);
    }
  }

  EEPROM.commit();
}

void scheduler_eeprom_load(scheduler_t *scheduler)
{
  // Keep track of the current EEPROM writing-address
  int addr_ind = 0;

  // Loop all days
  for (int i = 0; i < 7; i++)
  {
    // Loop all their slots
    for (int j = 0; j < SCHEDULER_MAX_INTERVALS_PER_DAY; j++)
    {
      scheduler_interval_t *slot = &(scheduler->daily_schedules[i][j]);

      // Read start, end and the identifier into this slot
      scheduler_eeprom_read_time(&addr_ind, &(slot->start));
      scheduler_eeprom_read_time(&addr_ind, &(slot->end));
      slot->identifier = EEPROM.read(addr_ind++);
    }
  }
}

void scheduler_schedule_print(scheduler_t *scheduler)
{
  Serial.println("====================< Schedule >====================");
  // Loop all days
  for (int i = 0; i < 7; i++)
  {
    // Print name of the day
    Serial.printf("%s:\n", scheduler_weekday_name((scheduler_weekday_t) i));

    // Loop all their slots
    for (int j = 0; j < SCHEDULER_MAX_INTERVALS_PER_DAY; j++)
    {
      scheduler_interval_t slot = scheduler->daily_schedules[i][j];

      // Print this slot with all it's properties
      Serial.printf(
        "[%d] start=%02u:%02u:%02u, end=%02u:%02u:%02u, id=%03u, active=%s\n",
        j,
        slot.start.hours, slot.start.minutes, slot.start.seconds,
        slot.end.hours, slot.end.minutes, slot.end.seconds,
        slot.identifier,
        slot.active ? "yes" : "no"
      );
    }

    // Keep an empty line between days
    if (i != 6)
      Serial.println();
  }
  Serial.println("====================< Schedule >====================");
}