/**
 * @file web_server.h
 * @brief WebServer for robot control and monitoring
 * @details Built-in ESP32 WebServer with JSON API and embedded HTML UI
 */

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <WebServer.h>
#include "config.h"

// ============================================================================
// WEB SERVER CONFIGURATION
// ============================================================================

extern WebServer web_server;

// API Endpoints
#define ENDPOINT_ROOT "/"
#define ENDPOINT_CMD "/api/cmd"
#define ENDPOINT_STATUS "/api/status"
#define ENDPOINT_SCAN "/api/scan"
#define ENDPOINT_INFO "/api/info"

// ============================================================================
// WEB SERVER STATE TRACKING
// ============================================================================

struct WebServerState {
  bool running;              // Server status
  uint32_t requests_total;   // Total requests served
  uint32_t last_request_ms;  // Timestamp of last request
  String last_command;       // Last received command
};

extern WebServerState web_state;

// ============================================================================
// COMMAND TYPES
// ============================================================================

enum RobotCommand {
  CMD_STOP = 0,
  CMD_FORWARD = 1,
  CMD_BACKWARD = 2,
  CMD_LEFT = 3,
  CMD_RIGHT = 4,
  CMD_MODE_MANUAL = 5,
  CMD_MODE_AUTONOMOUS = 6,
  CMD_SCAN = 7,
  CMD_INVALID = -1
};

// ============================================================================
// FUNCTION DECLARATIONS
// ============================================================================

/**
 * @brief Initialize web server
 * @note Must be called after WiFi is connected
 * @return True if server started successfully
 */
bool web_init();

/**
 * @brief Handle incoming web requests (non-blocking)
 * @note Call this in main loop()
 */
void web_handle_client();

/**
 * @brief Send JSON status response to client
 * @param code HTTP response code (200, 404, etc.)
 * @param json JSON string to send
 */
void web_send_json(int code, const String& json);

/**
 * @brief Construct status JSON
 * @return JSON string with current robot state
 */
String web_build_status_json();

/**
 * @brief Construct scan results JSON
 * @return JSON string with left/center/right distances
 */
String web_build_scan_json();

/**
 * @brief Construct device info JSON
 * @return JSON string with device information
 */
String web_build_info_json();

/**
 * @brief Parse command from URL parameter
 * @param cmd_str Command string (e.g., "FORWARD", "STOP")
 * @return RobotCommand enum value
 */
RobotCommand web_parse_command(const String& cmd_str);

/**
 * @brief Get web server state
 * @return Reference to web_state struct
 */
WebServerState& web_get_state();

/**
 * @brief Stop web server
 */
void web_stop();

/**
 * @brief Log web server status to serial
 */
void web_print_status();

/**
 * @brief Get embedded HTML page
 * @return HTML string for web interface
 */
const char* web_get_html_page();

#endif // WEB_SERVER_H
