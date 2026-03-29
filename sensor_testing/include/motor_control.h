/**
 * @file motor_control.h
 * @brief Motor control functions for L298N driver
 * @details Handles all motor operations (forward, backward, left, right, stop)
 */

#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

#include <Arduino.h>
#include "config.h"

// ============================================================================
// MOTOR PIN DEFINITIONS (L298N Driver)
// ============================================================================

// Motor 1 (Left Motor)
#define MOTOR1_IN1 14   // GPIO 14 - IN1
#define MOTOR1_IN2 27   // GPIO 27 - IN2
#define MOTOR1_ENA 25   // GPIO 25 - ENA (PWM Speed Control)

// Motor 2 (Right Motor)
#define MOTOR2_IN3 26   // GPIO 26 - IN3
#define MOTOR2_IN4 33   // GPIO 33 - IN4
#define MOTOR2_ENB 32   // GPIO 32 - ENB (PWM Speed Control)

// LEDC PWM Channels
#define LEDC_CHANNEL_M1 0
#define LEDC_CHANNEL_M2 1

// ============================================================================
// MOTOR STATE TRACKING
// ============================================================================

struct MotorState {
  int speed_m1;  // Current speed Motor 1 (0-255)
  int speed_m2;  // Current speed Motor 2 (0-255)
  String status; // Current action (Forward, Backward, etc.)
};

extern MotorState motor_state;

// ============================================================================
// FUNCTION DECLARATIONS
// ============================================================================

/**
 * @brief Initialize motor pins and PWM channels
 * @note Must be called in setup()
 */
void motor_init();

/**
 * @brief Move robot forward at specified speed
 * @param speed Motor speed (0-255)
 */
void motor_forward(int speed);

/**
 * @brief Move robot backward at specified speed
 * @param speed Motor speed (0-255)
 */
void motor_backward(int speed);

/**
 * @brief Turn robot left at specified speed
 * @note Left motor stops, right motor forward
 * @param speed Motor speed (0-255)
 */
void motor_left(int speed);

/**
 * @brief Turn robot right at specified speed
 * @note Left motor forward, right motor stops
 * @param speed Motor speed (0-255)
 */
void motor_right(int speed);

/**
 * @brief Stop both motors immediately
 */
void motor_stop();

/**
 * @brief Set motor speed with safety constraints
 * @param speed Desired speed (will be constrained to 0-255)
 * @return Actual speed applied
 */
int motor_set_speed(int speed);

/**
 * @brief Get current motor speeds
 * @return Reference to motor_state struct
 */
MotorState& motor_get_state();

/**
 * @brief Log motor status to serial
 */
void motor_print_status();

#endif // MOTOR_CONTROL_H
