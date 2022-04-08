#ifndef web_server_socket_fs_h
#define web_server_socket_fs_h

#include <SD.h>
#include <AsyncWebSocket.h>
#include <blvckstd/dbglog.h>
#include <blvckstd/enumlut.h>
#include <blvckstd/jsonh.h>
#include <blvckstd/partial_strdup.h>

#include "untar.h"

#define WEB_SERVER_SOCKET_FS_PATH "/api/fs"
#define WEB_SERFER_SOCKET_FS_CMD_TASK_PRIO 2
#define WEB_SERFER_SOCKET_FS_TASK_QUEUE_LEN 10

#define _EVALS_WEB_SERVER_SOCKET_FS_RESPONSE(FUN) \
  FUN(WSFS_NON_BINARY_DATA,         0)            \
  FUN(WSFS_EMPTY_REQUEST,           1)            \
  FUN(WSFS_COMMAND_UNKNOWN,         2)            \
  FUN(WSFS_PARAM_MISSING,           3)            \
  FUN(WSFS_TARGET_NOT_EXISTING,     4)            \
  FUN(WSFS_NOT_A_DIR,               5)            \
  FUN(WSFS_IS_A_DIR,                6)            \
  FUN(WSFS_COULD_NOT_DELETE_FILE,   7)            \
  FUN(WSFS_COULD_NOT_DELETE_DIR,    8)            \
  FUN(WSFS_DELETED,                 9)            \
  FUN(WSFS_DIR_EXISTS,             10)            \
  FUN(WSFS_FILE_EXISTS,            11)            \
  FUN(WSFS_DIR_CREATED,            12)            \
  FUN(WSFS_FILE_CREATED,           13)            \
  FUN(WSFS_COULD_NOT_CREATE_FILE,  14)            \
  FUN(WSFS_COULD_NOT_CREATE_DIR,   15)            \
  FUN(WSFS_FILE_FOUND,             16)            \
  FUN(WSFS_UNTARED,                17)            \
  FUN(WSFS_TAR_CORRUPTED,          18)            \
  FUN(WSFS_TAR_INTERNAL,           19)            \
  FUN(WSFS_TAR_CHILD_NOT_CREATED,  20)            \
  FUN(WSFS_PROGRESS,               21)            

ENUM_TYPEDEF_FULL_IMPL(web_server_socket_fs_response, _EVALS_WEB_SERVER_SOCKET_FS_RESPONSE);

#define _EVALS_FILE_REQ_TYPE(FUN) \
  FUN(FRT_FETCH_LIST,     0)      \
  FUN(FRT_DELETE_DIR,     1)      \
  FUN(FRT_UNTAR,          2)       

ENUM_TYPEDEF_FULL_IMPL(file_req_type, _EVALS_FILE_REQ_TYPE);

/**
 * @brief Represents an async file request by a user which requires processing in another task.
 */
typedef struct file_req_task_arg
{
  AsyncWebSocketClient *client;
  char *path;
  file_req_type_t type;
  File file;
  bool processed;
} file_req_task_arg_t;

/**
 * @brief Represents a context argument for the untar callbacks
 */
typedef struct untar_req_cb_arg {
  File *curr_handle;
  File *tar_file_handle;
  file_req_task_arg_t *req;
  char *containing_dir;
} untar_req_cb_arg_t;

/**
 * @brief Initialize the websocket in conjunction with a webserver
 * 
 * @param wsrv Webserver to host the endpoint on
 */
void web_server_socket_fs_init(AsyncWebServer *wsrv);

/**
 * @brief Clean up websocket-related resources, call this periodically inside the main loop
 */
void web_server_socket_fs_cleanup();

#endif