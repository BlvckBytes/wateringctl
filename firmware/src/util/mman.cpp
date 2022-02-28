#include "util/mman.h"

static volatile size_t mman_alloc_count, mman_dealloc_count;

/*
============================================================================
                                 Meta Info                                  
============================================================================
*/

mman_meta_t *mman_fetch_meta(void *ptr)
{
  // Do nothing for nullptrs
  if (ptr == NULL) return NULL;

  // Fetch the meta info allocated before the data block and
  // assure that it's actually a managed resource
  mman_meta_t *meta = (mman_meta_t *) ((char *) ptr - sizeof(mman_meta_t));
  if (ptr != meta->ptr)
  {
    dbgerr("Invalid resource passed to \"mman_fetch_meta\"!\n");
    return NULL;
  }

  return meta;
}

/*
============================================================================
                                 Allocation                                 
============================================================================
*/

/**
 * @brief Allocate a new meta-info structure as well as it's trailing data block
 * 
 * @param block_size Size of one data block in bytes
 * @param num_blocks Number of blocks with block_size
 * @param zero_init Whether or not to zero-initialize all blocks
 * @param cf Cleanup function
 * @return mman_meta_t Pointer to the meta-info
 */
INLINED static mman_meta_t *mman_create(
  size_t block_size,
  size_t num_blocks,
  bool zero_init,
  mman_cleanup_f_t cf,
  clfn_t cf_wrapped
)
{
  mman_meta_t *meta = (mman_meta_t *) malloc(
    sizeof(mman_meta_t) // Meta information
    + (block_size * num_blocks) // Data blocks
  );

  *meta = (mman_meta_t) {
    .ptr = meta + 1,
    .block_size = block_size,
    .num_blocks = num_blocks,
    .cf = cf,
    .cf_wrapped = cf_wrapped,
    .refs = 1
  };

  if (zero_init)
    memset(meta->ptr, 0x0, num_blocks * block_size);

  return meta;
}

void *mman_alloc(size_t block_size, size_t num_blocks, mman_cleanup_f_t cf)
{
  // INFO: Increment the allocation count for debugging purposes
  atomic_increment(&mman_alloc_count);

  // Create new meta-info and return a pointer to the data block
  return mman_create(block_size, num_blocks, false, cf, NULL)->ptr;
}

void **mman_wrap(void *ptr, clfn_t cf)
{
  // INFO: Increment the allocation count for debugging purposes
  atomic_increment(&mman_alloc_count);

  // Create new meta-info and return a pointer to the data block
  // The data-block is a pointer to the pointer that's being wrapped
  // It will point to the passed-in ptr
  mman_meta_t *meta = mman_create(sizeof(void *), 1, false, NULL, cf);
  void **refptr = (void **) meta->ptr;
  *refptr = ptr;

  return refptr;
}

void *mman_calloc(size_t block_size, size_t num_blocks, mman_cleanup_f_t cf)
{
  // INFO: Increment the allocation count for debugging purposes
  atomic_increment(&mman_alloc_count);

  // Create new meta-info and return a pointer to the data block
  return mman_create(block_size, num_blocks, true, cf, NULL)->ptr;
}

mman_meta_t *mman_realloc(void **ptr_ptr, size_t block_size, size_t num_blocks)
{
  // Receiving a pointer to the pointer to the reference, deref once
  void *ptr = *ptr_ptr;

  // Fetch the meta info allocated before the data block
  mman_meta_t *meta = mman_fetch_meta(ptr);
  if (ptr != meta->ptr)
  {
    dbgerr("ERROR: Invalid resource passed to \"mman_realloc\"!\n");
    return NULL;
  }

  // Reallocate whole meta object
  meta = (mman_meta_t *) realloc(meta,
    sizeof(mman_meta_t) // Meta information
    + (block_size * num_blocks) // Data blocks
  );

  // Update the copied meta-block
  meta->ptr = meta + 1;
  meta->block_size = block_size;
  meta->num_blocks = num_blocks;

  // Update the outside pointer
  *ptr_ptr = meta->ptr;
  return meta;
}

/*
============================================================================
                                Deallocation                                
============================================================================
*/

mman_result_t mman_dealloc_force(void *ptr)
{
  mman_meta_t *meta = mman_fetch_meta(ptr);
  if (!meta)
  {
    dbgerr("ERROR: mman_dealloc_force received unknown ref!\n");
    return MMAN_INVREF;
  }

  // Call additional cleanup function on the meta-block
  if (meta->cf) meta->cf(meta);

  // Call additional cleanup function on the wrapped pointer
  // This means derefing the pointer to the pointer that's to be passed to cf_wrapped
  else if(meta->cf_wrapped)
    meta->cf_wrapped(*((void **) ptr));

  // Free the whole allocated (meta- + data-) blocks by the head-ptr
  free(meta);

  // INFO: Increment the deallocation count for debugging purposes
  atomic_increment(&mman_dealloc_count);
  return MMAN_DEALLOCED;
}

mman_result_t mman_dealloc(void *ptr)
{
  if (!ptr) return MMAN_NULLREF;
  mman_meta_t *meta = mman_fetch_meta(ptr);
  if (!meta)
    return MMAN_INVREF;

  // Decrease number of active references
  // Do nothing as long as active references remain
  if (atomic_decrement(&meta->refs) > 0) return MMAN_STILL_USED;
  return mman_dealloc_force(meta->ptr);
}

void mman_dealloc_nr(void *ptr)
{
  mman_dealloc(ptr);
}

void mman_dealloc_attr(void *ptr_ptr)
{
  mman_dealloc(*((void **) ptr_ptr));
}

/*
============================================================================
                                 Referencing                                
============================================================================
*/

void *mman_ref(void *ptr)
{
  mman_meta_t *meta = mman_fetch_meta(ptr);
  if (!meta) return NULL;

  // Increment number of references and return pointer to the data block
  atomic_increment(&meta->refs);
  return meta->ptr;
}

/*
============================================================================
                                  Debugging                                 
============================================================================
*/

void mman_print_info()
{
  // Print as errors to also have this screen in non-info-debug mode
  size_t ac = mman_alloc_count, deac = mman_dealloc_count;
  dbgerr("----------< MMAN Statistics >----------\n");
  dbgerr("> Allocated: %lu\n", ac);
  dbgerr("> Deallocated: %lu\n", deac);
  dbgerr("----------< MMAN Statistics >----------\n");
}