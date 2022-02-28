#include "util/uminmax.h"

uint64_t u64_min(uint64_t a, uint64_t b)
{
  return (a < b) ? a : b;
}

uint64_t u64_max(uint64_t a, uint64_t b)
{
  return (a > b) ? a : b;
}