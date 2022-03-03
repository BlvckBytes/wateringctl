#include "time_provider.h"

WiFiUDP udpClient;
NTPClient ntpClient(udpClient, TIME_PROVIDER_POOL, TIME_PROVIDER_UTC_OFFS_S);

bool time_provider_init()
{
  ntpClient.begin();
  bool ret = ntpClient.update();

  if (ret)
    dbginf("Started the NTP client!");

  return ret;
}

bool time_provider_update()
{
  return ntpClient.update();
}

void time_provider_scheduler_routine(scheduler_weekday_t *day, scheduler_time_t *time)
{
  *day = (scheduler_weekday_t) ntpClient.getDay();
  time->seconds = ntpClient.getSeconds();
  time->minutes = ntpClient.getMinutes();
  time->hours = ntpClient.getHours();
}
