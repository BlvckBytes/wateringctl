#ifndef valve_control_h
#define valve_control_h

#include "scheduler.h"

void valve_control_scheduler_routine(
  scheduler_edge_t edge,
  uint8_t identifier,
  scheduler_weekday_t curr_day,
  scheduler_time_t curr_time
);

#endif