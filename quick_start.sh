#!/bin/bash
# Quick setup and launch script for ROS 2 robot with SLAM

echo "╔════════════════════════════════════════════╗"
echo "║  ROS 2 Robot SLAM Quick Start Script       ║"
echo "╚════════════════════════════════════════════╝"

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Check if ROS 2 is sourced
if ! command -v ros2 &> /dev/null; then
    echo -e "${YELLOW}⚠ ROS 2 not found. Sourcing setup...${NC}"
    source /opt/ros/humble/setup.bash
fi

# Source workspace
source /home/ros/ros2_ws/install/setup.bash

# Show options
echo ""
echo -e "${GREEN}Available options:${NC}"
echo "  1) Bringup (robot controller only)"
echo "  2) SLAM (mapping with visualization)"
echo "  3) Navigation (requires pre-built map)"
echo "  4) Test robotcontroller"
echo "  5) Check topics"
echo "  6) Install dependencies"
echo ""
read -p "Select option (1-6): " choice

case $choice in
    1)
        echo -e "${GREEN}Starting robot bringup...${NC}"
        ros2 launch esp32_serial_bridge bringup.launch.py port:=/dev/ttyACM1
        ;;
    2)
        echo -e "${GREEN}Starting SLAM with slam_toolbox...${NC}"
        echo -e "${YELLOW}Tips:${NC}"
        echo "  - Move robot slowly to generate map"
        echo "  - Use 'ros2 topic pub /cmd_vel ...' to command motion"
        echo "  - Save map when done: ros2 service call /save_map slam_toolbox/srv/SaveMap \"{name: {data: '/home/ros/ros2_ws/maps/my_map'}}\""
        echo ""
        ros2 launch esp32_serial_bridge slam.launch.py port:=/dev/ttyACM1
        ;;
    3)
        echo -e "${GREEN}Starting navigation setup...${NC}"
        echo -e "${YELLOW}Note:${NC} Load your saved map for localization"
        ros2 launch esp32_serial_bridge navigation.launch.py port:=/dev/ttyACM1
        ;;
    4)
        echo -e "${GREEN}Testing robot controller connection...${NC}"
        echo ""
        echo "Topics being published:"
        ros2 topic list | grep -E "odom|scan|imu" || echo "No topics found - check connection"
        ;;
    5)
        echo -e "${GREEN}Available ROS 2 topics:${NC}"
        ros2 topic list
        echo ""
        echo -e "${YELLOW}Checking message rates:${NC}"
        ros2 topic hz /odom 2>/dev/null &
        ros2 topic hz /scan 2>/dev/null &
        sleep 3
        pkill -f "topic hz"
        ;;
    6)
        echo -e "${GREEN}Installing dependencies...${NC}"
        sudo apt update
        sudo apt install -y \
            ros-humble-slam-toolbox \
            ros-humble-rviz2 \
            ros-humble-robot-state-publisher \
            ros-humble-tf-transformations \
            python3-tf-transformations \
            python3-serial
        echo -e "${GREEN}✓ Dependencies installed${NC}"
        ;;
    *)
        echo -e "${RED}Invalid option${NC}"
        exit 1
        ;;
esac
