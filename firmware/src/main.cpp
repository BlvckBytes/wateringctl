#include <Arduino.h>
#include <EEPROM.h>

#include "scheduler.h"
#include "web_server.h"
#include "time_provider.h"
#include "wifi_handler.h"
#include "valve_control.h"

scheduler_t scheduler;

void setup()
{
  Serial.begin(115200);
  EEPROM.begin(SCHEDULER_EEPROM_FOOTPRINT);

  // Nothing will work without an active WIFi connection
  // Block until connection succeeds
  while (!wfh_sta_connect_dhcp());

  // Initialize the time provider
  // Block until we get a time reading out of it, as time
  // is super critical on this system
  while (!time_provider_init());

  // Create a new scheduler that's hooked up to it's dependencies
  scheduler = scheduler_make(
    valve_control_scheduler_routine,
    time_provider_scheduler_routine
  );
  dbginf("Created the scheduler!");
  
  // Load the persistent schedule from ROM
  scheduler_eeprom_load(&scheduler);
  dbginf("Loaded scheduler's schedule from EEPROM!");

  // Start listening for web requests
  web_server_init(&scheduler);
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