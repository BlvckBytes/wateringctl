#include "web_socket.h"

ENUM_LUT_FULL_IMPL(web_socket_event, _EVALS_WEB_SOCKET_EVENT);

AsyncWebSocket ws(WEB_SOCKET_PATH);

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

void web_socket_init(AsyncWebServer *wsrv)
{
  ws.onEvent(onEvent);
  wsrv->addHandler(&ws);
  dbginf("Started the websocket server!");
}

void web_socket_cleanup()
{
  ws.cleanupClients();
}

void web_socket_broadcast(uint8_t *message, size_t length)
{
  ws.binaryAll(message, length);
}

void web_socket_broadcast_event(web_socket_event_t event, char *arg)
{
  const char *event_str = web_socket_event_name(event);
  scptr char *msg = strfmt_direct("%s;%s", event_str, arg == NULL ? "" : arg);
  web_socket_broadcast((uint8_t *) msg, strlen(msg));
}