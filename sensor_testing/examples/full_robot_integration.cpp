/*
 * EXAMPLE: Full Robot Integration
 * Motors + Encoders + Servo + Distance Sensor + IMU
 * 
 * This demonstrates how to integrate motor control with your existing sensors.
 * Adapt this pattern to your main.cpp as needed.
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <VL53L0X.h>
#include <ESP32Servo.h>
#include "motors_encoders.h"

// ============= SENSOR OBJECTS =============
Adafruit_MPU6050 mpu;
VL53L0X distance_sensor;
Servo servo;

// ============= GLOBAL VARIABLES =============
struct {
    float accel_x, accel_y, accel_z;
    float gyro_x, gyro_y, gyro_z;
    float temperature;
} imu_data;

uint16_t distance_mm = 0;
int current_servo_angle = 90;

// ============= SETUP FUNCTIONS =============

void setup_i2c() {
    Serial.println("Initializing I2C (GPIO 21-SDA, GPIO 22-SCL)...");
    Wire.begin(21, 22, 100000);
    delay(100);
    
    // Scan for devices
    Serial.println("Scanning I2C bus...");
    for (byte address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        if (Wire.endTransmission() == 0) {
            Serial.print("  Found device at 0x");
            Serial.println(address, HEX);
        }
    }
}

void setup_mpu6050() {
    Serial.println("Initializing MPU6050...");
    if (!mpu.begin(0x68)) {
        Serial.println("ERROR: MPU6050 not found!");
        return;
    }
    Serial.println("✓ MPU6050 initialized");
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
}

void setup_vl53l0x() {
    Serial.println("Initializing VL53L0X...");
    if (!distance_sensor.init(0x29)) {
        Serial.println("ERROR: VL53L0X not found!");
        return;
    }
    Serial.println("✓ VL53L0X initialized");
    distance_sensor.setTimeout(500);
}

void setup_servo() {
    Serial.println("Initializing Servo (GPIO 13)...");
    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    servo.setPeriodHertz(50);
    servo.attach(13, 1000, 2000);
    servo.write(90);
    current_servo_angle = 90;
    Serial.println("✓ Servo initialized at 90°");
}

// ============= DATA COLLECTION =============

void read_mpu6050() {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    
    imu_data.accel_x = a.acceleration.x;
    imu_data.accel_y = a.acceleration.y;
    imu_data.accel_z = a.acceleration.z;
    imu_data.gyro_x = g.gyro.x;
    imu_data.gyro_y = g.gyro.y;
    imu_data.gyro_z = g.gyro.z;
    imu_data.temperature = temp.temperature;
}

void read_vl53l0x() {
    distance_mm = distance_sensor.readRangeSingleMillimeters();
}

// ============= MOTOR CONTROL EXAMPLES =============

/**
 * Example 1: Simple forward motion with speed from serial
 */
void simple_forward_control(int speed) {
    set_both_motors(speed);
}

/**
 * Example 2: Motor speed synchronization
 * Adjusts Motor 2 to match Motor 1's RPM
 */
void synchronized_motor_control(int target_speed) {
    set_motor1_speed(target_speed);
    
    // Read RPM and adjust Motor 2 to match
    float rpm1 = get_motor1_rpm();
    float rpm2 = get_motor2_rpm();
    
    if (rpm1 > rpm2 + 10) {
        // Motor 1 too fast, slow it down
        set_motor1_speed(target_speed - 10);
    } else if (rpm2 > rpm1 + 10) {
        // Motor 2 too fast, slow it down
        set_motor2_speed(target_speed - 10);
    }
}

/**
 * Example 3: Obstacle avoidance
 * Use VL53L0X to detect obstacles and stop
 */
void obstacle_avoiding_drive(int target_speed) {
    if (distance_mm < 300) {
        // Obstacle detected - stop
        stop_all_motors();
        Serial.println("OBSTACLE DETECTED! Stopping...");
    } else {
        // Clear path - drive
        set_both_motors(target_speed);
    }
}

/**
 * Example 4: Sweep pattern
 * Servo sweeps while motors move forward
 */
void sweep_and_drive(int motor_speed) {
    // Move motors
    set_both_motors(motor_speed);
    
    // Sweep servo
    static unsigned long sweep_start = millis();
    unsigned long sweep_time = (millis() - sweep_start) % 4000;
    
    if (sweep_time < 2000) {
        // Forward sweep 90→180
        current_servo_angle = map(sweep_time, 0, 2000, 90, 180);
    } else {
        // Reverse sweep 180→90
        current_servo_angle = map(sweep_time, 2000, 4000, 180, 90);
    }
    
    servo.write(current_servo_angle);
}

/**
 * Example 5: Distance-based speed control
 * Faster when far, slower when close
 */
void adaptive_speed_control() {
    int speed = map(distance_mm, 100, 1000, 50, 255);
    speed = constrain(speed, 50, 255);
    set_both_motors(speed);
}

// ============= DEBUG OUTPUT =============

void print_full_status() {
    static unsigned long last_print = 0;
    if (millis() - last_print < 1000) return;
    last_print = millis();
    
    Serial.println("\n========== FULL ROBOT STATUS ==========");
    
    // IMU Data
    Serial.print("IMU - Accel(g): ");
    Serial.print(imu_data.accel_x, 2); Serial.print(", ");
    Serial.print(imu_data.accel_y, 2); Serial.print(", ");
    Serial.print(imu_data.accel_z, 2);
    Serial.print(" | Temp: ");
    Serial.print(imu_data.temperature, 1); Serial.println("°C");
    
    // Distance Sensor
    Serial.print("Distance: ");
    Serial.print(distance_mm); Serial.println(" mm");
    
    // Motors
    Serial.print("Motor 1 - Speed: ");
    Serial.print(motor1_speed_percent, 1); Serial.print("% | RPM: ");
    Serial.print(get_motor1_rpm(), 2); Serial.println();
    
    Serial.print("Motor 2 - Speed: ");
    Serial.print(motor2_speed_percent, 1); Serial.print("% | RPM: ");
    Serial.print(get_motor2_rpm(), 2); Serial.println();
    
    // Servo
    Serial.print("Servo: ");
    Serial.print(current_servo_angle); Serial.println("°");
    
    Serial.println("=======================================");
}

// ============= MAIN SETUP & LOOP =============

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n\n=== FULL ROBOT INTEGRATION TEST ===\n");
    
    // Initialize all systems
    setup_i2c();
    delay(500);
    setup_mpu6050();
    delay(500);
    setup_vl53l0x();
    delay(500);
    setup_servo();
    delay(500);
    setup_motors();
    delay(500);
    setup_encoders();
    
    Serial.println("✓ All systems initialized!");
    Serial.println("\nStarting autonomous operation...\n");
}

void loop() {
    // ============= DATA COLLECTION =============
    read_mpu6050();
    read_vl53l0x();
    update_rpm_calculation();
    
    // ============= CONTROL LOGIC =============
    // Choose one of these control modes:
    
    // Mode 1: Simple forward at 150/255 speed
    // simple_forward_control(150);
    
    // Mode 2: Motor synchronization
    // synchronized_motor_control(150);
    
    // Mode 3: Obstacle avoidance
    obstacle_avoiding_drive(150);
    
    // Mode 4: Sweep and drive
    // sweep_and_drive(100);
    
    // Mode 5: Adaptive speed based on distance
    // adaptive_speed_control();
    
    // ============= STATUS OUTPUT =============
    print_full_status();
    
    delay(10);
}

/*
 * CONTROL MODES SUMMARY:
 * 
 * 1. simple_forward_control(speed)
 *    - Both motors at constant speed
 *    - Useful for testing motor response
 * 
 * 2. synchronized_motor_control(speed)
 *    - Automatically adjusts Motor 2 to match Motor 1 RPM
 *    - Prevents vehicle drift
 * 
 * 3. obstacle_avoiding_drive(speed)
 *    - Checks VL53L0X distance
 *    - Stops if obstacle within 300mm
 *    - Otherwise drives at target speed
 *    - Requires: distance_mm < 300 to stop
 * 
 * 4. sweep_and_drive(speed)
 *    - Motors move forward
 *    - Servo sweeps side-to-side (90→180→90)
 *    - Great for sensor sweep patterns
 * 
 * 5. adaptive_speed_control()
 *    - Speed varies with distance to obstacle
 *    - Maps 100-1000mm to 50-255 speed
 *    - Faster when far, slower when close
 * 
 * TO USE: Uncomment desired mode in loop(), comment out others
 */

/*
 * INTEGRATION CHECKLIST:
 * 
 * ✓ Include motors_encoders.h header
 * ✓ Call setup_motors() and setup_encoders() in setup()
 * ✓ Call update_rpm_calculation() in main loop
 * ✓ Use set_motor*_speed() to control motors
 * ✓ Use get_motor*_rpm() to read speed feedback
 * ✓ Combine with other sensors as needed
 * ✓ Test each control mode independently first
 * ✓ Verify encoders are counting correctly
 * ✓ Check motor direction (swap if reversed)
 * 
 * NEXT: Add to your main.cpp following this pattern
 */
