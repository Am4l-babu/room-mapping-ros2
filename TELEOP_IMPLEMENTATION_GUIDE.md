# TELEOP SETUP - STEP BY STEP

## 🚀 QUICKEST START (3 steps - 2 minutes)

### Step 1: Make scripts executable
```bash
cd ~/ros2_ws
chmod +x simple_teleop.py
chmod +x src/my_robot_controllers/scripts/sync_teleop_node.py
```

### Step 2: Terminal 1 - Start robot hardware
```bash
source ~/ros2_ws/install/setup.bash

# Start motor control (communicates with ESP32)
ros2 run esp32_serial_bridge esp32_bridge_node
```

### Step 3: Terminal 2 - Keyboard control + visualization
```bash
source ~/ros2_ws/install/setup.bash

# Launch everything
ros2 launch my_robot_description teleop_robot.launch.py enable_teleop:=true enable_rviz:=true
```

**That's it!** You now have:
- ✅ Motor control communicating with ESP32
- ✅ Keyboard teleoperation (will open in new window)
- ✅ RViz2 showing LiDAR scans and robot position
- ✅ Real-time command visualization

---

## 🎮 KEYBOARD COMMANDS (Once Running)

```
w - Move forward (press multiple times to go faster)
s - Move backward
a - Turn left
d - Turn right
SPACE - Stop
q - Quit
```

---

## 📡 WHAT'S HAPPENING BEHIND THE SCENES

```
Keyboard Input (in teleop window)
    ↓
    └─→ /cmd_vel topic (velocity command)
        ↓
    ├─→ ESP32 Motor Controller (commands motors)
    │   ├─→ Left motor
    │   └─→ Right motor
    │
    ├─→ RViz2 (visualizes command)
    │
    └─→ Odometry feedback
        └─→ /odom topic (robot calculates position from encoders)
            ├─→ RViz2 displays position
            └─→ SLAM uses to build map

LiDAR Scanning (independent):
    Scans continuously at 8 Hz (every 125ms)
    Publishes to /scan topic
    RViz2 shows as point cloud
```

**Key Point**: LiDAR keeps scanning at consistent rate regardless of motor commands, so when both real and sim robots get same /cmd_vel → they move the same way → LiDAR scans are synchronized!

---

## 📊 VERIFY EVERYTHING IS WORKING

### Check 1: Topics are publishing
```bash
source ~/ros2_ws/install/setup.bash

# In new terminal, list all active topics
ros2 topic list

# You should see:
# /cmd_vel           ← keyboard publishes commands here
# /odom              ← robot odometry  
# /imu               ← IMU data from ESP32
# /scan              ← LiDAR data
# /tf                ← coordinate transforms
# /clock             ← timing
```

### Check 2: Commands are being sent
```bash
# Watch what the keyboard sends (press keys while running this)
ros2 topic echo /cmd_vel

# When you press 'w' you'll see:
# linear:
#   x: 0.10000000149
# angular:
#   z: 0.0
```

### Check 3: Robot is moving
```bash
# Watch odometry (should change when robot moves)
ros2 topic echo /odom --once

# When robot moves, pose.pose.position.x should increase
```

### Check 4: LiDAR is scanning
```bash
# Check laser data
ros2 topic echo /scan --once

# Should show sensor_msgs/msg/LaserScan with range data
```

If all 4 checks work → ✅ **System is fully operational!**

---

## 🔴 TROUBLESHOOTING GUIDE

### Problem: "Cannot connect to /dev/ttyUSB0"
```bash
# Find the correct port
ls /dev/tty* | grep -E "USB|ACM"

# You should see /dev/ttyUSB0 or /dev/ttyACM0

# Edit the launch parameter:
nano ~/ros2_ws/src/my_robot_description/launch/teleop_robot.launch.py

# Find: robot_controller.py parameters
# Change: 'port': '/dev/ttyUSB0' → use your actual port

# Rebuild
colcon build
```

### Problem: Keyboard teleop window won't open
```bash
# Use standalone Python script instead:
source ~/ros2_ws/install/setup.bash
python3 ~/ros2_ws/simple_teleop.py

# This launches in the same terminal (doesn't need xterm)
```

### Problem: Robot doesn't respond to keyboard
```bash
# 1. Make sure teleop window has FOCUS (click on it!)
# 2. Check that esp32_bridge_node is running:
ros2 node list | grep esp32

# If not running, start it:
ros2 run esp32_serial_bridge esp32_bridge_node

# 3. Check motor controller is subscribed to /cmd_vel:
ros2 node info /esp32_bridge | grep Subscriptions
```

### Problem: LiDAR not showing in RViz
```bash
# 1. Check LiDAR data exists:
ros2 topic info /scan

# 2. In RViz:
#    - Add Display → LaserScan
#    - Set Topic to /scan
#    - Check Fixed Frame is "map"
```

### Problem: "ros2 command not found"
```bash
# You forgot to source the setup file!
source ~/ros2_ws/install/setup.bash

# Or add to .bashrc permanently:
echo "source ~/ros2_ws/install/setup.bash" >> ~/.bashrc
source ~/.bashrc
```

---

## 🌐 REAL + SIMULATED ROBOT (Optional Advanced)

If you want BOTH real and sim robot responding to same keyboard:

### Terminal 1: Real robot
```bash
source ~/ros2_ws/install/setup.bash
ros2 run esp32_serial_bridge esp32_bridge_node
```

### Terminal 2: Gazebo simulation
```bash
source ~/ros2_ws/install/setup.bash

# Install Gazebo if needed:
sudo apt install ros2-humble-gazebo-*

# Launch simulation:
ros2 launch gazebo_ros gazebo.launch.py
```

### Terminal 3: Single teleop commands BOTH
```bash
source ~/ros2_ws/install/setup.bash

# Our sync script sends to both /real/cmd_vel and /sim/cmd_vel
python3 ~/ros2_ws/simple_teleop.py --ros-args -p enable_sim:=true
```

### Terminal 4: Visualize both in RViz
```bash
source ~/ros2_ws/install/setup.bash
rviz2
```

In RViz, add:
- Real robot LaserScan: /scan → Color red
- Sim robot LaserScan: /sim/scan → Color green
- See both robots moving together! ✨

---

## 📝 CHECKLIST BEFORE STARTING

Before you begin, ensure you have:

- [ ] Ubuntu 20.04 or 22.04
- [ ] ROS 2 installed (Humble or Iron)
- [ ] Workspace setup at ~/ros2_ws
- [ ] ESP32 firmware flashed with motor code
- [ ] ESP32 connected to computer via USB
- [ ] LiDAR connected to computer
- [ ] ROS 2 packages built:
  ```bash
  cd ~/ros2_ws
  colcon build
  ```

---

## ⚡ PERFORMANCE TIPS

### Make keyboard response faster
Edit `/home/ros/ros2_ws/simple_teleop.py`:
```python
self.get_parameter('linear_scale').value  # Increase from 0.1 to 0.2
self.get_parameter('angular_scale').value  # Increase from 0.2 to 0.3
```

### Smoother motion in RViz
In RViz Displays:
- LaserScan → Color Transformer: Intensity
- Odometry → Arrow Length: 1.0

### Monitor CPU usage
```bash
# See if ROS 2 is using too much CPU
top -p $(pgrep -f ros2)

# Should be <20% for typical teleoperation
```

---

## 🎯 FINAL TEST: Try This Sequence

1. **Start everything** (follow Quick Start above)

2. **In RViz**: You should see a grid with robot in center

3. **Verify teleop window has focus** (click on it - make sure it's active)

4. **Press 'w'** → Robot should move forward in RViz → LiDAR points should move forward

5. **Press 'a'** → Robot should rotate left in RViz

6. **Press SPACE** → Robot should stop

7. **Press 'q'** → Exit program

If all 7 steps work: **✅ FULLY OPERATIONAL!**

---

## 📊 WHAT'S SYNCHRONIZED

When both real and sim robots use same /cmd_vel topic:

✅ **Synchronized**: 
- Motor commands (same velocities)
- Time delay (both execute at same time)
- Motion profiles (both accelerate same way)

✅ **Automatically in sync**:
- LiDAR scanning (8 Hz regardless of motion)
- IMU reading (50 Hz regardless of motion)
- Odometry updates (100 Hz regardless of motion)

**Result**: Both robots move identically → LiDAR scans identical → Sensor fusion identical

---

## 🚨 EMERGENCY STOP

If robot goes crazy:

```bash
# Immediately send zero velocity (in any terminal)
ros2 topic pub /cmd_vel geometry_msgs/msg/Twist '{}' -1

# Or press SPACE in teleop window
```

**Hardware Safety**: Motor watchdog will stop ESP32 if WiFi dies (500ms timeout from Phase 3)

---

## 📚 NEXT STEPS (After Basic Teleop Works)

1. **Validate motion accuracy**
   ```bash
   source ~/ros2_ws/install/setup.bash
   python3 VALIDATE_STRAIGHT_LINE.py
   ```

2. **Test rotation accuracy**
   ```bash
   python3 VALIDATE_ROTATION.py
   ```

3. **Verify safety (WiFi failure)**
   ```bash
   python3 VALIDATE_WIFI_FAILURE.py
   ```

4. **Build full map with SLAM**
   ```bash
   ros2 launch slam_toolbox online_async.launch.py
   ```

---

**Status**: Ready to implement  
**Time to setup**: 5 minutes  
**Time to first test**: 2 minutes  
**Complexity**: Beginner

## 👉 **START HERE**: Go to paragraph "Quick Start" above and follow all 3 steps!
