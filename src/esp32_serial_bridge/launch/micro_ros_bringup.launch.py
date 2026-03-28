"""
ROS 2 Launch file for micro-ROS WiFi robot bringup
PHASE 1 & 3: WiFi Transport + Safety

Starts:
  1. micro-ROS Agent (bridges ESP32 to ROS 2 over UDP)
  2. Robot Bridge Node (topic relay + watchdog)
  3. TF2 component for transforms
"""

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, LogInfo
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    
    # Declare arguments
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
        }],
        output='screen',
        remappings=[
            # Remap input topics from micro-ROS agent
            ('/odom_in', '/esp32/odom'),
            ('/imu_in', '/esp32/imu'),
            ('/scan_in', '/esp32/scan'),
            # Output topics available to ecosystem
            ('/odom', '/odom'),        # Standard odometry
            ('/imu', '/imu'),          # Standard IMU
            ('/scan', '/scan'),        # Standard laser scan
        ]
    )
    
    # TF2 static broadcaster (for reference frame)
    static_tf = Node(
        package='tf2_ros',
        executable='static_transform_publisher',
        arguments=['0', '0', '0', '0', '0', '0', 'map', 'odom'],
        output='log',
    )
    
    return LaunchDescription([
        agent_port_arg,
        watchdog_timeout_arg,
        LogInfo(msg="Launching micro-ROS WiFi robot system..."),
        agent,
        robot_bridge,
        static_tf,
    ])
