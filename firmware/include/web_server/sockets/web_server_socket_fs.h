#ifndef web_server_socket_fs_h
#define web_server_socket_fs_h

#include <SD.h>
#include <AsyncWebSocket.h>
#include <blvckstd/dbglog.h>
#include <blvckstd/enumlut.h>
#include <blvckstd/jsonh.h>
#include <blvckstd/partial_strdup.h>

#define WEB_SERVER_SOCKET_FS_PATH "/api/fs"

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
  FUN(WSFS_DELETED,                 9)            

ENUM_TYPEDEF_FULL_IMPL(web_server_socket_fs_response, _EVALS_WEB_SERVER_SOCKET_FS_RESPONSE);

/**
 * @brief Represents an async recursive file deletion request
 */
typedef struct rec_fdel_req
{
  AsyncWebSocketClient *client;
  char *path;
} rec_fdel_req_t;

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