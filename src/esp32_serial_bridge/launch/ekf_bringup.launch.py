#!/usr/bin/env python3
"""
Phase 4: EKF Sensor Fusion Integration
Launch file for robot_localization Extended Kalman Filter

This EKF node fuses:
  - Odometry (/odom): Encoder-based position from ESP32
  - IMU (/imu): Orientation + angular velocity from MPU6050

Output:
  - /odometry/filtered: Fused odometry with improved accuracy
  - /pose/filtered: Filtered pose (position + orientation)
  - TF: odom → base_link (updated by EKF)

Key improvements over raw odometry:
  - Smoother trajectory (reduces noise)
  - Corrects odometry drift (encoders accumulate error)
  - Combines encoder precision with IMU orientation
  - Provides velocity estimates
"""

import os
from launch import LaunchDescription
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
from launch.substitutions import PathJoinSubstitution


def generate_launch_description():
    # Get package share directory
    esp32_bridge_share = FindPackageShare('esp32_serial_bridge')
    
    # Path to EKF configuration
    ekf_config_dir = PathJoinSubstitution(
        [esp32_bridge_share, 'config']
    )
    ekf_config_file = PathJoinSubstitution(
        [ekf_config_dir, 'ekf_phase4.yaml']
    )
    
    # ============================================================
    # EKF NODE - Main sensor fusion processor
    # ============================================================
    ekf_node = Node(
        package='robot_localization',
        executable='ekf_node',
        name='ekf_filter_node',
        output='screen',
        parameters=[ekf_config_file],
        remappings=[
            # Input topics (don't rename - use direct agent topics)
            ('odom', 'odom'),           # From ESP32 encoders
            ('imu', 'imu'),             # From ESP32 IMU
            
            # Output topics
            ('odometry/filtered', 'odometry/filtered'),  # Fused odometry
            ('accel/filtered', 'accel/filtered'),        # Fused acceleration
            ('pose/filtered', 'pose/filtered'),          # Filtered pose
        ],
        # Allow some time for the node to initialize
        on_exit_shutdown=False,
    )
    
    # ============================================================
    # POSE PUBLISHER (Optional) - For visualization
    # ============================================================
    # If you want a /pose topic in addition to /pose/filtered
    pose_pub_node = Node(
        package='robot_localization',
        executable='ekf_node',
        name='ekf_localization_node',
        output='screen',
        parameters=[ekf_config_file],
        remappings=[
            ('odometry/filtered', 'odometry/filtered'),
            ('pose/filtered', 'pose/filtered'),
        ],
        # Condition: Set once_on_set=true if you only want poses on updates
    )
    
    # ============================================================
    # STATIC TF BROADCASTER - map → odom transform
    # ============================================================
    # This is typically set once at startup
    # (map and odom frames are initially the same)
    static_tf_node = Node(
        package='tf2_ros',
        executable='static_transform_publisher',
        name='static_tf_publisher',
        output='screen',
        arguments=[
            # Translation: None (frames are colocated initially)
            '0', '0', '0',
            # Rotation: Identity quaternion (no rotation)
            '0', '0', '0', '1',
            # Frames
            'map', 'odom'
        ]
    )
    
    # ============================================================
    # RETURN LAUNCH DESCRIPTION
    # ============================================================
    ld = LaunchDescription()
    
    # Add nodes to launch
    ld.add_action(ekf_node)
    ld.add_action(static_tf_node)
    
    return ld


if __name__ == '__main__':
    ld = generate_launch_description()
    print(f"EKF Launch Configuration:\n{ld}")
