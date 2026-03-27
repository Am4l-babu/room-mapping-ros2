/*
 * Motor Driver & Encoder Header
 * For L298N with 2 motors and HC-89 IR encoders
 */

#ifndef MOTORS_ENCODERS_H
#define MOTORS_ENCODERS_H

#include <Arduino.h>

// ============= PIN DEFINITIONS =============
extern const int MOTOR1_IN1;
extern const int MOTOR1_IN2;
extern const int MOTOR1_PWM;
extern const int MOTOR2_IN3;
extern const int MOTOR2_IN4;
extern const int MOTOR2_PWM;
extern const int ENCODER1_PIN;
extern const int ENCODER2_PIN;

extern const int PWM_FREQ;
extern const int PWM_RESOLUTION;
extern const int ENCODER_SLOTS;

// ============= GLOBAL VARIABLES =============
extern volatile unsigned long encoder1_count;
extern volatile unsigned long encoder2_count;
extern float motor1_rpm;
extern float motor2_rpm;

// ============= INITIALIZATION =============
void setup_motors();
void setup_encoders();

// ============= MOTOR CONTROL =============
void set_motor1_speed(int speed);  // -255 to +255
void set_motor2_speed(int speed);  // -255 to +255
void set_both_motors(int speed);   // -255 to +255
void stop_all_motors();

// ============= SPEED & RPM =============
void update_rpm_calculation();
float get_motor1_rpm();
float get_motor2_rpm();
unsigned long get_motor1_pulses();
unsigned long get_motor2_pulses();
float get_motor_distance_mm(int motor, float wheel_diameter_mm);
void reset_encoder_counts();

// ============= DEBUG =============
void print_motor_status();
void motor_speed_test();

#endif // MOTORS_ENCODERS_H
