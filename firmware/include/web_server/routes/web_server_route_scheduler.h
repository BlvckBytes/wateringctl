#ifndef web_server_route_scheduler_h
#define web_server_route_scheduler_h

#include "web_server/web_server_common.h"
#include "web_server/routes/web_server_route_any_options.h"
#include "scheduler.h"

/*
============================================================================
                              Initialization                                
============================================================================
*/

void web_server_route_scheduler_init(scheduler_t *scheduler_ref, AsyncWebServer *wsrv);

#endif