#include "scheduler_time.h"

char *scheduler_time_stringify(const scheduler_time_t *time)
{
  return strfmt_direct("%02d:%02d:%02d", time->hours, time->minutes, time->seconds);
}

void scheduler_time_decrement_bound(scheduler_time_t *time, size_t seconds)
{
  // Calculate hours, minutes and seconds deltas
  size_t delta_h = seconds / 3600;
  size_t delta_m = (seconds % 3600) / 60;
  size_t delta_s = seconds % 3600 % 60;

  // Subtract hours with bound, as there is nothing to borrow from
  time->hours = time->hours >= delta_h ? time->hours - delta_h : 0;

  // Subtract minutes since there are plenty left
  if (time->minutes >= delta_m)
    time->minutes -= delta_m;

  // Subtract minutes with borrow
  else
  {
    // Nothing to borrow, cap at zero
    if (time->hours == 0)
    {
      time->minutes = 0;
    }

    // Borrow an hour and subtract the missing difference
    else
    {
      time->hours--;
      time->minutes = 60 - (delta_m - time->minutes);
    }
  }

  // Subtract seconds since there are plenty left
  if (time->seconds >= delta_s)
    time->seconds -= delta_s;

  // Subtract seconds with borrow
  else
  {
    if (time->minutes == 0)
    {
      // Nothing to borrow, cap at zero
      if (time->hours == 0)
        time->seconds = 0;

      // Borrow an hour, refill minutes and subtract the missing difference
      else
      {
        time->hours--;
        time->minutes = 59;
        time->seconds = 60 - (delta_s - time->seconds);
      }
    }

    // Borrow a minute and subtract the missing difference
    else
    {
      time->minutes--;
      time->seconds = 60 - (delta_s - time->seconds);
    }
  }
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