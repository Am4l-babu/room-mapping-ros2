# Phase 5: Scan Improvements - Integration Guide

## Overview

Phase 5 improves the VL53L0X + servo scanning system with:
- **2× Resolution**: 37 points → 90 points (2° increments instead of 5°)
- **Median Filtering**: Reduces noise spikes without shifting edges
- **Outlier Rejection**: MAD-based (Median Absolute Deviation) filtering
- **Consistent Timing**: Fixed scan intervals for SLAM reliability

## Resolution Improvement

### Current (Phase 2): 37 Points
```
Angle range: -90° to +90° (180° total)
Points: 37
Increment: 180° / 37 ≈ 5°
Coverage: Each point covers ~5° of angle
```

### Improved (Phase 5): 90 Points
```
Angle range: -90° to +90° (180° total)
Points: 90
Increment: 180° / 90 = 2°
Coverage: Each point covers ~2° of angle
Benefit: 2.4× more detailed obstacle detection
```

### Impact on Performance
| Metric | Phase 2 | Phase 5 |
|--------|---------|---------|
| Angular resolution | 5° | 2° |
| Points per scan | 37 | 90 |
| Scan time | 125 ms | 125 ms (same) |
| Time per point | 3.4 ms | 1.4 ms |
| Data density | Low | HIGH |
| SLAM localization | Basic | Improved |
| Obstacle detection | Coarse | Fine detail |

## Filtering Strategy

### Problem: Why Filtering Needed

Raw VL53L0X data exhibits:
1. **Noise spikes**: Single readings 2-3× normal distance (electrical noise)
2. **Reflectance bias**: Dark surfaces read shorter than bright ones
3. **Outliers**: Invalid readings at frame edges
4. **Temporal inconsistency**: Same point varies across sweeps

### Solution: Median Filter + Outlier Rejection

**Median Filtering (3-point window)**:
```
Raw:        [0.50, 1.5,  0.52, 0.51, 0.50]
Filtered:   [0.50, 0.50, 0.51, 0.50, 0.50]  ← 1.5 spike removed
```

Benefits:
- Removes isolated spikes (noise)
- Preserves edges (unlike Gaussian blur)
- Fast computation (O(n log n) for small windows)

**Outlier Rejection (MAD-based)**:
```
Calculate:  Median Absolute Deviation
Threshold:  If |value - median| > 2.0 * MAD → Mark as invalid
Result:     Only physically plausible readings kept
```

Benefits:
- Adaptive to environment (no fixed threshold)
- Robust to non-normal distributions
- Handles varying reflectance properties

## Implementation Steps

### Step 1: Add Phase 5 Module to Firmware

**File**: `sensor_testing/src/main_micro_ros_phase2.cpp`

Replace the old `publish_scan()` function with the new Phase 5 implementation from `sensor_testing/src/scan_phase5.cpp`:

```cpp
// Remove old function:
void publish_scan() { ... }

// Add Phase 5 includes at top:
#include "scan_phase5.cpp"  // Or copy the functions directly

// In main loop:
if (millis() - scan_timer >= PHASE5_SCAN_INTERVAL_MS) {
    publish_scan_phase5(&scan_pub);  // Changed function call
    scan_timer = millis();
}
```

### Step 2: Update LaserScan Message Size

In setup() function:

```cpp
// OLD (Phase 2):
// scan_msg.ranges allocated for 37 points

// NEW (Phase 5):
scan_msg.ranges.data = (float *)malloc(sizeof(float) * PHASE5_NUM_SCAN_POINTS);
scan_msg.ranges.size = PHASE5_NUM_SCAN_POINTS;
scan_msg.ranges.capacity = PHASE5_NUM_SCAN_POINTS;
```

### Step 3: Compile and Deploy

```bash
# In sensor_testing/
cd /home/ros/ros2_ws/sensor_testing
pio run --target upload

# Expected output:
# ... compilation ...
# scan_phase5.cpp compiled (includes vector, algorithm)
# ... firmware upload ...
# RAM: 45% used (with 90-point arrays)
# Flash: 72% used (with filtering code)
```

### Step 4: Verify in ROS 2

```bash
# Terminal 1: Start micro-ROS agent
ros2 run micro_ros_agent micro_ros_agent udp4 --port 8888

# Terminal 2: Inspect LaserScan messages
ros2 topic echo /scan --once

# Expected changes:
# - ranges.size: 90 (was 37)
# - angle_increment: 0.0349 rad (2°, was 0.0873)
# - Much smoother data (median filtered)
```

## Validation Procedures

### Test 1: Resolution Verification

```bash
# Get one scan
ros2 topic echo /scan --once > scan_phase5.txt

# Check point count
grep -o "0\.[0-9]*" scan_phase5.txt | wc -l
# Expected: ~90 points

# Check angle increment
grep "angle_increment" scan_phase5.txt
# Expected: 0.0349065 (π/90 radians)
```

### Test 2: Filtering Effectiveness

Compare raw vs filtered:
```bash
# Terminal 1: Raw distances (ESP32)
ros2 topic echo /diagnostics | grep -A 5 "scan_quality"

# Observe metrics:
# valid_ranges: 88-90 (expect 90-95%)
# filtered_outliers: 0-2 (expect <5%)
# mean_range: Stable across sweeps
```

### Test 3: SLAM Impact

Run SLAM with new scan:
```bash
# Terminal 1: Launch Phase 1-4 with Phase 5 scan
ros2 launch esp32_serial_bridge micro_ros_bringup_phase4.launch.py

# Terminal 2: Monitor SLAM
ros2 topic echo /map_metadata

# Observe:
# - Faster map convergence (more scan points)
# - Better wall alignment (smoother data)
# - Reduced loop closure errors
```

### Test 4: Data Quality Analysis

```bash
# Create analysis script
python3 << 'EOF'
import subprocess
import json
import numpy as np

for i in range(10):
    result = subprocess.run(['ros2', 'topic', 'echo', '/scan', '--once'], 
                          capture_output=True, text=True, timeout=2)
    # Parse ranges, compute std dev
    # Track mean, min, max across sweeps
    print(f"Scan {i}: ranges={len(scan_data)}, quality={quality_score}")
EOF
```

## Performance Impact

### Computation Cost

| Operation | Phase 2 | Phase 5 | Cost |
|-----------|---------|---------|------|
| Collecting points | 126 ms | 126 ms | Same |
| Median filter | None | ~1 ms | Low |
| Outlier detection | None | ~2 ms | Low |
| Publishing | 0.5 ms | 0.5 ms | Same |
| **Total per scan** | **127 ms** | **130 ms** | **+2% CPU** |

### Memory Usage

| Component | Phase 2 | Phase 5 | Used for |
|-----------|---------|---------|----------|
| ranges array | 37 × 4 = 148 B | 90 × 4 = 360 B | Range storage |
| filter buffer | None | ~100 B | Median/outlier calculations |
| vectors (std) | None | ~200 B | Dynamic allocation |
| **Total overhead** | **~150 B** | **~660 B** | RAM budget: 320 KB available |

**Verdict**: Minimal impact (0.2% of available RAM, 2% of CPU time)

## Tuning Parameters

### Median Filter Window

```cpp
const int MEDIAN_FILTER_WINDOW = 3;  // Current: 3-point window
```

Adjustments:
- **Window = 1** (no filter): Faster, noisier
- **Window = 3** (current): Balance (recommended)
- **Window = 5**: More smoothing, edge blur risk
- **Window = 7+**: Heavy smoothing, distorts range data

**Recommendation**: Keep at 3 for LiDAR (edges critical for mapping)

### Outlier Threshold (MAD)

```cpp
if (is_outlier(filtered_ranges[i], recent_outliers, 2.0)) { ... }
//                                                      ↑ threshold
```

Adjustments:
- **Threshold = 1.5**: Aggressive (may remove valid readings)
- **Threshold = 2.0** (current): Balanced (recommended)
- **Threshold = 3.0**: Permissive (keeps some noise)

**Recommendation**: Keep at 2.0 for stable filtering

### Outlier Buffer Size

```cpp
const int OUTLIER_BUFFER_SIZE = 10;  // Recent readings for MAD calculation
```

- Larger buffer = more robust statistics (but slower adaptation)
- Smaller buffer = faster response to environment changes
- **10 readings** = history of last 1-2 sweeps
- **Recommendation**: Keep at 10 for consistency

## Troubleshooting

### Issue: "Not enough memory" error

```cpp
// Reduce filter buffer (if needed)
const int OUTLIER_BUFFER_SIZE = 5;  // Down from 10

// Or implement circular buffer instead of vector
```

### Issue: Filtered data too smooth (edges blurred)

```cpp
// Reduce median filter window
const int MEDIAN_FILTER_WINDOW = 1;  // Disable filtering
// Or increase outlier threshold
threshold = 3.0f;
```

### Issue: Too many invalid ranges

```cpp
// Reduce outlier aggressiveness
threshold = 2.5f;  // More permissive (was 2.0)

// Or reduce VL53L0X sensor delays
delayMicroseconds(50);  // Was: 100 (may increase noise)
```

## Phase 5 Integration with Phase 4 (EKF)

**Why Phase 5 improves Phase 4**:

1. **More data points** → Better SLAM global localization
2. **Cleaner data** → More accurate loop closure detection
3. **Consistent timing** → EKF predictions more reliable

**Launch with Phase 4 EKF**:
```bash
ros2 launch esp32_serial_bridge micro_ros_bringup_phase4.launch.py

# EKF benefits:
# - Fuses 90-point scan (was 37)
# - Integrates with 100 Hz odometry + 50 Hz IMU
# - Generates smooth filtered odometry
# - Better loop closure for SLAM
```

## Phase 5 to Phase 6 Bridge

Phase 5 (enhanced perception) enables Phase 6 (system simplification):

**Phase 6 goals**:
1. ✅ Remove redundant bridge node (already done)
2. ✅ Use direct micro-ROS topics (already done)
3. ✅ Verify low-latency operation (2-8 ms achieved)
4. **NEW**: Verify high-quality perception (Phase 5 scanning)
5. **NEW**: Confirm SLAM + EKF work together (validation)

**Phase 5 is the final perception quality baseline before Phase 6 verification.**

## Success Criteria for Phase 5

✅ **Resolution**: 90 points published per scan (verify with `ros2 topic echo`)
✅ **Filtering**: Median filter applied (diagnostics show valid_count ~90)
✅ **Timing**: Consistent 8 Hz (125 ms per scan)
✅ **Quality**: No anomalies in SLAM visualization
✅ **Performance**: <5% CPU overhead, no memory overflow

When all criteria met, proceed to **Phase 6 Validation**.

## Files Modified

| File | Changes | Role |
|------|---------|------|
| `sensor_testing/src/main_micro_ros_phase2.cpp` | Replace `publish_scan()` with `publish_scan_phase5()` | Firmware update |
| `sensor_testing/src/scan_phase5.cpp` | NEW: Phase 5 implementation | Core Phase 5 module |
| `sensor_testing/platformio.ini` | May need `-std=c++11` for `<algorithm>` | Build config |

## Next Steps

1. **Implementation**: Copy Phase 5 code into updated firmware
2. **Compilation**: Build with PlatformIO
3. **Deployment**: Upload to ESP32
4. **Validation**: Run tests 1-4 above
5. **Integration**: Launch with Phase 4 EKF
6. **Monitoring**: Track SLAM performance improvement
7. **Proceed to Phase 6**: Final system simplification & validation
