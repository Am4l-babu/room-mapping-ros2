#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

Adafruit_MPU6050 mpu;

// Pin configuration
const int MPU_SDA = 21;  // GPIO 21 for MPU6050 SDA (XDA)
const int MPU_SCL = 22;  // GPIO 22 for MPU6050 SCL (XSCL)

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n\n====================================");
  Serial.println("MPU6050 STANDALONE TEST");
  Serial.println("====================================\n");
  
  // Print pin configuration
  Serial.println("Pin Configuration:");
  Serial.println("  - XDA (SDA): GPIO 21");
  Serial.println("  - XSCL (SCL): GPIO 22");
  Serial.println("  - AD0: Ground (0x68) or VCC (0x69)");
  Serial.println("  - VCC: 3.3V or 5V");
  Serial.println("  - GND: GND\n");
  
  // Initialize I2C bus on GPIO 4-5 in MASTER MODE
  Serial.println("Step 1: Initializing I2C Bus (GPIO 21-22) in Master Mode...");
  Wire.begin(MPU_SDA, MPU_SCL, 100000);  // SDA, SCL, frequency - Master Mode
  delay(500);
  Serial.println("  ✓ I2C Bus initialized in Master Mode\n");
  
  // Scan for all devices on I2C Bus
  Serial.println("Step 2: Scanning I2C Bus (GPIO 21-22) for devices...");
  byte error;
  int devicesFound = 0;
  for (byte address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("  Found device at address: 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
      devicesFound++;
    }
  }
  
  if (devicesFound == 0) {
    Serial.println("  ⚠ ⚠ ⚠ NO DEVICES FOUND! ⚠ ⚠ ⚠");
    Serial.println("  Check your connections:");
    Serial.println("    - XDA pin connected to GPIO 21");
    Serial.println("    - XSCL pin connected to GPIO 22");
    Serial.println("    - VCC connected to 3.3V or 5V");
    Serial.println("    - GND connected to GND");
    Serial.println("    - AD0 pin (if present) properly configured\n");
  } else {
    Serial.println();
  }
  
  // Try to initialize MPU6050
  Serial.println("Step 3: Attempting to initialize MPU6050...\n");
  
  bool mpu_ok = false;
  
  // Try address 0x68 (AD0 connected to GND)
  Serial.println("  Trying address 0x68 (AD0 = GND)...");
  if (mpu.begin(0x68, &Wire)) {
    Serial.println("    ✓ SUCCESS! MPU6050 found at address 0x68");
    Serial.println("    Connection verified: GPIO 21 (SDA) & GPIO 22 (SCL)\n");
    mpu_ok = true;
  } else {
    Serial.println("    ✗ Not at 0x68\n");
    
    // Try address 0x69 (AD0 connected to VCC)
    Serial.println("  Trying address 0x69 (AD0 = VCC)...");
    if (mpu.begin(0x69, &Wire)) {
      Serial.println("    ✓ SUCCESS! MPU6050 found at address 0x69");
      Serial.println("    Connection verified: GPIO 21 (SDA) & GPIO 22 (SCL)\n");
      mpu_ok = true;
    } else {
      Serial.println("    ✗ Not at 0x69\n");
    }
  }
  
  if (!mpu_ok) {
    Serial.println("====================================");
    Serial.println("⚠ FAILED TO INITIALIZE MPU6050 ⚠");
    Serial.println("====================================");
    Serial.println("Troubleshooting:");
    Serial.println("1. Check all wiring connections");
    Serial.println("2. Verify GPIO 4 and GPIO 5 are not conflict");
    Serial.println("3. Check if MPU6050 power supply is stable");
    Serial.println("4. Try 3.3K pull-up resistors on SDA/SCL");
    Serial.println("5. Look for I2C address with scanner above");
    while (1) {
      delay(1000);
    }
  }
  
  // Configure MPU6050 ranges
  Serial.println("Step 4: Configuring MPU6050 settings...");
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  Serial.println("  ✓ Accelerometer range: ±8G");
  
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  Serial.println("  ✓ Gyro range: ±500°/s");
  
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  Serial.println("  ✓ Filter bandwidth: 21 Hz\n");
  
  // Print sensor information
  Serial.println("MPU6050 Sensor Information:");
  Serial.println("  - Accelerometer (3-axis): ±8G");
  Serial.println("  - Gyroscope (3-axis): ±500°/s");
  Serial.println("  - Temperature sensor\n");
  
  Serial.println("====================================");
  Serial.println("Starting Data Collection");
  Serial.println("====================================\n");
  
  delay(1000);
}

void loop() {
  // Get sensor events
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  
  // Print timestamp
  static unsigned long lastPrint = 0;
  unsigned long now = millis();
  if (now - lastPrint >= 500) {  // Print every 500ms for readability
    lastPrint = now;
    
    Serial.print("Time: ");
    Serial.print(now / 1000);
    Serial.print("s | ");
    
    // Print Accelerometer data
    Serial.print("Accel(g): ");
    Serial.print("X=");
    Serial.print(a.acceleration.x, 3);
    Serial.print(" Y=");
    Serial.print(a.acceleration.y, 3);
    Serial.print(" Z=");
    Serial.print(a.acceleration.z, 3);
    Serial.print(" | ");
    
    // Print Gyro data
    Serial.print("Gyro(°/s): ");
    Serial.print("X=");
    Serial.print(g.gyro.x, 2);
    Serial.print(" Y=");
    Serial.print(g.gyro.y, 2);
    Serial.print(" Z=");
    Serial.print(g.gyro.z, 2);
    Serial.print(" | ");
    
    // Print Temperature
    Serial.print("Temp(°C): ");
    Serial.println(temp.temperature, 1);
  }
}
