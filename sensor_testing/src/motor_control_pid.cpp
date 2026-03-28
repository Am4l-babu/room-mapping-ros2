/*
 * Improved Motor Control Implementation with PID
 * 
 * Real-time closed-loop RPM control for differential drive robots
 * Update frequency: 100 Hz (10ms cycle time)
 */

#include "motor_control_pid.h"
#include "motors_encoders.h"

// ============= GPIO PIN DEFINITIONS (from motors_encoders.cpp) =============
#define MOTOR1_IN1 14
#define MOTOR1_IN2 27
#define MOTOR1_PWM 25
#define MOTOR2_IN3 26
#define MOTOR2_IN4 33
#define MOTOR2_PWM 32
#define ENCODER1_PIN 4
#define ENCODER2_PIN 5
#define ENCODER_SLOTS 20
#define PWM_FREQ 5000
#define PWM_RESOLUTION 8

// ============= GLOBAL STATE =============
PIDController pid_motor1(PID_KP, PID_KI, PID_KD, -255, 255);
PIDController pid_motor2(PID_KP, PID_KI, PID_KD, -255, 255);

// Motion targets
float target_rpm_m1 = 0.0;
float target_rpm_m2 = 0.0;
float ramped_rpm_m1 = 0.0;  // Acceleration-limited target
float ramped_rpm_m2 = 0.0;

// Encoder feedback
volatile unsigned long encoder1_count = 0;
volatile unsigned long encoder2_count = 0;
volatile unsigned long encoder1_count_prev = 0;
volatile unsigned long encoder2_count_prev = 0;

// RPM calculation (fast update)
float current_rpm_m1 = 0.0;
float current_rpm_m2 = 0.0;

// Timing
unsigned long last_control_update = 0;
unsigned long last_rpm_update = 0;

// ============= INTERRUPT HANDLERS =============

void IRAM_ATTR encoder1_isr() {
    encoder1_count++;
}

void IRAM_ATTR encoder2_isr() {
    encoder2_count++;
}

// ============= INITIALIZATION =============

void setup_motor_control_pid() {
    // Setup direction pins
    pinMode(MOTOR1_IN1, OUTPUT);
    pinMode(MOTOR1_IN2, OUTPUT);
    pinMode(MOTOR1_PWM, OUTPUT);
    pinMode(MOTOR2_IN3, OUTPUT);
    pinMode(MOTOR2_IN4, OUTPUT);
    pinMode(MOTOR2_PWM, OUTPUT);
    
    // Setup PWM channels
    ledcSetup(0, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(MOTOR1_PWM, 0);
    ledcSetup(1, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(MOTOR2_PWM, 1);
    
    // Setup encoder pins and interrupts
    pinMode(ENCODER1_PIN, INPUT_PULLUP);
    pinMode(ENCODER2_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(ENCODER1_PIN), encoder1_isr, RISING);
    attachInterrupt(digitalPinToInterrupt(ENCODER2_PIN), encoder2_isr, RISING);
    
    // Initialize all motors to stop
    digitalWrite(MOTOR1_IN1, LOW);
    digitalWrite(MOTOR1_IN2, LOW);
    digitalWrite(MOTOR2_IN3, LOW);
    digitalWrite(MOTOR2_IN4, LOW);
    ledcWrite(0, 0);
    ledcWrite(1, 0);
    
    // Initialize PID controllers
    pid_motor1.reset();
    pid_motor2.reset();
    
    last_control_update = millis();
    last_rpm_update = millis();
    
    Serial.println("[PID] Motor control system initialized");
}

// ============= TARGET SETTING =============

void set_motor1_target_rpm(float rpm_target) {
    target_rpm_m1 = CLAMP(rpm_target, -MAX_RPM, MAX_RPM);
}

void set_motor2_target_rpm(float rpm_target) {
    target_rpm_m2 = CLAMP(rpm_target, -MAX_RPM, MAX_RPM);
}

void set_both_motors_target_rpm(float rpm_target) {
    set_motor1_target_rpm(rpm_target);
    set_motor2_target_rpm(rpm_target);
}

#define CLAMP(val, min, max) ((val) < (min) ? (min) : ((val) > (max) ? (max) : (val)))

void stop_motors_smooth() {
    set_motor1_target_rpm(0.0);
    set_motor2_target_rpm(0.0);
}

void stop_motors_emergency() {
    pid_motor1.reset();
    pid_motor2.reset();
    
    digitalWrite(MOTOR1_IN1, LOW);
    digitalWrite(MOTOR1_IN2, LOW);
    digitalWrite(MOTOR2_IN3, LOW);
    digitalWrite(MOTOR2_IN4, LOW);
    ledcWrite(0, 0);
    ledcWrite(1, 0);
    
    target_rpm_m1 = 0.0;
    target_rpm_m2 = 0.0;
    ramped_rpm_m1 = 0.0;
    ramped_rpm_m2 = 0.0;
}

// ============= MOTOR OUTPUT CONTROL =============

/**
 * Apply PWM to motor with direction control
 * @param pwm: -255 (full reverse) to +255 (full forward)
 */
void apply_motor1_pwm(int pwm) {
    if (pwm > 0) {
        // Forward
        digitalWrite(MOTOR1_IN1, LOW);
        digitalWrite(MOTOR1_IN2, HIGH);
        ledcWrite(0, pwm);
    } else if (pwm < 0) {
        // Reverse
        digitalWrite(MOTOR1_IN1, HIGH);
        digitalWrite(MOTOR1_IN2, LOW);
        ledcWrite(0, -pwm);
    } else {
        // Stop
        digitalWrite(MOTOR1_IN1, LOW);
        digitalWrite(MOTOR1_IN2, LOW);
        ledcWrite(0, 0);
    }
}

void apply_motor2_pwm(int pwm) {
    if (pwm > 0) {
        // Forward
        digitalWrite(MOTOR2_IN3, LOW);
        digitalWrite(MOTOR2_IN4, HIGH);
        ledcWrite(1, pwm);
    } else if (pwm < 0) {
        // Reverse
        digitalWrite(MOTOR2_IN3, HIGH);
        digitalWrite(MOTOR2_IN4, LOW);
        ledcWrite(1, -pwm);
    } else {
        // Stop
        digitalWrite(MOTOR2_IN3, LOW);
        digitalWrite(MOTOR2_IN4, LOW);
        ledcWrite(1, 0);
    }
}

// ============= ENCODER & RPM CALCULATION =============

/**
 * Calculate RPM from encoder pulses using sliding window
 * Called frequently (100 Hz)
 */
void update_rpm_fast() {
    // Calculate pulses in this period
    unsigned long pulses1 = encoder1_count - encoder1_count_prev;
    unsigned long pulses2 = encoder2_count - encoder2_count_prev;
    
    // Update previous counts
    encoder1_count_prev = encoder1_count;
    encoder2_count_prev = encoder2_count;
    
    // RPM = (pulses / slots_per_rev) * (60 sec / sampling_period_sec)
    float sample_time_sec = UPDATE_PERIOD_MS / 1000.0f;
    current_rpm_m1 = (pulses1 / (float)ENCODER_SLOTS) * (60.0f / sample_time_sec);
    current_rpm_m2 = (pulses2 / (float)ENCODER_SLOTS) * (60.0f / sample_time_sec);
}

// ============= ACCELERATION RAMPING =============

/**
 * Apply smooth acceleration limits to prevent jerky motion
 * Uses first-order low-pass filter on target RPM
 */
void apply_acceleration_limit() {
    float max_delta_rpm = (MAX_ACCELERATION / UPDATE_FREQ_HZ); // RPM change per cycle
    
    // Motor 1
    float delta_m1 = target_rpm_m1 - ramped_rpm_m1;
    if (delta_m1 > max_delta_rpm) {
        ramped_rpm_m1 += max_delta_rpm;
    } else if (delta_m1 < -max_delta_rpm) {
        ramped_rpm_m1 -= max_delta_rpm;
    } else {
        ramped_rpm_m1 = target_rpm_m1;
    }
    
    // Motor 2
    float delta_m2 = target_rpm_m2 - ramped_rpm_m2;
    if (delta_m2 > max_delta_rpm) {
        ramped_rpm_m2 += max_delta_rpm;
    } else if (delta_m2 < -max_delta_rpm) {
        ramped_rpm_m2 -= max_delta_rpm;
    } else {
        ramped_rpm_m2 = target_rpm_m2;
    }
}

// ============= MAIN CONTROL LOOP =============

void motor_control_update() {
    unsigned long now = millis();
    unsigned long elapsed = now - last_control_update;
    
    // Execute control loop at 100 Hz
    if (elapsed >= UPDATE_PERIOD_MS) {
        last_control_update = now;
        
        // 1. Calculate RPM from encoder pulses
        update_rpm_fast();
        
        // 2. Apply acceleration limits
        apply_acceleration_limit();
        
        // 3. Update PID controllers with new targets
        pid_motor1.set_target_rpm(ramped_rpm_m1);
        pid_motor2.set_target_rpm(ramped_rpm_m2);
        
        // 4. Compute PID outputs (PWM values)
        int pwm_out_m1 = pid_motor1.update(current_rpm_m1);
        int pwm_out_m2 = pid_motor2.update(current_rpm_m2);
        
        // 5. Apply PWM to motors
        apply_motor1_pwm(pwm_out_m1);
        apply_motor2_pwm(pwm_out_m2);
    }
}

// ============= FEEDBACK & DIAGNOSTICS =============

float get_motor1_rpm() {
    return current_rpm_m1;
}

float get_motor2_rpm() {
    return current_rpm_m2;
}

float get_motor1_target_rpm() {
    return target_rpm_m1;
}

float get_motor2_target_rpm() {
    return target_rpm_m2;
}

unsigned long get_motor1_pulses() {
    return encoder1_count;
}

unsigned long get_motor2_pulses() {
    return encoder2_count;
}

void reset_motor_encoders() {
    encoder1_count = 0;
    encoder2_count = 0;
    encoder1_count_prev = 0;
    encoder2_count_prev = 0;
}

float get_motor1_pid_error() {
    return pid_motor1.get_error();
}

float get_motor2_pid_error() {
    return pid_motor2.get_error();
}

void recalibrate_pid_gains(float kp, float ki, float kd) {
    pid_motor1.set_gains(kp, ki, kd);
    pid_motor2.set_gains(kp, ki, kd);
    Serial.print("[PID] Gains updated: Kp=");
    Serial.print(kp);
    Serial.print(" Ki=");
    Serial.print(ki);
    Serial.print(" Kd=");
    Serial.println(kd);
}

void print_motor_control_status() {
    Serial.println("\n╔═══════════ PID MOTOR CONTROL STATUS ═══════════╗");
    
    Serial.print("Motor 1 | Target: ");
    Serial.print(target_rpm_m1, 1);
    Serial.print(" RPM | Ramped: ");
    Serial.print(ramped_rpm_m1, 1);
    Serial.print(" | Actual: ");
    Serial.print(current_rpm_m1, 1);
    Serial.print(" | Error: ");
    Serial.print(get_motor1_pid_error(), 2);
    Serial.println(" RPM");
    
    Serial.print("Motor 2 | Target: ");
    Serial.print(target_rpm_m2, 1);
    Serial.print(" RPM | Ramped: ");
    Serial.print(ramped_rpm_m2, 1);
    Serial.print(" | Actual: ");
    Serial.print(current_rpm_m2, 1);
    Serial.print(" | Error: ");
    Serial.print(get_motor2_pid_error(), 2);
    Serial.println(" RPM");
    
    Serial.print("Encoder Counts | M1: ");
    Serial.print(encoder1_count);
    Serial.print(" | M2: ");
    Serial.println(encoder2_count);
    
    Serial.println("╚═════════════════════════════════════════════════╝");
}
