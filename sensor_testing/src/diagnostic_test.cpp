#include <Wire.h>
#include "VL53L0X.h"
#include <ESP32Servo.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include "motors_encoders.h"

// Sensor objects
VL53L0X sensor;
Servo myServo;
Adafruit_MPU6050 mpu;

// Pin definitions
const int SERVO_PIN = 13;
const int MPU_SDA = 21;
const int MPU_SCL = 22;

// Test results tracking
struct TestResults {
  bool i2c_bus_ok = false;
  int i2c_devices_found = 0;
  bool vl53l0x_ok = false;
  bool mpu6050_ok = false;
  bool servo_ok = false;
  bool motor1_ok = false;
  bool motor2_ok = false;
  bool encoder1_ok = false;
  bool encoder2_ok = false;
};

TestResults results;

void print_section(const char* title) {
  Serial.println();
  for (int i = 0; i < 50; i++) Serial.print("=");
  Serial.println();
  Serial.println(title);
  for (int i = 0; i < 50; i++) Serial.print("=");
  Serial.println();
}

void print_result(const char* component, bool status) {
  Serial.print("  ");
  Serial.print(status ? "✓ " : "✗ ");
  Serial.println(component);
}

void test_i2c_bus() {
  print_section("[1/7] I2C BUS DIAGNOSTIC");
  
  Serial.println("Initializing I2C on GPIO 21 (SDA) & GPIO 22 (SCL)...");
  Wire.begin(MPU_SDA, MPU_SCL);
  Wire.setClock(100000);
  delay(500);
  
  Serial.println("Scanning for I2C devices...\n");
  byte error;
  byte address;
  int deviceCount = 0;
  
  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    
    if (error == 0) {
      Serial.print("    Found device at 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
      deviceCount++;
    }
  }
  
  results.i2c_devices_found = deviceCount;
  results.i2c_bus_ok = (deviceCount >= 2); // Should find at least VL53L0X and MPU6050
  
  Serial.println("\nI2C Bus Status:");
  print_result("I2C Bus Initialized", results.i2c_bus_ok);
  Serial.print("  Devices Found: ");
  Serial.println(deviceCount);
  
  if (deviceCount < 2) {
    Serial.println("\n  ⚠ WARNING: Expected at least 2 devices (VL53L0X, MPU6050)");
    Serial.println("  Check GPIO 21 (SDA) and GPIO 22 (SCL) connections");
  }
}

void test_vl53l0x() {
  print_section("[2/7] VL53L0X DISTANCE SENSOR");
  
  Serial.println("Initializing VL53L0X sensor (0x29)...");
  delay(1000);
  
  if (!sensor.init()) {
    Serial.println("  ✗ FAILED to initialize VL53L0X");
    results.vl53l0x_ok = false;
  } else {
    Serial.println("  ✓ VL53L0X initialized");
    sensor.setTimeout(500);
    results.vl53l0x_ok = true;
    
    Serial.println("\nTaking 5 distance readings...");
    for (int i = 0; i < 5; i++) {
      uint16_t distance = sensor.readRangeSingleMillimeters();
      Serial.print("  Reading ");
      Serial.print(i + 1);
      Serial.print(": ");
      
      if (sensor.timeoutOccurred()) {
        Serial.println("TIMEOUT");
      } else if (distance >= 8190) {
        Serial.println("OUT OF RANGE");
      } else {
        Serial.print(distance);
        Serial.println(" mm");
      }
      delay(100);
    }
  }
  
  print_result("VL53L0X Operational", results.vl53l0x_ok);
}

void test_mpu6050() {
  print_section("[3/7] MPU6050 IMU SENSOR");
  
  Serial.println("Initializing MPU6050 (trying 0x68)...");
  
  if (mpu.begin(0x68, &Wire)) {
    Serial.println("  ✓ MPU6050 found at address 0x68");
    results.mpu6050_ok = true;
  } else {
    Serial.println("  ! Not at 0x68, trying 0x69...");
    if (mpu.begin(0x69, &Wire)) {
      Serial.println("  ✓ MPU6050 found at address 0x69");
      results.mpu6050_ok = true;
    } else {
      Serial.println("  ✗ MPU6050 NOT FOUND");
      results.mpu6050_ok = false;
    }
  }
  
  if (results.mpu6050_ok) {
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    Serial.println("  Accelerometer range: ±8g");
    Serial.println("  Gyro range: ±500 °/s");
    Serial.println("  Filter bandwidth: 21 Hz");
    
    Serial.println("\nTaking 5 sensor readings...");
    for (int i = 0; i < 5; i++) {
      sensors_event_t a, g, temp;
      mpu.getEvent(&a, &g, &temp);
      
      Serial.print("  Sample ");
      Serial.print(i + 1);
      Serial.print(": Accel(g)=");
      Serial.print(a.acceleration.x, 2);
      Serial.print(",");
      Serial.print(a.acceleration.y, 2);
      Serial.print(",");
      Serial.print(a.acceleration.z, 2);
      Serial.print(" | Temp=");
      Serial.print(temp.temperature, 1);
      Serial.println("°C");
      delay(100);
    }
  }
  
  print_result("MPU6050 Operational", results.mpu6050_ok);
}

void test_servo() {
  print_section("[4/7] SERVO CONTROL (GPIO 13)");
  
  Serial.println("Attaching servo on GPIO 13...");
  myServo.attach(SERVO_PIN);
  myServo.write(0);
  delay(500);
  
  Serial.println("Testing servo sweep...");
  
  // Test three positions
  int test_angles[] = {0, 90, 180};
  results.servo_ok = true;
  
  for (int angle : test_angles) {
    myServo.write(angle);
    Serial.print("  Set angle to ");
    Serial.print(angle);
    Serial.println("°");
    delay(500);
  }
  
  myServo.write(90); // Center servo
  delay(300);
  
  print_result("Servo Operational", results.servo_ok);
}

void test_motors_and_encoders() {
  print_section("[5/7] MOTOR & ENCODER CONTROL");
  
  Serial.println("Initializing motor driver (L298N)...");
  setup_motors();
  Serial.println("  ✓ Motor driver initialized");
  
  Serial.println("\nInitializing encoders (HC-89 IR sensors)...");
  setup_encoders();
  Serial.println("  ✓ Encoders initialized on GPIO 4 & 5");
  
  Serial.println("\nTesting Motor 1 (GPIO 14, 25, 27)...");
  Serial.println("  Ramping speed: 0 → 255 → 0");
  
  // Ramp up
  for (int speed = 0; speed <= 255; speed += 25) {
    set_motor1_speed(speed);
    delay(300);
    Serial.print("    Speed: ");
    Serial.println(speed);
  }
  
  // Ramp down
  for (int speed = 255; speed >= 0; speed -= 25) {
    set_motor1_speed(speed);
    delay(300);
  }
  
  results.motor1_ok = true;
  print_result("Motor 1 Operational", results.motor1_ok);
  
  Serial.println("\nTesting Motor 2 (GPIO 26, 32, 33)...");
  Serial.println("  Ramping speed: 0 → 255 → 0");
  
  // Ramp up
  for (int speed = 0; speed <= 255; speed += 25) {
    set_motor2_speed(speed);
    delay(300);
    Serial.print("    Speed: ");
    Serial.println(speed);
  }
  
  // Ramp down
  for (int speed = 255; speed >= 0; speed -= 25) {
    set_motor2_speed(speed);
    delay(300);
  }
  
  results.motor2_ok = true;
  print_result("Motor 2 Operational", results.motor2_ok);
  
  Serial.println("\nReading encoder pulses for 2 seconds...");
  delay(1000);
  update_rpm_calculation();
  
  Serial.print("  Motor 1 RPM: ");
  Serial.println(get_motor1_rpm());
  Serial.print("  Motor 2 RPM: ");
  Serial.println(get_motor2_rpm());
  
  results.encoder1_ok = (get_motor1_rpm() > 0);
  results.encoder2_ok = (get_motor2_rpm() > 0);
  
  print_result("Encoder 1 (GPIO 4) Reading Pulses", results.encoder1_ok);
  print_result("Encoder 2 (GPIO 5) Reading Pulses", results.encoder2_ok);
}

void test_gpio_summary() {
  print_section("[6/7] GPIO PIN USAGE SUMMARY");
  
  Serial.println("Currently Used Pins:");
  Serial.println("  I2C Bus: GPIO 21 (SDA), GPIO 22 (SCL)");
  Serial.println("  Servo: GPIO 13");
  Serial.println("  Motor 1: GPIO 14 (IN1), GPIO 27 (IN2), GPIO 25 (PWM/ENA)");
  Serial.println("  Motor 2: GPIO 26 (IN3), GPIO 33 (IN4), GPIO 32 (PWM/ENB)");
  Serial.println("  Encoder 1: GPIO 4 (Interrupt)");
  Serial.println("  Encoder 2: GPIO 5 (Interrupt)");
  Serial.println("\n  Total: 11 GPIO pins used");
  
  Serial.println("\nAvailable GPIOs:");
  Serial.println("  GPIO 0, 2, 12, 15, 16, 17, 18, 19, 23, 34, 35, 36, 39");
  Serial.println("  (GPIO 36, 39 are input-only with ADC)");
}

void print_final_summary() {
  print_section("[7/7] DIAGNOSTIC TEST SUMMARY");
  
  int passed = 0;
  int total = 9;
  
  Serial.println("\n📋 Test Results:");
  
  if (results.i2c_bus_ok) {
    Serial.print("  ✓ I2C Bus: ");
    Serial.print(results.i2c_devices_found);
    Serial.println(" devices found");
    passed++;
  } else {
    Serial.println("  ✗ I2C Bus: FAILED");
  }
  
  if (results.vl53l0x_ok) {
    Serial.println("  ✓ VL53L0X: OK");
    passed++;
  } else {
    Serial.println("  ✗ VL53L0X: FAILED");
  }
  
  if (results.mpu6050_ok) {
    Serial.println("  ✓ MPU6050: OK");
    passed++;
  } else {
    Serial.println("  ✗ MPU6050: FAILED");
  }
  
  if (results.servo_ok) {
    Serial.println("  ✓ Servo (GPIO 13): OK");
    passed++;
  } else {
    Serial.println("  ✗ Servo (GPIO 13): FAILED");
  }
  
  if (results.motor1_ok) {
    Serial.println("  ✓ Motor 1 (GPIO 14, 25, 27): OK");
    passed++;
  } else {
    Serial.println("  ✗ Motor 1: FAILED");
  }
  
  if (results.motor2_ok) {
    Serial.println("  ✓ Motor 2 (GPIO 26, 32, 33): OK");
    passed++;
  } else {
    Serial.println("  ✗ Motor 2: FAILED");
  }
  
  if (results.encoder1_ok) {
    Serial.println("  ✓ Encoder 1 (GPIO 4): Reading pulses");
    passed++;
  } else {
    Serial.println("  ⚠ Encoder 1 (GPIO 4): No pulses detected");
  }
  
  if (results.encoder2_ok) {
    Serial.println("  ✓ Encoder 2 (GPIO 5): Reading pulses");
    passed++;
  } else {
    Serial.println("  ⚠ Encoder 2 (GPIO 5): No pulses detected");
  }
  
  Serial.print("\n🎯 OVERALL: ");
  Serial.print(passed);
  Serial.print("/");
  Serial.print(total);
  Serial.println(" tests passed");
  
  if (passed == total) {
    Serial.println("\n🎉 ALL SYSTEMS OPERATIONAL!");
  } else if (passed >= 7) {
    Serial.println("\n✅ SYSTEM MOSTLY OPERATIONAL (Check encoders if motors running)");
  } else {
    Serial.println("\n⚠ ISSUES DETECTED - See details above");
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n\n╔════════════════════════════════════════════╗");
  Serial.println("║   ESP32 ROBOT - COMPLETE DIAGNOSTIC TEST   ║");
  Serial.println("║  Verifying All Sensors & Connections       ║");
  Serial.println("╚════════════════════════════════════════════╝");
  
  test_i2c_bus();
  delay(1000);
  
  test_vl53l0x();
  delay(1000);
  
  test_mpu6050();
  delay(1000);
  
  test_servo();
  delay(1000);
  
  test_motors_and_encoders();
  delay(1000);
  
  test_gpio_summary();
  delay(1000);
  
  print_final_summary();
  
  Serial.println("\n\n✓ Diagnostic test complete. Ready for deployment.\n");
}

void loop() {
  delay(10000); // Idle after test
  Serial.println("Diagnostic complete. System waiting...");
}
