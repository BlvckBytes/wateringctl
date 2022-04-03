#ifndef web_server_route_fs_h
#define web_server_route_fs_h

#include "web_server/web_server_common.h"
#include "web_server/routes/web_server_route_any_options.h"

/**
 * @brief Represents an async recursive file deletion request
 */
typedef struct rec_fdel_req
{
  AsyncWebServerRequest *request;
  char *path;
} rec_fdel_req_t;

/*
============================================================================
                              Initialization                                
============================================================================
*/

void web_server_route_fs_init(AsyncWebServer *wsrv);

#endif