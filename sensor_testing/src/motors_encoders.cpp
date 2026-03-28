/*
 * Motor Driver & Encoder Control for ESP32 with L298N
 * 2x Motors with HC-89 IR Encoders (20-slot)
 * 
 * Hardware:
 * - L298N Driver with 2 motors
 * - HC-89 IR encoders (1 per motor, 20 slots)
 * - GPIO pins mapped as per configuration
 */

#include <Arduino.h>

// ============= GPIO PIN DEFINITIONS =============
// L298N Motor Driver Pins
#define MOTOR1_IN1 14   // Motor 1 Direction Pin A
#define MOTOR1_IN2 27   // Motor 1 Direction Pin B
#define MOTOR1_PWM 25   // Motor 1 Speed Control (PWM)
#define MOTOR2_IN3 26   // Motor 2 Direction Pin A
#define MOTOR2_IN4 33   // Motor 2 Direction Pin B
#define MOTOR2_PWM 32   // Motor 2 Speed Control (PWM)

// HC-89 Encoder Sensor Pins (Digital Output)
#define ENCODER1_PIN 4  // Motor 1 Encoder (Interrupt)
#define ENCODER2_PIN 5  // Motor 2 Encoder (Interrupt)

// I2C (Already used)
#define SDA_PIN 21
#define SCL_PIN 22

// Servo (Already used)
#define SERVO_PIN 13

// ============= PWM CONFIGURATION =============
#define PWM_FREQ 5000       // 5 kHz PWM frequency
#define PWM_RESOLUTION 8    // 8-bit resolution (0-255)
#define MOTOR1_PWM_CH 0     // PWM channel for Motor 1
#define MOTOR2_PWM_CH 1     // PWM channel for Motor 2

// ============= ENCODER CONFIGURATION =============
#define ENCODER_SLOTS 20    // 20 slots per revolution
#define SAMPLE_TIME 1000    // Sample time in milliseconds for RPM calculation

// ============= GLOBAL VARIABLES =============
volatile unsigned long encoder1_count = 0;
volatile unsigned long encoder2_count = 0;
volatile unsigned long encoder1_count_prev = 0;
volatile unsigned long encoder2_count_prev = 0;

unsigned long last_sample_time = 0;
float motor1_rpm = 0.0;
float motor2_rpm = 0.0;
float motor1_speed_percent = 0.0;  // 0-100%
float motor2_speed_percent = 0.0;  // 0-100%

// ============= INTERRUPT SERVICE ROUTINES =============
/**
 * Interrupt handler for Motor 1 encoder
 * Triggered on rising edge of encoder output
 */
void IRAM_ATTR encoder1_isr() {
    encoder1_count++;
}

/**
 * Interrupt handler for Motor 2 encoder
 * Triggered on rising edge of encoder output
 */
void IRAM_ATTR encoder2_isr() {
    encoder2_count++;
}

// ============= MOTOR CONTROL FUNCTIONS =============

/**
 * Initialize motor control pins and PWM channels
 */
void setup_motors() {
    // Motor 1 pins
    pinMode(MOTOR1_IN1, OUTPUT);
    pinMode(MOTOR1_IN2, OUTPUT);
    pinMode(MOTOR1_PWM, OUTPUT);
    
    // Motor 2 pins
    pinMode(MOTOR2_IN3, OUTPUT);
    pinMode(MOTOR2_IN4, OUTPUT);
    pinMode(MOTOR2_PWM, OUTPUT);
    
    // Configure PWM for Motor 1 (ENA pin)
    ledcSetup(MOTOR1_PWM_CH, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(MOTOR1_PWM, MOTOR1_PWM_CH);
    
    // Configure PWM for Motor 2 (ENB pin)
    ledcSetup(MOTOR2_PWM_CH, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(MOTOR2_PWM, MOTOR2_PWM_CH);
    
    // Initialize all motor pins to LOW
    digitalWrite(MOTOR1_IN1, LOW);
    digitalWrite(MOTOR1_IN2, LOW);
    digitalWrite(MOTOR2_IN3, LOW);
    digitalWrite(MOTOR2_IN4, LOW);
    ledcWrite(MOTOR1_PWM_CH, 0);
    ledcWrite(MOTOR2_PWM_CH, 0);
    
    last_sample_time = millis();
    
    Serial.println("Motors initialized!");
}

/**
 * Initialize encoder interrupt pins
 */
void setup_encoders() {
    pinMode(ENCODER1_PIN, INPUT);
    pinMode(ENCODER2_PIN, INPUT);
    
    // Attach interrupts to encoder pins (rising edge)
    attachInterrupt(digitalPinToInterrupt(ENCODER1_PIN), encoder1_isr, RISING);
    attachInterrupt(digitalPinToInterrupt(ENCODER2_PIN), encoder2_isr, RISING);
    
    Serial.println("Encoders initialized!");
}

/**
 * Set motor 1 direction and speed
 * @param speed: -255 (full reverse) to +255 (full forward), 0 = stop
 */
void set_motor1_speed(int speed) {
    speed = constrain(speed, -255, 255);
    motor1_speed_percent = (abs(speed) / 255.0) * 100.0;
    
    if (speed > 0) {
        // Forward
        digitalWrite(MOTOR1_IN1, LOW);
        digitalWrite(MOTOR1_IN2, HIGH);
        ledcWrite(MOTOR1_PWM_CH, speed);
    } else if (speed < 0) {
        // Reverse
        digitalWrite(MOTOR1_IN1, HIGH);
        digitalWrite(MOTOR1_IN2, LOW);
        ledcWrite(MOTOR1_PWM_CH, abs(speed));
    } else {
        // Stop
        digitalWrite(MOTOR1_IN1, LOW);
        digitalWrite(MOTOR1_IN2, LOW);
        ledcWrite(MOTOR1_PWM_CH, 0);
    }
}

/**
 * Set motor 2 direction and speed
 * @param speed: -255 (full reverse) to +255 (full forward), 0 = stop
 */
void set_motor2_speed(int speed) {
    speed = constrain(speed, -255, 255);
    motor2_speed_percent = (abs(speed) / 255.0) * 100.0;
    
    if (speed > 0) {
        // Forward
        digitalWrite(MOTOR2_IN3, LOW);
        digitalWrite(MOTOR2_IN4, HIGH);
        ledcWrite(MOTOR2_PWM_CH, speed);
    } else if (speed < 0) {
        // Reverse
        digitalWrite(MOTOR2_IN3, HIGH);
        digitalWrite(MOTOR2_IN4, LOW);
        ledcWrite(MOTOR2_PWM_CH, abs(speed));
    } else {
        // Stop
        digitalWrite(MOTOR2_IN3, LOW);
        digitalWrite(MOTOR2_IN4, LOW);
        ledcWrite(MOTOR2_PWM_CH, 0);
    }
}

/**
 * Set both motors to same speed
 * @param speed: -255 to +255
 */
void set_both_motors(int speed) {
    set_motor1_speed(speed);
    set_motor2_speed(speed);
}

/**
 * Stop both motors immediately
 */
void stop_all_motors() {
    set_motor1_speed(0);
    set_motor2_speed(0);
}

// ============= ENCODER & SPEED CALCULATION =============

/**
 * Update RPM calculation based on encoder pulses
 * Call this function periodically (e.g., in main loop)
 */
void update_rpm_calculation() {
    unsigned long current_time = millis();
    unsigned long time_elapsed = current_time - last_sample_time;
    
    if (time_elapsed >= SAMPLE_TIME) {
        // Calculate Motor 1 RPM
        unsigned long motor1_pulses = encoder1_count - encoder1_count_prev;
        motor1_rpm = (motor1_pulses * 60000.0) / (time_elapsed * ENCODER_SLOTS);
        
        // Calculate Motor 2 RPM
        unsigned long motor2_pulses = encoder2_count - encoder2_count_prev;
        motor2_rpm = (motor2_pulses * 60000.0) / (time_elapsed * ENCODER_SLOTS);
        
        // Update previous counts for next iteration
        encoder1_count_prev = encoder1_count;
        encoder2_count_prev = encoder2_count;
        last_sample_time = current_time;
    }
}

/**
 * Get motor 1 RPM
 */
float get_motor1_rpm() {
    return motor1_rpm;
}

/**
 * Get motor 2 RPM
 */
float get_motor2_rpm() {
    return motor2_rpm;
}

/**
 * Get motor 1 encoder pulse count
 */
unsigned long get_motor1_pulses() {
    return encoder1_count;
}

/**
 * Get motor 2 encoder pulse count
 */
unsigned long get_motor2_pulses() {
    return encoder2_count;
}

/**
 * Reset encoder counts to zero
 */
void reset_encoder_counts() {
    encoder1_count = 0;
    encoder2_count = 0;
    encoder1_count_prev = 0;
    encoder2_count_prev = 0;
}

/**
 * Calculate distance traveled by motor (in revolutions or mm)
 * @param motor: 1 or 2
 * @param wheel_diameter_mm: diameter of encoder wheel in mm
 * @return distance in mm
 */
float get_motor_distance_mm(int motor, float wheel_diameter_mm) {
    float wheel_circumference_mm = 3.14159 * wheel_diameter_mm;
    unsigned long pulse_count = (motor == 1) ? encoder1_count : encoder2_count;
    float revolutions = pulse_count / (float)ENCODER_SLOTS;
    return revolutions * wheel_circumference_mm;
}

// ============= DEBUG & MONITORING =============

/**
 * Print motor and encoder status to Serial
 */
void print_motor_status() {
    Serial.println("\n========== MOTOR & ENCODER STATUS ==========");
    
    // Motor 1 Status
    Serial.print("Motor 1: ");
    Serial.print(motor1_speed_percent);
    Serial.print("% | RPM: ");
    Serial.print(motor1_rpm, 2);
    Serial.print(" | Pulses: ");
    Serial.println(encoder1_count);
    
    // Motor 2 Status
    Serial.print("Motor 2: ");
    Serial.print(motor2_speed_percent);
    Serial.print("% | RPM: ");
    Serial.print(motor2_rpm, 2);
    Serial.print(" | Pulses: ");
    Serial.println(encoder2_count);
    
    Serial.println("==========================================");
}

/**
 * Quick speed test - ramp motors up and down
 */
void motor_speed_test() {
    Serial.println("\n--- Motor Speed Test Started ---");
    
    // Ramp up Motor 1
    for (int speed = 0; speed <= 255; speed += 50) {
        set_motor1_speed(speed);
        delay(500);
        print_motor_status();
    }
    
    // Ramp down Motor 1
    for (int speed = 255; speed >= 0; speed -= 50) {
        set_motor1_speed(speed);
        delay(500);
        print_motor_status();
    }
    
    // Repeat for Motor 2
    for (int speed = 0; speed <= 255; speed += 50) {
        set_motor2_speed(speed);
        delay(500);
        print_motor_status();
    }
    
    for (int speed = 255; speed >= 0; speed -= 50) {
        set_motor2_speed(speed);
        delay(500);
        print_motor_status();
    }
    
    Serial.println("--- Motor Speed Test Complete ---\n");
}

// ============= EXAMPLE MAIN SETUP & LOOP =============
/*
 * To use this code, call these functions in your main setup/loop:
 * 
 * void setup() {
 *     Serial.begin(115200);
 *     setup_motors();
 *     setup_encoders();
 * }
 * 
 * void loop() {
 *     // Update RPM calculation
 *     update_rpm_calculation();
 *     
 *     // Control motors (example: gradual speed increase)
 *     set_both_motors(100);  // 100/255 speed
 *     
 *     // Check status periodically
 *     static unsigned long last_print = 0;
 *     if (millis() - last_print > 1000) {
 *         print_motor_status();
 *         last_print = millis();
 *     }
 * }
 */
