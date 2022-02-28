#include "util/strclone.h"

char *strclone_s(const char *origin, size_t max_len)
{
  // Validate that string length is within constraints
  size_t len = strlen(origin);
  if (len > max_len) return NULL;

  // Create a "carbon copy"
  scptr char *clone = (char *) mman_alloc(sizeof(char), len + 1, NULL);
  for (size_t i = 0; i < len; i++)
    clone[i] = origin[i];
  clone[len] = 0;

  return (char *) mman_ref(clone);
}

char *strclone(const char *origin)
{
  return strfmt_direct("%s", origin);
}