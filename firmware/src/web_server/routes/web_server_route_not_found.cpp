#include "web_server/routes/web_server_route_not_found.h"

static void web_server_route_not_found(AsyncWebServerRequest *request)
{
  String url = request->url();
  
  // API route requested, show 404 json errors
  if (url.startsWith("/api"))
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

  if (
    // Is a file
    url.lastIndexOf("/") < url.lastIndexOf(".") &&

    // Is not index.html
    url.lastIndexOf("index.html") < 0
  )
  {
    request->send(404);
    return;
  }

  // Start out with the default web-root index.html
  const char *path = WEB_SERVER_STATIC_PATH "index.html";

  // File manager request, respond with the file manager's index.html
  if (url.startsWith("/fileman"))
    path = WEB_SERVER_STATIC_PATH "fileman/index.html";

  // File requested, fall back to index.html (used for Angular)
  File index = SD.open(path);
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