/*
 * PID Controller Implementation
 * ESP32 closed-loop motor speed control
 */

#include "pid_controller.h"

// Clamp function
#define CLAMP(val, min, max) ((val) < (min) ? (min) : ((val) > (max) ? (max) : (val)))

PIDController::PIDController(float kp, float ki, float kd, int pwm_min, int pwm_max)
    : Kp(kp), Ki(ki), Kd(kd), pwm_min(pwm_min), pwm_max(pwm_max),
      target_rpm(0.0), current_rpm(0.0), previous_error(0.0), 
      integral_error(0.0), derivative_error(0.0), dt(0.0), 
      last_update_time(millis()), last_pwm(0) {
    
    // Anti-windup limit (prevent integral saturation)
    integral_max = (float)pwm_max / (Ki > 0 ? Ki : 1.0);
}

void PIDController::set_gains(float kp, float ki, float kd) {
    Kp = kp;
    Ki = ki;
    Kd = kd;
    integral_max = (float)pwm_max / (Ki > 0 ? Ki : 1.0);
}

void PIDController::set_target_rpm(float rpm) {
    target_rpm = rpm;
}

int PIDController::update(float current_rpm) {
    unsigned long current_time = millis();
    dt = (current_time - last_update_time) / 1000.0f; // Convert to seconds
    last_update_time = current_time;
    
    // Prevent division by zero on first call
    if (dt <= 0.0f) dt = 0.001f;
    
    // Calculate error
    float error = target_rpm - current_rpm;
    
    // Proportional term
    float p_term = Kp * error;
    
    // Integral term (with anti-windup)
    integral_error += error * dt;
    integral_error = CLAMP(integral_error, -integral_max, integral_max);
    float i_term = Ki * integral_error;
    
    // Derivative term (with low-pass filtering to reduce noise)
    float error_rate = (dt > 0) ? (error - previous_error) / dt : 0.0f;
    derivative_error = derivative_error * 0.7f + error_rate * 0.3f; // Low-pass filter
    float d_term = Kd * derivative_error;
    
    // Calculate output
    float pid_output = p_term + i_term + d_term;
    
    // Clamp to PWM range
    int pwm_output = (int)CLAMP(pid_output, pwm_min, pwm_max);
    
    // Store for next iteration
    previous_error = error;
    current_rpm = current_rpm;
    last_pwm = pwm_output;
    
    return pwm_output;
}

void PIDController::reset() {
    previous_error = 0.0f;
    integral_error = 0.0f;
    derivative_error = 0.0f;
    target_rpm = 0.0f;
    last_update_time = millis();
    last_pwm = 0;
}
