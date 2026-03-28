# Micro-ROS WiFi Configuration Examples

## WiFi Credentials Template

**File**: `/home/ros/ros2_ws/sensor_testing/src/main_micro_ros.cpp`

### Edit Lines ~30:

```cpp
// ============= WIFI CONFIGURATION =============
// EDIT THESE WITH YOUR NETWORK CREDENTIALS
const char* WIFI_SSID = "MyHomeNetwork";      // Your 2.4 GHz WiFi SSID
const char* WIFI_PASSWORD = "Password123";   // WiFi password
const char* AGENT_IP = "192.168.1.100";      // Laptop IP (same WiFi)
const uint16_t AGENT_PORT = 8888;            // micro-ROS agent UDP port
```

### Find Your Network Details

```bash
# 1. Get your laptop's WiFi IP:
hostname -I
# or
ip addr show | grep -E "inet.*192|inet.*10"

# 2. Verify WiFi network:
iwconfig wlan0
# or (newer systems):
nmcli dev wifi list

# 3. Check 2.4 GHz availability:
nmcli dev wifi list | grep -E "2\.4|802.11"
```

---

## ROS 2 Queue Depths & QoS Tuning

### Current Settings (Recommended for WiFi)

**File**: `/home/ros/ros2_ws/src/esp32_serial_bridge/esp32_serial_bridge/micro_ros_robot_bridge.py`

```python
# BEST_EFFORT QoS for sensor data (tolerates WiFi loss)
self.qos_sensor = QoSProfile(
    history=QoSHistoryPolicy.KEEP_LAST,
    depth=10,                              # Queue 10 messages
    reliability=QoSReliabilityPolicy.BEST_EFFORT,
    durability=QoSDurabilityPolicy.VOLATILE
)

# RELIABLE QoS for commands (guarantees delivery)
self.qos_command = QoSProfile(
    history=QoSHistoryPolicy.KEEP_LAST,
    depth=5,                               # Queue 5 commands
    reliability=QoSReliabilityPolicy.RELIABLE,
    durability=QoSDurabilityPolicy.VOLATILE
)
```

### Tuning for Performance

**If experiencing packet loss > 5%**:
```python
# Reduce publish rates (less data over WiFi):
const unsigned long ODOM_INTERVAL_MS = 20;   # 100 Hz → reduce to 50 Hz
const unsigned long IMU_INTERVAL_MS = 20;    # 50 Hz (OK, don't reduce)

# Or: Increase queue depth:
'depth': 20,  # Was 10, now 20
```

**If experiencing latency > 100 ms**:
```python
# Check WiFi signal:
iwconfig wlan0  # Look for "Signal level" > -60 dBm

# Or: Use 5 GHz band (if supported and nearby):
# NOTE: ESP32 standard doesn't support 5 GHz
```

---

## Launch File Parameters

### Using micro_ros_bringup.launch.py

```bash
# Defaults:
ros2 launch esp32_serial_bridge micro_ros_bringup.launch.py

# Custom port:
ros2 launch esp32_serial_bridge micro_ros_bringup.launch.py agent_port:=9000

# Extended watchdog timeout (1 second instead of 500 ms):
ros2 launch esp32_serial_bridge micro_ros_bringup.launch.py watchdog_timeout_ms:=1000

# All parameters:
ros2 launch esp32_serial_bridge micro_ros_bringup.launch.py \
  agent_port:=8888 \
  watchdog_timeout_ms:=500
```

---

## Motor Control Velocity Mapping

### Differential Drive Conversion

**File**: `/home/ros/ros2_ws/sensor_testing/src/main_micro_ros.cpp`

Current mapping (lines ~280):

```cpp
// Convert twist to motor speeds using differential drive kinematics:
float target_v_m1 = current_linear_x - (current_angular_z * WHEEL_SEPARATION / 2.0);
float target_v_m2 = current_linear_x + (current_angular_z * WHEEL_SEPARATION / 2.0);

// Motor 1 (left):   v_m1 = v_linear - ω * L/2
// Motor 2 (right):  v_m2 = v_linear + ω * L/2
// where:
//   v_linear = forward speed (m/s)
//   ω = angular velocity (rad/s)
//   L = wheel separation (m) = 0.18
```

### Tuning for Your Robot

If robot curves left/right despite symmetric commands:

```cpp
// Add calibration offset:
float target_v_m1 = (current_linear_x - (current_angular_z * WHEEL_SEPARATION / 2.0)) * 1.05;  // 5% faster
float target_v_m2 = current_linear_x + (current_angular_z * WHEEL_SEPARATION / 2.0);
```

---

## Encoder Pulse Conversion

### Hardware Configuration

**File**: `/home/ros/ros2_ws/sensor_testing/src/main_micro_ros.cpp` + `motors_encoders.h`

Current settings:

```cpp
const float WHEEL_DIAMETER = 0.065;       // 65 mm
const float WHEEL_SEPARATION = 0.18;      // 180 mm (axle width)
const int ENCODER_SLOTS = 20;             // 20 slots per revolution

// Calculated:
const float METERS_PER_SLOT = (M_PI * WHEEL_DIAMETER) / ENCODER_SLOTS;
// = (3.14159 * 0.065) / 20
// = 0.010210 m/pulse
```

### Measurement Instructions

```bash
# 1. Physically measure wheel diameter:
# Mark a point on wheel, roll forward one full rotation
# Measure distance traveled: ____ mm

# Calculate from measurement:
wheel_diameter = distance_traveled_mm / 3.14159 / 1000

# 2. Measure wheel separation (axle width):
# Measure distance between wheel centers: ____ mm

# 3. Count encoder slots:
# Look at wheel hub or datasheet: ____ slots

# Example: 65 mm diameter, 20 slots, 180 mm separation
```

### Updating Parameters

If measurements differ from defaults:

```cpp
// Update in main_micro_ros.cpp:
const float WHEEL_DIAMETER = 0.070;       // Your measurement (m)
const float WHEEL_SEPARATION = 0.19;      // Your measurement (m)
const int ENCODER_SLOTS = 20;             // Count from hub
```

Recompile and upload:
```bash
pio run -e micro_ros_wifi --target upload
```

---

## IMU Calibration (MPU6050)

### Calibration Process

**File**: `/home/ros/ros2_ws/sensor_testing/src/calibration.cpp`

**Run calibration firmware**:
```bash
pio run -e calibration --target upload --upload-port /dev/ttyACM0
pio device monitor --port /dev/ttyACM0 --baud 115200
```

**Follow on-screen instructions**:
```
1. Place robot on level surface
2. Run accelerometer calibration
3. Keep stationary for gyroscope calibration
4. Note offsets displayed
5. Record in main_micro_ros.cpp
```

### Applying Calibration Offsets

After getting offsets from calibration:

```cpp
// In main_micro_ros.cpp, adjust MPU setup:
// Add to setup_sensors():

float accel_offset_x = 0.05;   // From calibration
float accel_offset_y = -0.02;
float accel_offset_z = -9.81;

// Then in publish_imu():
imu_msg.linear_acceleration.x = a.acceleration.x + accel_offset_x;
imu_msg.linear_acceleration.y = a.acceleration.y + accel_offset_y;
imu_msg.linear_acceleration.z = a.acceleration.z + accel_offset_z;
```

---

## Watchdog Timeout Tuning

### Safety Considerations

**File**: `/home/ros/ros2_ws/src/esp32_serial_bridge/esp32_serial_bridge/micro_ros_robot_bridge.py`

Current setting: **500 ms**

```python
self.watchdog_timeout_sec = self.get_parameter('watchdog_timeout_ms').value / 1000.0
# = 500 / 1000 = 0.5 seconds

if elapsed > self.watchdog_timeout_sec:
    # Motors stop
```

### Recommendations

| Scenario | Timeout | Reasoning |
|----------|---------|-----------|
| Lab/slow robot | 1000 ms | 1 second gives grace period |
| Field robot | 500 ms | Faster safety response |
| Fast robot | 250 ms | Aggressive timeout |
| Testing | 5000 ms | Disable for testing (set to 0 to disable) |

### Disable Watchdog (DANGEROUS)

```python
# In micro_ros_robot_bridge.py:
const unsigned long CMD_VEL_TIMEOUT_MS = 0;  // 0 = DISABLED (set to 500 for safety)
```

---

## WiFi Network Optimization

### For Stable Connection

```bash
# 1. Check WiFi channel congestion:
sudo iwlist scan | grep -E "Channel:|ESSID:"

# 2. Use less congested channel (typically 1, 6, or 11 in 2.4 GHz):
# (Configure in your WiFi router settings)

# 3. Place ESP32 closer to router (within 5 meters recommended for WiFi)

# 4. Check for interference:
sudo iwconfig wlan0 | grep "Noise"
# Good: Noise level is low (< -90 dBm)
```

### Speed Test

Measure WiFi throughput from laptop:

```bash
# Install iperf3:
sudo apt-get install iperf3

# On a separate machine (or use iperf3 server mode):
iperf3 -s

# From laptop:
iperf3 -c 192.168.1.1 -t 10 -R
# Should see: > 50 Mbps (adequate for robot)
```

---

## ROS 2 Time Synchronization (Phase 2)

### Current Implementation (Phase 1)

```cpp
// In main_micro_ros.cpp - uses ROS system time:
rcl_time_point_value_t now = rcl_clock_system_default_get_current_time_nanoseconds();
```

**Limitation**: No synchronization with agent clock

### Phase 2 Implementation (Coming Soon)

Will implement:
1. Request ROS 2 system time from agent
2. Calculate offset between ESP32 clock and ROS time
3. Apply offset to all timestamps

---

## Diagnostic Commands

### Monitor System Health

```bash
# Terminal 1: Watch message rates
watch -n 1 'ros2 topic hz /odom /imu /scan'

# Terminal 2: Monitor diagnostics
ros2 topic echo /diagnostics --field 'status' | grep -A 5 "watchdog"

# Terminal 3: Check TF tree
ros2 run tf2_tools view_frames
# Generates frames.pdf showing odom → base_link

# Terminal 4: Record data for analysis
ros2 bag record /odom /imu /scan -o robot_session
```

### Playback Recorded Data

```bash
# After session, analyze recorded data:
ros2 bag play robot_session --loop

# Compute statistics:
ros2 bag play robot_session | ros2 topic hz /odom
```

---

**Next**: Refer to [WIFI_SETUP_GUIDE.md](WIFI_SETUP_GUIDE.md) for step-by-step instructions.
