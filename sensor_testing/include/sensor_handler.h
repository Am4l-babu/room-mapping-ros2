/**
 * @file sensor_handler.h
 * @brief VL53L0X distance sensor and servo control
 * @details Handles sensor reading and servo scanning for autonomous navigation
 */

#ifndef SENSOR_HANDLER_H
#define SENSOR_HANDLER_H

#include <Arduino.h>
#include <VL53L0X.h>
#include <Servo.h>
#include "config.h"

// ============================================================================
// SENSOR PIN DEFINITIONS
// ============================================================================

// VL53L0X I2C Configuration
#define VL53L0X_SDA 21   // GPIO 21 - I2C SDA
#define VL53L0X_SCL 22   // GPIO 22 - I2C SCL
#define VL53L0X_ADDRESS 0x29

// Servo Motor
#define SERVO_PIN 13     // GPIO 13 - Servo control

// ============================================================================
// SENSOR STATE TRACKING
// ============================================================================

struct SensorState {
  uint16_t front_distance;   // Current distance (mm)
  uint16_t left_distance;    // Scan left distance (mm)
  uint16_t center_distance;  // Scan center distance (mm)
  uint16_t right_distance;   // Scan right distance (mm)
  int servo_angle;           // Current servo angle (0-180)
  bool sensor_ready;         // Sensor initialization status
};

extern SensorState sensor_state;
extern Servo servo_motor;
extern VL53L0X distance_sensor;

// ============================================================================
// FUNCTION DECLARATIONS
// ============================================================================

/**
 * @brief Initialize distance sensor (VL53L0X) and servo
 * @note Must be called in setup()
 * @return True if initialization successful
 */
bool sensor_init();

/**
 * @brief Read current distance from sensor (non-blocking)
 * @return Distance in millimeters, capped at SENSOR_MAX_RANGE_MM
 */
uint16_t sensor_read_distance();

/**
 * @brief Move servo to specified angle (non-blocking)
 * @param angle Servo angle (0-180 degrees)
 * @note Uses SERVO_LEFT_ANGLE, SERVO_CENTER_ANGLE, SERVO_RIGHT_ANGLE from config
 */
void servo_move(int angle);

/**
 * @brief Move servo to left position
 * @note Servo angle = SERVO_LEFT_ANGLE from config.h
 */
void servo_left();

/**
 * @brief Move servo to center position
 * @note Servo angle = SERVO_CENTER_ANGLE from config.h
 */
void servo_center();

/**
 * @brief Move servo to right position
 * @note Servo angle = SERVO_RIGHT_ANGLE from config.h
 */
void servo_right();

/**
 * @brief Perform 3-point directional scan
 * @details Scans left, center, right positions and stores distances
 * @return True when scan complete, false if still scanning
 * @note Uses non-blocking timing based on millis()
 */
bool sensor_scan_directions();

/**
 * @brief Get current sensor readings
 * @return Reference to sensor_state struct
 */
SensorState& sensor_get_state();

/**
 * @brief Get latest distance reading
 * @return Distance in millimeters
 */
uint16_t sensor_get_distance();

/**
 * @brief Get scan results from last 3-point scan
 * @param left Distance at left position (output parameter)
 * @param center Distance at center position (output parameter)
 * @param right Distance at right position (output parameter)
 */
void sensor_get_scan_results(uint16_t& left, uint16_t& center, uint16_t& right);

/**
 * @brief Check if distance indicates obstacle
 * @param distance Distance in millimeters
 * @return True if distance < OBSTACLE_THRESHOLD_MM
 */
bool sensor_obstacle_detected(uint16_t distance);

/**
 * @brief Log sensor status to serial
 */
void sensor_print_status();

#endif // SENSOR_HANDLER_H
