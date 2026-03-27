# Motor Driver & Encoder Wiring Guide

## Overview
- **Motor Driver**: L298N (2 motors)
- **Motors**: 2x DC Motors with 20-slot encoders
- **Encoder Type**: HC-89 IR Sensor (block detection)
- **Microcontroller**: ESP32 Dev Board
- **Existing Peripherals**: Servo, MPU6050, VL53L0X (keep operational)

---

## Pin Mapping

### L298N Motor Driver Connections

#### Power Connections (These go to external power supply, NOT ESP32):
- **L298N GND** → Common GND with ESP32
- **L298N +12V** → External 12V power supply
- **ESP32 GND** → External power supply GND (common reference)

#### Motor 1 Control (Left Motor)
| L298N Pin | ESP32 GPIO | Function | Purpose |
|-----------|-----------|----------|---------|
| IN1 | GPIO 14 | Motor direction A | Forward/Reverse control |
| IN2 | GPIO 27 | Motor direction B | Forward/Reverse control |
| ENA | GPIO 25 | PWM speed control | Speed 0-255 |
| OUT1 | Motor+ | Motor power | Motor positive terminal |
| OUT2 | Motor- | Motor power | Motor negative terminal |

#### Motor 2 Control (Right Motor)
| L298N Pin | ESP32 GPIO | Function | Purpose |
|-----------|-----------|----------|---------|
| IN3 | GPIO 26 | Motor direction A | Forward/Reverse control |
| IN4 | GPIO 33 | Motor direction B | Forward/Reverse control |
| ENB | GPIO 32 | PWM speed control | Speed 0-255 |
| OUT3 | Motor+ | Motor power | Motor positive terminal |
| OUT4 | Motor- | Motor power | Motor negative terminal |

### HC-89 Encoder Sensors

#### Motor 1 Encoder
| HC-89 Pin | ESP32 GPIO | Function | Notes |
|-----------|-----------|----------|-------|
| VCC | 3.3V | Power (use 3.3V regulator if needed) | Connect to ESP32 3.3V output |
| GND | GND | Ground | Common ground |
| DO | GPIO 4 | Digital Output | TTL square wave (interrupt pin) |
| AO | Not used | Analog output | Optional, not used |

#### Motor 2 Encoder
| HC-89 Pin | ESP32 GPIO | Function | Notes |
|-----------|-----------|----------|-------|
| VCC | 3.3V | Power | Connect to ESP32 3.3V output |
| GND | GND | Ground | Common ground |
| DO | GPIO 5 | Digital Output | TTL square wave (interrupt pin) |
| AO | Not used | Analog output | Optional, not used |

### Existing I2C Sensors (Keep in place)

| Sensor | Pin | Component | Notes |
|--------|-----|-----------|-------|
| VL53L0X | GPIO 21 (SDA) | Distance sensor | 0x29 address |
| VL53L0X | GPIO 22 (SCL) | Distance sensor | 100kHz I2C |
| MPU6050 | GPIO 21 (SDA) | IMU | 0x68 address |
| MPU6050 | GPIO 22 (SCL) | IMU | Same I2C bus |

### Servo (Keep as is)

| Servo Pin | ESP32 GPIO | Function |
|-----------|-----------|----------|
| Signal | GPIO 13 | Servo PWM control |
| VCC | 5V | Servo power |
| GND | GND | Servo ground |

---

## GPIO Availability Summary

### Used Pins:
- **I2C Bus**: GPIO 21 (SDA), GPIO 22 (SCL)
- **Servo**: GPIO 13
- **Motor 1**: GPIO 14, 25, 27
- **Motor 2**: GPIO 26, 32, 33
- **Encoder 1**: GPIO 4
- **Encoder 2**: GPIO 5

### Total Used: 11 GPIO pins

### Still Available on ESP32:
- GPIO 0, 2, 12, 15, 16, 17, 18, 19, 23, 34, 35, 36, 39
- Note: GPIO 36, 39 are input-only (ADC)

---

## Circuit Diagram Text Representation

```
ESP32 Dev Board
┌─────────────────────────────────────────────────────────┐
│                                                           │
│  GND ──────────────────────────────────────────────┐      │
│                                                   │      │
│  3.3V ─────┬────────────────────────────────────┐ │      │
│            │                                    │ │      │
│  GPIO4 ────┤ Motor1 Encoder (HC-89)  DO        │ │      │
│  GPIO5 ────┤ Motor2 Encoder (HC-89)  DO        │ │      │
│            │                                    │ │      │
│  GPIO13 ──→ Servo Signal                       │ │      │
│  GPIO14 ──→ Motor1 IN1                         │ │      │
│  GPIO25 ──→ Motor1 ENA (PWM)                   │ │      │
│  GPIO27 ──→ Motor1 IN2                         │ │      │
│  GPIO26 ──→ Motor2 IN3                         │ │      │
│  GPIO32 ──→ Motor2 ENB (PWM)                   │ │      │
│  GPIO33 ──→ Motor2 IN4                         │ │      │
│                                                │ │      │
│  GPIO21 ──→ I2C SDA (VL53L0X, MPU6050)        │ │      │
│  GPIO22 ──→ I2C SCL (VL53L0X, MPU6050)        │ │      │
│                                                │ │      │
└────────────────────────────────────────────────┼─┼──────┘
                                                │ │
                            ┌───────────────────┘ │
                            │                     │
         ┌──────────────────┴─────────────────────┘
         │
         │  L298N Motor Driver Module
         │  ┌─────────────────────────────────────┐
         │  │                                     │
    ┌────┤GND                                    │
    │    │                                      │
    │    │  Motor1 Control:                     │
    │    │  IN1 ← GPIO14                        │
    │    │  IN2 ← GPIO27                        │
    │    │  ENA ← GPIO25 (PWM)                  │
    │    │  OUT1 → Motor1+                      │
    │    │  OUT2 → Motor1-                      │
    │    │                                      │
    │    │  Motor2 Control:                     │
    │    │  IN3 ← GPIO26                        │
    │    │  IN4 ← GPIO33                        │
    │    │  ENB ← GPIO32 (PWM)                  │
    │    │  OUT3 → Motor2+                      │
    │    │  OUT4 → Motor2-                      │
    │    │                                      │
    │    │  Power Supply (External):            │
    │    │  +12V ← External PSU                 │
    │    │  GND ← External PSU (common ref)    │
    │    │                                      │
    │    └─────────────────────────────────────┘
    │
    └─── Common Ground Reference
         (All devices share same GND)
```

---

## Assembly Steps

1. **Connect Ground Reference**
   - Connect ESP32 GND to L298N GND
   - Connect L298N GND to external power supply GND
   - This ensures all devices share a common ground reference

2. **Connect L298N Motor Driver to ESP32**
   - IN1 (GPIO14), IN2 (GPIO27) → Motor 1 direction
   - ENA (GPIO25/PWM) → Motor 1 speed control
   - IN3 (GPIO26), IN4 (GPIO33) → Motor 2 direction
   - ENB (GPIO32/PWM) → Motor 2 speed control

3. **Connect Motors to L298N**
   - Motor 1 wires → OUT1, OUT2
   - Motor 2 wires → OUT3, OUT4

4. **Connect Encoders to ESP32**
   - Motor 1 Encoder DO → GPIO 4
   - Motor 2 Encoder DO → GPIO 5
   - Both Encoders VCC → 3.3V
   - Both Encoders GND → GND

5. **Verify Existing Connections**
   - I2C Bus (GPIO 21, 22) for sensors
   - Servo signal (GPIO 13)

---

## Power Supply Considerations

### External Power for L298N:
- Recommended: 12V 2A minimum power supply
- Motor current varies by load
- Each motor can draw 1-2A continuously

### ESP32 Power:
- 5V USB or external regulated 5V
- Typical current: 80-200mA

### Important**: Keep external L298N power supply separate from ESP32 power!
- This prevents voltage spikes from affecting ESP32
- Use common GND reference only

---

## Encoder Operation

### HC-89 Specifications:
- **Detection Method**: Infrared block detection
- **Output**: Digital square wave (TTL logic)
- **Rising Edge**: Each slot passing sensor triggers rising edge
- **Trigger Frequency**: 20 pulses per full motor revolution
- **Output Pin**: DO (pull-up may be needed)

### Encoder Pulse Pattern:
```
Time  │  Encoder Output (DO)
──────┼──────────────
      │  ┌─┐     ┌─┐     ┌─
      │  │ │     │ │     │ 
──────┴──┘ └─────┘ └─────┘
```

Each rising edge = one slot detected
20 rising edges = 1 full revolution

### Speed Calculation:
```
RPM = (Pulses × 60) / (20 × Sample_Time_in_Seconds)

Example:
- 200 pulses in 1 second
- RPM = (200 × 60) / (20 × 1) = 600 RPM
```

---

## Testing Procedure

1. **Build and Upload Motor Test**:
   ```bash
   cd ~/ros2_ws/sensor_testing
   pio run -e motor_test -t upload --upload-port /dev/ttyACM1
   ```

2. **Open Serial Monitor**:
   ```bash
   pio device monitor --port /dev/ttyACM1 --baud 115200
   ```

3. **Observe Output**:
   - Motor 1 should ramp 0→255→0
   - Motor 2 should ramp 0→255→0
   - Encoder counts should increment
   - RPM should be calculated and displayed

4. **Verify Encoders**:
   - Pulse count should increase as motors spin
   - RPM calculation should be reasonable
   - No negative RPM unless motor reverses

---

## Troubleshooting

### Issue: Motors don't spin
- Check L298N power supply connection (12V)
- Verify GPIO pins are correctly connected
- Test with serial monitor to verify GPIO values
- Check motor polarity (swap OUT1/OUT2 if reversed)

### Issue: Encoders not counting
- Check GPIO 4, 5 connections
- Verify encoder VCC is 3.3V
- Check encoder GND connection
- Verify encoder wheel position (should be between IR sensor)
- Try interrupts on both rising and falling edges

### Issue: Inconsistent RPM readings
- Check encoder wheel for damage or misalignment
- Ensure encoder is properly mounted
- Inspect for stable power supply voltage
- Verify no electrical noise from motor

### Issue: I2C sensors stop working after motor startup
- Add capacitor to motor power lines (100µF)
- Keep L298N power supply separate from ESP32
- Add pull-up resistors to I2C lines (if needed)
- Check for common ground reference

---

## Next Steps

1. Build and upload motor test environment
2. Verify motor rotation and encoder counting
3. Integrate motor functions into main.cpp
4. Implement motor synchronization algorithms
5. Add obstacle avoidance using VL53L0X
