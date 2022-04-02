#ifndef scheduler_macros_h
#define scheduler_macros_h

// Maximum number of intervals a day can have
#define SCHEDULER_MAX_INTERVALS_PER_DAY 5

// Total intervals managed
#define SCHEDULER_TOTAL_INTERVALS (SCHEDULER_MAX_INTERVALS_PER_DAY * 7)

// Disabled states, bytepacked
#define SCHEDULER_NUM_DISABLED_BYTES ((SCHEDULER_TOTAL_INTERVALS + 7) / 8)

// Number of bytes the scheduler needs to persist it's schedule in the EEPROM
// 7 weekdays times max_per_day times 7 bytes per interval (3 start, 3 end, 1 identifier)
#define SCHEDULER_EEPROM_FOOTPRINT (                            \
  7 * SCHEDULER_TOTAL_INTERVALS   /* Intervals */               \
  + SCHEDULER_NUM_DISABLED_BYTES  /* Interval disabled bytes */ \
  + 1                             /* Day disable bits */        \
)

#endif