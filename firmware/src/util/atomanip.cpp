#include "util/atomanip.h"

size_t atomic_add(volatile size_t *target, const size_t value)
{
  size_t old, n;

  // Try to compare and swap atomically until succeeded
  do {
    old = *target;
    n = old + value;
  } while (!__sync_bool_compare_and_swap(target, old, n));

  // Return the new value
  return n;
}

size_t atomic_increment(volatile size_t *target)
{
  return atomic_add(target, 1);
}

size_t atomic_decrement(volatile size_t *target)
{
  return atomic_add(target, -1);
}