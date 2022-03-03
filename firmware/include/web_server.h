#ifndef web_server_h
#define web_server_h

#include <ESPAsyncWebServer.h>
#include <stdarg.h>

#include "scheduler.h"
#include "valve_control.h"
#include <blvckstd/mman.h>
#include <blvckstd/longp.h>
#include <blvckstd/strfmt.h>
#include <blvckstd/jsonh.h>

#define WEB_SERVER_PORT 80

typedef struct web_server_request_body
{
  uint8_t *content;             // Body content bytes
} web_server_request_body_t;

/**
 * @brief Define all routes and start the webserver
 */
void web_server_init(scheduler_t *scheduler, valve_control_t *valve_control);

#endif