#include "web_server/routes/web_server_route_any_options.h"

/*
============================================================================
                                OPTIONS ...                                 
============================================================================
*/

void web_server_route_any_options(AsyncWebServerRequest *request)
{
  web_server_empty_ok(request);
}