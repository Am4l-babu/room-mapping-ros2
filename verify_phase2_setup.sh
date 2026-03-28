#!/bin/bash
# Quick verification script for Phase 2 PID motor control implementation
# Run this BEFORE and AFTER deployment to verify all components

echo "╔═════════════════════════════════════════════════════════════╗"
echo "║  PHASE 2 VERIFICATION CHECKLIST - PID Motor Control        ║"
echo "╚═════════════════════════════════════════════════════════════╝"

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

check_count=0
pass_count=0

# Check 1: PID controller files exist
echo ""
echo "📋 Checking PID Controller Files..."
if [ -f "/home/ros/ros2_ws/sensor_testing/include/pid_controller.h" ]; then
    echo -e "${GREEN}✓${NC} pid_controller.h found"
    ((pass_count++))
else
    echo -e "${RED}✗${NC} pid_controller.h NOT found"
fi
((check_count++))

if [ -f "/home/ros/ros2_ws/sensor_testing/src/pid_controller.cpp" ]; then
    echo -e "${GREEN}✓${NC} pid_controller.cpp found"
    ((pass_count++))
else
    echo -e "${RED}✗${NC} pid_controller.cpp NOT found"
fi
((check_count++))

# Check 2: Motor control PID files
echo ""
echo "📋 Checking Motor Control PID Files..."
if [ -f "/home/ros/ros2_ws/sensor_testing/include/motor_control_pid.h" ]; then
    echo -e "${GREEN}✓${NC} motor_control_pid.h found"
    ((pass_count++))
else
    echo -e "${RED}✗${NC} motor_control_pid.h NOT found"
fi
((check_count++))

if [ -f "/home/ros/ros2_ws/sensor_testing/src/motor_control_pid.cpp" ]; then
    echo -e "${GREEN}✓${NC} motor_control_pid.cpp found"
    ((pass_count++))
else
    echo -e "${RED}✗${NC} motor_control_pid.cpp NOT found"
fi
((check_count++))

if [ -f "/home/ros/ros2_ws/sensor_testing/src/motor_control_pid_test.cpp" ]; then
    echo -e "${GREEN}✓${NC} motor_control_pid_test.cpp found"
    ((pass_count++))
else
    echo -e "${RED}✗${NC} motor_control_pid_test.cpp NOT found"
fi
((check_count++))

# Check 3: ROS 2 node
echo ""
echo "📋 Checking ROS 2 Improved Node..."
if [ -f "/home/ros/ros2_ws/src/esp32_serial_bridge/esp32_serial_bridge/robot_controller_improved.py" ]; then
    echo -e "${GREEN}✓${NC} robot_controller_improved.py found"
    ((pass_count++))
else
    echo -e "${RED}✗${NC} robot_controller_improved.py NOT found"
fi
((check_count++))

# Check 4: PlatformIO configuration
echo ""
echo "📋 Checking PlatformIO Configuration..."
if grep -q "motor_control_pid" "/home/ros/ros2_ws/sensor_testing/platformio.ini"; then
    echo -e "${GREEN}✓${NC} motor_control_pid environment in platformio.ini"
    ((pass_count++))
else
    echo -e "${RED}✗${NC} motor_control_pid environment NOT in platformio.ini"
fi
((check_count++))

# Check 5: Build PID firmware
echo ""
echo "📋 Attempting to Build PID Firmware..."
cd /home/ros/ros2_ws/sensor_testing 2>/dev/null

if command -v pio &> /dev/null; then
    export PLATFORMIO_CI_BUILD_DIR=.pio/build/motor_control_pid
    
    if pio run -e motor_control_pid 2>&1 | grep -q "Linking"; then
        echo -e "${GREEN}✓${NC} PID firmware builds successfully"
        ((pass_count++))
    else
        echo -e "${YELLOW}⚠${NC} PID firmware build incomplete (might need full pio setup)"
    fi
else
    echo -e "${YELLOW}⚠${NC} PlatformIO not available (skip build check)"
fi
((check_count++))

# Check 6: ROS 2 package structure
echo ""
echo "📋 Checking ROS 2 Package Structure..."
if [ -f "/home/ros/ros2_ws/src/esp32_serial_bridge/package.xml" ]; then
    echo -e "${GREEN}✓${NC} package.xml found"
    ((pass_count++))
else
    echo -e "${RED}✗${NC} package.xml NOT found"
fi
((check_count++))

# Check 7: Launch file exists
echo ""
echo "📋 Checking Launch Files..."
if [ -f "/home/ros/ros2_ws/src/esp32_serial_bridge/launch/slam.launch.py" ]; then
    echo -e "${GREEN}✓${NC} slam.launch.py found"
    ((pass_count++))
else
    echo -e "${RED}✗${NC} slam.launch.py NOT found"
fi
((check_count++))

# Check 8: Serial port available
echo ""
echo "📋 Checking Serial Port..."
if ls /dev/ttyACM* &> /dev/null; then
    echo -e "${GREEN}✓${NC} Serial port found: $(ls /dev/ttyACM*)"
    ((pass_count++))
else
    echo -e "${YELLOW}⚠${NC} No /dev/ttyACM* device (connect ESP32)"
fi
((check_count++))

# Summary
echo ""
echo "╔═════════════════════════════════════════════════════════════╗"
echo "║  VERIFICATION SUMMARY                                       ║"
echo "╠═════════════════════════════════════════════════════════════╣"
echo "║  Files:    $pass_count / $check_count checks passed        ║"
echo "╚═════════════════════════════════════════════════════════════╝"

if [ $pass_count -eq $check_count ]; then
    echo -e "${GREEN}✓ All checks passed! System ready for Phase 2 deployment${NC}"
    exit 0
else
    failed=$((check_count - pass_count))
    echo -e "${RED}✗ $failed check(s) failed. Review above and fix issues.${NC}"
    exit 1
fi
