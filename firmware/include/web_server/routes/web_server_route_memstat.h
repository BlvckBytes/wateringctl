#ifndef web_server_route_memstat_h
#define web_server_route_memstat_h

#include "web_server/web_server_common.h"
#include "web_server/routes/web_server_route_any_options.h"

/*
============================================================================
                              Initialization                                
============================================================================
*/

void web_server_route_memstat_init(AsyncWebServer *wsrv);

#endif