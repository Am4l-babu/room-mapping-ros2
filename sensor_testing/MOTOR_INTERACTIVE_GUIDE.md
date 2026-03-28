# 🎮 Interactive Motor Driver Test - User Guide

## ✅ Status: UPLOADED & READY

**Upload Successful!** The interactive motor test code is now running on your ESP32.

---

## 🚀 How to Use

### 1. Connect Serial Monitor
```bash
cd /home/ros/ros2_ws/sensor_testing
pio device monitor --port /dev/ttyACM0 --baud 115200
```

### 2. Available Commands

Send these commands via serial to control the motors:

| Command | Function | Result |
|---------|----------|--------|
| **f** | Forward | Both motors move forward at current speed |
| **b** | Backward | Both motors move backward at current speed |
| **r** | Right Turn | Left motor faster, right motor slower (pivot right) |
| **l** | Left Turn | Right motor faster, left motor slower (pivot left) |
| **s** | Stop | Both motors stop immediately |
| **+** | Increase Speed | Speed +25 PWM (max 255) |
| **-** | Decrease Speed | Speed -25 PWM (min 25) |
| **?** | Show Menu | Display all commands |

### 3. Example Session

```
╔════════════════════════════════════════════╗
║      MOTOR DRIVER INTERACTIVE TEST         ║
╚════════════════════════════════════════════╝

Commands:
  f - Forward (both motors)
  b - Backward (both motors)
  r - Right turn (left motor faster)
  l - Left turn (right motor faster)
  s - Stop
  + - Increase speed
  - - Decrease speed
  ? - Show menu

Current Speed: 150/255

> f
→ FORWARD
[Speed: 150/255] Motor 1 RPM: 50.63 | Motor 2 RPM: 70.25

> +
⬆ Speed increased to: 175

> s
⏹ STOP

> r
↻ RIGHT TURN
[Speed: 175/255] Motor 1 RPM: 45.20 | Motor 2 RPM: 22.15
```

---

## 🎯 Features

### Real-Time Feedback
- **Speed Display**: Shows current PWM value (0-255)
- **RPM Monitoring**: Displays both motor speeds in real-time
- **Live Status**: See both encoders reading wheel pulses

### Motor Control Modes
- **Forward/Backward**: Direct motion control
- **Turns**: Differential speed control for turning
- **Speed Adjustment**: Smooth acceleration/deceleration in 25-unit steps
- **Smooth Operation**: 50ms loop for stable motor control

### Safety Features
- Speed limited to 25-255 PWM range
- Instant stop command
- RPM verification ensures motors are responding
- Echo feedback shows your input

---

## 📊 Speed Levels

```
Speed Range: 25 - 255 PWM

25   = Very slow crawl
75   = Slow speed
150  = Medium speed (default)
175  = Fast speed
225  = Very fast
255  = Maximum speed
```

---

## 🔧 GPIO Assignment

| Motor | GPIO Pins | Function |
|-------|-----------|----------|
| Motor 1 | 14, 27, 25 | Direction + PWM speed control |
| Motor 2 | 26, 33, 32 | Direction + PWM speed control |
| Encoder 1 | GPIO 4 | Interrupt-based pulse counting |
| Encoder 2 | GPIO 5 | Interrupt-based pulse counting |

---

## 💡 Testing Tips

1. **Start at Medium Speed** (default 150)
   - Send `f` to move forward
   - Observe motor response and RPM readings

2. **Test Turns**
   - Send `r` or `l` to verify differential speed control
   - Check if RPM shows different values for each motor

3. **Increase Speed Gradually**
   - Send multiple `+` commands to ramp up speed
   - Watch RPM increase accordingly

4. **Emergency Stop**
   - Send `s` instantly stops both motors

5. **Encoder Verification**
   - RPM > 0 means encoder is detecting wheel pulses
   - If RPM = 0, check encoder connections (GPIO 4, 5)

---

## 📝 Memory Usage

- **RAM**: 6.7% (22,096 bytes)
- **Flash**: 21.4% (280,909 bytes)
- **Status**: ✅ Plenty of space for ROS 2 integration

---

## 🚀 Next Steps

After testing motors here, you can:
1. Integrate motor control into main ROS 2 node
2. Implement autonomous navigation
3. Add sensor-based obstacle avoidance
4. Layer SLAM for room mapping

---

## ❓ Troubleshooting

### Motors Don't Move
- Check L298N power connections (external 12V)
- Verify GPIO pins match your hardware
- Check motor connections to OUT1-OUT4

### RPM Shows 0
- Verify encoder connection to GPIO 4 & 5
- Check encoder wheel alignment
- Ensure encoders have power (3.3V)

### Speed Changes Aren't Smooth
- Normal behavior - speed updates in 25-unit steps
- Use `+` or `-` multiple times for intermediate speeds
- Motor inertia causes gradual RPM changes

### Serial Input Not Working
- Check baud rate: **115200**
- Try pressing Enter after command
- Clear serial buffer: `?` followed by Enter

---

## 📖 Commands Quick Reference

```bash
# Quick test sequence
f           # Move forward
+           # Speed up
+           # Speed up more
s           # Stop
r           # Turn right
l           # Turn left
b           # Go backward
s           # Stop
?           # Show full menu
```

---

## 🎉 You're Ready!

Your interactive motor test is live and responding to commands.
Connect your serial monitor and start sending commands!
