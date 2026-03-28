"""
ROS 2 Launch file for robot bringup with controller
"""

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, LogInfo
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    
    # Declare arguments
    port_arg = DeclareLaunchArgument(
        'port',
        default_value='/dev/ttyACM1',
        description='Serial port for ESP32'
    )
    
    # Robot Controller Node
    robot_controller = Node(
        package='esp32_serial_bridge',
        executable='robot_controller',
        name='robot_controller',
        parameters=[{
            'port': LaunchConfiguration('port'),
            'baud': 115200,
            'wheel_diameter': 0.065,
            'wheel_separation': 0.18,
            'max_speed': 0.5,
        }],
        output='screen',
    )
    
    # Robot State Publisher (for TF and visualization)
    robot_state_publisher = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        name='robot_state_publisher',
        parameters=[{
            'robot_description': open('/home/ros/ros2_ws/src/my_robot_description/urdf/my_robot.urdf').read(),
        }],
        output='screen',
    )
    
    return LaunchDescription([
        port_arg,
        LogInfo(msg="Starting robot controller"),
        robot_controller,
        robot_state_publisher,
    ])
