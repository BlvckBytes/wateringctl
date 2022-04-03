#ifndef web_server_error_h
#define web_server_error_h

#include <blvckstd/enumlut.h>

#define _EVALS_WEB_SERVER_ERROR(FUN)             \
  FUN(RESOURCE_NOT_FOUND,               0)       \
  /* Body parsing */                             \
  FUN(BODY_TOO_LONG,                    1)       \
  FUN(NO_CONTENT,                       2)       \
  FUN(NOT_JSON,                         3)       \
  FUN(INVALID_JSON,                     4)       \
  /* Body interpretation */                      \
  FUN(BODY_MALFORMED,                   5)       \
  /* Identifiers */                              \
  FUN(NON_NUM_ID,                       6)       \
  FUN(OUT_OF_RANGE_ID,                  7)       \
  /* Scheduler */                                \
  FUN(INVALID_WEEKDAY,                  8)       \
  FUN(INDEX_EMPTY,                      9)       \
  /* Valves */                                   \
  FUN(VALVE_ALREADY_ACTIVE,            10)       \
  FUN(VALVE_NOT_ACTIVE,                11)       \
  FUN(VALVE_ALIAS_DUP,                 12)       \
  FUN(VALVE_TIMER_ALREADY_ACTIVE,      13)       \
  FUN(VALVE_TIMER_NOT_ACTIVE,          14)       \
  FUN(VALVE_TIMER_IN_CONTROL,          15)       \
  FUN(VALVE_TIMER_ZERO,                16)

ENUM_TYPEDEF_FULL_IMPL(web_server_error, _EVALS_WEB_SERVER_ERROR);

#endif