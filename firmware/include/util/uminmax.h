#ifndef uminmax_h
#define uminmax_h

#include <inttypes.h>

/**
 * @brief Find the minimum value of two u64's
 * 
 * @param a First value
 * @param b Second value
 * @return uint64_t Smaller value of the two
 */
uint64_t u64_min(uint64_t a, uint64_t b);

/**
 * @brief Find the maximum value of two u64's
 * 
 * @param a First value
 * @param b Second value
 * @return uint64_t Greater value of the two
 */
uint64_t u64_max(uint64_t a, uint64_t b);

#endif