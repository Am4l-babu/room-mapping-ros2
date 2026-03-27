# Motor & Encoder Connections - Visual Reference

## Quick Pinout Reference Card

```
┌─────────────────────────────────────────────────────┐
│            ESP32 GPIO CONNECTIONS                    │
├─────────────────────────────────────────────────────┤
│                                                      │
│  MOTOR CONTROL PINS:                                │
│  ├─ Motor1 IN1 (Direction) ........ GPIO 14         │
│  ├─ Motor1 IN2 (Direction) ........ GPIO 27         │
│  ├─ Motor1 ENA (Speed PWM) ........ GPIO 25         │
│  ├─ Motor2 IN3 (Direction) ........ GPIO 26         │
│  ├─ Motor2 IN4 (Direction) ........ GPIO 33         │
│  └─ Motor2 ENB (Speed PWM) ........ GPIO 32         │
│                                                      │
│  ENCODER INPUT PINS:                                │
│  ├─ Encoder1 DO ................... GPIO 4          │
│  └─ Encoder2 DO ................... GPIO 5          │
│                                                      │
│  EXISTING (DO NOT CHANGE):                          │
│  ├─ I2C SDA (Sensors) ............. GPIO 21         │
│  ├─ I2C SCL (Sensors) ............. GPIO 22         │
│  ├─ Servo Signal .................. GPIO 13         │
│  ├─ GND (Reference) ............... GND (4 pins)    │
│  └─ 3.3V (Encoder Power) .......... 3.3V            │
│                                                      │
└─────────────────────────────────────────────────────┘


┌──────────────────────────────────────────────────────────┐
│         L298N MOTOR DRIVER CONNECTIONS                    │
├──────────────────────────────────────────────────────────┤
│                                                            │
│  FROM ESP32 GPIO:              TO L298N PIN:             │
│  ─────────────────────────────────────────────           │
│  GPIO 14 ...................... IN1                      │
│  GPIO 27 ...................... IN2                      │
│  GPIO 25 ...................... ENA (PWM)               │
│  GPIO 26 ...................... IN3                      │
│  GPIO 33 ...................... IN4                      │
│  GPIO 32 ...................... ENB (PWM)               │
│  GND .......................... GND (shared)             │
│                                                            │
│  EXTERNAL POWER:           FROM POWER SUPPLY:            │
│  ─────────────────────────────────────────────           │
│  +12V ......................... +12V terminals            │
│  GND .......................... GND terminals (common)    │
│                                                            │
│  MOTOR OUTPUTS:           TO MOTORS:                     │
│  ─────────────────────────────────────────────           │
│  OUT1 ......................... Motor1 Positive           │
│  OUT2 ......................... Motor1 Negative           │
│  OUT3 ......................... Motor2 Positive           │
│  OUT4 ......................... Motor2 Negative           │
│                                                            │
└──────────────────────────────────────────────────────────┘


┌──────────────────────────────────────────────────────────┐
│         HC-89 ENCODER SENSOR CONNECTIONS                  │
├──────────────────────────────────────────────────────────┤
│                                                            │
│  MOTOR 1 ENCODER:                                         │
│  ────────────────────                                     │
│  HC-89 VCC .................... 3.3V                     │
│  HC-89 GND .................... GND                      │
│  HC-89 DO (Output) ............ GPIO 4                   │
│  HC-89 AO (Analog) ............ Not Used                 │
│                                                            │
│  MOTOR 2 ENCODER:                                         │
│  ────────────────────                                     │
│  HC-89 VCC .................... 3.3V                     │
│  HC-89 GND .................... GND                      │
│  HC-89 DO (Output) ............ GPIO 5                   │
│  HC-89 AO (Analog) ............ Not Used                 │
│                                                            │
│  NOTE: Both encoders share 3.3V and GND rails            │
│                                                            │
└──────────────────────────────────────────────────────────┘
```

---

## Wiring Checklist

### Power Connections ⚠️ CRITICAL
- [ ] ESP32 GND connected to L298N GND
- [ ] L298N GND connected to external PSU GND (common ground)
- [ ] External 12V connected to L298N +12V
- [ ] **NOT**: Connect external 12V to ESP32 (use USB)

### Motor 1 Control (Left Motor)
- [ ] GPIO 14 → L298N IN1 (direction)
- [ ] GPIO 27 → L298N IN2 (direction)
- [ ] GPIO 25 → L298N ENA (PWM speed)
- [ ] Motor wires → L298N OUT1 & OUT2
- [ ] Power: From L298N outputs (12V)

### Motor 2 Control (Right Motor)
- [ ] GPIO 26 → L298N IN3 (direction)
- [ ] GPIO 33 → L298N IN4 (direction)
- [ ] GPIO 32 → L298N ENB (PWM speed)
- [ ] Motor wires → L298N OUT3 & OUT4
- [ ] Power: From L298N outputs (12V)

### Encoder 1 (Left Motor)
- [ ] HC-89 VCC → ESP32 3.3V
- [ ] HC-89 GND → ESP32 GND
- [ ] HC-89 DO → GPIO 4

### Encoder 2 (Right Motor)
- [ ] HC-89 VCC → ESP32 3.3V
- [ ] HC-89 GND → ESP32 GND
- [ ] HC-89 DO → GPIO 5

### Existing Sensors (VERIFY UNCHANGED)
- [ ] VL53L0X SDA → GPIO 21
- [ ] VL53L0X SCL → GPIO 22
- [ ] MPU6050 SDA → GPIO 21
- [ ] MPU6050 SCL → GPIO 22
- [ ] Servo Signal → GPIO 13

---

## Signal Flow Diagram

```
┌─────────────────────────────────────────────────────────┐
│                    ESP32 Dev Board                       │
│                                                           │
│  ┌─────────────────────────────────────────────────┐    │
│  │  MOTOR CONTROL                                 │    │
│  │  ─────────────────────────────────────────     │    │
│  │  set_motor1_speed(200)                          │    │
│  │      │                                           │    │
│  │      ├─→ GPIO 14 (OUT) ── High/Low ──┐         │    │
│  │      ├─→ GPIO 27 (OUT) ── High/Low ──┼──┐      │    │
│  │      └─→ GPIO 25 (PWM) ── 5kHz ─────┤  │      │    │
│  │                                       │  │      │    │
│  └───────────────────────────────────────┼──┼──────┘    │
│                                          │  │            │
│        L298N Motor Driver               │  │            │
│        ┌──────────────────────────┐     │  │            │
│        │ IN1 ◄── GPIO 14 ─────────┘     │  │            │
│        │ IN2 ◄── GPIO 27 ────────────┐  │  │            │
│        │ ENA ◄── GPIO 25 (PWM) ──────┴──┘  │            │
│        │                              │     │            │
│        │ OUT1 ··· Motor1+ ───→ ┌─────┴─────┤            │
│        │ OUT2 ··· Motor1- ───→ │   Motor   │            │
│        │                        │    Coil  │            │
│        │ +12V ◄── External PSU  └─────┬────┘            │
│        │ GND  ◄── Common GND ────────┐│                 │
│        └──────────────────────────────┼┼─────────────┐  │
│                                       ││             │  │
│  ┌────────────────────────────────────┼┼─────────┐  │  │
│  │  ENCODER FEEDBACK                 │ │        │  │  │
│  │  ──────────────────────────────   │ │        │  │  │
│  │  Motor Rotates (20 slots)          │ │        │  │  │
│  │      │                             │ │        │  │  │
│  │      ├─→ HC-89 Sensor             │ │        │  │  │
│  │      │   Detects slot             │ │        │  │  │
│  │      │   Rising edge trigger      │ │        │  │  │
│  │      │                            │ │        │  │  │
│  │      └─→ GPIO 4 ◄─── DO output ──┼─┼────────┘  │  │
│  │                                   │ │           │  │
│  │      ISR Fired (interrupt)        │ │           │  │
│  │      encoder1_count++              │ │           │  │
│  │                                    │ │           │  │
│  │      update_rpm_calculation()       │ │           │  │
│  │      motor1_rpm = (count*60)/(20*t)│ │           │  │
│  │                                     │ │           │  │
│  └─────────────────────────────────────┼─┼───────────┘  │
│                                        │ │              │
│ ┌───────────────────────────────────────┼─┴────┐        │
│ │ PARALLEL: Same for Motor 2            │      │        │
│ │ GPIO 26, 33, 32 → L298N Motor2        │      │        │
│ │ GPIO 5 ← HC-89 Encoder2 DO            │      │        │
│ └────────────────────────────────────────┴──────┘        │
│                                                           │
│ ┌─────────────────────────────────────────────────┐     │
│ │ SEPARATE I2C BUS (UNCHANGED)                    │     │
│ │ GPIO 21 = SDA (VL53L0X, MPU6050)               │     │
│ │ GPIO 22 = SCL (VL53L0X, MPU6050)               │     │
│ └─────────────────────────────────────────────────┘     │
│                                                           │
└─────────────────────────────────────────────────────────┘
```

---

## Speed Calculation Example

```
Encoder Output Timing:

Time:  0ms   50ms  100ms  150ms  200ms  250ms  300ms
DO:    ┌─┐    ┌─┐    ┌─┐    ┌─┐    ┌─┐    ┌─┐
       │ │    │ │    │ │    │ │    │ │    │ │
───────┘ └────┘ └────┘ └────┘ └────┘ └────┘ └───
       ↑      ↑      ↑      ↑      ↑      ↑
    ISR fired (20 times per full rotation)

In 1 second (1000ms):
- Assume 50 rising edges detected
- encoder1_count = 50
- Revolutions = 50 / 20 slots/rev = 2.5 revolutions
- RPM = 2.5 rev/sec × 60 sec/min = 150 RPM

Formula:
  motor_rpm = (pulses × 60) / (ENCODER_SLOTS × time_seconds)
  motor_rpm = (50 × 60) / (20 × 1) = 3000 / 20 = 150 RPM
```

---

## Communication Levels

```
Logic Level Matching:

┌────────────────── LEVELS ──────────────────┐
│                                            │
│ ESP32 GPIO Output     → L298N IN Pins      │
│ 0V (LOW) ............ 0V (Stop direction) │
│ 3.3V (HIGH) ........ 3.3V (Forward dir.)  │
│ ✓ Compatible (L298N accepts 3.3V input)  │
│                                            │
│ PWM Signal (GPIO 25/32) → L298N ENA/ENB  │
│ 0V (0% duty) ......... No motor power     │
│ 3.3V (100% duty) .... Full motor power    │
│ Frequency: 5 kHz .... Fast enough         │
│ ✓ Compatible                              │
│                                            │
│ HC-89 Encoder (GPIO 4/5)                  │
│ Output: TTL Logic (0V/3.3V)               │
│ Input: ESP32 GPIO (accepts 3.3V)          │
│ ✓ Compatible                              │
│                                            │
└────────────────────────────────────────────┘
```

---

## Physical Layout Tips

**PCB Layout Best Practices**:
1. Keep motor power lines away from signal lines
2. Use separate ground planes if possible
3. Add 100µF capacitor at L298N power input
4. Add 0.1µF ceramic cap at each signal
5. Shield encoder wires if experiencing noise

**Cable Recommendations**:
- Motor power: 18AWG or thicker (12V)
- Signal lines: 26AWG twisted pair (data lines)
- Ground: Continuous return path (minimum 18AWG)
- Encoder cables: Shielded if >1 meter

**Mechanical Mounting**:
- Mount encoder wheel firmly on motor shaft
- Ensure encoder sensor faces wheel (not mount)
- Test encoder wheel by hand (should trigger pulses)
- Prevent vibration (use rubber washers)

---

## Next: Ready to Build?

```bash
# Step 1: Verify all connections against this guide
# Step 2: Build motor test
cd ~/ros2_ws/sensor_testing
pio run -e motor_test

# Step 3: Upload
pio run -e motor_test -t upload --upload-port /dev/ttyACM1

# Step 4: Monitor
pio device monitor --port /dev/ttyACM1 --baud 115200

# Step 5: Watch motors spin and encoders count!
```

Good luck! 🤖
