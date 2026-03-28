# WiFi Micro-ROS Robot System - Implementation Checklist

**Status**: Ready for deployment  
**Scope**: PHASE 1 (Communication) + PHASE 3 (Safety)  
**Date**: March 28, 2026  

---

## 🎯 BEFORE YOU START

### Prerequisites

- [ ] ROS 2 Humble/Iron installed (test: `ros2 --version`)
- [ ] Workspace built: `colcon build --packages-select esp32_serial_bridge`
- [ ] micro_ros_agent installed (or: `colcon build --packages-select micro_ros_agent`)
- [ ] PlatformIO CLI installed (test: `pio --version`)
- [ ] ESP32 DevKit with WiFi antenna
- [ ] USB cable for initial upload
- [ ] WiFi network available (2.4 GHz)
- [ ] Workspace sourced: `source /home/ros/ros2_ws/install/setup.bash`

### Verify Setup

```bash
# Check ROS 2:
ros2 --version
# Expected: ROS 2 Humble/Iron

# Check workspace:
ls /home/ros/ros2_ws/install/esp32_serial_bridge/
# Expected: lib/ and share/ directories exist

# Check PlatformIO:
pio boards list | grep esp32dev
# Expected: esp32dev board listed
```

---

## ✅ STEP 1: CONFIGURE WIFI CREDENTIALS

**File**: `/home/ros/ros2_ws/sensor_testing/src/main_micro_ros.cpp`

### 1.1 Find Laptop WiFi IP

```bash
# Method 1 (Linux):
hostname -I | awk '{print $1}'
# Expected output: 192.168.X.X

# Method 2 (detailed):
nmcli dev show | grep DNS

# Record: _______________
```

### 1.2 Identify WiFi SSID and Password

```bash
# List available networks:
nmcli dev wifi list
# Look for your SSID (without [ ])

# Record SSID: _______________
# Record Password: _______________
```

### 1.3 Edit ESP32 Code

**File to edit**: `/home/ros/ros2_ws/sensor_testing/src/main_micro_ros.cpp`

**Lines ~30 (find by searching for "WIFI_CONFIGURATION")**:

```cpp
// ============= WIFI CONFIGURATION =============
const char* WIFI_SSID = "YOUR_SSID";          // ← CHANGE THIS
const char* WIFI_PASSWORD = "YOUR_PASSWORD";  // ← CHANGE THIS
const char* AGENT_IP = "192.168.1.100";       // ← CHANGE TO YOUR LAPTOP IP (from 1.1)
const uint16_t AGENT_PORT = 8888;             // ← Leave as is
```

**Example**:
```cpp
const char* WIFI_SSID = "HomeNetwork";
const char* WIFI_PASSWORD = "MyPassword123";
const char* AGENT_IP = "192.168.1.50";
const uint16_t AGENT_PORT = 8888;
```

### 1.4 Verify Edit

```bash
# Check credentials are set:
grep "const char\*" /home/ros/ros2_ws/sensor_testing/src/main_micro_ros.cpp | head -4
# Should show your SSID and IP
```

✅ **Checkpoint**: Credentials configured

---

## ✅ STEP 2: SETUP MICRO-ROS LIBRARY

**This step is required ONE TIME**

### 2.1 Install micro_ros_setup

```bash
cd /home/ros/ros2_ws

# If not already installed:
git clone https://github.com/micro-ROS/micro_ros_setup.git src/micro_ros_setup

colcon build --packages-select micro_ros_setup --symlink-install

source install/setup.bash
```

### 2.2 Configure Toolchain for ESP32

```bash
# This creates micro-ROS libraries for ESP32:
ros2 run micro_ros_setup configure_toolchain esp32 --dev-mode

# Expected output:
# ...
# Setup complete
```

**⏱️ This takes 5-15 minutes. Be patient.**

### 2.3 Verify Installation

```bash
# Check if libraries exist:
ls ~/.platformio/libraries/ | grep micro_ros
# Should list: micro_ros_arduino (or similar)

# Or check in PlatformIO:
pio lib list
# Should show micro-ROS libraries
```

✅ **Checkpoint**: micro-ROS library configured

---

## ✅ STEP 3: BUILD ESP32 FIRMWARE

**Location**: `/home/ros/ros2_ws/sensor_testing/`

### 3.1 Clean Previous Builds

```bash
cd /home/ros/ros2_ws/sensor_testing

pio run -e micro_ros_wifi --target clean
```

### 3.2 Build Firmware

```bash
pio run -e micro_ros_wifi

# Expected: Successful compilation ending with:
# SHA256 digest calculated
# Image successfully generated
```

**If you get errors** → See [Troubleshooting](#step-7-troubleshooting) section

### 3.3 Verify Binary

```bash
ls -lh .pio/build/micro_ros_wifi/firmware.elf
# Expected: File size ~500 KB - 1 MB
```

✅ **Checkpoint**: Firmware built successfully

---

## ✅ STEP 4: UPLOAD TO ESP32

**Physical Setup**:
- [ ] ESP32 connected via USB cable
- [ ] Serial port detected by system

### 4.1 Detect Serial Port

```bash
# Find ESP32 port:
pio device list
# Expected: /dev/ttyACM0, /dev/ttyUSB0, or COM3 (Windows)

# If not shown, try:
ls /dev/ttyACM* /dev/ttyUSB*
# Choose one (usually /dev/ttyACM0)

# Record: _______________
```

### 4.2 Upload Firmware

```bash
cd /home/ros/ros2_ws/sensor_testing

pio run -e micro_ros_wifi --target upload --upload-port /dev/ttyACM0

# Expected output:
# Chip is ESP32-D0WDQ6 (revision 1)
# Connecting....._____
# Detecting chip type... ESP32
```

**Upload takes 10-30 seconds**

### 4.3 Monitor Serial Output

While upload happens, open another terminal:

```bash
pio device monitor --port /dev/ttyACM0 --baud 115200

# Expected output (wait 5-10 seconds):
# ===== ESP32 micro-ROS Robot Controller =====
# PHASE 1 & 3: WiFi Transport + Safety
# Motors and encoders initialized
# MPU6050 initialized
# VL53L0X initialized
# Connecting to WiFi: HomeNetwork
# .....
# WiFi connected!
# IP Address: 192.168.1.X
# micro-ROS initialized
# Setup complete! Running main loop...
```

✅ **Checkpoint**: ESP32 booting successfully and connecting to WiFi

---

## ✅ STEP 5: BUILD ROS 2 PACKAGES

**Location**: `/home/ros/ros2_ws/`

### 5.1 Build Bridge Node

```bash
cd /home/ros/ros2_ws

colcon build --packages-select esp32_serial_bridge

# Expected:
# Summary: 1 package finished [X.XXs]
```

If errors, ensure you've sourced setup:
```bash
source /home/ros/ros2_ws/install/setup.bash
```

### 5.2 Verify Build

```bash
which micro_ros_robot_bridge
# Expected: /home/ros/ros2_ws/install/esp32_serial_bridge/bin/micro_ros_robot_bridge

# Or:
ros2 pkg list | grep esp32_serial_bridge
# Expected: esp32_serial_bridge listed
```

✅ **Checkpoint**: ROS 2 packages built

---

## ✅ STEP 6: LAUNCH COMPLETE SYSTEM

### 6.1 Terminal 1: Start micro-ROS Agent

```bash
# Source workspace:
source /home/ros/ros2_ws/install/setup.bash

# Launch agent:
ros2 launch esp32_serial_bridge micro_ros_bringup.launch.py agent_port:=8888

# Expected output:
# [micro_ros_agent-1] UDP4 Agent listening on port 8888
# [robot_bridge-2] micro-ROS Robot Bridge initialized
# [robot_bridge-2] Waiting for micro-ROS agent...
# [robot_bridge-2] Connected to micro-ROS agent
# [robot_bridge-2] Watchdog timeout: 500 ms
```

### 6.2 Terminal 2: Verify Topics

```bash
# Source workspace:
source /home/ros/ros2_ws/install/setup.bash

# Wait 5 seconds for connections, then:
ros2 topic list

# Expected output (should include):
# /cmd_vel
# /odom
# /imu
# /scan
# /diagnostics
```

### 6.3 Terminal 3: Check Message Rates

```bash
source /home/ros/ros2_ws/install/setup.bash

# Monitor message rates:
ros2 topic hz /odom /imu /scan

# Expected output (after 10 seconds):
# /odom: 100.02 Hz
# /imu:  50.03 Hz
# /scan: 8.01 Hz
```

**All rates within ±10% of expected**: ✅ Perfect!

### 6.4 Terminal 4: Send Test Command

```bash
source /home/ros/ros2_ws/install/setup.bash

# Publish velocity command:
ros2 topic pub /cmd_vel geometry_msgs/msg/Twist '{linear: {x: 0.1}, angular: {z: 0.0}}'

# Expected (in Terminal 1 logs):
# [robot_bridge-2] cmd_vel: linear=0.100, angular=0.000
# [micro_ros_agent-1] Received Twist command
```

**Observe**: Motors should start spinning (if not loaded, they may spin free)

### 6.5 Test Watchdog

```bash
# In Terminal 4, stop the command with Ctrl+C

# Wait 600 ms (> 500 ms watchdog timeout)

# Expected (in Terminal 1 logs):
# [robot_bridge-2] WATCHDOG: /cmd_vel timeout (0.50s) → STOP

# Motors should stop immediately
```

✅ **Checkpoint**: System operational with safety watchdog working

---

## ✅ STEP 7: MONITOR DIAGNOSTICS

### 7.1 Check System Health

```bash
source /home/ros/ros2_ws/install/setup.bash

# View diagnostics:
ros2 topic echo /diagnostics

# Expected output:
# ---
# status:
#   - name: micro_ros_robot_bridge
#     message: 'All systems nominal'
#     level: 0  # 0=OK, 1=WARN, 2=ERROR
#     values:
#       - key: odom_rate_hz
#         value: '100.03'
#       - key: imu_rate_hz
#         value: '50.02'
#       - key: scan_rate_hz
#         value: '8.01'
#       - key: watchdog_active
#         value: 'True'
```

### 7.2 Record Performance Metrics

**Message Rates**:
- Odometry: _____ Hz (expect: 100 ± 10)
- IMU: _____ Hz (expect: 50 ± 5)
- Scan: _____ Hz (expect: 8 ± 1)

**Watchdog**:
- [ ] Triggers on timeout
- [ ] Motors stop immediately
- [ ] Can resume with new command

**Latency**:
```bash
# Measure total latency (time from cmd_vel to motor response):
# Typically: 10-50 ms (WiFi adds ~20-30 ms)
```

---

## ⚠️ STEP 8: TROUBLESHOOTING

### Issue: ESP32 won't connect to WiFi

**Check**:
1. Password is correct (no spaces, case-sensitive)
2. SSID exists: `nmcli dev wifi list`
3. WiFi is 2.4 GHz (5 GHz not supported)
4. Antenna is attached to ESP32

**Fix**:
```bash
# Restart ESP32:
# Press physical RESET button for 2 seconds

# Or: Upload again (forces reboot)
pio run -e micro_ros_wifi --target upload --upload-port /dev/ttyACM0
```

---

### Issue: Topics not publishing (0 Hz)

**Check**:
```bash
# 1. Is ESP32 connected to WiFi? (Serial monitor shows ✓)
# 2. Is micro-ROS agent running? (Terminal 1 shows "UDP4 Agent listening")
# 3. Do topics exist? (ros2 topic list shows /odom, /imu, /scan)
```

**Fix**:
```bash
# Restart entire system:

# Terminal 1: Stop agent (Ctrl+C)
# Terminal 2-4: Stop all commands (Ctrl+C)

# Restart ESP32:
# Press physical RESET button

# Relaunch system
```

---

### Issue: Watchdog timeout not triggered

**Check**:
```bash
# 1. Is watchdog_timeout_ms set correctly? (should be 500)
ros2 launch esp32_serial_bridge micro_ros_bringup.launch.py watchdog_timeout_ms:=500

# 2. Is /cmd_vel actually stopping? (use ros2 topic pub)
```

**Fix**:
```bash
# Force watchdog trigger:
ros2 topic pub --once /cmd_vel geometry_msgs/msg/Twist '{}'

# Wait 600+ ms, should see "WATCHDOG" message
```

---

### Issue: Compilation error (rcl/publisher.h not found)

**Fix**:
```bash
# Reinstall micro-ROS toolchain:
ros2 run micro_ros_setup configure_toolchain esp32 --dev-mode

# Clean and rebuild:
cd /home/ros/ros2_ws/sensor_testing
pio run -e micro_ros_wifi --target clean
pio run -e micro_ros_wifi
```

---

## ✅ FINAL CHECKLIST

- [ ] Credentials configured (WiFi SSID/password/IP)
- [ ] Micro-ROS library installed
- [ ] ESP32 firmware compiled
- [ ] Firmware uploaded to ESP32
- [ ] ESP32 shows "WiFi connected" in serial
- [ ] ROS 2 packages built
- [ ] micro-ROS agent running (port 8888)
- [ ] Topics exist and publishing (/odom, /imu, /scan)
- [ ] Message rates correct (100, 50, 8 Hz)
- [ ] /cmd_vel commands received
- [ ] Motors respond to /cmd_vel
- [ ] Watchdog timeout triggers on 500 ms no-command
- [ ] Motors stop on watchdog timeout
- [ ] System resumes after new /cmd_vel command

---

## 🚀 WHAT'S NEXT?

System is now operational with **PHASE 1 & 3** (WiFi Transport + Safety) complete!

### Recommended Next Steps:

1. **Test Integration**:
   - [ ] Use `ros2 bag record` to capture /odom + /imu + /scan
   - [ ] Analyze consistency of timestamps
   - [ ] Verify odometry drift over 5-10 minute run

2. **Then Proceed to PHASE 2** (Time Synchronization):
   - Enable ROS 2 NTP clock sync on ESP32
   - Ensure all timestamps aligned with laptop

3. **Later: PHASE 4-6** (Fusion, Sensing, Robustness):
   - Install robot_localization (EKF)
   - Improve scan resolution
   - Add multi-robot support

---

## 📚 REFERENCE DOCUMENTS

- **Setup Details**: [WIFI_SETUP_GUIDE.md](WIFI_SETUP_GUIDE.md)
- **Configuration**: [MICRO_ROS_CONFIG_GUIDE.md](MICRO_ROS_CONFIG_GUIDE.md)
- **Architecture**: [DIAGNOSTIC_REPORT.md](DIAGNOSTIC_REPORT.md)
- **Implementation**: [PHASE_1_3_SUMMARY.md](PHASE_1_3_SUMMARY.md)

---

## 📞 SUPPORT

If stuck:
1. Check relevant document above
2. Search for your issue in [WIFI_SETUP_GUIDE.md](WIFI_SETUP_GUIDE.md#troubleshooting)
3. Verify all prerequisites in "Before You Start" section

---

**🎉 Phase 1 & 3 System Ready!**

Start from STEP 1 and follow sequentially. Each step has a checkpoint ✅.

