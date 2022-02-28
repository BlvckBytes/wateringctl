#ifndef mman_h
#define mman_h

#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#include "util/compattrs.h"
#include "util/atomanip.h"
#include "util/dbglog.h"
#include "util/common_types.h"

/*
============================================================================
                                   Macros                                   
============================================================================
*/

// Marks a memory managed variable
#define scptr __attribute__((cleanup(mman_dealloc_attr)))

/*
============================================================================
                                  Typedefs                                  
============================================================================
*/

// Forward ref
typedef struct mman_meta mman_meta_t;

/**
 * @brief Cleanup function used for external destructuring of a resource
 */
typedef void (*mman_cleanup_f_t)(mman_meta_t *);

/**
 * @brief Meta-information of a memory managed resource
 */
typedef struct mman_meta
{
  // Pointer to the resource
  void *ptr;

  // Size of one data block
  size_t block_size;

  // Number of blocks with block_size
  size_t num_blocks;

  // Cleanup function invoked before the whole malloc gets free'd
  // INFO: This is used in conjunction with mman_alloc resources
  mman_cleanup_f_t cf;

  // Cleanup function invoked before the whole malloc gets free'd
  // INFO: This is used in conjunction with mman_wrap resources
  clfn_t cf_wrapped;

  // Number of active references pointing at this resource
  volatile size_t refs;
} mman_meta_t;

typedef enum mman_result
{
  MMAN_NULLREF,             // Null reference received
  MMAN_INVREF,              // Invalid reference received (not mman allocated)
  MMAN_STILL_USED,          // This resource is still in use
  MMAN_DEALLOCED            // Successfully deallocated
} mman_result_t;

/*
============================================================================
                                 Meta Info                                  
============================================================================
*/

/**
 * @brief Fetch the meta-block of a managed resource
 * 
 * @param ptr Pointer to the data-block of a managed resource
 * @return mman_meta_t* Retrieved meta-block or NULL if the resource is invalid
 */
mman_meta_t *mman_fetch_meta(void *ptr);

/*
============================================================================
                                 Allocation                                 
============================================================================
*/

/**
 * @brief Allocate memory and get a managed reference to it
 * 
 * @param block_size Size of one data block
 * @param size Number of blocks to allocate
 * @param cf Function for additional cleanup operations on pointers inside the data-block,
 * leave this as NULL when none exist and nothing has been allocated separately
 * @return void* Pointer to the resource, NULL if no space left
 */
void *mman_alloc(size_t block_size, size_t num_blocks, mman_cleanup_f_t cf);

/**
 * @brief Wrap an existing pointer to also be managed in an mman-style
 * 
 * @param ptr Pointer to be wrapped
 * @param cf Cleanup function for that pointer
 * @return void** Pointer to the managed pointer
 */
void **mman_wrap(void *ptr, clfn_t cf);

/**
 * @brief Allocate zero-initialized memory and get a managed reference to it
 * 
 * @param block_size Size of one data block
 * @param size Number of blocks to allocate
 * @param cf Function for additional cleanup operations on pointers inside the data-block,
 * leave this as NULL when none exist and nothing has been allocated separately
 * @return void* Pointer to the resource, NULL if no space left
 */
void *mman_calloc(size_t block_size, size_t num_blocks, mman_cleanup_f_t cf);

/**
 * @brief Reallocate a managed datablock
 * 
 * @param ptr_ptr Pointer to the pointer to the resource
 * @param new_size New size of the data block
 * @return mman_meta_t* Pointer to the leading meta-block of the new data-block
 */
mman_meta_t *mman_realloc(void **ptr_ptr, size_t block_size, size_t num_blocks);

/*
============================================================================
                                Deallocation                                
============================================================================
*/

/**
 * @brief Deallocates a mman allocated resource manually
 * 
 * WARNING: This ignores the number of references and can be treated
 * as a force deallocator!
 * 
 * @param ptr Pointer to the resource
 * 
 * @returns mman_result_t Operation result
 */
mman_result_t mman_dealloc_force(void *ptr);

/**
 * @brief Deallocate a managed resource when it goes out of scope and
 * has no references left pointing at it. Returns the operation result.
 * 
 * @param ptr Pointer to the resource
 * 
 * @returns mman_result_t Operation result
 */
mman_result_t mman_dealloc(void *ptr);

/**
 * @brief Deallocate a managed resource when it goes out of scope and
 * has no references left pointing at it. Doesn't return an operation result.
 * 
 * @param ptr Pointer to the resource
 */
void mman_dealloc_nr(void *ptr);

/**
 * @brief Deallocate a managed resource when it goes out of scope and
 * has no references left pointing at it.
 * 
 * WARNING: Only to be used by compiler attributes!
 * 
 * @param ptr_ptr Pointer to the pointer to the resource
 */
void mman_dealloc_attr(void *ptr_ptr);

/*
============================================================================
                                 Referencing                                
============================================================================
*/

/**
 * @brief Create a new reference to be shared with other consumers
 * 
 * @param ptr Pointer to the managed resource
 * @return void* Pointer to be shared
 */
void *mman_ref(void *ptr);

/*
============================================================================
                                  Debugging                                 
============================================================================
*/

/**
 * @brief Prints informations about the current alloc/dealloc status on stdout
 */
void mman_print_info();

#endif