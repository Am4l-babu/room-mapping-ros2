/*
 * Improved Motor Control with PID & High-Frequency Encoder Sampling
 * 
 * IMPROVEMENTS:
 * - Closed-loop RPM control (PID per wheel)
 * - 100+ Hz encoder sampling (fast update cycle)
 * - Target RPM interface (instead of raw PWM)
 * - Smooth acceleration control
 * - Direct ROS 2 integration
 */

#ifndef MOTOR_CONTROL_PID_H
#define MOTOR_CONTROL_PID_H

#include <Arduino.h>
#include "pid_controller.h"

// ============= CONFIGURATION =============
#define UPDATE_FREQ_HZ 100      // Control update frequency (100 Hz = 10 ms)
#define UPDATE_PERIOD_MS 10     // 10ms = 100 Hz

// PID Tuning Parameters (tuned for small DC motors with encoders)
#define PID_KP 1.2              // Proportional gain
#define PID_KI 0.3              // Integral gain
#define PID_KD 0.1              // Derivative gain

// Motor speed constraints
#define MAX_RPM 120.0           // Maximum motor RPM
#define MIN_RPM 0.0
#define MAX_ACCELERATION 50.0   // RPM/sec - smooth acceleration limiting

// ============= MOTOR CONTROL INTERFACE =============

/**
 * Initialize motors and PID controllers
 */
void setup_motor_control_pid();

/**
 * Set target RPM for motor 1 (left wheel)
 * @param rpm_target: 0 to MAX_RPM (positive for forward)
 */
void set_motor1_target_rpm(float rpm_target);

/**
 * Set target RPM for motor 2 (right wheel)
 * @param rpm_target: 0 to MAX_RPM
 */
void set_motor2_target_rpm(float rpm_target);

/**
 * Set both motors to same target RPM
 */
void set_both_motors_target_rpm(float rpm_target);

/**
 * Stop both motors with PID deceleration
 */
void stop_motors_smooth();

/**
 * Stop motors immediately (emergency stop)
 */
void stop_motors_emergency();

/**
 * CRITICAL: Call this periodically (e.g., in main loop at 100 Hz)
 * Implements:
 * - Encoder pulse counting
 * - RPM calculation
 * - PID update cycle
 * - Smooth acceleration ramp
 * - PWM output generation
 */
void motor_control_update();

/**
 * Get current RPM feedback (for diagnostics)
 */
float get_motor1_rpm();
float get_motor2_rpm();

/**
 * Get target RPM (what controller is trying to achieve)
 */
float get_motor1_target_rpm();
float get_motor2_target_rpm();

/**
 * Get encoder pulse counts (for odometry)
 */
unsigned long get_motor1_pulses();
unsigned long get_motor2_pulses();

/**
 * Reset encoder counts
 */
void reset_motor_encoders();

/**
 * Print debug info
 */
void print_motor_control_status();

// ============= INTERNAL STATE MANAGEMENT =============

/**
 * Get current PID error (target - actual) for each motor
 */
float get_motor1_pid_error();
float get_motor2_pid_error();

/**
 * Recalibrate PID gains at runtime
 */
void recalibrate_pid_gains(float kp, float ki, float kd);

#endif // MOTOR_CONTROL_PID_H
