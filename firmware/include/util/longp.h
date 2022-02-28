#ifndef longp_h
#define longp_h

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

typedef enum {
  LONGP_SUCCESS,
  LONGP_OVERFLOW,
  LONGP_UNDERFLOW,
  LONGP_INCONVERTIBLE
} longp_errno_t;

/**
 * @brief Converts a string to a long
 * 
 * @param out Output buffer pointer
 * @param s String to parse
 * @param base Base to parse in
 * @return longp_errno_t Result of operation
 */
longp_errno_t longp(long *out, const char *s, int base);

#endif