#ifndef scheduler_h
#define scheduler_h

#include <inttypes.h>
#include <Arduino.h>
#include <EEPROM.h>

#include <blvckstd/enumlut.h>
#include <blvckstd/jsonh.h>
#include <blvckstd/mman.h>
#include <blvckstd/strfmt.h>
#include <blvckstd/partial_strdup.h>
#include <blvckstd/longp.h>

/*
  The scheduler schedules on-times over the period of one
  week, where each weekday may have multiple on-times.

  When an on-time is starting or ending, the scheduler's
  callback fires providing the new state as well as an identifier.

  The identifier has to be provided when an interval is being created.
*/

// Maximum number of intervals a day can have
#define SCHEDULER_MAX_INTERVALS_PER_DAY 5

// Number of bytes the scheduler needs to persist it's schedule in the EEPROM
// 7 weekdays times max_per_day times 7 bytes per interval (3 start, 3 end, 1 identifier)
#define SCHEDULER_EEPROM_FOOTPRINT (7 * SCHEDULER_MAX_INTERVALS_PER_DAY * 7)

// Day in the week
#define _EVALS_SCHEDULER_WEEKDAY(FUN)   \
  FUN(WEEKDAY_SU, 0x00) /* Sunday */    \
  FUN(WEEKDAY_MO, 0x01) /* Monday */    \
  FUN(WEEKDAY_TU, 0x02) /* Tuesday */   \
  FUN(WEEKDAY_WE, 0x03) /* Wednesday */ \
  FUN(WEEKDAY_TH, 0x04) /* Thursday */  \
  FUN(WEEKDAY_FR, 0x05) /* Friday */    \
  FUN(WEEKDAY_SA, 0x06) /* Saturday */

// Edge for describing interval begin or end events
#define _EVALS_SCHEDULER_EDGE(FUN)      \
  FUN(EDGE_OFF_TO_ON, 0x00)             \
  FUN(EDGE_ON_TO_OFF, 0x01)

ENUM_TYPEDEF_FULL_IMPL(scheduler_weekday, _EVALS_SCHEDULER_WEEKDAY);
ENUM_TYPEDEF_FULL_IMPL(scheduler_edge, _EVALS_SCHEDULER_EDGE);

typedef struct scheduler_time
{
  uint8_t hours;                       // Hours of the target time
  uint8_t minutes;                     // Minutes of the target time
  uint8_t seconds;                     // Seconds of the target time
} scheduler_time_t;

const scheduler_time_t SCHEDULER_TIME_MIDNIGHT = { 00, 00, 00 };

/**
 * @brief Stringify a time into the common "hh:mm:ss" format
 * 
 * @param time Time to stringify
 * @return char* Stringified time, mman-alloced
 */
char *scheduler_time_stringify(scheduler_time_t *time);

/**
 * @brief Parse a scheduler time from a given string using the
 * format <hours>:<minutes>:<seconds>, where each block needs to be an
 * integer between 0 and either 23 or 59, but blocks don't need to be
 * padded with zeros to match two digits
 * 
 * @param str Input string
 * @param err Error output buffer
 * @param out Output value buffer
 * 
 * @return true Parsing success
 * @return false On parsing errors, see err
 */
bool scheduler_time_parse(const char *str, char **err, scheduler_time_t *out);

/**
 * @brief Compare two scheduler times
 * 
 * @param a First scheduler time
 * @param b Second scheduler time
 * 
 * @return int 0 on equality, -1 if a occurrs before b, 1 if a occurrs after b
 */
int scheduler_time_compare(scheduler_time_t a, scheduler_time_t b);

// This function is used by the scheduler to get the current time and weekday from an external provider
typedef void (*scheduler_day_and_time_provider_t)(scheduler_weekday_t*, scheduler_time_t*);

typedef struct scheduler_interval
{
  scheduler_time_t start;              // Start of ON-time interval
  scheduler_time_t end;                // End of ON-time interval
  uint8_t identifier;                 // Identifier provided in the scheduler's callback
  bool active;                         // Whether or not this interval is currently active
} scheduler_interval_t;

/**
 * @brief Parsse an interval's writable values from json, using the following schema:
 * 
 * {
 *   "start": "<hours>:<minutes>:<seconds>",
 *   "end": "<hours>:<minutes>:<seconds>",
 *   "identifier": <integer>
 * }
 * 
 * @param json JSONH json node
 * @param err Error output buffer
 * @param out Value output buffer
 * 
 * @return true Parsing successful
 * @return false Parsing error, see err
 */
bool scheduler_interval_parse(htable_t *json, char **err, scheduler_interval_t *out);

/**
 * @brief Compare two scheduler intervals
 * 
 * @param a First scheduler interval
 * @param b Second scheduler interval
 * 
 * @return true Both intervals have the same time and target the same identifiers
 * @return false The intervals differ in some way
 */
bool scheduler_interval_equals(scheduler_interval_t a, scheduler_interval_t b);

/**
 * @brief Create a scheduler interval from it's user-parameters
 * 
 * @param start Time to start the interval
 * @param end Time to end the interval
 * @param identifier Identifier used for the callback
 */
scheduler_interval_t scheduler_interval_make(scheduler_time_t start, scheduler_time_t end, uint8_t identifier);

// Constant for an empty interval slot
const scheduler_interval_t SCHEDULER_INTERVAL_EMPTY = { SCHEDULER_TIME_MIDNIGHT, SCHEDULER_TIME_MIDNIGHT, 0x0, false };

// This callback provides the user with the occurring edge as well as the identifier
typedef void (*scheduler_callback_t)(scheduler_edge_t, uint8_t, scheduler_weekday_t, scheduler_time_t);

typedef struct scheduler
{
  scheduler_interval_t daily_schedules[7][SCHEDULER_MAX_INTERVALS_PER_DAY];  // Mapping days of the week to their schedules
  scheduler_callback_t callback;                                             // Callback used for interval state change reporting
  scheduler_day_and_time_provider_t dt_provider;                             // External day/time provider function

  scheduler_time_t last_tick_time;        // Time at which the last tick occurred
  scheduler_weekday_t last_tick_day;      // Day at which the last tick occurred
} scheduler_t;

/**
 * @brief Create a new scheduler with empty interval-lists
 * 
 * @param callback Callback for interval events
 * @param dt_provider External day/time provider
 */
scheduler_t scheduler_make(scheduler_callback_t callback, scheduler_day_and_time_provider_t dt_provider);

/**
 * @brief Register a new interval for a given day of the week
 * 
 * @param scheduler Scheduler handle
 * @param day Day of the week
 * @param interval Interval to register
 * 
 * @return true Interval registered successfully
 * @return false No interval slots remaining
 */
bool scheduler_register_interval(scheduler_t *scheduler, scheduler_weekday_t day, scheduler_interval_t interval);

/**
 * @brief Unregister a previously registered interval from a given day of the week
 * 
 * @param scheduler Scheduler handle
 * @param day Day of the week
 * @param interval Interval to unregister
 * 
 * @return true Interval unregistered successfully
 * @return false Could not find the target interval
 */
bool scheduler_unregister_interval(scheduler_t *scheduler, scheduler_weekday_t day, scheduler_interval_t interval);

/**
 * @brief Change a previously registered interval on a given day of the week
 * 
 * @param scheduler Scheduler handle
 * @param day Day of the week
 * @param from Currently registered interval
 * @param to New interval
 * 
 * @return true Interval unregistered successfully
 * @return false Could not find the target interval
 */
bool scheduler_change_interval(scheduler_t *scheduler, scheduler_weekday_t day, scheduler_interval_t from, scheduler_interval_t to);

/**
 * @brief Update the scheduler's internals, should be called in some kind of main-loop
 * 
 * @param scheduler Scheduler to tick
 */
void scheduler_tick(scheduler_t *scheduler);

/**
 * @brief Save a scheduler's schedule to the eeprom
 */
void scheduler_eeprom_save(scheduler_t *scheduler);

/**
 * @brief Load a scheduler's schedule from the eeprom
 */
void scheduler_eeprom_load(scheduler_t *scheduler);

/**
 * @brief Print a scheduler's schedule to the serial monitor
 */
void scheduler_schedule_print(scheduler_t *scheduler);

#endif