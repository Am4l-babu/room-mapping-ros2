# 🚀 ESP32 Robot ROS 2 Integration Summary

## ✅ What Has Been Set Up

### 1. **Robot Controller Node** (`robot_controller.py`)
   - Reads ESP32 serial data (RPM, encoder counts, sensor data)
   - Publishes:
     - `/odom` - Odometry with position and orientation
     - `/scan` - 37-point laser scan from VL53L0X
     - `/imu` - Accelerometer and gyroscope data
   - Subscribes to `/cmd_vel` to control motors
   - Broadcasts TF transforms: `odom → base_footprint`

### 2. **Launch Files**
   - **bringup.launch.py** - Basic robot startup (controller + state publisher)
   - **slam.launch.py** - SLAM mapping with slam_toolbox + RViz
   - **navigation.launch.py** - Navigation setup (requires saved map)

### 3. **SLAM Configuration**
   - **mapper_params_online_async.yaml** - Optimized parameters for small robot mapping
   - Online asynchronous SLAM for real-time mapping
   - Loop closure detection for drift correction

### 4. **RViz Configurations**
   - **slam.rviz** - Pre-configured for SLAM visualization
   - **navigation.rviz** - Pre-configured for navigation

---

## 📋 File Structure

```
/home/ros/ros2_ws/
├── ROS2_SETUP_GUIDE.md          ← Comprehensive setup guide
├── quick_start.sh                ← Quick launch script
├── src/
│   └── esp32_serial_bridge/
│       ├── esp32_serial_bridge/
│       │   ├── robot_controller.py    ← NEW: Main ROS 2 node
│       │   └── bridge_node.py         (kept for compatibility)
│       ├── launch/
│       │   ├── bringup.launch.py      ← NEW: Robot startup
│       │   ├── slam.launch.py         ← NEW: SLAM with visualization
│       │   └── navigation.launch.py   ← NEW: Navigation setup
│       ├── config/
│       │   ├── mapper_params_online_async.yaml  ← NEW: SLAM config
│       │   ├── slam.rviz              ← NEW: SLAM visualization
│       │   └── navigation.rviz        ← NEW: Navigation visualization
│       └── package.xml                (updated dependencies)
```

---

## 🎯 Quick Start (3 Steps)

### Step 1: Source and Build

```bash
cd /home/ros/ros2_ws
source /opt/ros/humble/setup.bash
colcon build --packages-select esp32_serial_bridge
source install/setup.bash
```

### Step 2: Verify ESP32 Connection

```bash
# Check serial port
ls -la /dev/ttyACM*

# Should see /dev/ttyACM1 (or whatever your port is)
# Make sure motor_interactive_test firmware is running
```

### Step 3: Launch SLAM

```bash
# Option A: Use quick start script
bash /home/ros/ros2_ws/quick_start.sh
# Select option 2 for SLAM

# Option B: Direct launch
ros2 launch esp32_serial_bridge slam.launch.py port:=/dev/ttyACM1
```

---

## 🔄 Data Flow

```
┌─────────────┐
│   ESP32     │  (running motor_interactive_test)
│  Connected  │  Serial @ 115200 baud
│  Sensors:   │
│ - Motors    │  Publishes serial messages:
│ - Encoders  │  RPM:50.63,70.25,125,142
│ - VL53L0X   │  SCAN:0,500
│ - MPU6050   │  SCAN:5,480
└──────┬──────┘  IMU:-0.88,0.05,6.91,...
       │
       │ Serial port (/dev/ttyACM1)
       │
       ▼
┌──────────────────────────┐
│  robot_controller ROS    │  (New node)
│  Processes data:         │  Converts encoder pulses to distance
│  - Odometry calculation  │  Converts servo angles to scan points
│  - Twist → PWM command   │  Publishes ROS messages
│  - TF broadcasting       │
└────┬────────────────────┐│
     │ ROS 2 Topics       ││
     ├─ /odom            ││
     ├─ /scan            ││
     ├─ /imu             ││
     ├─ /cmd_vel (input) ││
     └─ /tf              ││
       │                 ││
       ▼                 ▼▼
  ┌─────────────┐  ┌──────────────┐
  │ slam_toolbox│  │   RViz 2     │
  │ (Mapping)   │  │ (Visualization)
  │Builds map   │  │ Shows map &
  │Tracks pose  │  │ robot pose
  └─────────────┘  └──────────────┘
```

---

## 📊 ROS 2 Topics Reference

### Published (from robot_controller)

| Topic | Type | Update Rate | Description |
|-------|------|-------------|-------------|
| `/odom` | Odometry | 100 Hz (when motors moving) | Position + orientation |
| `/scan` | LaserScan | Complete sweep (37 points) | Distance measurements |
| `/imu` | Imu | With sensor data | Accel + gyro + temp |

### Subscribed (from SLAM)

| Topic | Type | From | Description |
|-------|------|------|-------------|
| `/cmd_vel` | Twist | Nav stack or user | Motor command |

### SLAM Topics (when running)

| Topic | Type | Description |
|-------|------|-------------|
| `/map` | OccupancyGrid | Generated map |
| `/pose_graph_markers` | Markers | Loop closure visualization |

---

## 🧪 Testing Checklist

### ✓ Test 1: Verify Connection
```bash
ros2 launch esp32_serial_bridge bringup.launch.py
# Check for "Connected to ESP32" message
```

### ✓ Test 2: Check Topics
```bash
ros2 topic list
# Should see: /odom, /scan, /tf, /tf_static
```

### ✓ Test 3: Monitor Odometry
```bash
ros2 topic echo /odom --once
# Check x, y, theta,w values
```

### ✓ Test 4: Send Motor Command
```bash
# Forward
ros2 topic pub -1 /cmd_vel geometry_msgs/Twist "linear: {x: 0.2}"

# Turn
ros2 topic pub -1 /cmd_vel geometry_msgs/Twist "angular: {z: 1.0}"

# Stop
ros2 topic pub -1 /cmd_vel geometry_msgs/Twist
```

### ✓ Test 5: Run SLAM
```bash
ros2 launch esp32_serial_bridge slam.launch.py
# Drive robot in figure-8 pattern
# Watch map being built in RViz
```

---

## 🛠️ Customization

### Change Serial Port
```bash
ros2 launch esp32_serial_bridge slam.launch.py port:=/dev/ttyUSB0
```

### Adjust Robot Parameters
Edit `bringup.launch.py`:
```python
parameters=[{
    'port': LaunchConfiguration('port'),
    'baud': 115200,
    'wheel_diameter': 0.065,      # Change here
    'wheel_separation': 0.18,     # Change here
    'max_speed': 0.5,              # Change here
}]
```

### Tune SLAM
Edit `config/mapper_params_online_async.yaml`:
```yaml
minimum_travel_distance: 0.5      # How far to move before matching
loop_search_max_linear_radius: 4.0  # Loop closure search distance
```

---

## 📡 Command Examples

### Start SLAM (Interactive)
```bash
ros2 launch esp32_serial_bridge slam.launch.py
# Drive with teleop or /cmd_vel publishing
# Ctrl+C to stop
```

### Save Map
```bash
ros2 service call /save_map slam_toolbox/srv/SaveMap \
"{name: {data: '/home/ros/ros2_ws/maps/bedroom'}}"
```

### Check ROS 2 Network
```bash
# See all nodes
ros2 node list

# See all topics
ros2 topic list -t

# Monitor a topic in real-time
ros2 topic echo /odom

# Check message rates
ros2 topic hz /scan
```

### Visualize TF Tree
```bash
ros2 run tf2_tools view_frames
# Creates transforms.pdf
```

---

## 🔍 Debugging

### No Topics Publishing

1. **Check ESP32 is connected:**
   ```bash
   lsof /dev/ttyACM1  # Should show Python process
   ```

2. **Check motor firmware:**
   ```bash
   cat /dev/ttyACM1 | head -20  # Should see serial data
   # Ctrl+C to exit
   ```

3. **Check node startup logs:**
   ```bash
   ros2 launch esp32_serial_bridge bringup.launch.py
   # Look for error messages
   ```

### Odometry Drifting
- Verify encoder connections (GPIO 4, 5)
- Check wheel diameter setting (default 0.065m)
- Run SLAM to correct drift with map matching

### Map Not Being Created
- Check `/scan` is publishing: `ros2 topic hz /scan`
- Check `/odom` is publishing: `ros2 topic hz /odom`
- Ensure robot is moving: `ros2 topic pub /cmd_vel ...`
- Check SLAM parameters in config file

---

## 🎓 ROS 2 Concepts

### Transforms (TF)
- **odom** frame: Origin at start position
- **base_footprint** frame: Robot center at ground level
- **base_link**: From URDF (chassis)
- **tof_sensor**: Servo-mounted distance sensor

### Odometry
- Calculated from encoder pulses
- Position = wheel rotations × circumference
- Orientation = differential wheel movements

### Laser Scan
- 37 points from -90° to +90° (5° increments)
- Each point is distance reading from servo position
- Combined into single scan message each sweep

---

## 📚 Next Steps

1. **Map your space:** Run SLAM, navigate room, save map
2. **Add navigation:** Use nav2 with saved map
3. **Autonomous goals:** Set waypoints, robot navigates
4. **Obstacle avoidance:** Tune local costmap
5. **Real-time monitoring:** Set up remote monitoring
6. **Data visualization:** Publish camera feed to RViz

---

## ✨ Features Enabled

- ✅ **Real-time odometry** from motor encoders
- ✅ **Laser scan** from rotating distance sensor
- ✅ **SLAM mapping** with loop closure
- ✅ **Transform broadcasting** for robot model
- ✅ **IMU data** from accelerometer/gyro
- ✅ **RViz visualization** pre-configured
- ✅ **Motor control** via /cmd_vel topic
- ✅ **Map persistence** and loading

---

## 🚨 Important Notes

1. **Firmware:** Motor interactive test (`motor_interactive_test.cpp`) must be running
2. **Serial Format:** Ensure ESP32 outputs: `RPM:`, `SCAN:`, `IMU:` prefixed data
3. **Baud Rate:** Must be 115200 (cannot change without recompiling)
4. **Port:** Update if not /dev/ttyACM1
5. **Permissions:** May need `chmod 666 /dev/ttyACM1` or add to dialout group

---

## 🎯 Success Indicators

✅ **Working correctly if you see:**
- RViz window opens with grid
- Green robot model in center
- Red laser scan points
- Map updates as robot moves
- `/odom` position increasing
- SLAM creating occupancy grid

---

## 📞 Support Resources

- [ROS 2 Documentation](https://docs.ros.org)
- [slam_toolbox GitHub](https://github.com/StanleyInnovation/slam_toolbox)
- [TF2 Tutorials](https://wiki.ros.org/tf2/Tutorials)
- [Launch Files](https://docs.ros.org/en/humble/Conceptual_overview/Launch_system.html)

---

**Setup Complete!** 🎉

Your ESP32 robot is now fully integrated with ROS 2 and ready for SLAM mapping and autonomous navigation.
