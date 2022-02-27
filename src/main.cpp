#include <Arduino.h>
#include <WiFi.h>
#include <NTPClient.h>

#include "scheduler.h"

#define W_SSID        "HL_HNET_2"
#define W_PASS        "mysql2001"
#define UTC_OFFS_S    3600

WiFiUDP udpClient;
NTPClient ntpClient(udpClient, "pool.ntp.org", UTC_OFFS_S);

scheduler_t scheduler;

void scheduler_callback(scheduler_edge_t edge, uint16_t identifier, scheduler_weekday_t curr_day, scheduler_time_t curr_time)
{
  Serial.printf(
    "[%s %d:%d:%d]: %s occurred for %" PRIu16 "!\n",
    scheduler_weekday_name(curr_day),
    curr_time.hours, curr_time.minutes, curr_time.seconds,
    scheduler_edge_name(edge),
    identifier
  );
}

void scheduler_day_and_time_provider(scheduler_weekday_t *day, scheduler_time_t *time)
{
  *day = (scheduler_weekday_t) ntpClient.getDay();
  time->seconds = ntpClient.getSeconds();
  time->minutes = ntpClient.getMinutes();
  time->hours = ntpClient.getHours();
}

void setup()
{
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(10);

  WiFi.begin(W_SSID, W_PASS);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.print("\nConnected to ssid=");
  Serial.print(WiFi.SSID().c_str());
  Serial.print(", ip=");
  Serial.println(WiFi.localIP());

  ntpClient.begin();
  ntpClient.update();
  Serial.println("Started NTP client!");

  scheduler = scheduler_make(scheduler_callback, scheduler_day_and_time_provider);
  Serial.println("Created the scheduler!");

  scheduler_weekday_t today = (scheduler_weekday_t) ntpClient.getDay();
  scheduler_interval_t i1 = scheduler_interval_make( { 20, 28, 00 }, { 20, 29, 00 }, 1);
  bool r1 = scheduler_register_interval(&scheduler, today, i1);

  Serial.printf("Registered scheduler interval (succ=%s)!\n", r1 ? "y" : "n");
}

void loop()
{
  ntpClient.update();
  scheduler_tick(&scheduler);
}