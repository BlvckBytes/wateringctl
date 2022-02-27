#include "web_server.h"

AsyncWebServer wsrv(WEB_SERVER_PORT);

void web_server_route_not_found(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

void web_server_init()
{
  wsrv.onNotFound(web_server_route_not_found);
  wsrv.begin();
}