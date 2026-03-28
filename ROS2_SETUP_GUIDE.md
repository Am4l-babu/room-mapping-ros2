# 🚀 ROS 2 Integration Guide for ESP32 Robot

## Overview

This guide walks you through setting up ROS 2 on your ESP32 robot with SLAM and navigation capabilities.

## Architecture

```
ESP32 Microcontroller
    ↓
[Serial Communication @ 115200 baud]
    ↓
robot_controller (ROS 2 Node)
    ├── Publishes: /odom, /scan, /imu
    ├── Subscribes: /cmd_vel
    └── Broadcasts: TF (odom → base_footprint)
    ↓
SLAM (slam_toolbox)
    ├── Reads: /scan, /odom
    └── Publishes: /map, /pose_graph_markers
    ↓
RViz2 Visualization
    └── Displays: Map, LaserScan, Robot TF
```

## Prerequisites

### 1. Install ROS 2 Dependencies

```bash
cd /home/ros/ros2_ws

# Install required packages
sudo apt update
sudo apt install -y \
    ros-humble-slam-toolbox \
    ros-humble-rviz2 \
    ros-humble-nav2-bringup \
    ros-humble-tf-transformations \
    python3-tf-transformations \
    python3-serial

# Source ROS 2
source /opt/ros/humble/setup.bash
echo "source /opt/ros/humble/setup.bash" >> ~/.bashrc
```

### 2. Build the Package

```bash
# Build the updated bridge package
colcon build --packages-select esp32_serial_bridge --symlink-install

# Source the workspace
source install/setup.bash
echo "source /home/ros/ros2_ws/install/setup.bash" >> ~/.bashrc
```

## Quick Start

### Step 1: Connect ESP32 and Verify Serial Port

```bash
# Find your ESP32 port
ls -la /dev/ttyACM* /dev/ttyUSB*

# Expected: /dev/ttyACM1 or similar
# Note: Update launch files if different from /dev/ttyACM1
```

### Step 2: Upload the Motor Control Code

Ensure the `motor_interactive_test` firmware is running on ESP32:

```bash
cd /home/ros/ros2_ws/sensor_testing
pio run -e motor_interactive -t upload --upload-port /dev/ttyACM1
```

### Step 3A: Start Robot (Bringup Only)

```bash
ros2 launch esp32_serial_bridge bringup.launch.py port:=/dev/ttyACM1
```

This will:
- ✅ Connect to ESP32
- ✅ Start robot controller node
- ✅ Launch robot state publisher
- ✅ Publish odometry and TF frames

### Step 3B: Run SLAM (Mapping)

In a **new terminal**:

```bash
ros2 launch esp32_serial_bridge slam.launch.py port:=/dev/ttyACM1
```

This will:
- ✅ Start robot bringup
- ✅ Launch slam_toolbox (async mapping)
- ✅ Open RViz2 with pre-configured SLAM view

**What to do in RViz:**
1. Drive the robot around using `ros2 topic pub /cmd_vel geometry_msgs/Twist "{linear: {x: 0.2}, angular: {z: 0}}"`
2. Watch the map being built in real-time
3. The `/map` topic shows the generated map
4. Press Ctrl+C when done mapping

### Step 3C: Navigation Setup

After mapping, save the map:

```bash
mkdir -p /home/ros/ros2_ws/maps

# In another terminal while SLAM is running
ros2 service call /save_map slam_toolbox/srv/SaveMap "{name: {data: '/home/ros/ros2_ws/maps/my_room'}}"
```

For navigation later:

```bash
ros2 launch esp32_serial_bridge navigation.launch.py port:=/dev/ttyACM1
```

## Understanding the Data Flow

### Serial Protocol from ESP32

The robot controller expects these formats from ESP32:

```
RPM:motor1_rpm,motor2_rpm,pulses_m1,pulses_m2
SCAN:angle_degrees,distance_mm
IMU:accel_x,accel_y,accel_z,gyro_x,gyro_y,gyro_z,temp
```

### Example Serial Output

```
RPM:50.63,70.25,125,142
SCAN:0,500
SCAN:5,450
SCAN:10,480
IMU:-0.88,0.05,6.91,0.1,0.05,-0.02,34.5
```

## ROS 2 Topics

### Published Topics

| Topic | Type | Description |
|-------|------|-------------|
| `/odom` | nav_msgs/Odometry | Robot odometry |
| `/scan` | sensor_msgs/LaserScan | Laser scans (37 points, -90° to +90°) |
| `/imu` | sensor_msgs/Imu | IMU measurements |

### Subscribed Topics

| Topic | Type | Description |
|-------|------|-------------|
| `/cmd_vel` | geometry_msgs/Twist | Velocity command |

### SLAM Topics (when running)

| Topic | Type | Description |
|-------|------|-------------|
| `/map` | nav_msgs/OccupancyGrid | Generated occupancy grid map |
| `/pose_graph_markers` | visualization_msgs/MarkerArray | Loop closures visualization |

## Testing

### Test 1: Verify Robot Controller Connection

```bash
# Terminal 1: Start bringup
ros2 launch esp32_serial_bridge bringup.launch.py

# Terminal 2: Check topics
ros2 topic list

# Expected output:
# /odom
# /scan  
# /tf
# /tf_static
```

### Test 2: Check Odometry

```bash
ros2 topic echo /odom

# Should see position and orientation updating
geometry_msgs.msg.Pose:
  position:
    x: 0.xx
    y: 0.xx
    z: 0.0
  orientation:
    x: 0.0
    y: 0.0
    z: 0.xx
    w: 0.xx
```

### Test 3: Check Laser Scan

```bash
ros2 topic echo /scan | head -20

# Should see 37 range measurements
ranges: [inf, inf, ..., 0.45, 0.48, ...]
```

### Test 4: Send Motor Command

```bash
# Move forward
ros2 topic pub /cmd_vel geometry_msgs/Twist '{"linear": {"x": 0.2}}'

# Turn right
ros2 topic pub /cmd_vel geometry_msgs/Twist '{"angular": {"z": 1.0}}'

# Stop
ros2 topic pub /cmd_vel geometry_msgs/Twist '{"linear": {"x": 0.0}}'
```

## Troubleshooting

### Issue: "Could not open serial port"

**Solution:**
```bash
# Check port
ls -la /dev/ttyACM*

# Give permissions
sudo chmod 666 /dev/ttyACM1

# Or add user to dialout group
sudo usermod -a -G dialout $USER
newgrp dialout
```

### Issue: No data in /odom

**Solution:**
1. Verify ESP32 is running motor_interactive test
2. Check serial port connection: `cat /dev/ttyACM1` (Ctrl+C to exit)
3. Check robot_controller node logs: Look for "Connected to ESP32"

### Issue: SLAM not creating map

**Solution:**
1. Check `/scan` topic is publishing: `ros2 topic hz /scan`
2. Ensure `/odom` is publishing: `ros2 topic hz /odom`
3. Move robot to generate scans
4. Check RViz: Ensure "Fixed Frame" is set to "map"

### Issue: Wrong encoder direction

If wheels go backward when robot goes forward:
- This is handled in the interactive motor test
- The direction reversal has already been applied to motor_encoders.cpp

## ROS 2 Concepts Used

### Transforms (TF2)
- **odom → base_footprint**: Published by robot_controller
- **base_footprint → base_link**: From URDF
- **base_link → tof_sensor**: From URDF

### Nodes
- `robot_controller`: Main interface with ESP32
- `robot_state_publisher`: Publishes static and dynamic TFs
- `slam_toolbox`: SLAM algorithm
- `rviz2`: Visualization

### Message Types
- **Odometry**: Position, orientation, velocity
- **LaserScan**: Distance measurements at angles
- **Twist**: Velocity command (linear.x, angular.z)
- **Imu**: Accelerometer and gyroscope data

## Next Steps

1. **Save your map**: `ros2 service call /save_map ...`
2. **Load map for localization**: Use nav2_map_server
3. **Add autonomous navigation**: Use nav2 stack
4. **Implement path planning**: Use nav2_planner
5. **Add barrier avoidance**: Use costmap_2d filters

## Advanced Configuration

### Change Serial Port

Update launch files or pass as argument:

```bash
ros2 launch esp32_serial_bridge slam.launch.py port:=/dev/ttyUSB0
```

### Adjust SLAM Parameters

Edit: `/home/ros/ros2_ws/src/esp32_serial_bridge/config/mapper_params_online_async.yaml`

Key tuning parameters:
- `minimum_travel_distance`: Distance robot must move before scan matching (default: 0.5m)
- `loop_search_max_linear_radius`: Max distance to search for loop closures (default: 4.0m)

### Modify Robot Physical Parameters

Edit: `bringup.launch.py`
- `wheel_diameter`: 0.065 (meters)
- `wheel_separation`: 0.18 (meters)
- `max_speed`: 0.5 (m/s)

## Performance Tips

✅ **For Better Mapping:**
- Move robot slowly (0.1-0.2 m/s)
- Drive in figure-8 patterns for loop closure
- Cover larger areas to improve map quality

✅ **For Better Odometry:**
- Ensure wheels have good traction
- Verify encoders are clean
- Check motor power supply voltage

✅ **For Faster Processing:**
- Reduce scan matching frequency if CPU bound
- Decrease `throttle_scans` parameter
- Close RViz if not needed for real-time operation

## References

- [ROS 2 Documentation](https://docs.ros.org)
- [slam_toolbox](https://github.com/StanleyInnovation/slam_toolbox)
- [nav2](https://navigation.ros.org)
- [TF2](https://wiki.ros.org/tf2)

---

## Support

For issues or questions, check:
1. ROS 2 logs: `ros2 topic echo /rosout`
2. Node logs: Check terminal output for error messages
3. ESP32 serial output: `ros2 run sensor_msgs msg
