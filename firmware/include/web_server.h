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
#include <blvckstd/enumlut.h>

#define WEB_SERVER_PORT 80

#define _EVALS_WEB_SERVER_ERROR(FUN)  \
  FUN(RESOURCE_NOT_FOUND,    0)       \
  /* Body parsing */                  \
  FUN(BODY_TOO_LONG,         1)       \
  FUN(NO_CONTENT,            2)       \
  FUN(NOT_JSON,              3)       \
  FUN(INVALID_JSON,          4)       \
  /* Body interpretation */           \
  FUN(BODY_MALFORMED,        5)       \
  /* Identifiers */                   \
  FUN(NON_NUM_ID,            6)       \
  FUN(OUT_OF_RANGE_ID,       7)       \
  /* Scheduler */                     \
  FUN(INVALID_WEEKDAY,       8)       \
  FUN(INDEX_EMPTY,           9)       \
  /* Valves */                        \
  FUN(VALVE_ALREADY_ACTIVE, 10)       \
  FUN(VALVE_NOT_ACTIVE,     11)       \
  FUN(VALVE_ALIAS_DUP,      12)

ENUM_TYPEDEF_FULL_IMPL(web_server_error, _EVALS_WEB_SERVER_ERROR);

typedef struct web_server_request_body
{
  uint8_t *content;             // Body content bytes
} web_server_request_body_t;

/**
 * @brief Define all routes and start the webserver
 */
void web_server_init(scheduler_t *scheduler, valve_control_t *valve_control);

#endif