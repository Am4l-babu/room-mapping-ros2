#!/bin/bash
#
# Phase 2 Quick Deployment Script
# Copies all Phase 2 files to correct locations and builds the system
#
# Usage: ./deploy_phase2.sh

set -e  # Exit on error

echo "================================================"
echo "Phase 2: Time Synchronization Deployment"
echo "================================================"

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# ============= STEP 1: Verify files exist =============
echo -e "${YELLOW}[1/5] Verifying Phase 2 files...${NC}"

files_to_check=(
    "/home/ros/ros2_ws/sensor_testing/src/time_sync.h"
    "/home/ros/ros2_ws/sensor_testing/src/main_micro_ros_phase2.cpp"
    "/home/ros/ros2_ws/src/esp32_serial_bridge/esp32_serial_bridge/micro_ros_robot_bridge_phase2.py"
)

for file in "${files_to_check[@]}"; do
    if [[ ! -f "$file" ]]; then
        echo -e "${RED}ERROR: Missing file: $file${NC}"
        exit 1
    fi
    echo -e "${GREEN}✓${NC} Found: $file"
done

# ============= STEP 2: Backup original files =============
echo -e "${YELLOW}[2/5] Backing up original files...${NC}"

BACKUP_DIR="/home/ros/ros2_ws/backups/phase2_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$BACKUP_DIR"

# Backup main.cpp
if [[ -f "/home/ros/ros2_ws/sensor_testing/src/main.cpp" ]]; then
    cp "/home/ros/ros2_ws/sensor_testing/src/main.cpp" "$BACKUP_DIR/"
    echo -e "${GREEN}✓${NC} Backed up: main.cpp"
fi

# Backup bridge node
if [[ -f "/home/ros/ros2_ws/src/esp32_serial_bridge/esp32_serial_bridge/micro_ros_robot_bridge.py" ]]; then
    cp "/home/ros/ros2_ws/src/esp32_serial_bridge/esp32_serial_bridge/micro_ros_robot_bridge.py" "$BACKUP_DIR/"
    echo -e "${GREEN}✓${NC} Backed up: micro_ros_robot_bridge.py"
fi

echo -e "${YELLOW}   Backups stored in: $BACKUP_DIR${NC}"

# ============= STEP 3: Deploy ESP32 firmware =============
echo -e "${YELLOW}[3/5] Deploying Phase 2 ESP32 firmware...${NC}"

# Copy time_sync.h (verify it's there)
if [[ ! -f "/home/ros/ros2_ws/sensor_testing/src/time_sync.h" ]]; then
    echo -e "${RED}ERROR: time_sync.h not found!${NC}"
    exit 1
fi
echo -e "${GREEN}✓${NC} time_sync.h present"

# Copy main.cpp
cp "/home/ros/ros2_ws/sensor_testing/src/main_micro_ros_phase2.cpp" \
   "/home/ros/ros2_ws/sensor_testing/src/main.cpp"
echo -e "${GREEN}✓${NC} Deployed: main.cpp (Phase 2 firmware)"

# ============= STEP 4: Deploy bridge node =============
echo -e "${YELLOW}[4/5] Deploying Phase 2 bridge node...${NC}"

cp "/home/ros/ros2_ws/src/esp32_serial_bridge/esp32_serial_bridge/micro_ros_robot_bridge_phase2.py" \
   "/home/ros/ros2_ws/src/esp32_serial_bridge/esp32_serial_bridge/micro_ros_robot_bridge.py"
echo -e "${GREEN}✓${NC} Deployed: micro_ros_robot_bridge.py (Phase 2)"

# ============= STEP 5: Build system =============
echo -e "${YELLOW}[5/5] Building and uploading...${NC}"

echo -e "${YELLOW}   Building PlatformIO firmware...${NC}"
cd /home/ros/ros2_ws/sensor_testing
pio run --environment micro_ros_wifi
if [[ $? -eq 0 ]]; then
    echo -e "${GREEN}✓${NC} PlatformIO build successful"
else
    echo -e "${RED}✗ PlatformIO build failed${NC}"
    exit 1
fi

echo -e "${YELLOW}   Ready to upload. Connect ESP32 via USB...${NC}"
read -p "Press ENTER when ESP32 is connected via USB: "

echo -e "${YELLOW}   Uploading to ESP32...${NC}"
pio run --environment micro_ros_wifi --target upload
if [[ $? -eq 0 ]]; then
    echo -e "${GREEN}✓${NC} Upload successful!"
else
    echo -e "${RED}✗ Upload failed${NC}"
    exit 1
fi

echo -e "${YELLOW}   Building ROS workspace...${NC}"
cd ~/ros2_ws
colcon build --packages-select esp32_serial_bridge
if [[ $? -eq 0 ]]; then
    echo -e "${GREEN}✓${NC} ROS workspace build successful"
else
    echo -e "${RED}✗ ROS workspace build failed${NC}"
    exit 1
fi

# ============= DEPLOYMENT COMPLETE =============
echo ""
echo -e "${GREEN}================================================"
echo "Phase 2 Deployment Complete! ✓"
echo "================================================${NC}"
echo ""
echo "Next steps:"
echo "1. Start micro-ROS agent (in one terminal):"
echo "   ros2 run micro_ros_agent micro_ros_agent udp4 --port 8888"
echo ""
echo "2. Source ROS setup (in another terminal):"
echo "   source ~/ros2_ws/install/setup.bash"
echo ""
echo "3. Launch the system:"
echo "   ros2 launch esp32_serial_bridge micro_ros_bringup.launch.py"
echo ""
echo "4. Monitor time sync (in another terminal):"
echo "   pio device monitor --baud 115200 | grep -i 'time sync'"
echo ""
echo "5. Validate timestamp synchronization:"
echo "   ros2 topic echo /diagnostics | grep timestamp"
echo ""
echo -e "${YELLOW}Full guide: /home/ros/ros2_ws/PHASE_2_4_5_6_IMPLEMENTATION_GUIDE.md${NC}"
