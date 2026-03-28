#!/usr/bin/env python3
"""
Simple Keyboard Teleoperation Launch File (no dependencies)
Simplified launch - just the keyboard teleoperation node
For manual testing, run: python3 -m my_robot_description.simple_teleop
"""

from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    """Generate launch description"""
    
    # For now, this is a placeholder
    # The teleop node can be run directly using:
    #   ros2 run my_robot_description simple_teleop
    # or:
    #   python3 -m my_robot_description.simple_teleop
    
    # Placeholder node  
    placeholder_node = Node(
        package='rclpy',
        executable='add_two_ints_server',  # This won't work, just placeholder
        output='screen'
    )
    
    return LaunchDescription([
        # No nodes - run teleop manually
    ])
