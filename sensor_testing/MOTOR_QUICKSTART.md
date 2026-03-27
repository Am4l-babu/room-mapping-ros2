# Motor Control Quick Start Guide

## TL;DR - Essential Commands

### Build and Upload Motor Test:
```bash
cd ~/ros2_ws/sensor_testing
pio run -e motor_test -t upload --upload-port /dev/ttyACM1
```

### Monitor Output:
```bash
pio device monitor --port /dev/ttyACM1 --baud 115200
```

---

## Hardware Checklist

Before running code, verify physical connections:

- [ ] L298N GND connected to ESP32 GND
- [ ] L298N +12V connected to external power (NOT ESP32)
- [ ] Motor 1 wires connected to L298N OUT1/OUT2
- [ ] Motor 2 wires connected to L298N OUT3/OUT4
- [ ] Motor 1 Encoder DO → GPIO 4
- [ ] Motor 2 Encoder DO → GPIO 5
- [ ] Both Encoders VCC → 3.3V
- [ ] Both Encoders GND → GND (common)
- [ ] I2C sensors (VL53L0X, MPU6050) on GPIO 21-22 (unchanged)
- [ ] Servo on GPIO 13 (unchanged)

---

## GPIO Pin Reference

| Function | GPIO | Status |
|----------|------|--------|
| Motor 1 Direction A | GPIO 14 | Control |
| Motor 1 Direction B | GPIO 27 | Control |
| Motor 1 Speed (PWM) | GPIO 25 | ⚠️ PWM |
| Motor 2 Direction A | GPIO 26 | Control |
| Motor 2 Direction B | GPIO 33 | Control |
| Motor 2 Speed (PWM) | GPIO 32 | ⚠️ PWM |
| Encoder 1 | GPIO 4 | ⚠️ Interrupt |
| Encoder 2 | GPIO 5 | ⚠️ Interrupt |
| I2C SDA | GPIO 21 | Shared |
| I2C SCL | GPIO 22 | Shared |
| Servo | GPIO 13 | Control |

**Status**: Control = Digital out, ⚠️ PWM = Pulse width modulation, ⚠️ Interrupt = Rising edge interrupt

---

## Code Quick Reference

### Initialize Systems:
```cpp
setup_motors();      // Configure L298N control pins and PWM
setup_encoders();    // Attach interrupt handlers to encoder pins
```

### Control Motors:
```cpp
set_motor1_speed(200);   // Motor 1: 200/255 forward
set_motor2_speed(-150);  // Motor 2: 150/255 reverse  
set_both_motors(100);    // Both motors: 100/255 forward
stop_all_motors();       // Emergency stop
```

### Get Speed Data:
```cpp
update_rpm_calculation();        // Must call periodically!
float rpm1 = get_motor1_rpm();   // Get RPM
float rpm2 = get_motor2_rpm();   // Get RPM
unsigned long p1 = get_motor1_pulses();  // Raw pulse count
unsigned long p2 = get_motor2_pulses();  // Raw pulse count
```

### Calculate Distance:
```cpp
// Assuming 65mm wheel diameter
float distance_mm = get_motor_distance_mm(1, 65.0);  // Motor 1 distance
```

---

## Test Flow

1. **Upload & Connect**:
   - `pio run -e motor_test -t upload --upload-port /dev/ttyACM1`
   - `pio device monitor --port /dev/ttyACM1 --baud 115200`

2. **Observe Test Sequence** (30 second cycle):
   - **0-5s**: Motor 1 ramps 0→255
   - **5-10s**: Motor 1 constant 200 speed
   - **10-15s**: Motor 2 ramps 0→255
   - **15-20s**: Motor 2 constant 200 speed
   - **20-25s**: Both motors 150 speed
   - **25-30s**: Stop & reset

3. **Expected Output** (every 500ms):
   ```
   ========== MOTOR & ENCODER STATUS ==========
   Motor 1: 78.43% | RPM: 523.45 | Pulses: 1042
   Motor 2: 0.00% | RPM: 0.00 | Pulses: 0
   ==========================================
   ```

---

## Integration with Existing Code

To add motors to your main sensing system:

### Option 1: Use Existing Main (Keeps servo)
```cpp
// In src/main.cpp, add at top:
#include "motors_encoders.h"

// In setup():
setup_motors();
setup_encoders();

// In loop():
update_rpm_calculation();
// Control motors as needed
set_both_motors(speed_value);
```

### Option 2: Use ros2_motors Environment
Build with both main and motors:
```bash
pio run -e esp32dev_motors -t upload --upload-port /dev/ttyACM1
```

---

## Common Issues & Solutions

### Motors don't spin
- Verify 12V power connected to L298N
- Check GPIO connections match pin mapping
- Test GPIO outputs with `digitalWrite(GPIO, HIGH/LOW)`

### Encoders not counting
- Verify GPIO 4, 5 connections
- Check encoder wheel is positioned between IR sensor
- Verify encoder power (3.3V) and GND

### Low RPM readings
- Check motor is spinning (manually rotate to test encoders)
- Verify PWM signal is being sent
- Check for motor stalling (mechanical jam)

### I2C sensors break after motor startup
- Add 100µF capacitor to L298N 12V line
- Verify common GND reference
- Ensure L298N power is separate from ESP32 power

---

## Performance Specs

### Motor Control:
- PWM Frequency: 5 kHz
- Resolution: 8-bit (0-255)
- Speed Range: -255 (full reverse) to +255 (full forward)

### Encoder:
- Slots per Revolution: 20
- Max RPM Measurement: ~6000 RPM (at 5kHz sample)
- Sample Time: 1000ms (1 second)
- Interrupt Type: Rising edge

### GPIO Performance:
- Interrupt Response: <1µs
- PWM Jitter: <10µs typical
- I2C Bus Clock: 100 kHz (unchanged)

---

## Next Optimizations

1. **Motor Synchronization**: Make both motors run at same speed
2. **Speed Feedback Loop**: Use RPM to adjust PWM automatically
3. **Distance Tracking**: Calculate position based on encoder pulses
4. **Obstacle Avoidance**: Combine with VL53L0X distance sensor
5. **IMU Orientation**: Use MPU6050 for turning/heading control

---

## PlatformIO Environments

```bash
# Motor test only (initial verification)
pio run -e motor_test -t upload --upload-port /dev/ttyACM1

# Full system (current setup - servo, sensors, motors)
pio run -e esp32dev_motors -t upload --upload-port /dev/ttyACM1
```

---

## Resources

- **L298N Datasheet**: Search "L298N motor driver datasheet"
- **HC-89 Specs**: "HC-89 interrupt sensor" or "speed sensor module"
- **ESP32 Interrupts**: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/intr_alloc.html
- **PlatformIO Docs**: https://docs.platformio.org/

