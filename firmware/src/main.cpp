#include <Arduino.h>
#include <EEPROM.h>

#include "scheduler.h"
#include "web_server.h"
#include "shift_register.h"
#include "time_provider.h"
#include "wifi_handler.h"
#include "valve_control.h"

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
    "%s@%02d:%02d:%02d - %s occurred for %" PRIu16 " (" QUOTSTR ")!\n",
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
  EEPROM.begin(
    SCHEDULER_EEPROM_FOOTPRINT
    + VALVE_CONTROL_EEPROM_FOOTPRINT
  );

  // Initialize shift register pins and clear initially
  shift_register_init();
  shift_register_clear();

  // Nothing will work without an active WIFi connection
  // Block until connection succeeds
  while (!wfh_sta_connect_dhcp());

  // Initialize the time provider
  // Block until we get a time reading out of it, as time
  // is super critical on this system
  while (!time_provider_init());

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
  dbginf("Loaded scheduler's schedule from EEPROM!");

  // Start listening for web requests
  web_server_init(&scheduler, &valvectl);
  dbginf("Started the web server!");
}

void loop()
{
  // Only process data if an active WIFi connection exists
  if (!wfh_sta_ensure_connected())
    return;

  // Only process data if we got real-time on this system
  if (!time_provider_update())
    return;

  scheduler_tick(&scheduler);
}