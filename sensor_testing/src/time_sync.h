/*
 * ESP32 micro-ROS Time Synchronization Module
 * PHASE 2: Synchronized timestamps between ESP32 and ROS 2
 *
 * Problem: ESP32 uses millis() (internal clock), ROS 2 uses system time
 * Solution: Fetch ROS system time, calculate offset, apply to all timestamps
 *
 * This module maintains time offset synchronization with minimal overhead.
 */

#ifndef TIME_SYNC_H
#define TIME_SYNC_H

#include <stdint.h>
#include <rcl/time.h>

// ============= TIME SYNCHRONIZATION STATE =============
typedef struct {
    int64_t offset_ns;              // Offset: ROS time - ESP32 time (nanoseconds)
    uint64_t last_sync_ms;          // Last synchronization timestamp (ESP32 millis)
    uint64_t sync_interval_ms;      // Re-synchronize every N milliseconds
    bool is_synchronized;           // Flag: synchronized with ROS time
    int sync_count;                 // Number of successful synchronizations
} TimeSync;

// Global time synchronization state
static TimeSync time_sync_state = {
    .offset_ns = 0,
    .last_sync_ms = 0,
    .sync_interval_ms = 30000,      // Sync every 30 seconds
    .is_synchronized = false,
    .sync_count = 0
};

// ============= TIME SYNCHRONIZATION FUNCTIONS =============

/**
 * Initialize time synchronization module
 * Call this in setup() after micro-ROS is initialized
 */
inline void time_sync_init() {
    time_sync_state.last_sync_ms = millis();
}

/**
 * Fetch current ROS system time and calculate offset
 * Should be called periodically (every 30 seconds recommended)
 * 
 * Returns: True if sync successful, False if failed
 */
inline bool time_sync_update(rcl_clock_t* clock) {
    if (!clock) return false;
    
    // Get ROS2 system time in nanoseconds
    int64_t ros_time_ns = rcl_clock_system_default_get_current_time_nanoseconds();
    
    if (ros_time_ns == 0) {
        return false;  // Failed to get time
    }
    
    // Get ESP32 time in milliseconds, convert to nanoseconds
    uint64_t esp32_time_ms = millis();
    int64_t esp32_time_ns = (int64_t)esp32_time_ms * 1000000LL;
    
    // Calculate offset: ROS time - ESP32 time
    // This offset is then added to ESP32 times to get ROS times
    time_sync_state.offset_ns = ros_time_ns - esp32_time_ns;
    time_sync_state.last_sync_ms = esp32_time_ms;
    time_sync_state.is_synchronized = true;
    time_sync_state.sync_count++;
    
    // Debug output
    Serial.printf("[TIME_SYNC] Synchronized #%d: offset=%lld ns (%.3f ms)\n",
                  time_sync_state.sync_count,
                  time_sync_state.offset_ns,
                  (double)time_sync_state.offset_ns / 1e6);
    
    return true;
}

/**
 * Check if time needs re-synchronization
 * Returns: True if re-sync needed, False if current offset still valid
 */
inline bool time_sync_needs_update() {
    uint64_t elapsed_ms = millis() - time_sync_state.last_sync_ms;
    return (elapsed_ms > time_sync_state.sync_interval_ms);
}

/**
 * Get synchronized timestamp for ROS messages
 * Applies offset to ESP32 millis() to get ROS system time
 * 
 * Returns: ROS system time in nanoseconds (synchronized)
 */
inline int64_t time_sync_get_timestamp_ns() {
    uint64_t esp32_time_ms = millis();
    int64_t esp32_time_ns = (int64_t)esp32_time_ms * 1000000LL;
    
    // Apply offset to get synchronized time
    int64_t synchronized_time_ns = esp32_time_ns + time_sync_state.offset_ns;
    
    return synchronized_time_ns;
}

/**
 * Convert synchronized timestamp to ROS message format (seconds + nanoseconds)
 * 
 * Returns: Struct with sec and nanosec fields
 */
inline rcl_time_point_value_t time_sync_get_timestamp() {
    int64_t sync_time_ns = time_sync_get_timestamp_ns();
    return sync_time_ns;  // rcl_time_point_value_t is int64 nanoseconds
}

/**
 * Get synchronization status
 * Returns: Number of successful synchronizations (0 if not synced)
 */
inline int time_sync_get_status() {
    return time_sync_state.sync_count;
}

/**
 * Get current offset in milliseconds (for diagnostics)
 * Returns: Offset in ms (informational only)
 */
inline double time_sync_get_offset_ms() {
    return (double)time_sync_state.offset_ns / 1e6;
}

#endif  // TIME_SYNC_H
