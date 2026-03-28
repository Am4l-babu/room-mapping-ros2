"""
ROS 2 Launch file for ESP32 + micro-ROS WiFi Robot System
PHASES 1-4: WiFi Transport + Safety + Time Sync + EKF Fusion

Starts:
  1. micro-ROS Agent (bridges ESP32 to ROS 2 over UDP)
  2. Robot Bridge Node (topic relay + watchdog)
  3. EKF Node (sensor fusion: encoder + IMU)
  4. TF transforms (map → odom → base_link)

Key Components:
  - Phase 1: WiFi connectivity + non-blocking timers
  - Phase 2: Time synchronization (ESP32 ↔ ROS clock)
  - Phase 3: Motor safety watchdog (500 ms timeout)
  - Phase 4: EKF sensor fusion (fused odometry)
"""

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, LogInfo, IncludeLaunchDescription
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    
    # ============================================================
    # LAUNCH ARGUMENTS
    # ============================================================
    
    agent_port_arg = DeclareLaunchArgument(
        'agent_port',
        default_value='8888',
        description='UDP port for micro-ROS agent'
    )
    
    watchdog_timeout_arg = DeclareLaunchArgument(
        'watchdog_timeout_ms',
        default_value='500',
        description='Command velocity watchdog timeout in milliseconds'
    )
    
    enable_ekf_arg = DeclareLaunchArgument(
        'enable_ekf',
        default_value='true',
        description='Enable EKF sensor fusion (Phase 4)'
    )
    
    # ============================================================
    # CORE NODES
    # ============================================================
    
    # Micro-ROS Agent (bridges UDP to DDS)
    agent = Node(
        package='micro_ros_agent',
        executable='micro_ros_agent',
        name='micro_ros_agent',
        arguments=[
            'udp4',
            '-p', LaunchConfiguration('agent_port'),
            '-v6'  # Verbose debug output
        ],
        output='screen',
    )
    
    # Robot Bridge Node (safety + TF)
    robot_bridge = Node(
        package='esp32_serial_bridge',
        executable='micro_ros_robot_bridge',
        name='robot_bridge',
        parameters=[{
            'enable_diagnostics': True,
            'watchdog_timeout_ms': LaunchConfiguration('watchdog_timeout_ms'),
            'enable_timestamp_diagnostics': True,  # Phase 2 feature
        }],
        output='screen',
        remappings=[
            # Input topics from micro-ROS agent
            ('/odom_in', '/esp32/odom'),
            ('/imu_in', '/esp32/imu'),
            ('/scan_in', '/esp32/scan'),
            # Output topics available to ROS ecosystem
            ('/odom', '/odom'),        # Standard odometry
            ('/imu', '/imu'),          # Standard IMU
            ('/scan', '/scan'),        # Standard laser scan
        ]
    )
    
    # ============================================================
    # TF2 TRANSFORMS
    # ============================================================
    
    # Static TF: map → odom (identity initially)
    static_tf_map_odom = Node(
        package='tf2_ros',
        executable='static_transform_publisher',
        name='static_tf_map_odom',
        arguments=['0', '0', '0', '0', '0', '0', 'map', 'odom'],
        output='log',
    )
    
    # ============================================================
    # PHASE 4: EKF SENSOR FUSION
    # ============================================================
    # Extended Kalman Filter fuses encoder odometry + IMU
    # Input: /odom (100 Hz) + /imu (50 Hz)
    # Output: /odometry/filtered (fused, smoother trajectory)
    
    esp32_bridge_share = FindPackageShare('esp32_serial_bridge')
    ekf_config_path = PathJoinSubstitution(
        [esp32_bridge_share, 'config', 'ekf_phase4.yaml']
    )
    
    ekf_node = Node(
        package='robot_localization',
        executable='ekf_node',
        name='ekf_filter_node',
        output='screen',
        parameters=[ekf_config_path],
        remappings=[
            # Input topics (direct from agent, Phase 2 synchronized)
            ('odom', 'odom'),           # Encoder-based odometry (100 Hz)
            ('imu', 'imu'),             # MPU6050 IMU (50 Hz)
            
            # Output topics for navigation stack
            ('odometry/filtered', 'odometry/filtered'),  # Fused odometry
            ('accel/filtered', 'accel/filtered'),        # Fused acceleration
        ],
    )
    
    # Static TF: odom → base_link (identity, EKF updates this dynamically)
    static_tf_odom_base = Node(
        package='tf2_ros',
        executable='static_transform_publisher',
        name='static_tf_odom_base',
        arguments=['0', '0', '0', '0', '0', '0', 'odom', 'base_link'],
        output='log',
    )
    
    # ============================================================
    # BUILD LAUNCH DESCRIPTION
    # ============================================================
    
    ld = LaunchDescription()
    
    # Add launch arguments
    ld.add_action(agent_port_arg)
    ld.add_action(watchdog_timeout_arg)
    ld.add_action(enable_ekf_arg)
    
    # Add info message
    ld.add_action(LogInfo(msg="[ESP32 Robot Launch] Starting micro-ROS system with EKF fusion..."))
    
    # Add core nodes
    ld.add_action(agent)              # Phase 1: WiFi transport
    ld.add_action(robot_bridge)       # Phase 3: Safety + Phase 2 time sync
    
    # Add transforms
    ld.add_action(static_tf_map_odom)
    ld.add_action(static_tf_odom_base)
    
    # Add EKF node (Phase 4)
    ld.add_action(ekf_node)
    
    return ld


if __name__ == '__main__':
    generate_launch_description()
