#ifndef web_server_h
#define web_server_h

#include <ESPAsyncWebServer.h>

#define WEB_SERVER_PORT 80

/**
 * @brief Define all routes and start the webserver
 */
void web_server_init();

#endif