#include "web_server/routes/web_server_route_memstat.h"

/*
============================================================================
                                GET /memstat                                
============================================================================
*/

static void web_server_route_memstat(AsyncWebServerRequest *request)
{
  size_t mac = mman_get_alloc_count(), mdeac = mman_get_dealloc_count();
  scptr char *resp = strfmt_direct("%lu %lu %lu\n", esp_get_free_heap_size(), mac, mdeac);
  request->send(200, "text/plain", resp);
}

/*
============================================================================
                              Initialization                                
============================================================================
*/

void web_server_route_memstat_init(AsyncWebServer *wsrv)
{
  // /memstat, Memory statistics for debugging purposes
  wsrv->on("/api/memstat", HTTP_GET, web_server_route_memstat);
  wsrv->on("/api/memstat", HTTP_OPTIONS, web_server_route_any_options);
}