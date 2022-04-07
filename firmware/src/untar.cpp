#include "untar.h"

ENUM_LUT_FULL_IMPL(tar_result, _EVALS_TAR_RESULT);
ENUM_LUT_FULL_IMPL(tar_entry_type, _EVALS_TAR_ENTRY_TYPE);

/*
============================================================================
                              Internal Helpers                              
============================================================================
*/

#define TAR_IS_BASE256_ENCODED(buffer) (((unsigned char) buffer[0] & 0x80) > 0)
#define TAR_GET_NUM_BLOCKS(filesize) (int) ceil((double) filesize / (double) TAR_BLOCK_SIZE)

/**
 * @brief Parse a base256 string into a ULL number
 *
 * @param buffer String input buffer
 * @return unsigned long long Parsed ULL number
 */
static unsigned long long decode_base256(__attribute__((unused)) const unsigned char *buffer)
{
  // Decode the buffer's content (base-256) to a ULL number
  // Not implemented for space reasons - it's not needed.
  return TR_OK;
}

/**
 * @brief Get the size of the last block (remainder) of a file
 * 
 * @param filesize Size of full file
 * @return int Size of last block
 */
static int get_last_block_portion_size(uint64_t filesize)
{
  int partial = filesize % TAR_BLOCK_SIZE;
  return (partial > 0 ? partial : TAR_BLOCK_SIZE);
}

/**
 * @brief Read the next block using the read callback into the handle's buffer
 * 
 * @param handle Handle of target tar
 * @return tar_result_t Operation result
 */
tar_result_t read_block(tar_handle_t *handle)
{
  int num_read = handle->callbacks->read_cb(handle->read_buffer, TAR_BLOCK_SIZE, handle->context_data);
  if (num_read < TAR_BLOCK_SIZE)
  {
    dbgerr(
      "[TAR] Read has stopped short at (%d) count rather than (%d)",
      num_read, TAR_BLOCK_SIZE
    );
    return TR_ERR_READBLOCK;
  }

  handle->total_bytes_read += num_read;
  return TR_OK;
}

/**
 * @brief Trims a string by placing a terminator into the raw input 
 * (so it alters it in place) and returns a pointer to the first non-space character
 * 
 * @param raw Raw input string
 * @param length Length of the raw input string
 * @return char* Pointer to the first non-space character
 */
static const char *str_trim(char *raw, int length)
{
  int i = 0;
  int j = length - 1;
  int is_empty = 0;

  // Determine left padding.
  while ((raw[i] == 0 || raw[i] == ' '))
  {
    i++;
    if (i >= length)
    {
      is_empty = 1;
      break;
    }
  }

  if (is_empty == 1)
    return "";

  // Determine right padding.
  while ((raw[j] == 0 || raw[j] == ' '))
  {
    j--;
    if (j <= i)
      break;
  }

  // Place the terminator.
  raw[j + 1] = 0;

  // Return an offset pointer.
  return &raw[i];
}

/*
============================================================================
                                 Public API                                 
============================================================================
*/

tar_result_t parse_header(tar_handle_t *handle, tar_header_t *header)
{
  // Since the struct has exact matching fields, just copy the contents
  memcpy(header, handle->read_buffer, sizeof(tar_header_t));
  return TR_OK;
}

tar_result_t translate_header(tar_header_t *raw_header, tar_header_translated_t *parsed)
{
  char buffer[101];
  const char *buffer_ptr;
  const int R_OCTAL = 8;
  //
  memcpy(buffer, raw_header->filename, 100);
  buffer_ptr = str_trim(buffer, 100);
  strcpy(parsed->filename, buffer_ptr);
  parsed->filename[strlen(buffer_ptr)] = 0;

  // Check that the file name is not corrupted
  for (int i = 0; i < 100; i++)
  {
    char c = parsed->filename[i];

    if (c == 0)
      break;

    if (!(c >= 32 && c < 127))
      return TR_ERR_HEADERTRANS;
  }

  //
  memcpy(buffer, raw_header->filemode, 8);
  buffer_ptr = str_trim(buffer, 8);

  if (TAR_IS_BASE256_ENCODED(buffer) != 0)
    parsed->filemode = decode_base256((const unsigned char *)buffer_ptr);
  else
    parsed->filemode = strtoull(buffer_ptr, NULL, R_OCTAL);
  //
  memcpy(buffer, raw_header->uid, 8);
  buffer_ptr = str_trim(buffer, 8);

  if (TAR_IS_BASE256_ENCODED(buffer) != 0)
    parsed->uid = decode_base256((const unsigned char *)buffer_ptr);
  else
    parsed->uid = strtoull(buffer_ptr, NULL, R_OCTAL);
  //
  memcpy(buffer, raw_header->gid, 8);
  buffer_ptr = str_trim(buffer, 8);

  if (TAR_IS_BASE256_ENCODED(buffer) != 0)
    parsed->gid = decode_base256((const unsigned char *)buffer_ptr);
  else
    parsed->gid = strtoull(buffer_ptr, NULL, R_OCTAL);
  //
  memcpy(buffer, raw_header->filesize, 12);
  buffer_ptr = str_trim(buffer, 12);

  if (TAR_IS_BASE256_ENCODED(buffer) != 0)
    parsed->filesize = decode_base256((const unsigned char *)buffer_ptr);
  else
    parsed->filesize = strtoull(buffer_ptr, NULL, R_OCTAL);
  //
  memcpy(buffer, raw_header->mtime, 12);
  buffer_ptr = str_trim(buffer, 12);

  if (TAR_IS_BASE256_ENCODED(buffer) != 0)
    parsed->mtime = decode_base256((const unsigned char *)buffer_ptr);
  else
    parsed->mtime = strtoull(buffer_ptr, NULL, R_OCTAL);
  //
  memcpy(buffer, raw_header->checksum, 8);
  buffer_ptr = str_trim(buffer, 8);

  if (TAR_IS_BASE256_ENCODED(buffer) != 0)
    parsed->checksum = decode_base256((const unsigned char *)buffer_ptr);
  else
    parsed->checksum = strtoull(buffer_ptr, NULL, R_OCTAL);
  //
  parsed->type = tar_type_from_char(raw_header->type);

  memcpy(buffer, raw_header->link_target, 100);
  buffer_ptr = str_trim(buffer, 100);
  strcpy(parsed->link_target, buffer_ptr);
  parsed->link_target[strlen(buffer_ptr)] = 0;
  //
  memcpy(buffer, raw_header->ustar_indicator, 6);
  buffer_ptr = str_trim(buffer, 6);
  strcpy(parsed->ustar_indicator, buffer_ptr);
  parsed->ustar_indicator[strlen(buffer_ptr)] = 0;
  //
  memcpy(buffer, raw_header->ustar_version, 2);
  buffer_ptr = str_trim(buffer, 2);
  strcpy(parsed->ustar_version, buffer_ptr);
  parsed->ustar_version[strlen(buffer_ptr)] = 0;

  if (strcmp(parsed->ustar_indicator, "ustar") == 0)
  {
    //
    memcpy(buffer, raw_header->user_name, 32);
    buffer_ptr = str_trim(buffer, 32);
    strcpy(parsed->user_name, buffer_ptr);
    parsed->user_name[strlen(buffer_ptr)] = 0;
    //
    memcpy(buffer, raw_header->group_name, 32);
    buffer_ptr = str_trim(buffer, 32);
    strcpy(parsed->group_name, buffer_ptr);
    parsed->group_name[strlen(buffer_ptr)] = 0;
    //
    memcpy(buffer, raw_header->device_major, 8);
    buffer_ptr = str_trim(buffer, 8);

    if (TAR_IS_BASE256_ENCODED(buffer) != 0)
      parsed->device_major = decode_base256((const unsigned char *)buffer_ptr);
    else
      parsed->device_major = strtoull(buffer_ptr, NULL, R_OCTAL);
    //
    memcpy(buffer, raw_header->device_minor, 8);
    buffer_ptr = str_trim(buffer, 8);

    if (TAR_IS_BASE256_ENCODED(buffer) != 0)
    {
      parsed->device_minor = decode_base256((const unsigned char *)buffer_ptr);
    }
    else
    {
      parsed->device_minor = strtoull(buffer_ptr, NULL, R_OCTAL);
    }
  }
  else
  {
    strcpy(parsed->user_name, "");
    strcpy(parsed->group_name, "");

    parsed->device_major = 0;
    parsed->device_minor = 0;
  }
  return TR_OK;
}

void tar_setup(tar_callbacks_t *callbacks, void *context_data, tar_handle_t *handle)
{
  handle->callbacks = callbacks;
  handle->context_data = context_data;
  handle->read_buffer[TAR_BLOCK_SIZE] = 0;
}

tar_result_t tar_read(tar_handle_t *handle)
{
  // The end of the file is represented by two empty entries (which we
  // expediently identify by filename length).
  int empty_count = 0;
  while (empty_count < 2)
  {
    if (read_block(handle) != TR_OK)
      break;

    // If we haven't yet determined what format to support, read the
    // header of the next entry, now. This should be done only at the
    // top of the archive.

    tar_header_t header;
    if (parse_header(handle, &header) != TR_OK)
    {
      dbgerr("[TAR] Could not understand the header of the first entry");
      return TR_ERR_HEADERPARSE;
    }

    else if (strlen(header.filename) == 0)
      empty_count++;

    else
    {
      tar_header_translated_t header_translated;
      if (translate_header(&header, &header_translated) != 0)
      {
        dbgerr("[TAR] Could not translate header");
        return TR_ERR_HEADERTRANS;
      }

      if (handle->callbacks->header_cb(&header_translated, handle->context_data) != 0)
      {
        dbgerr("[TAR] Header callback failed");
        return TR_ERR_HEADERCB;
      }

      int i = 0;
      int num_blocks = TAR_GET_NUM_BLOCKS(header_translated.filesize);
      while (i < num_blocks)
      {
        if (read_block(handle) != 0)
        {
          dbgerr("[TAR] Could not read block, file too short");
          return TR_ERR_READBLOCK;
        }

        int current_data_size;
        if (i >= num_blocks - 1)
          current_data_size = get_last_block_portion_size(header_translated.filesize);
        else
          current_data_size = TAR_BLOCK_SIZE;

        handle->read_buffer[current_data_size] = 0;

        if (handle->callbacks->write_cb(
          &header_translated,
          handle->context_data,
          handle->read_buffer,
          current_data_size
        ) != 0)
          return TR_ERR_WRITECB;

        i++;
      }

      if (handle->callbacks->end_cb(&header_translated, handle->total_bytes_read, handle->context_data) != 0)
      {
        dbgerr("[TAR] End callback failed");
        return TR_ERR_ENDCB;
      }
    }
  }

  dbginf("[TAR] Tar expanding done");
  return TR_OK;
}

tar_entry_type_t tar_type_from_char(char raw_type)
{
  // '0' - '7'
  if (raw_type <= 55 && raw_type >= 48)
    return (tar_entry_type_t) (raw_type - '0');

  if (raw_type == 0)
    return (tar_entry_type_t) raw_type;

  if (raw_type == 'g')
    return T_GLOBALEXTENDED;

  if (raw_type == 'x')
    return T_EXTENDED;

  return T_OTHER;
}