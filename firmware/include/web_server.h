#ifndef web_server_h
#define web_server_h

#include <ESPAsyncWebServer.h>
#include <stdarg.h>

#include "scheduler.h"
#include <blvckstd/mman.h>
#include <blvckstd/longp.h>
#include <blvckstd/strfmt.h>
#include <blvckstd/jsonh.h>

#define WEB_SERVER_PORT 80

/**
 * @brief Define all routes and start the webserver
 */
void web_server_init(scheduler_t *scheduler);

#endif