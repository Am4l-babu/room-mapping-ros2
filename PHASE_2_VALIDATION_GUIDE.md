# Phase 2 Validation & Testing Guide

## Pre-Validation Checklist

Before running validation tests, ensure:

- [ ] Deployment script completed successfully
- [ ] ESP32 connected via USB
- [ ] WiFi credentials updated in code
- [ ] Micro-ROS agent running: `ros2 run micro_ros_agent micro_ros_agent udp4 --port 8888`
- [ ] Bridge node running: Started by launch file
- [ ] ROS 2 environment sourced: `source ~/ros2_ws/install/setup.bash`

---

## Validation Test Suite

### Test 1: Serial Connection & Boot (5 minutes)

**Purpose**: Verify ESP32 boots and connects to WiFi

**Steps**:
```bash
# Monitor ESP32 serial output
cd ~/ros2_ws/sensor_testing
pio device monitor --baud 115200
```

**Expected Output**:
```
===== ESP32 micro-ROS Robot Controller =====
PHASES 1, 2, 3: Transport + Time Sync + Safety
Setup complete! Running main loop...
[INFO] Time sync initialized
[INFO] Publishing odom @ synchronized time
```

**Pass Criteria**:
- ✅ Shows setup complete message
- ✅ Time sync initialization succeeds
- ✅ No `ERROR:` messages

**Troubleshooting**:
| Issue | Solution |
|-------|----------|
| "Failed to connect to WiFi" | Check SSID/password in code, WiFi network reachable |
| "Setup complete" but no data | Check micro-ROS agent is running |
| "Time sync failed" | Ensure agent is running before ESP32 starts |

---

### Test 2: Message Publishing Rate (5 minutes)

**Purpose**: Verify messages publish at correct frequencies (100/50/8 Hz)

**Steps**:
```bash
# Terminal 1: Check odometry rate
ros2 topic hz /odom --window 100

# Terminal 2: Check IMU rate
ros2 topic hz /imu --window 100

# Terminal 3: Check scan rate
ros2 topic hz /scan --window 100
```

**Expected Output**:
```
Average rate: 100.12 Hz      # Odometry (should be 100 ± 5)
Average rate: 49.95 Hz       # IMU (should be 50 ± 2)
Average rate: 8.01 Hz        # Scan (should be 8 ± 1)
```

**Pass Criteria**:
- ✅ Odometry: 95-105 Hz
- ✅ IMU: 48-52 Hz
- ✅ Scan: 7-9 Hz

**Troubleshooting**:
| Rate vs Expected | Cause | Solution |
|------------------|-------|----------|
| Too low (50% of expected) | Firmware not deploying | Re-upload: `pio run --environment micro_ros_wifi --target upload` |
| Erratic/jumping | WiFi congestion | Change WiFi channel or room |
| Zero (no messages) | Agent or bridge not running | Check: `ros2 node list \| grep agent` |

---

### Test 3: Message Content & Structure (5 minutes)

**Purpose**: Verify message fields are populated correctly

**Steps**:
```bash
# Check odometry message structure
ros2 topic echo /odom --once

# Check IMU message structure
ros2 topic echo /imu --once

# Check scan message structure
ros2 topic echo /scan --once
```

**Expected Output (Odometry)**:
```
header:
  stamp:
    sec: 1711620123
    nanosec: 456789012
  frame_id: odom
child_frame_id: base_link
pose:
  pose:
    position:
      x: 0.123
      y: 0.456
      z: 0.0
    orientation:
      x: 0.0
      y: 0.0
      z: 0.123
      w: 0.992
  ...
```

**Pass Criteria**:
- ✅ Header has valid timestamp (sec and nanosec non-zero)
- ✅ Frame IDs are set: odom and base_link
- ✅ Pose position and orientation have values
- ✅ Values appear to match robot state

**Troubleshooting**:
| Issue | Solution |
|-------|----------|
| Fields are all zeros | Check motors/encoders connected and working |
| Timestamp always same | Time sync not updating - check micro-ROS agent connectivity |
| Wrong frame IDs | Check bridge is setting frame correctly |

---

### Test 4: Timestamp Validation (CRITICAL - 10 minutes)

**Purpose**: Verify Phase 2 core feature - timestamps are synchronized

**Step 1: Check timestamp is close to current time**
```bash
# Get current ROS time
ros2 topic echo /clock --once

# Get odometry timestamp
ros2 topic echo /odom --once | grep -A2 "stamp"

# Compare: Should match within ~100 ms
```

**Expected Output**:
```
Current ROS time: sec: 1711620123, nanosec: 123456789
Odom timestamp:   sec: 1711620123, nanosec: 100000000
Difference:       ~23 ms ✅
```

**Step 2: Monitor timestamp progression**
```bash
# Collect odometry timestamps for 10 seconds
ros2 topic echo /odom | grep -A1 "stamp" | head -30

# Verify they increase by ~10 ms each (100 Hz = 10 ms intervals)
```

**Expected Output**:
```
sec: 1711620123
nanosec: 100000000
--
nanosec: 110000000  # +10 ms
--
nanosec: 120000000  # +10 ms
...
```

**Pass Criteria**:
- ✅ Timestamp offset from current time < 100 ms
- ✅ Timestamps increase monotonically (never go backward)
- ✅ Intervals match message frequency (10 ms for 100 Hz)

**Troubleshooting**:
| Issue | Solution |
|-------|----------|
| Offset > 1 second | System clock on laptop not synced: `sudo ntpdate -s time.nist.gov` |
| Timestamps jump backward | Bridge still corrupting timestamps - verify bridge is updated |
| All timestamps identical | Time sync not calling update - check Phase 2 integration |

---

### Test 5: Timestamp Diagnostics (5 minutes)

**Purpose**: Verify Phase 2 diagnostics are working

**Steps**:
```bash
# Monitor diagnostics for timestamp information
ros2 topic echo /diagnostics --once | grep timestamp

# Or check with grep for clean output
ros2 topic echo /diagnostics | grep -E "timestamp_offset|watchdog"
```

**Expected Output**:
```
key: 'odom_timestamp_offset_mean_sec'
value: '0.000500'
--
key: 'odom_timestamp_offset_stddev_sec'
value: '0.000050'
--
key: 'imu_timestamp_offset_mean_sec'
value: '0.000480'
value: '0.000045'
--
key: 'scan_timestamp_offset_mean_sec'
value: '0.000520'
value: '0.000055'
```

**Pass Criteria**:
- ✅ All offset_mean values < 0.1 seconds (100 ms)
- ✅ All offset_stddev values < 0.01 seconds (10 ms)
- ✅ Watchdog shows active
- ✅ No ERROR messages in diagnostics

**What These Values Mean**:
- `offset_mean`: Average difference between message timestamp and wall-clock time
- `offset_stddev`: Standard deviation (consistency) - tight stddev = good sync
- Example: `mean=0.0005, stddev=0.00005` = **Excellent sync** (±0.5 ms typical, ±0.05 variation)

**Troubleshooting**:
| Diagnostic Issue | Solution |
|-----------------|----------|
| offset_mean > 0.5 sec | Check system clock, sync with NTP |
| offset_stddev > 0.05 sec | WiFi interference/congestion - move away or change channel |
| Missing diagnostics | Check bridge node is running: `ros2 node list` |

---

### Test 6: Time Sync Update Frequency (5 minutes)

**Purpose**: Verify time sync updates every 30 seconds as designed

**Steps**:
```bash
# Monitor serial output and count time sync updates
cd ~/ros2_ws/sensor_testing
pio device monitor --baud 115200 | grep -i "time sync"

# Let it run for 2-3 minutes (should see 4-6 messages)
```

**Expected Output** (every ~30 seconds):
```
[TIME] Time sync update: offset = +123 ms
[TIME] Time sync update: offset = +124 ms
[TIME] Time sync update: offset = +122 ms
[TIME] Time sync update: offset = +123 ms
```

**Pass Criteria**:
- ✅ Updates appear approximately every 30 seconds
- ✅ Offset stays relatively constant (±10 ms)
- ✅ No error messages

**Troubleshooting**:
| Issue | Solution |
|-------|----------|
| No updates | Check micro-ROS agent running, try restarting ESP32 |
| Offset jumping dramatically | WiFi connection unstable - check signal strength |
| Updates too frequent | Code issue - verify main_micro_ros_phase2.cpp is deployed |

---

### Test 7: Bridge Timestamp Preservation (10 minutes)

**Purpose**: Verify the critical fix - bridge preserves timestamps

**Steps**:

```bash
# Collect raw timestamps and bridge republished timestamps
# Terminal 1: Record all odometry updates for 1 sec
ros2 topic echo /odom | head -5 | grep -A2 "stamp"

# Also check the actual message came through (not modified)
ros2 topic echo /odom --once | grep -A20 "header"
```

**Expected Output**:
```
header:
  stamp:
    sec: 1711620123
    nanosec: 500000000
  frame_id: odom
child_frame_id: base_link
pose:
  pose:
    position:
      x: 0.123
      y: 0.456
```

**Pass Criteria**:
- ✅ Frame IDs are set (odom, base_link)
- ✅ Position fields have actual values (not zeros)
- ✅ Timestamp matches acquisition time (within 100 ms)

**Advanced Validation** (comparing raw vs bridged):
```bash
# If you have access to raw micro-ROS topics (before bridge relay)
# Compare timestamps to ensure bridge doesn't modify them

# This would show if bridge corrupts timestamps:
ros2 topic echo /odom_in    # Raw from agent (if available)
ros2 topic echo /odom       # Republished by bridge
# Timestamps should be identical
```

**Troubleshooting**:
| Issue | Solution |
|-------|----------|
| Timestamps different from ESP32 | Bridge is corrupting them - verify microros_robot_bridge_phase2.py deployed |
| Frame IDs missing | Bridge not setting frames - check on_odom() function |
| Position always zero | Check motor/encoder hardware |

---

## Full System Validation (20 minutes)

### Complete Test Procedure

```bash
# 1. Terminal 1: Start micro-ROS agent
ros2 run micro_ros_agent micro_ros_agent udp4 --port 8888

# 2. Terminal 2: Launch main system
source ~/ros2_ws/install/setup.bash
ros2 launch esp32_serial_bridge micro_ros_bringup.launch.py

# 3. Terminal 3: Run validation (use commands from tests above)
# Test message rates
ros2 topic hz /odom --window 50
ros2 topic hz /imu --window 50

# Test timestamps
ros2 topic echo /odom --once | grep -A2 "stamp"

# Test diagnostics
ros2 topic echo /diagnostics --once | grep timestamp
```

### Validation Report Template

After running tests, fill out:

```
Phase 2 Validation Report
========================

Date: ______________
Time: ______________

Test 1: Serial Connection
- WiFi connects: ☐ Yes ☐ No
- Time sync initializes: ☐ Yes ☐ No
- No ERROR messages: ☐ Yes ☐ No

Test 2: Message Rates
- Odometry: _____ Hz (expect 95-105)
- IMU: _____ Hz (expect 48-52)
- Scan: _____ Hz (expect 7-9)

Test 3: Message Content
- Odometry fields populated: ☐ Yes ☐ No
- IMU fields populated: ☐ Yes ☐ No
- Scan contains ranges: ☐ Yes ☐ No

Test 4: Timestamp Sync (CRITICAL)
- Offset < 100 ms: ☐ Yes ☐ No
- Monotonic increase: ☐ Yes ☐ No
- Intervals correct: ☐ Yes ☐ No

Test 5: Diagnostics
- Values present: ☐ Yes ☐ No
- Offset mean < 0.1 s: ☐ Yes ☐ No
- Offset stddev < 0.05 s: ☐ Yes ☐ No

Test 6: Time Sync Updates
- Updates every ~30 sec: ☐ Yes ☐ No
- Offset stable: ☐ Yes ☐ No

Test 7: Bridge Preservation
- Timestamps preserved: ☐ Yes ☐ No
- Frame IDs correct: ☐ Yes ☐ No

Overall Status: ☐ PASS ☐ FAIL

Notes:
_______________________
_______________________
```

---

## Success Criteria Summary

**Phase 2 is successfully deployed when:**

1. ✅ ESP32 connects and publishes at correct rates (Test 2)
2. ✅ Messages contain expected data (Test 3)
3. ✅ **Timestamps are synchronized and accurate** (Test 4) ← **Critical**
4. ✅ Diagnostics show good sync quality (Test 5)
5. ✅ Time sync updates periodically (Test 6)
6. ✅ Bridge preserves timestamps (Test 7)

If ANY of these fail, **Phase 2 is not ready for Phase 4 (EKF)**.

---

## Next Steps After Validation

### If Phase 2 PASSES:
✅ Proceed to Phase 4 deployment (EKF integration)
- Install robot_localization
- Deploy EKF configuration
- Validate filtered output

### If Phase 2 FAILS:
❌ Troubleshoot and re-validate
1. Check error messages in output
2. Refer to troubleshooting section above
3. Re-run specific failed test
4. Do NOT proceed to Phase 4 until all Phase 2 tests pass

---

## Quick Command Reference

```bash
# Check if services running
ros2 node list

# Monitor rates
ros2 topic hz /odom --window 100

# Check content
ros2 topic echo /odom --once

# Monitor diagnostics
ros2 topic echo /diagnostics

# ESP32 serial (from sensor_testing directory)
pio device monitor --baud 115200

# Check timestamps
ros2 topic echo /odom | grep -A2 stamp | head -30

# Full system launch
ros2 launch esp32_serial_bridge micro_ros_bringup.launch.py

# Micro-ROS agent
ros2 run micro_ros_agent micro_ros_agent udp4 --port 8888
```

---

**Phase 2 validation is the gate-keeper for Phase 4 (EKF) success.**

Validate thoroughly before proceeding.

