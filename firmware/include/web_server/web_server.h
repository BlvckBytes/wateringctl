#ifndef web_server_h
#define web_server_h

#include "web_server/web_server_error.h"
#include "web_server/web_server_common.h"
#include "web_server/routes/web_server_route_not_found.h"
#include "web_server/routes/web_server_route_scheduler.h"
#include "web_server/routes/web_server_route_valves.h"
#include "web_server/routes/web_server_route_any_options.h"
#include "web_server/routes/web_server_route_memstat.h"
#include "web_server/routes/web_server_route_fs.h"

#define WEB_SERVER_PORT 80

/**
 * @brief Define all routes and start the webserver
 */
void web_server_init(scheduler_t *scheduler, valve_control_t *valve_control);

#endif