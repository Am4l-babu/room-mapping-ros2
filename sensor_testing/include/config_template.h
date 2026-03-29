/**
 * @file config_template.h
 * @brief Configuration template for ESP32 WiFi Robot Car
 * 
 * INSTRUCTIONS:
 * 1. Copy this file to: config.h
 *    $ cp include/config_template.h include/config.h
 * 
 * 2. Edit config.h and replace placeholders with your actual values
 *
 * 3. NEVER commit config.h to git - it contains secrets!
 *    (Already configured in .gitignore)
 *
 * 4. Share config_template.h instead - others will create their own config.h
 */

#ifndef CONFIG_TEMPLATE_H
#define CONFIG_TEMPLATE_H

// ============================================================================
// WIFI CONFIGURATION
// ============================================================================
/**
 * WiFi Network Settings
 * Replace with your actual WiFi credentials
 */
#define WIFI_SSID "CHANGE_ME_WIFI_NAME"
#define WIFI_PASSWORD "CHANGE_ME_WIFI_PASSWORD"

// Connection timeout (seconds)
#define WIFI_CONNECT_TIMEOUT 30
#define WIFI_CONNECT_ATTEMPTS 40
#define WIFI_RETRY_DELAY_MS 500

// ============================================================================
// DEVICE CONFIGURATION
// ============================================================================
/**
 * Unique device identifier for multi-robot scenarios
 */
#define DEVICE_ID "ESP32_ROBOT_001"
#define DEVICE_NAME "WiFi Robot Car"

// ============================================================================
// WEB SERVER CONFIGURATION
// ============================================================================
/**
 * Web interface settings
 */
#define WEB_SERVER_PORT 80

// Status polling interval (milliseconds)
#define STATUS_UPDATE_INTERVAL 500

// ============================================================================
// MOTOR CONTROL CONFIGURATION
// ============================================================================
/**
 * Motor speed settings (0-255)
 */
#define DEFAULT_MOTOR_SPEED 150
#define MIN_MOTOR_SPEED 0
#define MAX_MOTOR_SPEED 255

// PWM Frequency (Hz)
#define MOTOR_PWM_FREQUENCY 1000
#define MOTOR_PWM_RESOLUTION 8  // bits

// ============================================================================
// SENSOR CONFIGURATION
// ============================================================================
/**
 * VL53L0X Distance Sensor
 */
#define SENSOR_READ_INTERVAL_MS 50
#define SENSOR_MAX_RANGE_MM 5000

// Obstacle detection threshold (mm)
#define OBSTACLE_THRESHOLD_MM 200
#define SAFEGUARD_MARGIN_MM 50

// ============================================================================
// AUTONOMOUS MODE CONFIGURATION
// ============================================================================
/**
 * Obstacle avoidance timing
 */
#define SCAN_STOP_DURATION_MS 500    // Halt before scanning
#define SCAN_DWELL_TIME_MS 300       // Time at each servo position
#define TURN_DURATION_MS 800         // Duration of turn maneuver

// Servo scan angles
#define SERVO_LEFT_ANGLE 150
#define SERVO_CENTER_ANGLE 90
#define SERVO_RIGHT_ANGLE 30

// ============================================================================
// LOGGING & DEBUG
// ============================================================================
/**
 * Serial logging settings
 */
#define SERIAL_BAUD_RATE 115200
#define SERIAL_TIMEOUT_MS 200

// Enable debug logging (1 = enabled, 0 = disabled)
#define DEBUG_MODE 1
#define DEBUG_MOTOR 1
#define DEBUG_SENSOR 1
#define DEBUG_WIFI 1
#define DEBUG_WEB 1

// ============================================================================
// FUTURE EXTENSIONS (Ready-to-use placeholders)
// ============================================================================

/**
 * MQTT Configuration (for future integration)
 */
#define MQTT_ENABLED 0  // Set to 1 when ready to enable MQTT
// #define MQTT_BROKER "mqtt.example.com"
// #define MQTT_PORT 1883
// #define MQTT_USERNAME "robot_user"
// #define MQTT_PASSWORD "CHANGE_ME_MQTT_PASSWORD"

/**
 * API Keys & Tokens (for future features)
 */
// #define API_KEY "CHANGE_ME_API_KEY"
// #define ACCESS_TOKEN "CHANGE_ME_ACCESS_TOKEN"

/**
 * OTA (Over-The-Air) Updates
 */
#define OTA_ENABLED 0  // Set to 1 when ready to enable OTA
// #define OTA_PASSWORD "CHANGE_ME_OTA_PASSWORD"

/**
 * Cloud Integration
 */
#define CLOUD_ENABLED 0  // Set to 1 when ready to enable cloud
// #define CLOUD_API_ENDPOINT "https://api.example.com"
// #define CLOUD_API_KEY "CHANGE_ME_CLOUD_API_KEY"

// ============================================================================
// SECURITY BEST PRACTICES
// ============================================================================
/**
 * CHECK BEFORE DEPLOYMENT:
 * 
 * ✓ All WIFI_* credentials are set to actual values (not CHANGE_ME_*)
 * ✓ config.h is NOT tracking in git (.gitignore should exclude it)
 * ✓ Only config_template.h is in version control
 * ✓ Never commit credentials to any branch
 * ✓ Use unique, strong passwords
 * ✓ Consider rotating credentials periodically
 * 
 * MULTI-DEVICE SETUP:
 * Each ESP32 should have its own config.h with:
 * - Unique DEVICE_ID
 * - Same WiFi credentials (or different network per device)
 * - Custom motor speeds if needed
 */

#endif // CONFIG_TEMPLATE_H
