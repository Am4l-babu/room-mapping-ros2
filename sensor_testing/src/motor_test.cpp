/*
 * Motor Driver & Encoder Test Program
 * Tests L298N motor driver with HC-89 encoders
 * 
 * Features:
 * - Initialize motors and encoders
 * - Run motor speed test (ramp up/down)
 * - Display real-time RPM and pulse counts
 * - Check encoder functionality
 */

#include <Arduino.h>
#include "motors_encoders.h"

void setup() {
    Serial.begin(115200);
    delay(2000);  // Wait for serial connection
    
    Serial.println("\n\n=== MOTOR & ENCODER TEST PROGRAM ===\n");
    
    // Initialize motor and encoder systems
    setup_motors();
    delay(500);
    setup_encoders();
    delay(500);
    
    Serial.println("\nSystem ready! Starting motor test...\n");
}

void loop() {
    // Update RPM calculations
    update_rpm_calculation();
    
    // Simple test sequence
    static unsigned long test_start = millis();
    unsigned long test_elapsed = millis() - test_start;
    
    // 30 second test cycle
    if (test_elapsed < 5000) {
        // Phase 1: Motor 1 forward ramp (0-255)
        int speed = map(test_elapsed, 0, 5000, 0, 255);
        set_motor1_speed(speed);
        
    } else if (test_elapsed < 10000) {
        // Phase 2: Motor 1 constant speed
        set_motor1_speed(200);
        
    } else if (test_elapsed < 15000) {
        // Phase 3: Motor 2 forward ramp
        int speed = map(test_elapsed - 10000, 0, 5000, 0, 255);
        set_motor1_speed(0);
        set_motor2_speed(speed);
        
    } else if (test_elapsed < 20000) {
        // Phase 4: Motor 2 constant speed
        set_motor2_speed(200);
        
    } else if (test_elapsed < 25000) {
        // Phase 5: Both motors same speed
        set_both_motors(150);
        
    } else {
        // Phase 6: Stop and reset
        stop_all_motors();
        reset_encoder_counts();
        test_start = millis();
    }
    
    // Print status every 500ms
    static unsigned long last_print = 0;
    if (millis() - last_print >= 500) {
        print_motor_status();
        last_print = millis();
    }
    
    delay(10);
}
