#ifndef web_socket_h
#define web_socket_h

#include <AsyncWebSocket.h>
#include <blvckstd/dbglog.h>
#include <blvckstd/enumlut.h>
#include <blvckstd/mman.h>

#define WEB_SOCKET_PATH "/wse"

#define _EVALS_WEB_SOCKET_EVENT(FUN)                                          \
  /*  Event name and id     |  Event parameters       */                      \
  FUN(WSE_INTERVAL_SCHED_ON,                0) /* <interval_index> */         \
  FUN(WSE_INTERVAL_SCHED_OFF,               1) /* <interval_index> */         \
  FUN(WSE_VALVE_ON,                         2) /* <valve_id> */               \
  FUN(WSE_VALVE_OFF,                        3) /* <valve_id> */               \
  FUN(WSE_VALVE_RENAME,                     4) /* <valve_id> */               \
  FUN(WSE_VALVE_DISABLE_ON,                 5) /* <valve_id> */               \
  FUN(WSE_VALVE_DISABLE_OFF,                6) /* <valve_id> */               \
  FUN(WSE_DAY_DISABLE_ON,                   7) /* <day> */                    \
  FUN(WSE_DAY_DISABLE_OFF,                  8) /* <day> */                    \
  FUN(WSE_INTERVAL_DISABLE_ON,              9) /* <day><index> */             \
  FUN(WSE_INTERVAL_DISABLE_OFF,            10) /* <day><index> */             \
  FUN(WSE_INTERVAL_START_CHANGE,           11) /* <day><index><start> */      \
  FUN(WSE_INTERVAL_END_CHANGE,             12) /* <day><index><end> */        \
  FUN(WSE_INTERVAL_IDENTIFIER_CHANGE,      13) /* <day><index><identifier> */ \
  FUN(WSE_INTERVAL_DELETED,                14) /* <day><index> */             \
  FUN(WSE_VALVE_TIMER_UPDATED,             15) /* <valve_id><timer> */

ENUM_TYPEDEF_FULL_IMPL(web_socket_event, _EVALS_WEB_SOCKET_EVENT);

/**
 * @brief Initialize the websocket in conjunction with a webserver
 * 
 * @param wsrv Webserver to host the endpoint on
 */
void web_socket_init(AsyncWebServer *wsrv);

/**
 * @brief Clean up websocket-related resources, call this periodically inside the main loop
 */
void web_socket_cleanup();

/**
 * @brief Broadcast a message to all connected clients
 * 
 * @param message Message to broadcast
 * @param length Length of message bytes
 */
void web_socket_broadcast(uint8_t *message, size_t length);

/**
 * @brief Broadcast an event to all connected clients
 * 
 * Protocol layout:
 * <event>;<arg>
 * 
 * @param event Event to broadcast
 * @param arg Event argument, leave NULL if not required by the event
 */
void web_socket_broadcast_event(web_socket_event_t event, char *arg);

#endif