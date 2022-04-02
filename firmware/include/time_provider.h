#ifndef time_provider_h
#define time_provider_h

#include "scheduler.h"
#include <NTPClient.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#define TIME_PROVIDER_POOL          "at.pool.ntp.org"
#define TIME_PROVIDER_UTC_OFFS_S    (60 * 60 * 2)

/**
 * @brief Initialize the time provider's internal NTP client
 * 
 * @return true NTP client updated successfully
 * @return false NTP client could not update
 */
bool time_provider_init();

/**
 * @brief Update time time provider's internal NTP client
 * 
 * @return true NTP client updated successfully
 * @return false NTP client could not update
 */
bool time_provider_update();

/**
 * @brief Public routine for the scheduler to be used to get the current time
 * 
 * @param day Current day output buffer
 * @param time Current time output buffer
 */
void time_provider_scheduler_routine(scheduler_weekday_t *day, scheduler_time_t *time);

#endif