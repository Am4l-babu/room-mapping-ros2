"""
ROS 2 Launch file for SLAM with slam_toolbox
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
    
    slam_params_file = DeclareLaunchArgument(
        'slam_params_file',
        default_value=os.path.join(
            FindPackageShare('esp32_serial_bridge').find('esp32_serial_bridge'),
            'config',
            'mapper_params_online_async.yaml'
        ),
        description='Full path to the ROS2 parameters file to use for the slam_toolbox node'
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
    
    # SLAM Toolbox Node
    slam_toolbox = Node(
        parameters=[
            LaunchConfiguration('slam_params_file'),
            {'use_sim_time': LaunchConfiguration('use_sim_time')}
        ],
        package='slam_toolbox',
        executable='async_slam_toolbox_node',
        name='slam_toolbox',
        output='screen'
    )
    
    # RViz for visualization
    rviz_config_file = os.path.join(
        FindPackageShare('esp32_serial_bridge').find('esp32_serial_bridge'),
        'config',
        'slam.rviz'
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
        slam_params_file,
        LogInfo(msg="Starting SLAM with slam_toolbox"),
        bringup_launch,
        slam_toolbox,
        rviz_node,
    ])
