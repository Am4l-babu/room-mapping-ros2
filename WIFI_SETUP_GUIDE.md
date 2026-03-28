# ESP32 + ROS 2 WiFi Robot System - PHASE 1 & 3 Setup Guide

**Status**: WiFi micro-ROS transport + safety watchdog implementation  
**Date**: March 28, 2026  
**Scope**: PHASE 1 (communication) + PHASE 3 (safety)  

---

## TABLE OF CONTENTS

1. [Hardware Checklist](#hardware-checklist)
2. [WiFi Configuration](#wifi-configuration)
3. [Micro-ROS Setup on ESP32](#micro-ros-setup-on-esp32)
4. [Building ESP32 Code](#building-esp32-code)
5. [Running the System](#running-the-system)
6. [Diagnostics & Monitoring](#diagnostics--monitoring)
7. [Troubleshooting](#troubleshooting)

---

## HARDWARE CHECKLIST

Before proceeding, verify you have:

- [ ] **ESP32 DevKit** with WiFi antenna (check physical antenna connector)
- [ ] **WiFi network** available (2.4 GHz recommended)
- [ ] **Laptop** with WiFi (same network as ESP32)
- [ ] **Micro-ROS Agent** installed (`colcon build ... micro-ros` in `/home/ros/ros2_ws`)
- [ ] **ROS 2 Humble/Iron** installed and sourced
- [ ] **PlatformIO CLI** installed (`pip install platformio`)
- [ ] **Serial USB cable** for initial flashing (optional: use WiFi after first upload)

### Network Configuration

Determine your network details:

```bash
# On laptop:
ifconfig | grep -A 5 "wlan\|wifi\|en"
# Look for: SSID, IP address (e.g., 192.168.1.100)

# Ping router to verify connectivity:
ping 192.168.1.1
```

**Record these:**
- WiFi SSID: `_______________`
- WiFi Password: `_______________`
- Laptop IP: `_______________`
- Router Gateway: `_______________`

---

## WIFI CONFIGURATION

### Step 1: Prepare WiFi Credentials

Edit the ESP32 code to add your WiFi credentials:

```cpp
// /home/ros/ros2_ws/sensor_testing/src/main_micro_ros.cpp (lines ~30)
const char* WIFI_SSID = "YOUR_SSID";          // Change this
const char* WIFI_PASSWORD = "YOUR_PASSWORD";  // Change this
const char* AGENT_IP = "192.168.1.100";       // Your laptop IP
const uint16_t AGENT_PORT = 8888;             // micro-ROS agent port
```

**❌ DO NOT COMMIT CREDENTIALS TO GIT!**

### Step 2: Configure Agent IP

Find your laptop's IP on the WiFi network:

```bash
# Linux/Mac:
hostname -I | awk '{print $1}'
# Output: 192.168.X.X

# Or more verbose:
ip addr show | grep "inet " | grep -v 127
```

Update `AGENT_IP` in `main_micro_ros.cpp` with this IP.

### Step 3: Verify Network Accessibility

From laptop, ensure the network is accessible:

```bash
# Test WiFi connection:
nmcli dev wifi list
# Should see your SSID listed

# Test network speed:
iperf3 -c 192.168.1.1 -t 10
# Should see >50 Mbps (good for WiFi)
```

---

## MICRO-ROS SETUP ON ESP32

### ⚠️ IMPORTANT: Micro-ROS Library Installation

The code uses the **micro-ROS Arduino library**, which requires special setup:

#### Option A: Using micro_ros_setup (Recommended)

```bash
# 1. Install micro_ros_setup package if not already done:
cd /home/ros/ros2_ws/src
git clone https://github.com/micro-ROS/micro_ros_setup.git

# 2. Build the package:
colcon build --packages-select micro_ros_setup
source /home/ros/ros2_ws/install/setup.bash

# 3. Configure toolchain for ESP32:
ros2 run micro_ros_setup configure_toolchain esp32 --dev-mode

# 4. Build micro-ROS library for ESP32:
ros2 run micro_ros_setup build_agent
```

After this, PlatformIO will automatically find the micro-ROS libraries.

#### Option B: Manual Library Installation

If `micro_ros_setup` is unavailable:

```bash
# Download and install micro-ROS Arduino library:
mkdir -p ~/Arduino/libraries
cd ~/Arduino/libraries
git clone https://github.com/micro-ROS/micro_ros_arduino.git

# Update PlatformIO lib_deps to include:
# https://github.com/micro-ROS/micro_ros_arduino
```

---

## BUILDING ESP32 CODE

### Clean Build

```bash
cd /home/ros/ros2_ws/sensor_testing

# Clean previous builds:
pio run -e micro_ros_wifi --target clean

# Build the ESP32 micro-ROS firmware:
pio run -e micro_ros_wifi
```

**Expected output:**
```
Linking intermediate.elf
Flash will be erased and reprogrammed...
SHA256 digest calculated
Image successfully generated
```

### Upload to ESP32

**First time (via USB)**:

```bash
# Detect serial port:
pio device list
# Output: /dev/ttyACM0, /dev/ttyUSB0, etc.

# Upload:
pio run -e micro_ros_wifi --target upload --upload-port /dev/ttyACM0

# Monitor serial output:
pio device monitor --port /dev/ttyACM0 --baud 115200
```

**Subsequent uploads (via WiFi)**:

```bash
# After first successful upload, ESP32 has WiFi configured.
# Can use OTA (Over-The-Air) updates:
pio run -e micro_ros_wifi --target upload --upload-port 192.168.1.X
```

**Monitor output during upload:**
```
===== ESP32 micro-ROS Robot Controller =====
PHASE 1 & 3: WiFi Transport + Safety
Motors and encoders initialized
MPU6050 initialized
VL53L0X initialized
Connecting to WiFi: YOUR_SSID
.....
WiFi connected!
IP Address: 192.168.1.X
micro-ROS initialized
Setup complete! Running main loop...
```

---

## RUNNING THE SYSTEM

### Terminal 1: Build ROS 2 Packages

```bash
cd /home/ros/ros2_ws

# Build the bridge node:
colcon build --packages-select esp32_serial_bridge

# Source the workspace:
source install/setup.bash
```

**Expected**:
```
Summary: 1 package finished
```

If errors occur, check [Troubleshooting](#troubleshooting).

### Terminal 2: Start micro-ROS Agent

```bash
# Make sure micro-ROS agent is installed:
colcon build --packages-select micro_ros_agent

# Source and run:
source /home/ros/ros2_ws/install/setup.bash

# Option A: Use launch file (RECOMMENDED)
ros2 launch esp32_serial_bridge micro_ros_bringup.launch.py agent_port:=8888

# Option B: Run agent directly
ros2 run micro_ros_agent micro_ros_agent udp4 -p 8888 -v6
```

**Expected output:**
```
UDP4 Agent listening on port 8888
Client connected
ESP32 (sub: /cmd_vel, pub: /odom, /imu, /scan)
```

### Terminal 3: Send Commands

```bash
source /home/ros/ros2_ws/install/setup.bash

# Publish velocity command:
ros2 topic pub /cmd_vel geometry_msgs/msg/Twist '{linear: {x: 0.1}, angular: {z: 0.0}}'

# Monitor odometry:
ros2 topic echo /odom --field 'pose.pose.position'

# Monitor IMU:
ros2 topic echo /imu --field 'linear_acceleration'

# Monitor scan:
ros2 topic echo /scan --field 'ranges' | head -20
```

### Terminal 4: Diagnostics

```bash
source /home/ros/ros2_ws/install/setup.bash

# Monitor message rates and health:
ros2 topic hz /odom /imu /scan

# Check diagnostics (published by bridge node):
ros2 topic echo /diagnostics

# Monitor complete system:
rqt_graph  # Shows topic connections
```

---

## DIAGNOSTICS & MONITORING

### Message Rate Verification

After system is running, verify rates match expectations:

| Topic | Expected Rate | Command |
|-------|---------------|---------|
| `/odom` | 100 Hz | `ros2 topic hz /odom` |
| `/imu` | 50 Hz | `ros2 topic hz /imu` |
| `/scan` | 8 Hz | `ros2 topic hz /scan` |
| `/cmd_vel` | Variable | `ros2 topic hz /cmd_vel` |

**Expected output:**
```
average rate: 100.03 Hz
min: 9.97ms, max: 10.12ms, std dev: 0.12ms
```

### Latency Measurement

Measure end-to-end latency from ESP32 to ROS 2:

```bash
# Check timestamp difference (ESP32 time vs ROS 2 time)
ros2 topic echo /odom --field 'header.stamp'
```

**Expected**: Consistent timestamps within ±50 ms (WiFi latency)

### Connection Status

Check if micro-ROS agent is connected:

```bash
# Run agent with -v6 (verbose):
ros2 run micro_ros_agent micro_ros_agent udp4 -p 8888 -v6

# Shows:
# - "Client connected" when ESP32 joins
# - "Publisher created ..." for each topic
# - Message counts for monitoring
```

### Watchdog Status

Monitor the safety watchdog for `/cmd_vel` timeout:

```bash
# Check bridge diagnostics:
ros2 topic echo /diagnostics

# Look for: watchdog_active = true/false
# And: cmd_vel_timeout_sec = <elapsed time>
```

If watchdog triggers (no `/cmd_vel` for 500 ms):
- Motors stop immediately
- Log shows: `"WATCHDOG: Motor timeout → STOP"`
- Resume movement by sending new `/cmd_vel` command

---

## TROUBLESHOOTING

### Issue 1: "ESP32 cannot connect to WiFi"

**Symptoms**: Serial monitor shows "Failed to connect to WiFi!"

**Diagnostics**:
```cpp
// Check credentials are correct:
// 1. SSID (no special characters)
// 2. Password (no spaces)
// 3. WiFi is 2.4 GHz (5 GHz not supported on older ESP32)
```

**Solutions**:
1. Verify WiFi credentials in `main_micro_ros.cpp`
2. Ensure 2.4 GHz band is enabled on your router
3. Check ESP32 antenna is connected (physical connector)
4. Try connecting to a different WiFi network
5. Power cycle ESP32

**Test WiFi directly:**
```cpp
// In main_micro_ros.cpp, add:
Serial.print("Free PSRAM: ");
Serial.println(esp_psram_get_size() / 1024);
```

---

### Issue 2: "ESP32 connects to WiFi but no micro-ROS agent connection"

**Symptoms**: 
- Serial shows "WiFi connected!" but
- Agent shows "No client" or times out

**Diagnostics**:
1. Check laptop IP matches `AGENT_IP` in code
2. Verify agent is running: `ros2 run micro_ros_agent ...`
3. Check firewall allows UDP 8888:
   ```bash
   # Linux:
   sudo ufw allow 8888/udp
   
   # macOS:
   sudo pfctl -f /etc/pf.conf  # Adjust firewall settings
   ```
4. Ping agent from ESP32 cannot be done (no `ping` in Arduino)
   Instead: Test UDP connectivity with agent logs

**Solution**:
```bash
# Check UDP is listening:
netstat -an | grep 8888
# Should show: 0.0.0.0:8888 LISTEN

# If not, agent crashed. Check logs:
ros2 run micro_ros_agent micro_ros_agent udp4 -p 8888 -v6 2>&1 | tee agent.log
```

---

### Issue 3: "Topics not publishing or very low rate"

**Symptoms**: `ros2 topic hz /odom` shows 0-5 Hz instead of 100 Hz

**Diagnostics**:
```bash
# Check message queue depth:
ros2 topic info /odom
# Output should show: "Subscription count: X" (> 0 means being received)

# Check for packet loss:
# Watch agent logs for "dropped messages" or "buffer overflow"
ros2 run micro_ros_agent micro_ros_agent udp4 -p 8888 -v6

# Check WiFi signal strength:
iwconfig wlan0  # or your wifi interface
# Should show "Signal level" > -60 dBm (good)
```

**Solutions**:
1. Reduce message rate in ESP32 code (temporary):
   ```cpp
   const unsigned long ODOM_INTERVAL_MS = 20;  // 50 Hz instead of 100 Hz
   ```
2. Increase UDP buffer sizes in agent
3. Move WiFi router closer to ESP32
4. Check for WiFi interference (2.4 GHz overlaps USB, microwave, etc.)

---

### Issue 4: "Watchdog timeout constantly triggering"

**Symptoms**: Serial monitor shows "WATCHDOG: Motor timeout" every 500 ms

**Cause**: `/cmd_vel` subscription is not working

**Solutions**:
```bash
# Verify /cmd_vel is being published:
ros2 topic pub /cmd_vel geometry_msgs/msg/Twist '{linear: {x: 0.1}}'

# Check agent is forwarding subscriptions:
ros2 run micro_ros_agent micro_ros_agent udp4 -p 8888 -v6
# Should show: "Subscription /cmd_vel created"

# Check QoS mismatch (publisher/subscriber):
# Bridge publishes with RELIABLE, ESP32 should subscribe with RELIABLE
```

---

### Issue 5: "Compilation error: rcl/publisher.h not found"

**Cause**: Micro-ROS library not installed

**Solution**:
```bash
# Run micro_ros_setup:
cd /home/ros/ros2_ws
source install/setup.bash
ros2 run micro_ros_setup configure_toolchain esp32 --dev-mode

# Then build firmware again:
cd /home/ros/ros2_ws/sensor_testing
pio run -e micro_ros_wifi --target clean
pio run -e micro_ros_wifi
```

---

### Issue 6: "Out of memory" or "Segmentation fault"

**Cause**: Insufficient SRAM for micro-ROS + motor control

**Diagnostics**:
```cpp
// Add to setup() in main_micro_ros.cpp:
Serial.print("Free SRAM: ");
Serial.println(esp_get_free_heap_size());

Serial.print("Free PSRAM: ");
Serial.println(esp_psram_get_size() - esp_psram_get_alloced());
```

**Solutions**:
1. Reduce message queue sizes in micro-ROS config
2. Use PSRAM (if available on your ESP32):
   ```cpp
   #define PSRAM heap allocation code
   ```
3. Disable non-essential sensors
4. Reduce publish rates (less memory for message buffers)

---

## NEXT STEPS

After confirming WiFi transport is working:

### Phase 2: Time Synchronization
- [ ] Enable ROS 2 NTP clock sync in agent
- [ ] Verify timestamp monotonicity across 60-second test

### Phase 4: Sensor Optimization
- [ ] Increase IMU rate: 10 Hz → 50 Hz
- [ ] Increase scan resolution: 37 → 90 points

### Phase 5: Sensor Fusion (EKF)
- [ ] Install `robot_localization` package
- [ ] Configure Kalman filter `ekf.yaml`
- [ ] Validate against SLAM

---

## REFERENCE

### Useful ROS 2 Commands

```bash
# List all nodes:
ros2 node list

# List all topics:
ros2 topic list

# Get topic info:
ros2 topic info /odom

# Record bag for replay:
ros2 bag record /odom /imu /scan

# Playback bag:
ros2 bag play rosbag2_<date>

# Check network interfaces:
ros2 daemon --version  # Verify ROS 2 networking

# View TF tree:
ros2 run tf2_tools view_frames
```

### ROS 2 vs micro-ROS Comparison

| Aspect | micro-ROS | Standard ROS 2 |
|--------|-----------|-----------------|
| **Middleware** | XRCE-DDS | DDS |
| **Transport** | UDP (WiFi, serial) | DDS middleware |
| **RAM** | ~50 KB | 100+ MB |
| **Latency** | 1-50 ms | <1 ms |
| **Suitable for** | Embedded (ESP32, ARM) | Full ROS2 nodes |

---

**Questions?** Check the diagnostic report: `/home/ros/ros2_ws/DIAGNOSTIC_REPORT.md`

---

**End of Setup Guide**
