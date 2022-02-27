#ifndef strfmt_h
#define strfmt_h

#include <stdbool.h>
#include <stdarg.h>
#include <string.h>

#include "util/mman.h"

// String wrapped in quotes used in combination with printf / logging
#define QUOTSTR "\"%s\""

// Replace empty strings with a questionmark
#define STRFMT_EMPTYMARK(str) strlen(str) == 0 ? "?" : str

/**
 * @brief Format a string and re-allocate it's buffer dynamically as needed
 * 
 * @param buf Output buffer pointer, has to be allocated externally
 * @param offs Offset in this buffer, leave at NULL for no offset
 * @param fmt Format string
 * @param ... Arguments for the format
 * 
 * @return true Result has been written to the buffer
 * @return false Couldn't allocate space or the buffer/format were NULL
 */
bool strfmt(char **buf, size_t *offs, const char *fmt, ...);

/**
 * @brief Format a string and re-allocate it's buffer dynamically as needed
 * 
 * @param buf Output buffer pointer, has to be allocated externally
 * @param offs Offset in this buffer, leave at NULL for no offset
 * @param fmt Format string
 * @param ap Arguments for the format
 * 
 * @return true Result has been written to the buffer
 * @return false Couldn't allocate space or the buffer/format were NULL
 */
bool vstrfmt(char **buf, size_t *offs, const char *fmt, va_list ap);

/**
 * @brief Format a string and directly get the result, without having to
 * provide a buffer as well as an offset tracker
 * 
 * @param fmt Format string
 * @param ... Arguments for the format
 * @return char* Formatted string or NULL on errors
 */
char *strfmt_direct(const char *fmt, ...);

#endif