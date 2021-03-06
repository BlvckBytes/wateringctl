#include <Arduino.h>

#include "scheduler.h"
#include "web_server/web_server.h"
#include "shift_register.h"
#include "web_server/sockets/web_server_socket_events.h"
#include "web_server/sockets/web_server_socket_fs.h"
#include "time_provider.h"
#include "wifi_handler.h"
#include "valve_control.h"
#include "status_led.h"
#include "sd_handler.h"

scheduler_t scheduler;
valve_control_t valvectl;

/**
 * @brief This routine will be invoked whenever scheduler events occur
 * and then toggle valves accordingly
 * 
 * @param edge Signal edge
 * @param identifier Valve identifier
 * @param curr_day Current day (for logging purposes)
 * @param curr_time Current time (for logging purposes)
 */
void scheduler_event_routine(
  scheduler_edge_t edge,
  uint8_t identifier,
  scheduler_weekday_t curr_day,
  scheduler_time_t curr_time
)
{
  // Invalid out-of-range identifier
  if (identifier >= VALVE_CONTROL_NUM_VALVES) 
    return;

  // Toggle the corresponding valve
  valve_control_toggle(&valvectl, identifier, edge == EDGE_OFF_TO_ON);
}

void setup()
{
  // Start serial for the later use of dbginf/dbgerr
  Serial.begin(115200);

  // Wait for the monitor to be ready
  delay(2000);

  // Initialize shift register pins and clear initially
  shift_register_init();
  shift_register_clear();
  dbginf("Initialized shift register(s)!");

  // Initialize the blinking status-led, which sets it to connecting mode
  status_led_init();
  dbginf("Initialized status-led!");

  sdh_init();

  // Nothing will work without an active WIFi connection
  // Block until connection succeeds
  while (!wfh_sta_connect_dhcp());

  // Initialize the time provider
  // Block until we get a time reading out of it, as time
  // is super critical on this system
  while (!time_provider_init());

  // Wifi connection and time data established
  status_led_set(STATLED_CONNECTED);

  // Create a new scheduler that's hooked up to it's dependencies
  scheduler = scheduler_make(
    scheduler_event_routine,
    time_provider_scheduler_routine
  );
  dbginf("Created the scheduler!");

  valvectl = valve_control_make();
  dbginf("Created the valve controller!");
  
  // Load the persistent schedule from file
  scheduler_file_load(&scheduler);
  dbginf("Loaded scheduler's schedule from file!");

  // Load the persistent valve aliases from file
  valve_control_file_load(&valvectl);
  dbginf("Loaded valve aliases from file!");

  // Start listening for web requests
  web_server_init(&scheduler, &valvectl);
  dbginf("Started the web server!");
}

void loop()
{
  // Update status led blinking cycle
  status_led_update();

  // Watch for SD card remove/insert
  sdh_watch_hotplug();

  if (
    !wfh_sta_ensure_connected()   // WiFi not connected
    || !time_provider_update()    // Time not available
  )
  {
    status_led_set(STATLED_CONNECTING);
    return;
  }

  web_server_socket_events_cleanup();
  web_server_socket_fs_cleanup();

  scheduler_tick(&scheduler, &valvectl);
  status_led_set(STATLED_CONNECTED);
}