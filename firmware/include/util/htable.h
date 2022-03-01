#ifndef htable_h
#define htable_h

#include <inttypes.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "util/mman.h"
#include "util/enumlut.h"
#include "util/atomanip.h"
#include "util/strclone.h"
#include "util/strfmt.h"
#include "util/common_types.h"

#define HTABLE_FNV_OFFSET 14695981039346656037UL
#define HTABLE_FNV_PRIME 1099511628211UL
#define HTABLE_MAX_KEYLEN 256
#define HTABLE_DUMP_LINEBUF 8
#define HTABLE_ITEMS_PER_SLOT 6

typedef void *(*htable_value_clone_f)(void *);

/**
 * @brief Used when a table is appended into another table
 */
typedef enum htable_append_mode
{
  HTABLE_AM_SKIP,                 // Skip the source's duplicate item
  HTABLE_AM_OVERRIDE,             // Override the destination's duplicate item
  HTABLE_AM_DUPERR                // Create a duplicate key error on duplicates
} htable_append_mode_t;

/**
 * @brief Represents htable operation results
 */
#define _EVALS_HTABLE_RESULT(FUN)                                                     \
  FUN(HTABLE_SUCCESS,            0x0) /* Operation has been successful */             \
  FUN(HTABLE_KEY_NOT_FOUND,      0x1) /* The requested key couldn't be located */     \
  FUN(HTABLE_KEY_ALREADY_EXISTS, 0x2) /* The requested key already exists */          \
  FUN(HTABLE_KEY_TOO_LONG,       0x3) /* The requested key has too many characters */ \
  FUN(HTABLE_FULL,               0x4) /* The table has reached it's defined limit */  \
  FUN(HTABLE_NULL_VALUE,         0x5) /* Tried to insert a null value */

ENUM_TYPEDEF_FULL_IMPL(htable_result, _EVALS_HTABLE_RESULT);

/**
 * @brief Represents an individual k-v pair entry in the table
 */
typedef struct htable_entry
{
  char *key;
  void *value;

  // Next link for the linked-list on this slot
  struct htable_entry *_next;
} htable_entry_t;

/**
 * @brief Represents a table having it's entries and a fixed size
 */
typedef struct
{
  // Actual table, list of entries
  htable_entry_t **slots;

  // Allocated number of slots
  size_t _slot_count;

  // Current number of items in the table
  size_t _item_count;

  // Maximum number of slots to be allocated when growing
  size_t _item_cap;

  // Cleanup function for the table items
  clfn_t _cf;
} htable_t;

/**
 * @brief Allocate a new, empty table
 * 
 * @param item_cap Maximum number of items stored
 * @param cf Cleanup function for the items
 * @return htable_t* Pointer to the new table
 */
htable_t *htable_make(size_t item_cap, clfn_t cf);

/**
 * @brief Insert a new item into the table
 * 
 * @param table Table reference
 * @param key Key to connect with the value
 * @param elem Pointer to the value
 * 
 * @return htable_result_t Result of this operation
 */
htable_result_t htable_insert(htable_t *table, const char *key, void *elem);

/**
 * @brief Check if the table already contains this key
 * 
 * @param table Table reference
 * @param key Key to check
 * 
 * @return true Key exists
 * @return false Key does not exist
 */
bool htable_contains(htable_t *table, const char *key);

/**
 * @brief Remove an element by it's key
 * 
 * @param table Table reference
 * @param key Key connected to the target value
 * 
 * @return htable_result_t Result of this operation
 */
htable_result_t htable_remove(htable_t *table, const char *key);

/**
 * @brief Get an existing key's connected value
 * 
 * @param table Table reference
 * @param key Key connected to the target value
 * @param output Output pointer buffer
 * 
 * @return htable_result_t Result of this operation
 */
htable_result_t htable_fetch(htable_t *table, const char *key, void **output);

/**
 * @brief Append a table's entries into another table
 * 
 * @param dest Destination to append to
 * @param src Source to append from
 * @param mode Mode of appending
 * @param cf Clone function used to copy over values
 * @return htable_result_t Operation result
 */
htable_result_t htable_append_table(htable_t *dest, htable_t *src, htable_append_mode_t mode, htable_value_clone_f cf);

/**
 * @brief Get a list of all existing keys inside the table
 * 
 * @param table Table reference
 * @param output String array pointer buffer
 * 
 * @returns Number of keys returned
 */
size_t htable_list_keys(htable_t *table, char ***output);

/**
 * @brief Dumps the current state of the table in a human readable format
 * 
 * @param table Table to dump
 * @param stringifier Stringifier function to apply for values, leave as NULL for internal casting
 * @return char* Formatted result string
 */
char *htable_dump_hr(htable_t *table, stringifier_t stringifier);

#endif