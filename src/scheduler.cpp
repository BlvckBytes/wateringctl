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

scheduler_interval_t scheduler_interval_make(scheduler_time_t start, scheduler_time_t end, uint16_t identifier)
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