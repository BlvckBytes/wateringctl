#ifndef __UNTAR_H
#define __UNTAR_H

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

#include <blvckstd/dbglog.h>
#include <blvckstd/enumlut.h>

#define TAR_BLOCK_SIZE 512

#define _EVALS_TAR_RESULT(FUN)     \
  FUN(TR_OK,                    0) \
  FUN(TR_CONTINUE,              1) \
  FUN(TR_EXPANDING_DONE,        2) \
  FUN(TR_ERROR,                 3) \
  FUN(TR_ERR_WRITECB,           4) \
  FUN(TR_ERR_HEADERCB,          5) \
  FUN(TR_ERR_ENDCB,             6) \
  FUN(TR_ERR_READBLOCK,         7) \
  FUN(TR_ERR_HEADERTRANS,       8) \
  FUN(TR_ERR_HEADERPARSE,       9) \
  FUN(TR_ERR_FILENAME,         10)  

#define _EVALS_TAR_ENTRY_TYPE(FUN) \
  FUN(T_NORMAL,           0)       \
  FUN(T_HARDLINK,         1)       \
  FUN(T_SYMBOLIC,         2)       \
  FUN(T_CHARSPECIAL,      3)       \
  FUN(T_BLOCKSPECIAL,     4)       \
  FUN(T_DIRECTORY,        5)       \
  FUN(T_FIFO,             6)       \
  FUN(T_CONTIGUOUS,       7)       \
  FUN(T_GLOBALEXTENDED,   8)       \
  FUN(T_EXTENDED,         9)       \
  FUN(T_OTHER,           10)        

ENUM_TYPEDEF_FULL_IMPL(tar_result, _EVALS_TAR_RESULT);
ENUM_TYPEDEF_FULL_IMPL(tar_entry_type, _EVALS_TAR_ENTRY_TYPE);

// Describes a header for TARs conforming to pre-POSIX.1-1988 .
typedef struct tar_header_s
{
  char filename[100];
  char filemode[8];
  char uid[8];
  char gid[8];
  char filesize[12];
  char mtime[12];
  char checksum[8];
  char type;
  char link_target[100];

  char ustar_indicator[6];
  char ustar_version[2];
  char user_name[32];
  char group_name[32];
  char device_major[8];
  char device_minor[8];
} tar_header_t;

typedef struct tar_header_translated_s
{
  char filename[101];
  unsigned long long filemode;
  unsigned long long uid;
  unsigned long long gid;
  unsigned long long filesize;
  unsigned long long mtime;
  unsigned long long checksum;
  tar_entry_type_t type;
  char link_target[101];
  char ustar_indicator[6];
  char ustar_version[3];
  char user_name[32];
  char group_name[32];
  unsigned long long device_major;
  unsigned long long device_minor;
} tar_header_translated_t;

/**
 * @brief Header callback, invoked when a header has been parsed
 */
typedef int (*entry_header_callback_t)
(
  tar_header_translated_t *header,
  void *context_data
);

/**
 * @brief Read callback, invoked when data needs to be read from the target file
 */
typedef int (*entry_read_callback_t)
(
  unsigned char *buf,
  size_t bufsize,
  void *context_data
);

/**
 * @brief Write callback, invoked when data regarding the file from the last header-callback needs to be written
 */
typedef int (*entry_write_callback_t)
(
  tar_header_translated_t *header,
  void *context_data,
  unsigned char *block,
  int length
);

/**
 * @brief End callback, invoked after all data has been written (if it's a file, not a dir), close
 * the resource from the header-callback here
 */
typedef int (*entry_end_callback_t)
(
  tar_header_translated_t *header,
  void *context_data
);

typedef struct tar_callbacks_s
{
  entry_header_callback_t header_cb;
  entry_read_callback_t read_cb;
  entry_write_callback_t write_cb;
  entry_end_callback_t end_cb;
} tar_callbacks_t;

typedef struct tar_handle_s
{
  tar_callbacks_t *callbacks;
  void *context_data;
  unsigned char read_buffer[TAR_BLOCK_SIZE + 1];
} tar_handle_t;

/**
 * @brief Set up a new tar handle (which also serves as a read buffer) for another tar file
 * 
 * @param callbacks Callbacks struct
 * @param context_data Context data for the callbacks
 * @param handle Handle output
 */
void tar_setup(tar_callbacks_t *callbacks, void *context_data, tar_handle_t *handle);

/**
 * @brief Read a tar file using a pre-made handle
 * 
 * @param handle Handle to read from
 * @return tar_result_t Operation result
 */
tar_result_t tar_read(tar_handle_t *handle);

/**
 * @brief Parse a raw header consisting of characters into the internally used struct
 * 
 * @param handle Handle of target tar
 * @param header Header structure output
 * @return tar_result_t Operation result code
 */
tar_result_t parse_header(tar_handle_t *handle, tar_header_t *header);

/**
 * @brief Translate a raw header to it's parsed internal structure
 * 
 * @param raw_header Raw header input
 * @param parsed Parsed header output
 * @return tar_result_t Operation result code
 */
tar_result_t translate_header(tar_header_t *raw_header, tar_header_translated_t *parsed);

/**
 * @brief Parse a raw tar entry type character into it's corresponding enum value
 * 
 * @param raw_type Raw character
 * @return tar_entry_type_t Parsed enum value
 */
tar_entry_type_t tar_type_from_char(char raw_type);

#endif
