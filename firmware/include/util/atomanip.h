#ifndef atomanip_h
#define atomanip_h

#include <stddef.h>

/**
 * @brief Atomically add to a given number
 * 
 * @param target Target number to add to
 * @param value Value to add
 * @return size_t New value of the variable
 */
size_t atomic_add(volatile size_t *target, const size_t value);

/**
 * @brief Atomically add one to a given number
 * 
 * @param target Target number to increment
 * @return size_t New value of the variable
 */
size_t atomic_increment(volatile size_t *target);

/**
 * @brief Atomically remove one from a given number
 * 
 * @param target Target number to decrement
 * @return size_t New value of the variable
 */
size_t atomic_decrement(volatile size_t *target);

#endif