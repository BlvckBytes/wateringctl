#include <Arduino.h>
#include <EEPROM.h>

#include "scheduler.h"
#include "web_server.h"
#include "shift_register.h"
#include "time_provider.h"
#include "wifi_handler.h"
#include "valve_control.h"
#include "status_led.h"

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
  valve_t valve = valvectl.valves[identifier];

  // Log event
  dbginf(
    "%s@%02d:%02d:%02d - %s occurred for %" PRIu16 " (" QUOTSTR ")!",
    scheduler_weekday_name(curr_day),
    curr_time.hours, curr_time.minutes, curr_time.seconds,
    scheduler_edge_name(edge),
    identifier,
    valve.alias
  );
}

void setup()
{
  // Start serial for the later use of dbginf/dbgerr
  Serial.begin(115200);

  // Start the eeprom with the total footprint size
  size_t eeprom_footprint = (
    SCHEDULER_EEPROM_FOOTPRINT
    + VALVE_CONTROL_EEPROM_FOOTPRINT
  );

  EEPROM.begin(eeprom_footprint);
  dbginf("Initialized EEPROM for %u bytes!", eeprom_footprint);

  // Initialize shift register pins and clear initially
  shift_register_init();
  shift_register_clear();
  dbginf("Initialized shift register(s)!");

  // Initialize the blinking status-led, which sets it to connecting mode
  status_led_init();
  dbginf("Initialized status-led!");

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
  
  // Load the persistent schedule from ROM
  scheduler_eeprom_load(&scheduler);
  dbginf("Loaded scheduler's schedule from EEPROM!");

  // Load the persistent valve aliases from ROM
  valve_control_eeprom_load(&valvectl);
  dbginf("Loaded valve aliases from EEPROM!");

  // Start listening for web requests
  web_server_init(&scheduler, &valvectl);
  dbginf("Started the web server!");
}

void loop()
{
  status_led_update();

  if (
    !wfh_sta_ensure_connected()   // WiFi not connected
    || !time_provider_update()    // Time not available
  )
  {
    status_led_set(STATLED_CONNECTING);
    return;
  }

  scheduler_tick(&scheduler);
  status_led_set(STATLED_CONNECTED);
}