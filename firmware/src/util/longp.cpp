#include "util/longp.h"

longp_errno_t longp(long *out, const char *s, int base)
{
  // Either leading spaces or empty string, inconvertible
  char *end;
  if (s[0] == '\0' || isspace(s[0]))
    return LONGP_INCONVERTIBLE;

  // Invoke and clean errno beforehand
  errno = 0;
  long l = strtol(s, &end, base);

  // Check overflow conditions
  if (l > INT_MAX || (errno == ERANGE && l == LONG_MAX))
    return LONGP_OVERFLOW;

  // Check underflow conditions
  if (l < INT_MIN || (errno == ERANGE && l == LONG_MIN))
    return LONGP_UNDERFLOW;

  // End of number is not end of string (trailing chars)
  if (*end != '\0')
    return LONGP_INCONVERTIBLE;

  // Set number in out buffer
  *out = l;
  return LONGP_SUCCESS;
}