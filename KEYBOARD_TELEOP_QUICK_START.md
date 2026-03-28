# KEYBOARD TELEOPERATION - QUICK START GUIDE

## 🚀 Getting Started (5 minutes)

### Option A: Using Built-in Teleop (RECOMMENDED - Easiest)

**Terminal 1: Install and run teleop**
```bash
# Install (one time only)
sudo apt update
sudo apt install ros2-humble-teleop-twist-keyboard

# Run
source ~/ros2_ws/install/setup.bash
ros2 run teleop_twist_keyboard teleop_twist_keyboard
```

**Terminal 2: Start your robot + RViz**
```bash
source ~/ros2_ws/install/setup.bash

# If you have micro-ROS agent running:
ros2 launch slam_toolbox online_async.launch.py

# Or start your motor controller
ros2 run esp32_serial_bridge esp32_bridge_node

# Start RViz
rviz2
```

**Terminal 3: Monitor the robot**
```bash
# In new terminal, watch the /cmd_vel topic
ros2 topic echo /cmd_vel
```

**Keyboard Controls** (when teleop window is focused):
```
w/x : increase/decrease speed (forward/backward)
a/d : rotate left/right
SPACE : STOP
q : quit
```

---

### Option B: Using Launch File (FASTEST)

```bash
source ~/ros2_ws/install/setup.bash

# Launch everything at once
ros2 launch my_robot_description teleop_robot.launch.py
```

This starts:
- ✅ Robot controller (ESP32 bridge)
- ✅ Keyboard teleoperation (in separate window)
- ✅ RViz2 visualization
- ✅ All TF frames

---

## 🎮 Complete Keyboard Controls

### Velocity Control
```
Forward:    w key → faster forward    (max +0.5 m/s)
Backward:   x key → faster backward   (max -0.5 m/s)
Left:       a key → turn left         (max +1.0 rad/s)
Right:      d key → turn right        (max -1.0 rad/s)
Stop:       SPACE → full stop
Exit:       q key → quit
```

### Speed Levels (Cumulative)

Press w multiple times:
- 1× w: 0.1 m/s
- 2× w: 0.2 m/s
- 3× w: 0.3 m/s
- 4× w: 0.4 m/s
- 5× w: 0.5 m/s (max)

---

## 📡 Verify Communication

### Step 1: Check ROS 2 is Running
```bash
source ~/ros2_ws/install/setup.bash
ros2 topic list
```

Expected output:
```
/cmd_vel          ← Keyboard teleop publishes here
/odom             ← Robot publishes odometry
/imu              ← IMU data
/scan             ← LiDAR data
```

### Step 2: Monitor Commands
```bash
# Watch what commands are being sent
ros2 topic echo /cmd_vel

# Expected when pressing 'w':
# ---
# linear:
#   x: 0.1
#   y: 0.0
#   z: 0.0
# angular:
#   x: 0.0
#   y: 0.0
#   z: 0.0
```

### Step 3: Check Robot Response
```bash
# Watch odometry to verify robot is moving
ros2 topic echo /odom --once

# If values change when you send commands: ✅ Working!
```

---

## 🔍 Troubleshooting

### Robot Doesn't Move

**Problem 1: Teleop not publishing**
```bash
# Check if teleop is sending commands
ros2 topic hz /cmd_vel

# Should show:
# average rate: 10.00
# If not: teleop window might not have focus
```

**Solution**: Click on the teleop window to ensure it has keyboard focus

---

**Problem 2: Robot controller not listening**
```bash
# Check ESP32 bridge is running
ros2 node list | grep esp32

# Should show: /esp32_bridge

# If not running:
ros2 run esp32_serial_bridge esp32_bridge_node \
  --ros-args -p port:=/dev/ttyUSB0 -p baudrate:=115200
```

---

**Problem 3: Serial port issue**
```bash
# Find correct serial port
ls /dev/tty*

# Common ports:
# /dev/ttyUSB0 - USB UART adapter
# /dev/ttyACM0 - Arduino/ESP32

# Test connection
cat /dev/ttyUSB0  # Should show data
```

---

### LiDAR Not Showing in RViz

**Check LiDAR is publishing**:
```bash
ros2 topic echo /scan --once

# Should show sensor_msgs/msg/LaserScan with ranges
```

**Add to RViz**:
1. Displays → Add → LaserScan
2. Set Topic: `/scan`
3. Color: adjust Min/Max values

---

### Teleop Window Won't Open

**Try without xterm** (simple terminal version):
```bash
source ~/ros2_ws/install/setup.bash

# Instead of:
# ros2 run teleop_twist_keyboard teleop_twist_keyboard

# Use this in same terminal as other nodes:
python3 ~/ros2_ws/src/my_robot_controllers/scripts/sync_teleop_node.py
```

---

## 🤖 Running Real Robot Only

**Terminal 1: Start robot hardware**
```bash
source ~/ros2_ws/install/setup.bash

# Start motor controller
ros2 run esp32_serial_bridge esp32_bridge_node &

# Start SLAM/LiDAR if available
ros2 launch slam_toolbox online_async.launch.py &
```

**Terminal 2: Keyboard control**
```bash
source ~/ros2_ws/install/setup.bash
ros2 run teleop_twist_keyboard teleop_twist_keyboard
```

**Terminal 3: Visualization**
```bash
source ~/ros2_ws/install/setup.bash
rviz2
```

---

## 🌐 Running Real + Simulated Robot Together

### Setup with Gazebo Simulation

**Terminal 1: Real robot**
```bash
source ~/ros2_ws/install/setup.bash
ros2 run esp32_serial_bridge esp32_bridge_node
```

**Terminal 2: Simulated robot (Gazebo)**
```bash
source ~/ros2_ws/install/setup.bash
ros2 launch gazebo_ros gazebo.launch.py
```

**Terminal 3: Teleop**
```bash
source ~/ros2_ws/install/setup.bash

# For real robot only:
ros2 run teleop_twist_keyboard teleop_twist_keyboard \
  --ros-args -r /cmd_vel:=/real/cmd_vel

# Opening second teleop for sim (in another terminal):
ros2 run teleop_twist_keyboard teleop_twist_keyboard \
  --ros-args -r /cmd_vel:=/sim/cmd_vel
```

**Terminal 4: RViz (view both)**
```bash
source ~/ros2_ws/install/setup.bash
rviz2 -d ~/ros2_ws/src/my_robot_description/rviz/dual_robot.rviz
```

---

## 📊 Monitor everything with rqt

```bash
rclpy
source ~/ros2_ws/install/setup.bash
rqt
```

Plugins to add (in rqt GUI):
- **Plugins → Topics → Topic Monitor** - See all active topics
- **Plugins → Visualization → RViz** - RViz inside rqt
- **Plugins → Introspection → Node Graph** - See all nodes and connections

---

## ⚡ Advanced: Custom Teleop with Limits

Create `/home/ros/ros2_ws/my_teleop.py`:

```python
#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from geometry_msgs.msg import Twist
import sys, select, termios, tty

class CustomTeleop(Node):
    def __init__(self):
        super().__init__('custom_teleop')
        self.pub = self.create_publisher(Twist, '/cmd_vel', 10)
        self.speed_x = 0.0
        self.speed_z = 0.0
        self.max_linear = 0.5
        self.max_angular = 1.0
        
    def send_cmd(self, x, z):
        msg = Twist()
        msg.linear.x = max(-self.max_linear, min(self.max_linear, x))
        msg.angular.z = max(-self.max_angular, min(self.max_angular, z))
        self.pub.publish(msg)
        print(f"\rSpeed: linear={msg.linear.x:.2f} m/s  angular={msg.angular.z:.2f} rad/s", end='')

# Usage:
# python3 my_teleop.py
```

---

## 📋 RViz Setup for Both Robots

**Add Displays**:
1. → Displays → Add
2. Select "LaserScan" → OK
3. Set Topic: `/scan`
4. Repeat for Odometry, TF, etc.

**Set frames to see everything**:
- Global Options → Fixed Frame: `map`
- Add LaserScan: `frame_id: laser`
- Add TF: Shows `map → odom → base_link → laser`

---

## ✅ Verification Checklist

Before considering setup complete:

- [ ] Teleop node runs without errors
- [ ] `ros2 topic echo /cmd_vel` shows movement data
- [ ] Robot responds to keyboard commands
- [ ] LiDAR appears in RViz
- [ ] Odometry updates in real-time
- [ ] No console errors

---

## 🔧 Emergency Stop

If robot won't stop:

```bash
# Immediately send zero velocity
ros2 topic pub /cmd_vel geometry_msgs/msg/Twist '{}' -1

# Or press SPACE in teleop window
```

**Watchdog will activate if WiFi dies** (motor stops within 500ms per Phase 3)

---

## 📚 Next Steps

1. **Test motion**: Drive robot in straight line, verify in RViz
2. **Validate drift**: Run `VALIDATE_STRAIGHT_LINE.py` (from validation framework)
3. **Test rotation**: Run `VALIDATE_ROTATION.py`
4. **Verify safety**: Run `VALIDATE_WIFI_FAILURE.py`

---

**Status**: Ready to use  
**Estimated Setup Time**: 10 minutes  
**Complexity**: Beginner-friendly

Start with Option A or B above!
