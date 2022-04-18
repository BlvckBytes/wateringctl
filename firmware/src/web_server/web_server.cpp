#include "web_server/web_server.h"

static AsyncWebServer wsrv(WEB_SERVER_PORT);

/*
============================================================================
                          Webserver Configuration                           
============================================================================
*/

void web_server_init(scheduler_t *scheduler, valve_control_t *valve_control)
{
  // Serve static files from SD, using index.html as a default file for / requests
  wsrv.serveStatic("/", SD, WEB_SERVER_STATIC_PATH).setDefaultFile("index.html");

  // Initialize routes
  web_server_route_scheduler_init(scheduler, &wsrv);
  web_server_route_valves_init(valve_control, &wsrv);
  web_server_route_not_found_init(&wsrv);
  web_server_route_memstat_init(&wsrv);

  // Initialize the websocket
  web_server_socket_events_init(&wsrv);
  web_server_socket_fs_init(&wsrv);

  // Start listening
  wsrv.begin();
}