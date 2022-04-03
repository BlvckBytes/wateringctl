#include "web_server/routes/web_server_route_not_found.h"

static void web_server_route_not_found(AsyncWebServerRequest *request)
{
  // API route requested, show 404 json errors
  if (request->url().startsWith("/api"))
  {
    web_server_error_resp(
      request,
      404, RESOURCE_NOT_FOUND,
      "The requested resource was not found (%s " QUOTSTR ")!",
      request->methodToString(),
      request->url().c_str()
    );
    return;
  }

  // File requested, fall back to index.html (used for Angular)
  File index = SD.open(WEB_SERVER_SD_ROOT "index.html");
  request->send(200, "text/html", index.readString());
  index.close();
}

/*
============================================================================
                              Initialization                                
============================================================================
*/

void web_server_route_not_found_init(AsyncWebServer *wsrv)
{
  // All remaining paths
  wsrv->onNotFound(web_server_route_not_found);
}