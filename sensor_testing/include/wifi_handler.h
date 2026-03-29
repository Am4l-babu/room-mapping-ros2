/**
 * @file wifi_handler.h
 * @brief WiFi connection and network management
 * @details Handles WiFi setup using credentials from config.h
 */

#ifndef WIFI_HANDLER_H
#define WIFI_HANDLER_H

#include <Arduino.h>
#include <WiFi.h>
#include "config.h"

// ============================================================================
// WIFI STATE TRACKING
// ============================================================================

struct WiFiState {
  bool connected;        // Connection status
  IPAddress local_ip;    // Assigned IP address
  int signal_strength;   // RSSI in dBm
  uint32_t connect_time; // Connection timestamp (millis)
  String ssid;          // Connected SSID
};

extern WiFiState wifi_state;

// ============================================================================
// FUNCTION DECLARATIONS
// ============================================================================

/**
 * @brief Initialize WiFi connection
 * @note Uses WIFI_SSID and WIFI_PASSWORD from config.h
 * @return True if connected successfully, false otherwise
 */
bool wifi_init();

/**
 * @brief Check if WiFi is currently connected
 * @return True if connected, false otherwise
 */
bool wifi_is_connected();

/**
 * @brief Reconnect to WiFi if disconnected
 * @return True if connected or reconnected, false if failed
 */
bool wifi_reconnect_if_needed();

/**
 * @brief Get current connection status
 * @return Reference to wifi_state struct
 */
WiFiState& wifi_get_state();

/**
 * @brief Get formatted IP address as string
 * @return IP address string (e.g., "192.168.1.100")
 */
String wifi_get_ip_string();

/**
 * @brief Get signal strength (RSSI)
 * @return Signal strength in dBm (negative value)
 */
int wifi_get_signal_strength();

/**
 * @brief Log WiFi status to serial
 */
void wifi_print_status();

/**
 * @brief Disconnect from WiFi
 */
void wifi_disconnect();

/**
 * @brief Scan available networks
 * @param max_results Maximum number of networks to return
 * @return Vector of network names
 */
std::vector<String> wifi_scan_networks(int max_results = 10);

#endif // WIFI_HANDLER_H
