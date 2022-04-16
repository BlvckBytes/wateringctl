#include "wifi_handler.h"

static long wfh_conn_last_check = millis();
static long wfh_last_recon = millis();

/**
 * @brief Search the target by it's SSID and choose the BSSID with the strongest
 * available RSSI (if there are multiple access-points using the same SSID,
 * otherwise just chooses the one existing)
 * 
 * @param channel Optional channel output parameter
 * @return uint8_t* BSSID of the target or NULL if no AP was available
 */
static uint8_t *wfh_sta_search_target(int32_t *channel)
{
  // Buffer the strongest bssid and it's rssi here, for sorting
  uint8_t *strongest_bssid = NULL;
  int32_t strongest_rssi = 0;
  int32_t strongest_channel = 0;

  // Scan all available networks of the area
  int16_t num_networks = WiFi.scanNetworks();
  for (int16_t i = 0; i < num_networks; i++)
  {
    // Not a network with the target SSID
    if (!WiFi.SSID(i).equals(WFH_SSID))
      continue;

    int32_t current_rssi = WiFi.RSSI(i);

    // Update the local buffer initially and for every stronger rssi than currently selected
    if (!strongest_bssid || current_rssi > strongest_rssi)
    {
      strongest_bssid = WiFi.BSSID(i);
      strongest_rssi = current_rssi;
      strongest_channel = WiFi.channel(i);
    }
  }

  // Also output the channel if a buffer variable has been provided
  if (channel)
    *channel = strongest_channel;

  return strongest_bssid;
}

bool wfh_sta_connect_dhcp()
{
  // Load wifi station info from var store
  dbginf("Attempting to connect to the STA \"%s\"...", WFH_SSID);

  // Don't override STA&AP mode
  if (WiFi.getMode() != WIFI_AP_STA)
    WiFi.mode(WIFI_STA);

  // Block until the target BSSID becomes available
  uint8_t *bssid = NULL;
  int32_t channel = 0;
  while (!(bssid = wfh_sta_search_target(&channel)))
  {
    dbginf("Could not find a STA with the SSID " QUOTSTR "!", WFH_SSID);
    delay(500);
    continue;
  }

  // Set hostname and init connection to the target
  WiFi.hostname(WFH_DEVN);
  WiFi.begin(WFH_SSID, WFH_PASS, channel, bssid);

  // Avait connection
  long started = millis();
  while (!wfh_sta_is_connected())
  {
    // Timed out
    if (millis() - started > WFH_TIMEOUT)
    {
      dbginf("WiFi connection timed out!");
      return false;
    }

    status_led_update();
    yield();
  }

  // String-length will be: 6x2 hex-chars and n-1 (so 5) ":", thus 17
  scptr char *bssid_str = (char *) mman_alloc(sizeof(char), 17, NULL);

  // Build the formatted hex string byte by byte
  size_t bssid_str_ind = 0;
  for (int i = 0; i < 6; i++)
    strfmt(&bssid_str, &bssid_str_ind, "%s%02X", i == 0 ? "" : ":", bssid[i]);

  dbginf("Connected to STA " QUOTSTR " (bssid: %s, channel: %d)", WFH_SSID, bssid_str, channel);
  dbginf("Received config from DHCP:");
  wfh_sta_dbg_conn_info();

  // Connection was a success
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
    dbginf("Reconnect initialized!");
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
    "}",
    format_ip_addr(WiFi.localIP()),
    format_ip_addr(WiFi.subnetMask()),
    format_ip_addr(WiFi.gatewayIP()),
    format_ip_addr(WiFi.dnsIP(0)),
    format_ip_addr(WiFi.dnsIP(1))
  );
}