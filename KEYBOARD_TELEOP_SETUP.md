# Keyboard Teleoperation Setup for Real + Simulated Robot

## Overview

This guide shows how to:
1. Control your robot with keyboard in RViz2
2. Run simulated and real robot simultaneously  
3. Keep LiDAR scanning synchronized

---

## 🎮 Option 1: Using ROS 2 Teleop Keyboard (RECOMMENDED)

### Installation

```bash
# Install teleop_twist_keyboard package
sudo apt update
sudo apt install ros2-humble-teleop-twist-keyboard

# Or with colcon if it's a package in your workspace
cd ~/ros2_ws
colcon build --packages-select teleop_twist_keyboard
```

### Quick Start

**Terminal 1: Start ROS 2 nodes and robot**
```bash
source ~/ros2_ws/install/setup.bash

# Start your robot's nodes (adjust based on your setup)
ros2 launch slam_toolbox online_async.launch.py

# OR if you have a robot launch file:
ros2 launch my_robot_bringup robot.launch.py
```

**Terminal 2: Start keyboard teleop**
```bash
source ~/ros2_ws/install/setup.bash

ros2 run teleop_twist_keyboard teleop_twist_keyboard --ros-args -r /cmd_vel:=/cmd_vel
```

**Terminal 3: Open RViz2**
```bash
source ~/ros2_ws/install/setup.bash

rviz2
```

### Keyboard Controls

Once running:

```
        w
        ↑
    a ← + → d
        ↓
        s

w/x : increase/decrease linear velocity (forward/backward)
a/d : increase/decrease angular velocity (turn left/right)
space : force stop
q : quit
```

### Expected Topics

The `teleop_twist_keyboard` publishes to `/cmd_vel`:

```bash
# Check the published messages:
ros2 topic echo /cmd_vel

# You should see:
# geometry_msgs/msg/Twist:
#   linear:
#     x: 0.20000000298
#   angular:
#     z: 0.0
```

---

## 🤖 Option 2: RViz2 Interactive Markers (Alternative)

### Install Interactive Markers

```bash
sudo apt install ros2-humble-interactive-markers
```

### Setup in RViz2

1. **Open RViz2**
   ```bash
   rviz2
   ```

2. **Add Display → By plugin → InteractiveMarkers**

3. **Set configuration**:
   - Topic: `/cmd_vel_marker` 
   - Frame: `base_link`

4. **Use mouse to control**:
   - Drag forward/back for linear movement
   - Drag left/right for angular movement

---

## 🌐 Running Real + Simulated Robot Together

### Architecture Setup

```
┌─────────────────────────────────────────────────────┐
│               Keyboard Control                       │
│        (teleop_twist_keyboard or RViz)              │
└────────────────────┬────────────────────────────────┘
                     │
                     ↓ /cmd_vel topic
        ┌────────────┴────────────┐
        ↓                         ↓
   ┌─────────────┐          ┌──────────────┐
   │ Real Robot  │          │ Gazebo Sim   │
   │ (ESP32)     │          │ (optional)   │
   └─────────────┘          └──────────────┘
        ↓ /odom               ↓ /sim_odom
        │ /scan               │ /sim_scan
        │ /imu                │ /sim_imu
        └────────────┬────────┘
                     ↓
                  RViz2
              (visualize both)
```

### Step-by-Step Setup

#### Step 1: Create Namespace for Simultaneous Robots

**Terminal 1: Real Robot (no namespace)**
```bash
source ~/ros2_ws/install/setup.bash

# Start real robot nodes
ros2 launch my_robot_bringup robot.launch.py \
  namespace:=real \
  use_sim_time:=false
```

**Terminal 2: Simulated Robot (with gazebo)**
```bash
source ~/ros2_ws/install/setup.bash

# Start Gazebo simulation
ros2 launch my_robot_bringup gazebo.launch.py \
  namespace:=sim \
  use_sim_time:=true
```

#### Step 2: Keyboard Teleop (Commands Both)

**Terminal 3: Teleop listening to real robot**
```bash
source ~/ros2_ws/install/setup.bash

ros2 run teleop_twist_keyboard teleop_twist_keyboard \
  --ros-args -r /cmd_vel:=/real/cmd_vel
```

**Terminal 4: Teleop for simulated robot** (optional, separate window)
```bash
source ~/ros2_ws/install/setup.bash

ros2 run teleop_twist_keyboard teleop_twist_keyboard \
  --ros-args -r /cmd_vel:=/sim/cmd_vel
```

#### Step 3: View Both in RViz2

**Terminal 5: RViz2 with both robots**
```bash
source ~/ros2_ws/install/setup.bash

rviz2
```

**RViz2 Configuration**:

1. **Fixed Frame**: `map` (or `real/map` if using namespaces)

2. **Add Displays** → Select:
   - TF (shows transform trees)
   - LaserScan (shows LiDAR)
   - Odometry (shows estimated position)
   - RobotModel (shows 3D model)

3. **Add separate displays for sim**:
   - LaserScan → Topic: `/sim/scan`
   - Odometry → Topic: `/sim/odometry/filtered`

---

## 📡 Synchronizing LiDAR Scanning

### Verify LiDAR is Publishing

```bash
# Check real robot LiDAR
ros2 topic echo /scan --once

# Check simulated LiDAR (if using Gazebo)
ros2 topic echo /sim/scan --once
```

### Set LiDAR Frame Rate

Edit your robot launch file to ensure consistent rate:

```xml
<!-- In robot.launch.py -->
<node pkg="rplidar_ros" exec="rplidar_composition" name="rplidar">
    <param name="serial_port" value="/dev/ttyUSB0"/>
    <param name="frame_id" value="laser"/>
    <param name="angle_compensate" value="true"/>
    <param name="scan_mode" value="Standard"/>  <!-- or Express, Boost -->
</node>
```

### Monitor Synchronization

```bash
# In separate terminal, check message frequencies
ros2 topic hz /scan          # Real robot
ros2 topic hz /sim/scan      # Simulated robot
```

**Expected output**:
```
average rate: 8.00
min: 0.00s max: 0.12s std dev: 0.02s window: ..
```

Both should show ~8 Hz if using typical LiDAR settings.

---

## 🎯 Complete Working Example

### Create Launch File for Both Robots

Create `/home/ros/ros2_ws/src/my_robot_bringup/launch/dual_robot.launch.py`:

```python
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import PathJoinSubstitution, LaunchConfiguration
from launch_ros.substitutions import FindPackageShare
from launch_ros.actions import Node
import os

def generate_launch_description():
    
    # Paths
    robot_desc_pkg_share = FindPackageShare(package='my_robot_description').find('my_robot_description')
    robot_bringup_pkg_share = FindPackageShare(package='my_robot_bringup').find('my_robot_bringup')
    
    # ===== REAL ROBOT =====
    real_robot_nodes = [
        # Real robot robot_state_publisher
        Node(
            package='robot_state_publisher',
            executable='robot_state_publisher',
            namespace='real',
            parameters=[{
                'robot_description': open(os.path.join(robot_desc_pkg_share, 'urdf', 'robot.urdf')).read(),
                'frame_prefix': 'real/',
            }],
            remappings=[
                ('/tf', 'tf'),
                ('/tf_static', 'tf_static'),
            ]
        ),
        
        # Real robot LiDAR (if using USB RPLIDAR)
        Node(
            package='rplidar_ros',
            executable='rplidar_composition',
            namespace='real',
            parameters=[{
                'serial_port': '/dev/ttyUSB0',
                'frame_id': 'laser',
                'angle_compensate': True,
                'scan_mode': 'Standard',
            }],
            output='screen'
        ),
        
        # Real robot motor controller
        Node(
            package='esp32_serial_bridge',
            executable='esp32_bridge_node',
            namespace='real',
            parameters=[{
                'port': '/dev/ttyUSB1',  # Adjust to your ESP32 port
                'baudrate': 115200,
            }],
            remappings=[
                ('/cmd_vel', 'cmd_vel'),
                ('/odom', 'odom'),
            ]
        ),
    ]
    
    # ===== SIMULATED ROBOT (Gazebo) =====
    gazebo_nodes = [
        # Gazebo server
        Node(
            package='gazebo_ros',
            executable='gzserver',
            arguments=['-s', 'libgazebo_ros_factory.so'],
            output='screen'
        ),
        
        # Gazebo client (GUI)
        Node(
            package='gazebo_ros',
            executable='gzclient',
            output='screen'
        ),
        
        # Spawn simulated robot
        Node(
            package='gazebo_ros',
            executable='spawn_entity.py',
            arguments=['-entity', 'robot_sim',
                      '-file', os.path.join(robot_desc_pkg_share, 'urdf', 'robot_gazebo.urdf'),
                      '-x', '0', '-y', '1', '-z', '0'],
            namespace='sim',
            output='screen'
        ),
    ]
    
    # ===== TELEOP KEYBOARD =====
    teleop_nodes = [
        # Real robot keyboard control
        Node(
            package='teleop_twist_keyboard',
            executable='teleop_twist_keyboard',
            namespace='real',
            remappings=[('/cmd_vel', 'cmd_vel')],
            output='screen'
        ),
        
        # Simulated robot keyboard control (separate terminal)
        # Node(
        #     package='teleop_twist_keyboard',
        #     executable='teleop_twist_keyboard',
        #     namespace='sim',
        #     remappings=[('/cmd_vel', 'cmd_vel')],
        #     output='screen'
        # ),
    ]
    
    # ===== VISUALIZATION =====
    rviz_config_file = os.path.join(robot_desc_pkg_share, 'rviz', 'robot.rviz')
    
    rviz_node = Node(
        package='rviz2',
        executable='rviz2',
        arguments=['-d', rviz_config_file],
        output='screen'
    )
    
    return LaunchDescription(
        real_robot_nodes +
        # gazebo_nodes +      # Uncomment for simulation
        teleop_nodes +
        [rviz_node]
    )
```

### Usage

```bash
# Build
cd ~/ros2_ws
colcon build

# Launch everything
source install/setup.bash
ros2 launch my_robot_bringup dual_robot.launch.py
```

---

## 🚨 Troubleshooting

### Robot Not Responding to Keyboard

**Check if teleop is publishing**:
```bash
ros2 topic echo /cmd_vel
```

If no messages appear, ensure teleop is running in correct terminal.

### LiDAR Not Appearing in RViz

1. Check frame name:
   ```bash
   ros2 topic info /scan
   ```

2. Verify it matches RViz display setting

3. Check TF tree:
   ```bash
   ros2 run tf2_tools view_frames.py
   ```

### Both Robots Moving Together?

Verify they're subscribed to same topic:

```bash
# Real robot
ros2 node info /robot_controller | grep Subscribers

# Simulated robot  
ros2 node info /gazebo | grep Subscribers
```

Both should show they're listening to `/cmd_vel`.

---

## 📊 Monitor Topics in Real-Time

Use `rqt_gui` for visualization:

```bash
rqt_gui
```

Add plugins:
- **Topic Monitor** → See all active topics and rates
- **Plot** → Graph velocity commands vs actual motion
- **TF Tree** → Verify frame relationships

---

## 🎓 Next Steps

1. **Test basic teleoperation** with real robot only
2. **Add simulation** once real robot works
3. **Tune velocity limits** for smooth control:
   ```python
   # In teleop config
   max_linear_velocity: 0.5  m/s
   max_angular_velocity: 1.0 rad/s
   ```

4. **Add safety features**:
   - Emergency stop button
   - Velocity limits by robot type
   - Collision avoidance (future phase)

---

**Status**: Ready to implement  
**Complexity**: Medium  
**Time**: 30-45 minutes to set up fully
