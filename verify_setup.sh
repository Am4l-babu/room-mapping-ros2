#!/bin/bash

# ROS 2 Robot Integration - Step-by-Step Setup Script
# Run this to verify everything is set up correctly

echo "╔════════════════════════════════════════════════════════╗"
echo "║     ROS 2 SLAM Robot - Setup Verification             ║"
echo "║                    ESP32 Edition                       ║"
echo "╚════════════════════════════════════════════════════════╝"
echo ""

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Check function
check_command() {
    if command -v $1 &> /dev/null; then
        echo -e "${GREEN}✓${NC} $2"
        return 0
    else
        echo -e "${RED}✗${NC} $2 (NOT FOUND)"
        return 1
    fi
}

# Step 1: Check ROS 2
echo -e "${BLUE}Step 1: Checking ROS 2${NC}"
if [ -z "$ROS_DISTRO" ]; then
    echo -e "${YELLOW}⚠ ROS 2 not sourced. Sourcing now...${NC}"
    source /opt/ros/humble/setup.bash
fi

check_command "ros2" "ROS 2 CLI"
check_command "colcon" "Colcon build tool"
check_command "rviz2" "RViz2 visualization"

# Step 2: Check Python packages
echo ""
echo -e "${BLUE}Step 2: Checking Python packages${NC}"
python3 -c "import serial" 2>/dev/null && echo -e "${GREEN}✓${NC} pyserial" || echo -e "${RED}✗${NC} pyserial"
python3 -c "import tf_transformations" 2>/dev/null && echo -e "${GREEN}✓${NC} tf_transformations" || echo -e "${RED}✗${NC} tf_transformations"

# Step 3: Check workspace
echo ""
echo -e "${BLUE}Step 3: Checking workspace${NC}"
if [ -d "/home/ros/ros2_ws/install" ]; then
    echo -e "${GREEN}✓${NC} Install directory found"
else
    echo -e "${RED}✗${NC} Install directory NOT found"
fi

# Step 4: Check ESP32 hardware
echo ""
echo -e "${BLUE}Step 4: Checking ESP32 hardware${NC}"
if ls /dev/ttyACM* &> /dev/null; then
    PORTS=$(ls /dev/ttyACM* 2>/dev/null | wc -l)
    echo -e "${GREEN}✓${NC} Found $PORTS serial port(s): $(ls /dev/ttyACM*)"
else
    if ls /dev/ttyUSB* &> /dev/null; then
        echo -e "${YELLOW}⚠${NC} Found USB ports instead: $(ls /dev/ttyUSB*)"
        echo "      Update launch files to use the correct port"
    else
        echo -e "${RED}✗${NC} No serial ports found (ESP32 not connected)"
    fi
fi

# Step 5: Check ROS package
echo ""
echo -e "${BLUE}Step 5: Checking ROS packages${NC}"
source /home/ros/ros2_ws/install/setup.bash 2>/dev/null

if ros2 pkg list | grep -q "esp32_serial_bridge"; then
    echo -e "${GREEN}✓${NC} esp32_serial_bridge package found"
else
    echo -e "${RED}✗${NC} esp32_serial_bridge package NOT found (build it first)"
fi

# Step 6: Check required ROS packages
echo ""
echo -e "${BLUE}Step 6: Checking required ROS dependencies${NC}"
packages=("slam_toolbox" "rviz2" "robot_state_publisher" "tf2_ros")
for pkg in "${packages[@]}"; do
    if ros2 pkg list 2>/dev/null | grep -q "^$pkg"; then
        echo -e "${GREEN}✓${NC} $pkg installed"
    else
        echo -e "${RED}✗${NC} $pkg NOT installed"
    fi
done

# Step 7: Show next steps
echo ""
echo -e "${BLUE}════════════════════════════════════════════════════════${NC}"
echo -e "${GREEN}Setup Status Complete!${NC}"
echo ""

echo "NEXT STEPS:"
echo ""
echo "1. Source your workspace:"
echo -e "   ${YELLOW}source /opt/ros/humble/setup.bash${NC}"
echo -e "   ${YELLOW}source /home/ros/ros2_ws/install/setup.bash${NC}"
echo ""
echo "2. Verify ESP32 is running motor_interactive_test firmware"
echo ""
echo "3. Start SLAM:"
echo -e "   ${YELLOW}ros2 launch esp32_serial_bridge slam.launch.py port:=/dev/ttyACM1${NC}"
echo ""
echo "4. In RViz:"
echo "   - Drive the robot around"
echo "   - Watch the map being built"
echo "   - Press Ctrl+C when done"
echo ""
echo "5. Save your map:"
echo -e "   ${YELLOW}ros2 service call /save_map slam_toolbox/srv/SaveMap \"{name: {data: '/home/ros/ros2_ws/maps/my_room'}}\"${NC}"
echo ""
echo "For more details, see:"
echo "   - ROS2_SETUP_GUIDE.md"
echo "   - ROS2_INTEGRATION_SUMMARY.md"
echo ""
echo -e "${BLUE}════════════════════════════════════════════════════════${NC}"
