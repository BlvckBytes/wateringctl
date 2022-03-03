#include "valve_control.h"

void valve_control_scheduler_routine(
  scheduler_edge_t edge,
  uint8_t identifier,
  scheduler_weekday_t curr_day,
  scheduler_time_t curr_time
)
{
  dbginf(
    "[%s %d:%d:%d]: %s occurred for %" PRIu16 "!\n",
    scheduler_weekday_name(curr_day),
    curr_time.hours, curr_time.minutes, curr_time.seconds,
    scheduler_edge_name(edge),
    identifier
  );
}