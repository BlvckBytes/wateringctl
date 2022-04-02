#ifndef scheduler_time_h
#define scheduler_time_h

#include <EEPROM.h>

#include <blvckstd/strfmt.h>
#include <blvckstd/longp.h>
#include <blvckstd/partial_strdup.h>

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
char *scheduler_time_stringify(const scheduler_time_t *time);

/**
 * @brief Decrement a time safely (having a lower-bound, midnight) by a certain amount
 * 
 * @param time Time to decrement in place
 * @param seconds Amount of seconds to decrement
 */
void scheduler_time_decrement_bound(scheduler_time_t *time, size_t seconds);

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

#endif