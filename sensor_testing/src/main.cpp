#include <Wire.h>
#include "VL53L0X.h"
#include <ESP32Servo.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

VL53L0X sensor;
Servo myServo;
Adafruit_MPU6050 mpu;

const int SERVO_PIN = 13;
const int MPU_SDA = 21;  // GPIO 21 for MPU6050 SDA (XDA)
const int MPU_SCL = 22;  // GPIO 22 for MPU6050 SCL (XSCL)
// MPU6050 Pin Connections:
// - XDA (SDA): GPIO 21
// - XSCL (SCL): GPIO 22
// - AD0: Ground (0x68) or VCC (0x69)
// - INT: GPIO (optional, not used here)

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n\n====================================");
  Serial.println("VL53L0X + MPU6050 + Servo Test");
  Serial.println("====================================");
  
  // Initialize I2C bus for VL53L0X & MPU6050 (GPIO 21-22)
  Serial.println("\n[1/3] Initializing I2C Bus (GPIO 21-22)...");
  Wire.begin(21, 22);
  Wire.setClock(100000);
  delay(500);
  Serial.println("  ✓ I2C Bus ready");
  
  // Scan for devices on I2C Bus
  Serial.println("  Scanning I2C Bus for devices...");
  byte error;
  int devicesFound = 0;
  for (byte address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("    Found device at address: 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
      devicesFound++;
    }
  }
  if (devicesFound == 0) {
    Serial.println("    ⚠ No I2C devices found!");
  }
  
  // Initialize VL53L0X with power-up delay
  Serial.println("[2/3] Initializing VL53L0X sensor...");
  delay(1000);  // Power-up delay for VL53L0X
  
  if (!sensor.init()) {
    Serial.println("  ✗ FAILED to initialize VL53L0X");
  } else {
    Serial.println("  ✓ VL53L0X initialized");
    sensor.setTimeout(500);
  }
  
  // Initialize MPU6050 with fallback
  Serial.println("[3/3] Initializing MPU6050 sensor...");
  delay(100);
  
  bool mpu_ok = false;
  if (mpu.begin(0x68, &Wire)) {
    Serial.println("  ✓ MPU6050 initialized at address 0x68");
    Serial.println("    - SDA: GPIO 21 ✓");
    Serial.println("    - SCL: GPIO 22 ✓");
    Serial.println("    - AD0: GND ✓");
    mpu_ok = true;
  } else {
    Serial.println("  ! MPU6050 not at 0x68, trying 0x69 (AD0 to VCC)...");
    if (mpu.begin(0x69, &Wire)) {
      Serial.println("  ✓ MPU6050 initialized at address 0x69");
      Serial.println("    - SDA: GPIO 21 ✓");
      Serial.println("    - SCL: GPIO 22 ✓");
      Serial.println("    - AD0: VCC ✓");
      mpu_ok = true;
    } else {
      Serial.println("  ✗ FAILED to initialize MPU6050 at any address");
      Serial.println("  Check connections:");
      Serial.println("    - XDA connected to GPIO 21");
      Serial.println("    - XSCL connected to GPIO 22");
      Serial.println("    - VCC connected to 3.3V or 5V");
      Serial.println("    - GND connected to GND");
    }
  }
  
  if (mpu_ok) {
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    Serial.println("  MPU6050 sensors configured successfully");
  }
  
  // Initialize servo
  Serial.println("\nInitializing Servo on GPIO 13...");
  myServo.attach(SERVO_PIN);
  myServo.write(0);
  Serial.println("  ✓ Servo ready\n");
  
  Serial.println("====================================");
  Serial.println("Starting servo sweep with sensor data");
  Serial.println("====================================\n");
  
  // Verify MPU6050 is reading data
  if (mpu_ok) {
    Serial.println("Testing MPU6050 data on pins GPIO 21 (SDA) & GPIO 22 (SCL):");
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
      Serial.print(" | Gyro(°/s)=");
      Serial.print(g.gyro.x, 1);
      Serial.print(",");
      Serial.print(g.gyro.y, 1);
      Serial.print(",");
      Serial.print(g.gyro.z, 1);
      Serial.println(" ✓");
      delay(200);
    }
    Serial.println("  ✓ MPU6050 data verified!\n");
  }
  
  delay(1000);
}

void loop() {
  // Forward sweep: 0 to 180 degrees
  for (int angle = 0; angle <= 180; angle += 2) {
    myServo.write(angle);
    delay(30);
    
    // Read sensors every 10 degrees
    if (angle % 10 == 0) {
      uint16_t distance = sensor.readRangeSingleMillimeters();
      sensors_event_t a, g, temp;
      mpu.getEvent(&a, &g, &temp);
      
      Serial.print("Angle: ");
      Serial.print(angle);
      Serial.print("° | Distance: ");
      
      if (sensor.timeoutOccurred()) {
        Serial.print("TIMEOUT");
      } else if (distance >= 8190) {
        Serial.print("---  ");
      } else {
        Serial.print(distance);
        Serial.print("mm ");
      }
      
      Serial.print("| Accel(g): ");
      Serial.print(a.acceleration.x, 2);
      Serial.print(",");
      Serial.print(a.acceleration.y, 2);
      Serial.print(",");
      Serial.print(a.acceleration.z, 2);
      
      Serial.print(" | Gyro(°/s): ");
      Serial.print(g.gyro.x, 1);
      Serial.print(",");
      Serial.print(g.gyro.y, 1);
      Serial.print(",");
      Serial.print(g.gyro.z, 1);
      
      Serial.println();
    }
  }
  
  // Backward sweep: 180 to 0 degrees
  for (int angle = 180; angle >= 0; angle -= 2) {
    myServo.write(angle);
    delay(30);
    
    // Read sensors every 10 degrees
    if (angle % 10 == 0) {
      uint16_t distance = sensor.readRangeSingleMillimeters();
      sensors_event_t a, g, temp;
      mpu.getEvent(&a, &g, &temp);
      
      Serial.print("Angle: ");
      Serial.print(angle);
      Serial.print("° | Distance: ");
      
      if (sensor.timeoutOccurred()) {
        Serial.print("TIMEOUT");
      } else if (distance >= 8190) {
        Serial.print("---  ");
      } else {
        Serial.print(distance);
        Serial.print("mm ");
      }
      
      Serial.print("| Accel(g): ");
      Serial.print(a.acceleration.x, 2);
      Serial.print(",");
      Serial.print(a.acceleration.y, 2);
      Serial.print(",");
      Serial.print(a.acceleration.z, 2);
      
      Serial.print(" | Gyro(°/s): ");
      Serial.print(g.gyro.x, 1);
      Serial.print(",");
      Serial.print(g.gyro.y, 1);
      Serial.print(",");
      Serial.print(g.gyro.z, 1);
      
      Serial.println();
    }
  }
}

