#include "web_server/sockets/web_server_socket_events.h"

ENUM_LUT_FULL_IMPL(web_socket_event, _EVALS_WEB_SERVER_SOCKET_EVENT);

static AsyncWebSocket ws(WEB_SERVER_SOCKET_EVENT_PATH);

static void onEvent(
  AsyncWebSocket *server,
  AsyncWebSocketClient *client,
  AwsEventType type,
  void *arg,
  uint8_t *data,
  size_t len
) {
  switch (type) {
    // No data will ever be received, this is transmission-only
    // Thus, just echo back what has been received - used for connection probing
    case WS_EVT_DATA:
    {
      client->binary(data, len);
      break;
    }

    case WS_EVT_CONNECT:
    case WS_EVT_DISCONNECT:
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void web_server_socket_events_init(AsyncWebServer *wsrv)
{
  ws.onEvent(onEvent);
  wsrv->addHandler(&ws);
  dbginf("Started the websocket server for " WEB_SERVER_SOCKET_EVENT_PATH "!");
}

void web_server_socket_events_cleanup()
{
  ws.cleanupClients();
}

void web_server_socket_events_broadcast(web_socket_event_t event, char *arg)
{
  const char *event_str = web_socket_event_name(event);
  scptr char *msg = strfmt_direct("%s;%s", event_str, arg == NULL ? "" : arg);
  ws.binaryAll(msg);
}
