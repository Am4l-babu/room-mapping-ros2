/*
 * PHASE 5: SCAN IMPROVEMENTS
 * Enhanced LiDAR scanning with higher resolution, filtering, and consistency
 * 
 * Improvements over Phase 2:
 * 1. Resolution: 37 points → 90 points (2× better detail)
 * 2. Filtering: Median filter (window 3) reduces noise spikes
 * 3. Outlier rejection: Invalid ranges (< min or > max) excluded
 * 4. Timing consistency: Fixed sweep timing for SLAM
 * 
 * Compile with Phase 2 firmware as base, then apply these changes.
 */

#include <Arduino.h>
#include <vector>
#include <algorithm>

// ============= PHASE 5 CONFIGURATION =============

// Resolution improvement (Phase 5)
const int PHASE5_NUM_SCAN_POINTS = 90;      // Was: 37 points
const float PHASE5_ANGLE_INCREMENT = M_PI / PHASE5_NUM_SCAN_POINTS;  // 2°, was 5°

// Filtering parameters (Phase 5)
const int MEDIAN_FILTER_WINDOW = 3;         // Median filtering for spike rejection
const int OUTLIER_BUFFER_SIZE = 10;         // Buffer for outlier detection

// Timing (Phase 5)
const unsigned long PHASE5_SCAN_INTERVAL_MS = 125;  // Still 8 Hz (same)

// ============= MEDIAN FILTER HELPER =============

/*
 * Apply median filter to range array
 * Reduces noise spikes without shifting edges
 */
float apply_median_filter(const std::vector<float>& ranges, int window_size) {
    if (ranges.empty()) return INFINITY;
    
    int half_window = window_size / 2;
    std::vector<float> window_values;
    
    // Collect window around current index
    for (int j = -half_window; j <= half_window; j++) {
        int idx = ranges.size() / 2 + j;  // Center window
        if (idx >= 0 && idx < (int)ranges.size()) {
            if (!std::isinf(ranges[idx])) {
                window_values.push_back(ranges[idx]);
            }
        }
    }
    
    if (window_values.empty()) return INFINITY;
    
    // Sort and return median
    std::sort(window_values.begin(), window_values.end());
    return window_values[window_values.size() / 2];
}

/*
 * Detect outliers using median absolute deviation (MAD)
 * More robust than standard deviation for non-normal distributions
 */
bool is_outlier(float value, const std::vector<float>& recent_values, float threshold = 1.5) {
    if (recent_values.empty()) return false;
    
    // Calculate median of recent values
    std::vector<float> sorted_values = recent_values;
    std::sort(sorted_values.begin(), sorted_values.end());
    float median = sorted_values[sorted_values.size() / 2];
    
    // Calculate MAD (median absolute deviation)
    std::vector<float> deviations;
    for (float v : recent_values) {
        deviations.push_back(fabsf(v - median));
    }
    std::sort(deviations.begin(), deviations.end());
    float mad = deviations[deviations.size() / 2];
    
    // Outlier if deviation > threshold * MAD
    float deviation = fabsf(value - median);
    return deviation > (threshold * mad);
}

// ============= PHASE 5 SCAN PUBLISHING =============

/*
 * Enhanced scan publishing with:
 * - 90-point resolution
 * - Median filtering
 * - Outlier rejection
 * - Consistent timing
 */
void publish_scan_phase5(rcl_publisher_t* scan_pub) {
    rcl_time_point_value_t now = time_sync_get_timestamp();  // Phase 2: synchronized time
    
    sensor_msgs__msg__LaserScan scan_msg;
    memset(&scan_msg, 0, sizeof(scan_msg));
    
    // Header with synchronized timestamp (Phase 2)
    scan_msg.header.stamp.sec = now / 1000000000;
    scan_msg.header.stamp.nanosec = now % 1000000000;
    scan_msg.header.frame_id = "base_scan";
    
    // Phase 5: Improved parameters
    scan_msg.angle_min = -M_PI / 2.0;       // -90°
    scan_msg.angle_max = M_PI / 2.0;        // +90° range
    scan_msg.angle_increment = PHASE5_ANGLE_INCREMENT;  // 2° increments
    scan_msg.time_increment = 0.0005;       // 0.5 ms per point (tight timing)
    scan_msg.scan_time = 0.125;             // 8 Hz → 125 ms per scan
    scan_msg.range_min = 0.05;              // Avoid noise floor
    scan_msg.range_max = 2.0;               // Realistic max range
    
    // Pre-allocate ranges array
    std::vector<float> ranges(PHASE5_NUM_SCAN_POINTS);
    std::vector<float> recent_outliers;     // For outlier detection
    
    // Collect raw measurements
    uint32_t valid_count = 0;
    for (int i = 0; i < PHASE5_NUM_SCAN_POINTS; i++) {
        // Read distance (milliseconds to meters)
        uint16_t raw_distance_mm = distance_sensor.readRangeContinuousMillimeters();
        float range_m = raw_distance_mm / 1000.0;
        
        // Phase 5: Outlier rejection (basic)
        if (range_m < scan_msg.range_min || range_m > scan_msg.range_max) {
            ranges[i] = INFINITY;  // Invalid
        } else {
            ranges[i] = range_m;
            valid_count++;
            
            // Track for outlier detection
            if (recent_outliers.size() < OUTLIER_BUFFER_SIZE) {
                recent_outliers.push_back(range_m);
            } else {
                recent_outliers[valid_count % OUTLIER_BUFFER_SIZE] = range_m;
            }
        }
        
        // Small delay for consistent timing across sweep
        delayMicroseconds(100);  // ~0.5 ms consistent timing
    }
    
    // Phase 5: Apply median filtering to reduce spikes
    std::vector<float> filtered_ranges(PHASE5_NUM_SCAN_POINTS);
    for (int i = 0; i < PHASE5_NUM_SCAN_POINTS; i++) {
        if (std::isinf(ranges[i])) {
            filtered_ranges[i] = INFINITY;  // Keep invalid as invalid
        } else {
            // Apply median filter (window 3)
            std::vector<float> window;
            const int FILTER_HALF = 1;
            
            for (int j = -FILTER_HALF; j <= FILTER_HALF; j++) {
                int idx = i + j;
                if (idx >= 0 && idx < PHASE5_NUM_SCAN_POINTS && !std::isinf(ranges[idx])) {
                    window.push_back(ranges[idx]);
                }
            }
            
            if (!window.empty()) {
                std::sort(window.begin(), window.end());
                filtered_ranges[i] = window[window.size() / 2];  // Median
            } else {
                filtered_ranges[i] = ranges[i];
            }
            
            // Phase 5: Outlier rejection (MAD-based)
            if (is_outlier(filtered_ranges[i], recent_outliers, 2.0)) {
                filtered_ranges[i] = INFINITY;  // Mark as invalid
            }
        }
    }
    
    // Copy filtered ranges to message
    // Note: message format supports dynamic-size array
    // In production, ensure LaserScan message is properly sized
    for (int i = 0; i < PHASE5_NUM_SCAN_POINTS; i++) {
        scan_msg.ranges[i] = filtered_ranges[i];
    }
    scan_msg.intensities.size = 0;  // No intensity data from single-value VL53L0X
    
    // Publish with synchronized timestamp
    rcl_publish(scan_pub, &scan_msg, NULL);
}

/*
 * INTEGRATION INSTRUCTIONS:
 * 
 * This Phase 5 function replaces the Phase 2 publish_scan() in main_micro_ros_phase2.cpp
 * 
 * In main loop, replace:
 *   if (millis() - scan_timer >= SCAN_INTERVAL_MS) {
 *       publish_scan();
 *       scan_timer = millis();
 *   }
 * 
 * With:
 *   if (millis() - scan_timer >= PHASE5_SCAN_INTERVAL_MS) {
 *       publish_scan_phase5(&scan_pub);
 *       scan_timer = millis();
 *   }
 * 
 * Also update message size in setup if needed:
 *   // Ensure LaserScan message can hold 90 points
 *   scan_msg.ranges.size = PHASE5_NUM_SCAN_POINTS;
 */

// ============= PHASE 5 DIAGNOSTICS =============

/*
 * Report scan quality metrics
 * Call periodically to monitor filtering effectiveness
 */
struct ScanMetrics {
    uint32_t total_points;
    uint32_t valid_ranges;
    uint32_t filtered_outliers;
    uint32_t invalid_ranges;
    float mean_range;
    float max_range;
    float min_range;
};

ScanMetrics analyze_scan_quality(const std::vector<float>& ranges) {
    ScanMetrics metrics = {0};
    metrics.total_points = ranges.size();
    
    std::vector<float> valid_ranges_list;
    
    for (float range : ranges) {
        if (std::isinf(range)) {
            metrics.invalid_ranges++;
        } else {
            metrics.valid_ranges++;
            valid_ranges_list.push_back(range);
            
            if (metrics.min_range == 0 || range < metrics.min_range) {
                metrics.min_range = range;
            }
            if (range > metrics.max_range) {
                metrics.max_range = range;
            }
        }
    }
    
    if (!valid_ranges_list.empty()) {
        float sum = 0;
        for (float r : valid_ranges_list) sum += r;
        metrics.mean_range = sum / valid_ranges_list.size();
    }
    
    metrics.filtered_outliers = metrics.total_points - metrics.valid_ranges;
    
    return metrics;
}

void print_scan_diagnostics(const ScanMetrics& metrics) {
    Serial.printf("[SCAN] Resolution: %lu points | Valid: %lu | Filtered: %lu | Invalid: %lu\n",
        metrics.total_points, metrics.valid_ranges, metrics.filtered_outliers, metrics.invalid_ranges);
    Serial.printf("[SCAN] Range: %.2f-%.2f m | Mean: %.2f m\n",
        metrics.min_range, metrics.max_range, metrics.mean_range);
}
