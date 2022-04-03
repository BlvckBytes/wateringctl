#ifndef web_server_route_valves_h
#define web_server_route_valves_h

#include "web_server/web_server_common.h"
#include "web_server/routes/web_server_route_any_options.h"
#include "valve_control.h"

/*
============================================================================
                              Initialization                                
============================================================================
*/

void web_server_route_valves_init(valve_control_t *valvectl_ref, AsyncWebServer *wsrv);

#endif