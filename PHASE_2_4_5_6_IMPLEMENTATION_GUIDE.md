# Phase 2, 4, 5, 6: Correctness & System Simplification
## Comprehensive Implementation Guide

**Status**: Phase 2 modules created (time sync, firmware, bridge) - ready for integration and testing

---

## Table of Contents
1. [Phase 2: Time Synchronization](#phase-2-time-synchronization)
2. [Phase 4: Sensor Fusion (EKF)](#phase-4-sensor-fusion-ekf)
3. [Phase 5: Scan Improvements](#phase-5-scan-improvements)
4. [Phase 6: System Simplification](#phase-6-system-simplification)
5. [Integration Steps](#integration-steps)
6. [Validation & Testing](#validation--testing)
7. [Troubleshooting](#troubleshooting)

---

## Phase 2: Time Synchronization

### Overview
**Objective**: Ensure all messages have accurate, synchronized timestamps from the moment of acquisition on ESP32.

**Why Critical**: 
- EKF requires timestamps to match acquisition time (errors break fusion)
- SLAM algorithms need consistent temporal references
- Without time sync, state estimation fails

### Problem Being Solved

**Before Phase 2** (Current Issue):
- ESP32 publishes messages with timestamps from internal `millis()` clock
- Bridge node receives messages and overwrites timestamps with laptop wall-clock time  
- Result: Timestamps don't match actual data acquisition time
- Effect: EKF cannot fuse correctly → localization fails

**After Phase 2** (Target):
- ESP32 clock synchronized with ROS 2 system clock
- All messages timestamped at acquisition with synchronized time
- Bridge preserves timestamps (doesn't overwrite)
- EKF receives correct temporal information → accurate fusion

### Implementation Files Created

#### File 1: `time_sync.h` (ESP32 Header)
**Location**: `/home/ros/ros2_ws/sensor_testing/src/time_sync.h`

**Functions**:
```cpp
// Initialize time sync module
void time_sync_init()

// Fetch ROS system time, calculate offset
// Call periodically (every 30 seconds)
bool time_sync_update(rcl_clock_t* clock)

// Get current time with ROS offset applied
rcl_time_point_value_t time_sync_get_timestamp()

// Check if re-sync needed
bool time_sync_needs_update()

// Get offset for diagnostics (milliseconds)
int64_t time_sync_get_offset_ms()

// Get diagnostic string
const char* time_sync_get_status()
```

#### File 2: `main_micro_ros_phase2.cpp` (Updated Firmware)
**Location**: `/home/ros/ros2_ws/sensor_testing/src/main_micro_ros_phase2.cpp`

**Key Changes from Phase 1**:
1. **Line 24**: Added `#include "time_sync.h"`
2. **Line 112**: Added `rcl_clock_t clock` for ROS time
3. **Line 114**: Added time sync timer variable
4. **Line 199**: Initialize time sync in `setup_micro_ros()`
5. **Lines 276-280**: Periodic time sync update (every 30 sec)
6. **Lines 309-310, 337-338, 356-357**: Use `time_sync_get_timestamp()` for all messages

**Integration Steps**:
```bash
# 1. Copy files to PlatformIO project
cp /home/ros/ros2_ws/sensor_testing/src/main_micro_ros_phase2.cpp \
   /home/ros/ros2_ws/sensor_testing/src/main.cpp

# 2. Verify time_sync.h is in same directory
ls -la /home/ros/ros2_ws/sensor_testing/src/time_sync.h

# 3. Build with PlatformIO
cd /home/ros/ros2_ws/sensor_testing
pio run --environment micro_ros_wifi

# 4. Upload to ESP32
pio run --environment micro_ros_wifi --target upload

# 5. Monitor serial output for time sync status
pio device monitor --baud 115200
```

**Expected Output on Serial Monitor**:
```
===== ESP32 micro-ROS Robot Controller =====
PHASES 1, 2, 3: Transport + Time Sync + Safety
Setup complete! Running main loop...
[INFO] Time sync initialized
[INFO] Time sync update: offset = +123 ms
[INFO] Publishing odom @ synchronized time
[INFO] Publishing imu @ synchronized time
```

#### File 3: `micro_ros_robot_bridge_phase2.py` (Updated Bridge)
**Location**: `/home/ros/ros2_ws/src/esp32_serial_bridge/esp32_serial_bridge/micro_ros_robot_bridge_phase2.py`

**Key Changes from Phase 1**:
1. **Lines 8-11**: Added docstring emphasizing timestamp preservation
2. **Lines 72-73**: New parameter `enable_timestamp_diagnostics`
3. **Lines 130**: NEW - timestamp offset tracking
4. **Lines 160-162**: Track timestamp offsets in diagnostics
5. **Lines 174-185**: NEW function `on_odom()` - CRITICAL: Preserves timestamp
6. **Line 179**: No longer overwrites `msg.header.stamp` (was the bug!)
7. **Lines 205-207**: NEW - timestamp offset tracking for IMU
8. **Lines 225-227**: NEW - timestamp offset tracking for scan
9. **Lines 280-295**: NEW - analyze timestamp offset statistics
10. **Lines 320-335**: NEW - include timestamp diagnostics in /diagnostics topic

**Integration Steps**:
```bash
# 1. Backup current bridge
mv ~/ros2_ws/src/esp32_serial_bridge/esp32_serial_bridge/micro_ros_robot_bridge.py \
   ~/ros2_ws/src/esp32_serial_bridge/esp32_serial_bridge/micro_ros_robot_bridge_phase1.py

# 2. Install updated bridge
cp /home/ros/ros2_ws/src/esp32_serial_bridge/esp32_serial_bridge/micro_ros_robot_bridge_phase2.py \
   ~/ros2_ws/src/esp32_serial_bridge/esp32_serial_bridge/micro_ros_robot_bridge.py

# 3. Rebuild ROS workspace
cd ~/ros2_ws
colcon build --packages-select esp32_serial_bridge

# 4. Launch system
source ~/ros2_ws/install/setup.bash
ros2 launch esp32_serial_bridge micro_ros_bringup.launch.py

# 5. Monitor timestamp diagnostics
ros2 topic echo /diagnostics | grep timestamp
```

**Expected Diagnostics Output**:
```
odom_timestamp_offset_mean_sec: 0.000500
odom_timestamp_offset_stddev_sec: 0.000050
imu_timestamp_offset_mean_sec: 0.000480
imu_timestamp_offset_stddev_sec: 0.000045
scan_timestamp_offset_mean_sec: 0.000520
scan_timestamp_offset_stddev_sec: 0.000055
```

### Phase 2 Validation

After deploying Phase 2, verify:

1. **Check time sync is working**:
   ```bash
   # Monitor ESP32 serial
   pio device monitor --baud 115200
   # Should see: "Time sync update: offset = +XXX ms" every 30 seconds
   ```

2. **Verify timestamps are synchronized**:
   ```bash
   # Check that message timestamps match acquisition time  
   ros2 topic echo /odom/header/stamp
   ros2 topic echo --once /odom
   # Timestamp in header should match current time (within 100 ms)
   ```

3. **Validate no timestamp corruption**:
   ```bash
   # Run for 1 minute and check timestamp consistency
   ros2 bag record /odom /imu /scan -o phase2_test
   # Play back and verify timestamps are monotonic:
   ros2 bag play phase2_test
   # Use rosbag2_py to check timestamp consistency
   ```

### Troubleshooting Phase 2

**Issue**: "Time sync update: offset = very large value (> 1 second)"
- **Cause**: ROS system clock not set correctly on laptop
- **Solution**: Sync system time: `sudo ntpdate -s time.nist.gov`

**Issue**: ESP32 loses WiFi connection during time sync
- **Cause**: rcl time request blocks WiFi stack
- **Solution**: Use non-blocking time fetch (already implemented in time_sync.h)

**Issue**: "Time sync: Failed to get ROS time"
- **Cause**: Agent not running or not responding
- **Solution**: Verify micro-ROS agent is running: `ps aux | grep agent`

---

## Phase 4: Sensor Fusion (EKF)

### Overview
**Objective**: Fuse encoder odometry + IMU data for improved localization accuracy

**Dependencies**:
```bash
sudo apt install ros-humble-robot-localization
```

### Architecture

**Input Sources**:
- **Odometry** (`/odom` @ 100 Hz): Position + velocity from encoders
- **IMU** (`/imu` @ 50 Hz): Orientation + angular velocity from MPU6050  

**Processing**:
- Extended Kalman Filter (robot_localization `ekf_node`)
- Runs at 50 Hz (limited by IMU rate)

**Outputs**:
- `/odometry/filtered`: Best estimate combining both sources
- `/pose/filtered`: Position + orientation
- `odom → base_link` TF: Updated by EKF

### EKF Configuration

**File Created**: `/home/ros/ros2_ws/src/esp32_serial_bridge/config/ekf_phase4.yaml`

**Key Parameters Explained**:

1. **Frequencies & Timeouts**:
   ```yaml
   frequency: 50              # Process at 50 Hz (IMU rate)
   sensor_timeout: 2.0        # Drop sensor if no data for 2 sec
   ```

2. **Frame Setup**:
   ```yaml
   map_frame: map
   odom_frame: odom           # Fusion frame (relative coordinates)
   base_link_frame: base_link
   world_frame: odom          # Output frame
   ```

3. **Odometry Input** (encoders - high noise, but absolute position):
   ```yaml
   odom0_config: [ x, y, -, -, -, yaw, vx, vy, -, -, -, vyaw ]
   # Uses: position (x,y,yaw) + velocity (vx,vy,vyaw)
   # Doesn't use: z, roll, pitch (2D robot)
   ```

4. **IMU Input** (accelerometer + gyro - low noise, derivative):
   ```yaml
   imu0_config: [ -, -, -, roll, pitch, -, -, -, -, vroll, vpitch, vyaw, ax, ay, az ]
   # Uses: orientation (roll,pitch) + angular velocity + acceleration
   # Doesn't use: position (IMU can't estimate absolute position)
   ```

5. **Noise Covariances**:
   ```yaml
   process_noise_covariance: [0.05, 0.05, 0.1, 0.03, 0.03, 0.03]
   # Larger values = less trust in motion model
   # For mobile robot without acceleration input, use these values
   ```

### Phase 4 Integration Steps

1. **Install robot_localization**:
   ```bash
   sudo apt install ros-humble-robot-localization
   ```

2. **Create launch file** (`ekf_bringup.launch.py`):
   ```python
   from launch import LaunchDescription
   from launch_ros.actions import Node
   from ament_index_python.packages import get_package_share_directory
   import os
   
   def generate_launch_description():
       config_dir = os.path.join(
           get_package_share_directory('esp32_serial_bridge'),
           'config'
       )
       
       ekf_params = os.path.join(config_dir, 'ekf_phase4.yaml')
       
       ekf_node = Node(
           package='robot_localization',
           executable='ekf_node',
           name='ekf_filter_node',
           output='screen',
           parameters=[ekf_params],
           remappings=[
               ('odometry/filtered', 'odometry/filtered'),
               ('accel/filtered', 'accel/filtered'),
           ]
       )
       
       return LaunchDescription([ekf_node])
   ```

3. **Update main launch file** (`micro_ros_bringup.launch.py`):
   ```python
   # Add to launch description:
   ekf_launch = IncludeLaunchDescription(
       PythonLaunchDescriptionSource(
           os.path.join(config_dir, '..', 'launch', 'ekf_bringup.launch.py')
       )
   )
   # Add ekf_launch to ld.add_action()
   ```

4. **Build and launch**:
   ```bash
   cd ~/ros2_ws
   colcon build --packages-select esp32_serial_bridge
   source install/setup.bash
   
   # Launch with EKF
   ros2 launch esp32_serial_bridge micro_ros_bringup.launch.py
   ```

5. **Monitor EKF output**:
   ```bash
   # Check filtered odometry (should be smoother than raw)
   ros2 topic echo /odometry/filtered
   
   # Compare with raw encoder odometry
   ros2 topic echo /odom
   ```

### Phase 4 Tuning Guide

If EKF output is poor, adjust covariances:

1. **If EKF drifts (ignores measurements)**:
   - Problem: EKF trusts motion model too much
   - Solution: Increase `process_noise_covariance` values

2. **If EKF jumps (over-trusts noisy sensors)**:
   - Problem: EKF trusts sensor measurements too much
   - Solution: Decrease covariance in sensor message (set larger in msg)

3. **If EKF slowly diverges**:
   - Problem: Process noise allows too much drift
   - Solution: Increase process noise, or reduce sensor covariance

### Phase 4 Diagnostics

```bash
# Check EKF is running
ros2 node list | grep ekf

# Get EKF parameters
ros2 param get /ekf_filter_node frequency

# Check TF tree (should have odom → base_link)
ros2 run tf2_tools view_frames
```

---

## Phase 5: Scan Improvements

### Overview
**Objective**: Improve VL53L0X + servo scan quality for SLAM/navigation

**Current State**:
- 37 measurement points (large 5° increments)  
- No filtering
- Prone to outliers

**Target State**:
- 90+ measurement points (fine 2° increments)
- Median filtering (window 3)
- Outlier rejection (> 2.0 m or < 0.05 m)
- Consistent timing

### Implementation

**File**: Update `/home/ros/ros2_ws/sensor_testing/src/main_micro_ros_phase2.cpp`

**New Scan Publishing Function**:
```cpp
// Phase 5: Improved scan with filtering
void publish_scan_phase5() {
    rcl_time_point_value_t now = time_sync_get_timestamp();
    
    memset(&scan_msg, 0, sizeof(scan_msg));
    
    scan_msg.header.stamp.sec = now / 1000000000;
    scan_msg.header.stamp.nanosec = now % 1000000000;
    scan_msg.header.frame_id = "base_scan";
    
    // Phase 5: 90 points instead of 37 (2° instead of 5° increments)
    const int NUM_POINTS = 90;  // Was 37
    scan_msg.angle_min = -M_PI / 2;
    scan_msg.angle_max = M_PI / 2;
    scan_msg.angle_increment = M_PI / NUM_POINTS;
    scan_msg.time_increment = 0.001;
    scan_msg.scan_time = 0.1;
    scan_msg.range_min = 0.05;   // Avoid noise floor
    scan_msg.range_max = 2.0;    // Realistic max
    
    // Collect measurements with filtering
    std::vector<float> ranges(NUM_POINTS);
    for (int i = 0; i < NUM_POINTS; i++) {
        float range = distance_sensor.readRangeContinuousMillimeters() / 1000.0;
        
        // Phase 5: Outlier rejection
        if (range < scan_msg.range_min || range > scan_msg.range_max) {
            range = INFINITY;  // Invalid reading
        }
        
        ranges[i] = range;
    }
    
    // Phase 5: Median filtering (window 3)
    std::vector<float> filtered_ranges = apply_median_filter(ranges, 3);
    
    // Copy to message
    for (int i = 0; i < NUM_POINTS; i++) {
        scan_msg.ranges[i] = filtered_ranges[i];
    }
    
    rcl_publish(&scan_pub, &scan_msg, NULL);
}

// Median filter helper
std::vector<float> apply_median_filter(const std::vector<float>& data, int window) {
    std::vector<float> result(data.size());
    int half_window = window / 2;
    
    for (size_t i = 0; i < data.size(); i++) {
        std::vector<float> window_values;
        for (int j = -half_window; j <= half_window; j++) {
            int idx = i + j;
            if (idx >= 0 && idx < (int)data.size()) {
                window_values.push_back(data[idx]);
            }
        }
        
        if (!window_values.empty()) {
            std::sort(window_values.begin(), window_values.end());
            result[i] = window_values[window_values.size() / 2];
        } else {
            result[i] = data[i];
        }
    }
    
    return result;
}
```

### Phase 5 Integration Steps

1. **Update scan message size** in configuration:
   ```cpp
   // Increase message size for 90 points
   #define MAX_SCAN_POINTS 90
   ```

2. **Update servo sweep pattern**:
   ```cpp
   // Sweep servo appropriately for 90 points
   const float SERVO_STEP = 180.0 / 90;  // 2° per point
   ```

3. **Rebuild and test**:
   ```bash
   cd ~/ros2_ws/sensor_testing
   pio run --environment micro_ros_wifi
   pio run --environment micro_ros_wifi --target upload
   ```

4. **Verify improved scan**:
   ```bash
   # Record before/after scans
   ros2 bag record /scan -o phase5_test
   
   # Visualize with RViz
   ros2 run rviz2 rviz2
   # Add LaserScan display, select /scan
   ```

---

## Phase 6: System Simplification

### Overview
**Objective**: Remove redundant bridge node, reduce latency and complexity

**Current Architecture** (with bridge):
```
ESP32 → WiFi → micro-ROS Agent → Bridge Node → /odom, /imu, /scan
                    ↓
                Extra latency (5-20 ms)
```

**Target Architecture** (after Phase 6):
```
ESP32 → WiFi → micro-ROS Agent → /odom, /imu, /scan
                    ↓
                Reduced latency (< 5 ms)
                Direct ROS-native topics
```

### Why Bridge is Redundant

The micro-ROS Agent already:
1. Receives data from ESP32 over UDP
2. Publishes to ROS 2 topics (/odom, /imu, /scan)
3. Subscribes to ROS 2 topics (/cmd_vel)

The Bridge Node currently:
1. Subscribes to those ROS 2 topics (with remapping)
2. Re-publishes to different topic names
3. Adds 5-20 ms latency per message

**Redundancy**: Step 2 with renamed topics is unnecessary!

### Phase 6 Implementation

**Prerequisites**:
- Phase 2 complete (time sync ✓)
- Phase 4 complete (EKF working ✓)
- Phase 5 complete (scan quality ✓)

**Step 1: Update micro-ROS Agent Launch**

Remove topic remappings:
```bash
# Before Phase 6:
ros2 run micro_ros_agent micro_ros_agent udp4 --port 8888 \
  --ros-domain-id 0

# After Phase 6 (same - agent already publishes direct topics)
# No changes needed to agent command
```

**Step 2: Remove Bridge from Launch File**

Before:
```python
# Launch bridge (performs topic relay)
robot_bridge_node = Node(
    package='esp32_serial_bridge',
    executable='micro_ros_robot_bridge.py',
    ...
)
ld.add_action(robot_bridge_node)
```

After:
```python
# Bridge completely removed
# Agent publishes directly to /odom, /imu, /scan
```

**Step 3: Update Launch Configuration**

Create simplified launch file:
```python
from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    # Just the agent
    agent = Node(
        package='micro_ros_setup',
        executable='agent',
        arguments=['udp4', '--port', '8888'],
        ...
    )
    
    # EKF remains
    ekf_node = Node(...)
    
    return LaunchDescription([agent, ekf_node])
```

**Step 4: Verify Topics Are Published**

```bash
# After removing bridge, check topics
ros2 topic list
# Should see:
# /odom (direct from agent)
# /imu (direct from agent)
# /scan (direct from agent)
# /odometry/filtered (from EKF)
```

**Step 5: Update Navigation Stack Inputs**

If using SLAM or navigation:
```yaml
# Before Phase 6:
odom_topic: /odom_in  # from bridge
cmd_vel_topic: /cmd_vel

# After Phase 6:
odom_topic: /odom     # direct from agent
cmd_vel_topic: /cmd_vel  # same
```

### Phase 6 Validation

1. **Latency Comparison**:
   ```bash
   # Measure latency before/after
   ros2 topic info /odom -v
   # Should show:
   # Publisher count: 1 (was 2 with bridge)
   # Subscriber count: N
   ```

2. **Message Integrity**:
   ```bash
   # Verify messages are unchanged
   ros2 topic echo /odom | head -20
   # Should have proper x,y,theta values
   ```

3. **System Load**:
   ```bash
   # Monitor CPU usage
   # Bridge node removed = less CPU
   top -p $(pgrep -f micro_ros_robot_bridge)
   ```

---

## Integration Steps

### Quick Start (All Phases)

1. **Build Phase 2 firmware**:
   ```bash
   cd ~/ros2_ws/sensor_testing
   pio run --environment micro_ros_wifi --target upload
   ```

2. **Update bridge node**:
   ```bash
   cp micro_ros_robot_bridge_phase2.py \
      ~/ros2_ws/src/esp32_serial_bridge/esp32_serial_bridge/micro_ros_robot_bridge.py
   ```

3. **Install EKF package**:
   ```bash
   sudo apt install ros-humble-robot-localization
   ```

4. **Build ROS workspace**:
   ```bash
   cd ~/ros2_ws
   colcon build --packages-select esp32_serial_bridge
   ```

5. **Launch full system**:
   ```bash
   source ~/ros2_ws/install/setup.bash
   ros2 launch esp32_serial_bridge micro_ros_bringup.launch.py
   ```

### Verification Checklist

- [ ] ESP32 connects and publishes data
- [ ] Time sync offset < 100 ms  
- [ ] Message rates match spec (100/50/8 Hz)
- [ ] EKF node running and producing filtered output
- [ ] RViz shows scan data with high resolution (90 points)
- [ ] No CPU overload (< 30% on laptop)
- [ ] TF tree shows: map → odom → base_link

---

## Troubleshooting

### General

**Symptom**: "micro-ROS agent not found"
```bash
# Start agent in separate terminal
ros2 run micro_ros_agent micro_ros_agent udp4 --port 8888
```

**Symptom**: Messages not appearing
```bash
# Check WiFi connection
esp32 serial: pio device monitor --baud 115200
# Check agent is running and listening
ps aux | grep agent
```

### Phase 2 Specific

**Symptom**: "Time sync failed"
```
- Check ROS time: date
- Sync with: sudo ntpdate -s time.nist.gov
- Restart agent and ESP32
```

### Phase 4 Specific

**Symptom**: "EKF diverges from real position"
```
- Increase process_noise_covariance values
- Check that IMU and odom covariances are reasonable
- Verify timestamps are synchronized (Phase 2 working)
```

### Phase 5 Specific

**Symptom**: "Scan has gaps or noise"
```
- Verify servo is sweeping properly
- Check distance sensor I2C communication
- Increase median filter window (5 instead of 3)
```

### Phase 6 Specific

**Symptom**: "Topics disappear after removing bridge"
```
- Verify agent is still running
- Check agent command: ros2 run micro_ros_agent...
- Verify /odom, /imu, /scan appear in ros2 topic list
```

---

## Summary Table

| Phase | Focus | Key File | Status |
|-------|-------|----------|--------|
| 2 | Time Sync | time_sync.h | ✅ Created |
| 2 | Firmware | main_micro_ros_phase2.cpp | ✅ Created |
| 2 | Bridge | micro_ros_robot_bridge_phase2.py | ✅ Created |
| 4 | EKF Config | ekf_phase4.yaml | ✅ Created |
| 5 | Scan Improve | main_micro_ros.cpp (updated) | ⏳ Needs integration |
| 6 | Simplification | Launch file (update) | ⏳ Needs integration |

**Next Action**: Deploy Phase 2 files to ESP32 and validate time sync is working.

