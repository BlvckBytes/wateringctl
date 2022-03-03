#ifndef wifi_handler_h
#define wifi_handler_h

#include <IPAddress.h>
#include <WiFi.h>

#define WFH_SSID        "HL_HNET_2"
#define WFH_PASS        "mysql2001"
#define WFH_DEVN        "wateringctl"

#define WFH_TIMEOUT 15000
#define WFH_CONN_STATUS_CACHE 1000
#define WFH_RECONN_COOLDOWN 2000

// IP address formatting before printing
#define format_ip_addr(ip) ip.toString().c_str()

#include <blvckstd/dbglog.h>

/**
 * @brief Establish a WiFi connection to the station specified in variable
 * store and retrieve the configuration using DHCP
 * @return true Connection successful
 * @return false Could not connect
 */
bool wfh_sta_connect_dhcp();

/**
 * @brief Yields the active state of a current station connection
 * 
 * @return true Active connection exists
 * @return false Device not connected
 */
bool wfh_sta_is_connected();

/**
 * @brief Ensure that an active connection exists, reconnect otherwise
 * 
 * @return true When an active connection is present or when reconnect succeeded
 * @return false When no connection exists and reconnecting failed
 */
bool wfh_sta_ensure_connected();

/**
 * @brief Print the currently active station connection info
 */
void wfh_sta_dbg_conn_info();

#endif