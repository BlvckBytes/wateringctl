#ifndef dynarr_h
#define dynarr_h

#include <stddef.h>
#include <stdbool.h>

#include "util/mman.h"
#include "util/enumlut.h"
#include "util/strfmt.h"
#include "util/common_types.h"

/**
 * @brief Represents the dynamic array, keeping track of it's
 * size and cleanup method
 */
typedef struct
{
  // Actual table
  void **items;

  // Current allocated size of the array
  size_t _array_size;

  // Maximum size the array can grow to
  size_t _array_cap;

  // Cleanup function for the array items
  clfn_t _cf;
} dynarr_t;

#define _EVALS_DYNARR_RES(FUN)                                                \
  FUN(DYNARR_SUCCESS,          0x0) /* Successful operation */                \
  FUN(DYNARR_INDEX_NOT_FOUND,  0x1) /* Key at requested index not existing */ \
  FUN(DYNARR_FULL,             0x2) /* No more space for more items */        \
  FUN(DYNARR_EMPTY,            0x3) /* No more item to pop */

ENUM_TYPEDEF_FULL_IMPL(dynarr_result, _EVALS_DYNARR_RES);

/**
 * @brief Make a new, empty array
 * 
 * @param array_size Size of the array
 * @param array_max_size Maximum size of the array, set to array_size for no automatic growth
 * @param cf Cleanup function for the items
 * @return dynarr_t* Pointer to the new array
 */
dynarr_t *dynarr_make(size_t array_size, size_t array_max_size, clfn_t cf);

/**
 * @brief Push a new item into the array
 * 
 * @param arr Array reference
 * @param item Item to push
 * @param slot Slot that has been pushed to, set to NULL if not needed
 * @return dynarr_result_t Operation result
 */
dynarr_result_t dynarr_push(dynarr_t *arr, void *item, size_t *slot);

/**
 * @brief Set an item at a specific location in the array
 * 
 * @param arr Array reference
 * @param index Array index
 * @param item Item to set
 * @return dynarr_result_t Operation result
 */
dynarr_result_t dynarr_set_at(dynarr_t *arr, size_t index, void *item);

/**
 * @brief Remove an item at a specific location from the array
 * 
 * @param arr Array reference
 * @param index Array index
 * @return dynarr_result_t Operation result
 */
dynarr_result_t dynarr_remove_at(dynarr_t *arr, size_t index, void **out);

/**
 * @brief Dumps the current state of the array in a human readable format
 * 
 * @param arr Array to dump
 * @param stringifier Stringifier function to apply for values, leave as NULL for internal casting
 * @return char* Formatted result string
 */
char *dynarr_dump_hr(dynarr_t *arr, stringifier_t stringifier);

/**
 * @brief Get as a standard C array
 * 
 * @param arr Array to get
 * @param out Buffer of result
 * @return size_t Size of resulting array
 */
size_t dynarr_as_array(dynarr_t *arr, void ***out);

/**
 * @brief Clear the whole array by clearing all individual slots
 * 
 * @param arr Array to clear
 */
void dynarr_clear(dynarr_t *arr);

#endif