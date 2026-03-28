/*
 * PID Motor Controller for ESP32
 * Closed-loop speed control per wheel using encoder feedback
 * 
 * Maintains target RPM and automatically adjusts PWM output
 */

#ifndef PID_CONTROLLER_H
#define PID_CONTROLLER_H

#include <Arduino.h>

class PIDController {
private:
    // PID coefficients
    float Kp, Ki, Kd;
    
    // State tracking
    float target_rpm;
    float current_rpm;
    float previous_error;
    float integral_error;
    float derivative_error;
    
    // Output limits
    int pwm_min, pwm_max;
    
    // Timing
    unsigned long last_update_time;
    float dt; // Delta time in seconds
    
    // Anti-windup
    float integral_max;
    
public:
    /**
     * Constructor - initializes PID controller
     * @param kp Proportional gain
     * @param ki Integral gain
     * @param kd Derivative gain
     * @param pwm_min Minimum PWM output
     * @param pwm_max Maximum PWM output
     */
    PIDController(float kp, float ki, float kd, int pwm_min = 0, int pwm_max = 255);
    
    /**
     * Set PID coefficients at runtime
     */
    void set_gains(float kp, float ki, float kd);
    
    /**
     * Set target RPM (setpoint)
     */
    void set_target_rpm(float rpm);
    
    /**
     * Update with current feedback (should be called at high frequency)
     * @param current_rpm Current measured RPM from encoder
     * @return PWM value to apply (-255 to +255)
     */
    int update(float current_rpm);
    
    /**
     * Reset controller state (call when starting new motion)
     */
    void reset();
    
    /**
     * Get current error
     */
    float get_error() const { return target_rpm - current_rpm; }
    
    /**
     * Get integral error (for diagnostics)
     */
    float get_integral() const { return integral_error; }
    
    /**
     * Get last computed PWM
     */
    int get_last_pwm() const { return last_pwm; }
    
private:
    int last_pwm; // Cache last PWM output
};

#endif // PID_CONTROLLER_H
