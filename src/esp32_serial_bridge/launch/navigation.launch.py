"""
ROS 2 Launch file for navigation with Nav2
"""

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, LogInfo, IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
import os


def generate_launch_description():
    
    # Declare arguments
    port_arg = DeclareLaunchArgument(
        'port',
        default_value='/dev/ttyACM1',
        description='Serial port for ESP32'
    )
    
    use_sim_time = DeclareLaunchArgument(
        'use_sim_time',
        default_value='false',
        description='Use simulation time'
    )
    
    # Robot Bringup
    bringup_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            PathJoinSubstitution([
                FindPackageShare('esp32_serial_bridge'),
                'launch',
                'bringup.launch.py'
            ])
        ),
        launch_arguments={
            'port': LaunchConfiguration('port'),
        }.items()
    )
    
    # RViz for navigation
    rviz_config_file = os.path.join(
        FindPackageShare('esp32_serial_bridge').find('esp32_serial_bridge'),
        'config',
        'navigation.rviz'
    )
    
    rviz_node = Node(
        package='rviz2',
        executable='rviz2',
        name='rviz2',
        arguments=['-d', rviz_config_file],
        output='screen'
    )
    
    return LaunchDescription([
        port_arg,
        use_sim_time,
        LogInfo(msg="Starting robot with navigation setup"),
        LogInfo(msg="Note: Load a map or run SLAM first before navigation"),
        bringup_launch,
        rviz_node,
    ])
