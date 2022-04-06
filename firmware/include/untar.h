#ifndef __UNTAR_H
#define __UNTAR_H

#include <SD.h>

#include <string.h>
#include <math.h>
#include <stdlib.h>

#include <blvckstd/dbglog.h>
#include <blvckstd/compattrs.h>
#include <blvckstd/enumlut.h>

#define _EVALS_TAR_RESULT(FUN)         \
  FUN(TR_ERR_OPEN,              0)     \
  FUN(TR_ERR_READ,              1)     \
  FUN(TR_ERR_FIRST_HEADER,      2)     \
  FUN(TR_ERR_TRAN_HEADER,       3)     \
  FUN(TR_ERR_HEADER_CB,         4)     \
  FUN(TR_ERR_DATA_CB,           5)     \
  FUN(TR_ERR_END_CB,            6)     \
  FUN(TR_SUCCESS,               7)     

ENUM_TYPEDEF_FULL_IMPL(tar_result, _EVALS_TAR_RESULT);

#define _EVALS_TAR_ENTRY_TYPE(FUN)     \
  FUN(T_NORMAL,                 0)     \
  FUN(T_HARDLINK,               1)     \
  FUN(T_SYMBOLIC,               2)     \
  FUN(T_CHARSPECIAL,            3)     \
  FUN(T_BLOCKSPECIAL,           4)     \
  FUN(T_DIRECTORY,              5)     \
  FUN(T_FIFO,                   6)     \
  FUN(T_CONTIGUOUS,             7)     \
  FUN(T_GLOBALEXTENDED,         8)     \
  FUN(T_EXTENDED,               9)     \
  FUN(T_OTHER,                 10)     

ENUM_TYPEDEF_FULL_IMPL(tar_entry_type, _EVALS_TAR_ENTRY_TYPE);

#define TAR_T_NORMAL1 0
#define TAR_T_NORMAL2 '0'
#define TAR_T_HARD '1'
#define TAR_T_SYMBOLIC '2'
#define TAR_T_CHARSPECIAL '3'
#define TAR_T_BLOCKSPECIAL '4'
#define TAR_T_DIRECTORY '5'
#define TAR_T_FIFO '6'
#define TAR_T_CONTIGUOUS '7'
#define TAR_T_GLOBALEXTENDED 'g'
#define TAR_T_EXTENDED 'x'

#define TAR_BLOCK_SIZE 512
#define TAR_HT_PRE11988 1
#define TAR_HT_P10031 2

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

typedef int (*tar_entry_header_callback_t)
(
  tar_header_translated_t *header,
  int entry_index,
  void *context_data
);

typedef int (*tar_entry_data_callback_t)
(
  tar_header_translated_t *header,
  int entry_index,
  void *context_data,
  unsigned char *block,
  int length
);

typedef int (*tar_entry_end_callback_t)
(
  tar_header_translated_t *header,
  int entry_index,
  void *context_data
);

typedef struct tar_entry_callbacks_s
{
  tar_entry_header_callback_t header_cb;
  tar_entry_data_callback_t data_cb;
  tar_entry_end_callback_t end_cb;
} tar_entry_callbacks_t;

tar_result_t tar_read(const char *file_path, tar_entry_callbacks_t *callbacks, void *context_data);
void tar_dump_header(tar_header_translated_t *header);
const char *tar_trim(char *raw, int length);
int tar_parse_header(unsigned const char buffer[512], tar_header_t *header);
int tar_translate_header(tar_header_t *raw_header, tar_header_translated_t *parsed);
tar_entry_type_t tar_get_type_from_char(char raw_type);

#endif