#include "wifi_handler.h"

static long wfh_conn_last_check = millis();
static long wfh_last_recon = millis();

bool wfh_sta_connect_dhcp()
{
  // Load wifi station info from var store
  dbginf("Attempting to connect to the STA \"%s\"...\n", WFH_SSID);

  // Don't override STA&AP mode
  if (WiFi.getMode() != WIFI_AP_STA)
    WiFi.mode(WIFI_STA);

  // Set hostname and init connection
  WiFi.hostname(WFH_DEVN);
  WiFi.begin(WFH_SSID, WFH_PASS);

  // Avait connection
  long started = millis();
  while (!wfh_sta_is_connected())
  {
    // Timed out
    if (millis() - started > WFH_TIMEOUT)
    {
      dbginf("WiFi connection timed out!\n");
      return false;
    }
  }

  dbginf("Received config from DHCP:\n");
  wfh_sta_dbg_conn_info();

  // Connection was a success, reset trials
  return true;
}

bool wfh_sta_is_connected()
{
  static bool result_cache = false;

  // Return cached value
  if (millis() - wfh_conn_last_check < WFH_CONN_STATUS_CACHE)
    return result_cache;

  // Write new value into cache
  result_cache = (
    // Status needs to be connected
    WiFi.status() == WL_CONNECTED &&

    // IP shall not be 0.0.0.0
    WiFi.localIP() != INADDR_NONE
  );

  wfh_conn_last_check = millis();
  return result_cache;
}

bool wfh_sta_ensure_connected()
{
  if (!wfh_sta_is_connected())
  {
    // Skip until cooldown expired
    if (millis() - wfh_last_recon < WFH_RECONN_COOLDOWN)
      return false;

    // Reconnect if connection broke for some reason
    dbginf("Reconnect initialized!\n");
    bool success = wfh_sta_connect_dhcp();

    // Reactivate cooldown
    wfh_last_recon = millis();

    // Successful reconnect, continue program
    if (success) {
      // Force conn status cache update
      wfh_conn_last_check -= WFH_CONN_STATUS_CACHE;
      return true;
    }

    // Unsuccessful
    return false;
  }

  // Active connection exists
  return true;
}

void wfh_sta_dbg_conn_info()
{
  dbginf(
    "{\n"
    "  local_ip: %s\n"
    "  subnet_mask: %s\n"
    "  gateway: %s\n"
    "  dns_1: %s\n"
    "  dns_2: %s\n"
    "}\n",
    format_ip_addr(WiFi.localIP()),
    format_ip_addr(WiFi.subnetMask()),
    format_ip_addr(WiFi.gatewayIP()),
    format_ip_addr(WiFi.dnsIP(0)),
    format_ip_addr(WiFi.dnsIP(1))
  );
}