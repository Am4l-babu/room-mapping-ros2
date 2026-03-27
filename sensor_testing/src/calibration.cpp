#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include "VL53L0X.h"

Adafruit_MPU6050 mpu;
VL53L0X sensor;

const int MPU_SDA = 21;
const int MPU_SCL = 22;

// Calibration storage
float accel_offset_x = 0, accel_offset_y = 0, accel_offset_z = 0;
float gyro_offset_x = 0, gyro_offset_y = 0, gyro_offset_z = 0;
float vl53_distance_offset = 0;

// Function prototypes
void print_menu();
void calibrate_accelerometer();
void calibrate_gyroscope();
void calibrate_vl53l0x();
void view_offsets();
void reset_offsets();
void monitor_sensors();

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n\n====================================");
  Serial.println("SENSOR CALIBRATION UTILITY");
  Serial.println("====================================\n");
  
  // Initialize I2C
  Wire.begin(MPU_SDA, MPU_SCL, 100000);
  delay(500);
  
  // Initialize MPU6050
  if (!mpu.begin(0x68, &Wire)) {
    Serial.println("✗ MPU6050 not found!");
    while (1) delay(1000);
  }
  Serial.println("✓ MPU6050 found\n");
  
  // Initialize VL53L0X
  if (!sensor.init()) {
    Serial.println("✗ VL53L0X not found!");
  } else {
    Serial.println("✓ VL53L0X found\n");
    sensor.setTimeout(500);
  }
  
  // Print menu
  print_menu();
}

void print_menu() {
  Serial.println("====================================");
  Serial.println("SELECT CALIBRATION:");
  Serial.println("====================================");
  Serial.println("1 = Accelerometer Calibration");
  Serial.println("2 = Gyroscope Calibration");
  Serial.println("3 = VL53L0X Distance Calibration");
  Serial.println("4 = View Current Offsets");
  Serial.println("5 = Reset All Offsets");
  Serial.println("6 = Sensor Data Monitoring");
  Serial.println("====================================\n");
  Serial.println("Send selection (1-6) via Serial:");
}

void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();
    Serial.write(cmd); // Echo
    Serial.println();
    
    switch(cmd) {
      case '1':
        calibrate_accelerometer();
        break;
      case '2':
        calibrate_gyroscope();
        break;
      case '3':
        calibrate_vl53l0x();
        break;
      case '4':
        view_offsets();
        break;
      case '5':
        reset_offsets();
        break;
      case '6':
        monitor_sensors();
        break;
      default:
        Serial.println("Invalid selection!");
    }
    
    print_menu();
  }
}

// ============ ACCELEROMETER CALIBRATION ============
void calibrate_accelerometer() {
  Serial.println("\n=== ACCELEROMETER CALIBRATION ===");
  Serial.println("Instructions:");
  Serial.println("1. Place sensor on FLAT, LEVEL surface");
  Serial.println("2. Keep ABSOLUTELY STILL");
  Serial.println("3. Press any key to START");
  Serial.println("4. Wait 10 seconds without moving\n");
  
  while (!Serial.available()) delay(100);
  while (Serial.available()) Serial.read(); // Clear buffer
  
  Serial.println("Calibrating... DO NOT MOVE SENSOR");
  
  float sum_x = 0, sum_y = 0, sum_z = 0;
  int samples = 200; // 2 seconds at ~100 Hz
  
  for (int i = 0; i < samples; i++) {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    
    sum_x += a.acceleration.x;
    sum_y += a.acceleration.y;
    sum_z += a.acceleration.z;
    
    delay(10);
    if (i % 50 == 0) Serial.print(".");
  }
  
  Serial.println(" Done!\n");
  
  // Calculate averages
  float avg_x = sum_x / samples;
  float avg_y = sum_y / samples;
  float avg_z = sum_z / samples;
  
  // X and Y should be 0, Z should be 9.81 (1g)
  accel_offset_x = avg_x - 0.0;
  accel_offset_y = avg_y - 0.0;
  accel_offset_z = avg_z - 9.81;
  
  Serial.println("=== CALIBRATION COMPLETE ===");
  Serial.print("Accel Offset X: ");
  Serial.println(accel_offset_x, 3);
  Serial.print("Accel Offset Y: ");
  Serial.println(accel_offset_y, 3);
  Serial.print("Accel Offset Z: ");
  Serial.println(accel_offset_z, 3);
  Serial.println();
}

// ============ GYROSCOPE CALIBRATION ============
void calibrate_gyroscope() {
  Serial.println("\n=== GYROSCOPE CALIBRATION ===");
  Serial.println("Instructions:");
  Serial.println("1. Place sensor on FLAT surface");
  Serial.println("2. Keep ABSOLUTELY STILL");
  Serial.println("3. Press any key to START");
  Serial.println("4. Wait 10 seconds without moving\n");
  
  while (!Serial.available()) delay(100);
  while (Serial.available()) Serial.read(); // Clear buffer
  
  Serial.println("Calibrating... DO NOT MOVE SENSOR");
  
  float sum_x = 0, sum_y = 0, sum_z = 0;
  int samples = 200; // 2 seconds at ~100 Hz
  
  for (int i = 0; i < samples; i++) {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    
    sum_x += g.gyro.x;
    sum_y += g.gyro.y;
    sum_z += g.gyro.z;
    
    delay(10);
    if (i % 50 == 0) Serial.print(".");
  }
  
  Serial.println(" Done!\n");
  
  // Calculate averages (when stationary, should all be 0)
  gyro_offset_x = sum_x / samples;
  gyro_offset_y = sum_y / samples;
  gyro_offset_z = sum_z / samples;
  
  Serial.println("=== CALIBRATION COMPLETE ===");
  Serial.print("Gyro Offset X (°/s): ");
  Serial.println(gyro_offset_x, 3);
  Serial.print("Gyro Offset Y (°/s): ");
  Serial.println(gyro_offset_y, 3);
  Serial.print("Gyro Offset Z (°/s): ");
  Serial.println(gyro_offset_z, 3);
  Serial.println("\nNote: Apply these offsets by subtracting from raw gyro readings");
  Serial.println();
}

// ============ VL53L0X CALIBRATION ============
void calibrate_vl53l0x() {
  Serial.println("\n=== VL53L0X CALIBRATION ===");
  Serial.println("Instructions:");
  Serial.println("1. Measure ACTUAL distance to target (in mm)");
  Serial.println("2. Enter the distance via Serial");
  Serial.println("3. Sensor will take 10 readings");
  Serial.println("4. Offset will be calculated\n");
  
  Serial.println("Enter ACTUAL distance in mm (e.g., 500): ");
  
  // Wait for input
  while (!Serial.available()) delay(100);
  int actual_distance = Serial.parseInt();
  
  Serial.print("Entered: ");
  Serial.print(actual_distance);
  Serial.println(" mm\n");
  
  Serial.println("Taking 10 sensor readings...");
  
  float sum_measured = 0;
  for (int i = 0; i < 10; i++) {
    uint16_t distance = sensor.readRangeSingleMillimeters();
    
    if (!sensor.timeoutOccurred()) {
      sum_measured += distance;
      Serial.print("  Reading ");
      Serial.print(i+1);
      Serial.print(": ");
      Serial.print(distance);
      Serial.println(" mm");
    } else {
      Serial.println("  Timeout!");
    }
    delay(100);
  }
  
  float avg_measured = sum_measured / 10;
  vl53_distance_offset = avg_measured - actual_distance;
  
  Serial.println("\n=== CALIBRATION COMPLETE ===");
  Serial.print("Average Measured: ");
  Serial.print(avg_measured, 1);
  Serial.println(" mm");
  Serial.print("Actual Distance: ");
  Serial.print(actual_distance);
  Serial.println(" mm");
  Serial.print("Distance Offset: ");
  Serial.print(vl53_distance_offset, 1);
  Serial.println(" mm");
  Serial.println("\nTo correct readings: measured_distance - offset");
  Serial.println();
}

// ============ VIEW OFFSETS ============
void view_offsets() {
  Serial.println("\n=== CURRENT CALIBRATION OFFSETS ===");
  Serial.println("Accelerometer (g):");
  Serial.print("  X: ");
  Serial.println(accel_offset_x, 4);
  Serial.print("  Y: ");
  Serial.println(accel_offset_y, 4);
  Serial.print("  Z: ");
  Serial.println(accel_offset_z, 4);
  
  Serial.println("\nGyroscope (°/s):");
  Serial.print("  X: ");
  Serial.println(gyro_offset_x, 4);
  Serial.print("  Y: ");
  Serial.println(gyro_offset_y, 4);
  Serial.print("  Z: ");
  Serial.println(gyro_offset_z, 4);
  
  Serial.println("\nVL53L0X Distance (mm):");
  Serial.print("  Offset: ");
  Serial.println(vl53_distance_offset, 1);
  Serial.println();
}

// ============ RESET OFFSETS ============
void reset_offsets() {
  accel_offset_x = 0;
  accel_offset_y = 0;
  accel_offset_z = 0;
  gyro_offset_x = 0;
  gyro_offset_y = 0;
  gyro_offset_z = 0;
  vl53_distance_offset = 0;
  
  Serial.println("\n✓ All offsets reset to 0\n");
}

// ============ SENSOR MONITORING ============
void monitor_sensors() {
  Serial.println("\n=== SENSOR DATA MONITORING ===");
  Serial.println("(Press any key to exit)\n");
  
  while (!Serial.available()) {
    // MPU6050 data
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    
    // VL53L0X data
    uint16_t distance = sensor.readRangeSingleMillimeters();
    
    // Print with offsets applied
    Serial.print("Accel(g): ");
    Serial.print(a.acceleration.x - accel_offset_x, 2);
    Serial.print(", ");
    Serial.print(a.acceleration.y - accel_offset_y, 2);
    Serial.print(", ");
    Serial.print(a.acceleration.z - accel_offset_z, 2);
    
    Serial.print(" | Gyro(°/s): ");
    Serial.print(g.gyro.x - gyro_offset_x, 2);
    Serial.print(", ");
    Serial.print(g.gyro.y - gyro_offset_y, 2);
    Serial.print(", ");
    Serial.print(g.gyro.z - gyro_offset_z, 2);
    
    Serial.print(" | Dist: ");
    if (!sensor.timeoutOccurred()) {
      Serial.print(distance - (int)vl53_distance_offset);
      Serial.println(" mm");
    } else {
      Serial.println("--- mm (timeout)");
    }
    
    delay(500);
  }
  
  // Clear buffer
  while (Serial.available()) Serial.read();
  Serial.println("\n✓ Exited monitoring\n");
}
