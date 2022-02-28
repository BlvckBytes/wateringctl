#ifndef strclone_h
#define strclone_h

#include <stddef.h>
#include <string.h>

#include "util/uminmax.h"
#include "util/strfmt.h"
#include "util/mman.h"

/**
 * @brief Clone a string safely by limiting it's length, returns NULL if the string was too large
 * 
 * @param origin Original string to be cloned
 * @param max_len Maximum length of the clone
 * @return char* Cloned string, NULL on errors
 */
char *strclone_s(const char *origin, size_t max_len);

/**
 * @brief Clone a string unsafely
 * 
 * @param origin Original string to be cloned
 * @return char* Cloned string, NULL on errors
 */
char *strclone(const char *origin);

#endif