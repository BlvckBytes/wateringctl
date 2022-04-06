#include "untar.h"

#define IS_BASE256_ENCODED(buffer) (((unsigned char)buffer[0] & 0x80) > 0)
#define GET_NUM_BLOCKS(filesize) (int)ceil((double)filesize / (double)TAR_BLOCK_SIZE)

ENUM_LUT_FULL_IMPL(tar_result, _EVALS_TAR_RESULT);
ENUM_LUT_FULL_IMPL(tar_entry_type, _EVALS_TAR_ENTRY_TYPE);

INLINED static int get_last_block_portion_size(int filesize)
{
  const int partial = filesize % TAR_BLOCK_SIZE;
  return (partial > 0 ? partial : TAR_BLOCK_SIZE);
}

int tar_parse_header(const unsigned char buffer[512], tar_header_t *header)
{
  memcpy(header, buffer, sizeof(tar_header_t));
  return 0;
}

INLINED static unsigned long long decode_base256(const char *buffer)
{
  return 0;
}

const char *tar_trim(char *raw, int length)
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

int tar_translate_header(tar_header_t *raw_header, tar_header_translated_t *parsed)
{
  char buffer[101];
  const char *buffer_ptr;
  const int R_OCTAL = 8;

  //

  memcpy(buffer, raw_header->filename, 100);
  buffer_ptr = tar_trim(buffer, 100);
  strcpy(parsed->filename, buffer_ptr);
  parsed->filename[strlen(buffer_ptr)] = 0;

  //

  memcpy(buffer, raw_header->filemode, 8);
  buffer_ptr = tar_trim(buffer, 8);

  if (IS_BASE256_ENCODED(buffer) != 0)
    parsed->filemode = decode_base256(buffer_ptr);
  else
    parsed->filemode = strtoull(buffer_ptr, NULL, R_OCTAL);

  //

  memcpy(buffer, raw_header->uid, 8);
  buffer_ptr = tar_trim(buffer, 8);

  if (IS_BASE256_ENCODED(buffer) != 0)
    parsed->uid = decode_base256(buffer_ptr);
  else
    parsed->uid = strtoull(buffer_ptr, NULL, R_OCTAL);

  //

  memcpy(buffer, raw_header->gid, 8);
  buffer_ptr = tar_trim(buffer, 8);

  if (IS_BASE256_ENCODED(buffer) != 0)
    parsed->gid = decode_base256(buffer_ptr);
  else
    parsed->gid = strtoull(buffer_ptr, NULL, R_OCTAL);

  //

  memcpy(buffer, raw_header->filesize, 12);
  buffer_ptr = tar_trim(buffer, 12);

  if (IS_BASE256_ENCODED(buffer) != 0)
    parsed->filesize = decode_base256(buffer_ptr);
  else
    parsed->filesize = strtoull(buffer_ptr, NULL, R_OCTAL);

  //

  memcpy(buffer, raw_header->mtime, 12);
  buffer_ptr = tar_trim(buffer, 12);

  if (IS_BASE256_ENCODED(buffer) != 0)
    parsed->mtime = decode_base256(buffer_ptr);
  else
    parsed->mtime = strtoull(buffer_ptr, NULL, R_OCTAL);

  //

  memcpy(buffer, raw_header->checksum, 8);
  buffer_ptr = tar_trim(buffer, 8);

  if (IS_BASE256_ENCODED(buffer) != 0)
    parsed->checksum = decode_base256(buffer_ptr);
  else
    parsed->checksum = strtoull(buffer_ptr, NULL, R_OCTAL);

  //

  parsed->type = tar_get_type_from_char(raw_header->type);

  memcpy(buffer, raw_header->link_target, 100);
  buffer_ptr = tar_trim(buffer, 100);
  strcpy(parsed->link_target, buffer_ptr);
  parsed->link_target[strlen(buffer_ptr)] = 0;

  //

  memcpy(buffer, raw_header->ustar_indicator, 6);
  buffer_ptr = tar_trim(buffer, 6);
  strcpy(parsed->ustar_indicator, buffer_ptr);
  parsed->ustar_indicator[strlen(buffer_ptr)] = 0;

  //

  memcpy(buffer, raw_header->ustar_version, 2);
  buffer_ptr = tar_trim(buffer, 2);
  strcpy(parsed->ustar_version, buffer_ptr);
  parsed->ustar_version[strlen(buffer_ptr)] = 0;

  if (strcmp(parsed->ustar_indicator, "ustar") == 0)
  {
    //

    memcpy(buffer, raw_header->user_name, 32);
    buffer_ptr = tar_trim(buffer, 32);
    strcpy(parsed->user_name, buffer_ptr);
    parsed->user_name[strlen(buffer_ptr)] = 0;

    //

    memcpy(buffer, raw_header->group_name, 32);
    buffer_ptr = tar_trim(buffer, 32);
    strcpy(parsed->group_name, buffer_ptr);
    parsed->group_name[strlen(buffer_ptr)] = 0;

    //

    memcpy(buffer, raw_header->device_major, 8);
    buffer_ptr = tar_trim(buffer, 8);

    if (IS_BASE256_ENCODED(buffer) != 0)
      parsed->device_major = decode_base256(buffer_ptr);
    else
      parsed->device_major = strtoull(buffer_ptr, NULL, R_OCTAL);

    //

    memcpy(buffer, raw_header->device_minor, 8);
    buffer_ptr = tar_trim(buffer, 8);

    if (IS_BASE256_ENCODED(buffer) != 0)
      parsed->device_minor = decode_base256(buffer_ptr);
    else
      parsed->device_minor = strtoull(buffer_ptr, NULL, R_OCTAL);
  }
  else
  {
    strcpy(parsed->user_name, "");
    strcpy(parsed->group_name, "");

    parsed->device_major = 0;
    parsed->device_minor = 0;
  }

  return 0;
}

static int read_block(File *f, unsigned char *buffer)
{

  int num_read = f->read(buffer, TAR_BLOCK_SIZE);
  if (num_read < TAR_BLOCK_SIZE)
  {
    dbgerr("[TAR] Read has stopped short at (%d) count rather than (%d). Quitting under error.", num_read, TAR_BLOCK_SIZE);
    return -1;
  }

  return 0;
}

tar_result_t tar_read(const char *file_path, tar_entry_callbacks_t *callbacks, void *context_data)
{
  unsigned char buffer[TAR_BLOCK_SIZE + 1];
  int i;

  File f;

  tar_header_t header;
  tar_header_translated_t header_translated;

  int num_blocks;
  int current_data_size;
  int entry_index = 0;
  int empty_count = 0;

  buffer[TAR_BLOCK_SIZE] = 0;

  if (!(f = SD.open(file_path, "r")))
  {
    dbgerr("[TAR] Could not open archive (%s).", file_path);
    return TR_ERR_OPEN;
  }

  // The end of the file is represented by two empty entries (which we
  // expediently identify by filename length).
  while (empty_count < 2)
  {
    if (read_block(&f, buffer) != 0)
    {
      return TR_ERR_READ;
      break;
    }

    // If we haven't yet determined what format to support, read the
    // header of the next entry, now. This should be done only at the
    // top of the archive.

    if (tar_parse_header(buffer, &header) != 0)
    {
      dbgerr("[TAR] Could not understand the header of the first entry in the TAR. (%s)", file_path);
      return TR_ERR_FIRST_HEADER;
    }

    else if (strlen(header.filename) == 0)
      empty_count++;

    else
    {
      if (tar_translate_header(&header, &header_translated) != 0)
      {
        dbgerr("[TAR] Could not translate header (%s).", file_path);
        return TR_ERR_TRAN_HEADER;
      }

      if (callbacks->header_cb(&header_translated, entry_index, context_data) != 0)
      {
        dbgerr("[TAR] Header callback failed (%s).", file_path);
        return TR_ERR_HEADER_CB;
      }

      i = 0;
      int received_bytes = 0;
      num_blocks = GET_NUM_BLOCKS(header_translated.filesize);
      while (i < num_blocks)
      {
        if (read_block(&f, buffer) != 0)
        {
          dbgerr("[TAR] Could not read block. File too short (%s).", file_path);
          return TR_ERR_READ;
        }

        if (i >= num_blocks - 1)
          current_data_size = get_last_block_portion_size(header_translated.filesize);
        else
          current_data_size = TAR_BLOCK_SIZE;

        buffer[current_data_size] = 0;

        if (callbacks->data_cb(&header_translated, entry_index, context_data, buffer, current_data_size) != 0)
        {
          dbgerr("[TAR] Data callback failed (%s).", file_path);
          return TR_ERR_DATA_CB;
        }

        i++;
        received_bytes += current_data_size;
      }

      if (callbacks->end_cb(&header_translated, entry_index, context_data) != 0)
      {
        dbgerr("[TAR] End callback failed (%s).", file_path);
        return TR_ERR_END_CB;
      }
    }

    entry_index++;
  }

  f.close();
  return TR_SUCCESS;
}

void tar_dump_header(tar_header_translated_t *header)
{
  dbginf("[TAR] ===========================================");
  dbginf("[TAR]       filename: %s", header->filename);
  dbginf("[TAR]       filemode: 0%o (%llu)", (unsigned int)header->filemode, header->filemode);
  dbginf("[TAR]            uid: 0%o (%llu)", (unsigned int)header->uid, header->uid);
  dbginf("[TAR]            gid: 0%o (%llu)", (unsigned int)header->gid, header->gid);
  dbginf("[TAR]       filesize: 0%o (%llu)", (unsigned int)header->filesize, header->filesize);
  dbginf("[TAR]          mtime: 0%o (%llu)", (unsigned int)header->mtime, header->mtime);
  dbginf("[TAR]       checksum: 0%o (%llu)", (unsigned int)header->checksum, header->checksum);
  dbginf("[TAR]           type: %d", header->type);
  dbginf("[TAR]    link_target: %s", header->link_target);
  dbginf("[TAR] ");

  dbginf("[TAR]      ustar ind: %s", header->ustar_indicator);
  dbginf("[TAR]      ustar ver: %s", header->ustar_version);
  dbginf("[TAR]      user name: %s", header->user_name);
  dbginf("[TAR]     group name: %s", header->group_name);
  dbginf("[TAR] device (major): %llu", header->device_major);
  dbginf("[TAR] device (minor): %llu", header->device_minor);
  dbginf("[TAR] ");

  dbginf("[TAR]   data blocks = %d", GET_NUM_BLOCKS(header->filesize));
  dbginf("[TAR]   last block portion = %d", get_last_block_portion_size(header->filesize));
  dbginf("[TAR] ===========================================");
  dbginf("[TAR] ");
}

tar_entry_type_t tar_get_type_from_char(char raw_type)
{
  switch (raw_type)
  {
  case TAR_T_NORMAL1:
  case TAR_T_NORMAL2:
    return T_NORMAL;

  case TAR_T_HARD:
    return T_HARDLINK;

  case TAR_T_SYMBOLIC:
    return T_SYMBOLIC;

  case TAR_T_CHARSPECIAL:
    return T_CHARSPECIAL;

  case TAR_T_BLOCKSPECIAL:
    return T_CHARSPECIAL;

  case TAR_T_DIRECTORY:
    return T_DIRECTORY;

  case TAR_T_FIFO:
    return T_FIFO;

  case TAR_T_CONTIGUOUS:
    return T_CONTIGUOUS;

  case TAR_T_GLOBALEXTENDED:
    return T_GLOBALEXTENDED;

  case TAR_T_EXTENDED:
    return T_EXTENDED;
  }

  return T_OTHER;
}