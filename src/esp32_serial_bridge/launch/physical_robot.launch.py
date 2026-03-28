from pathlib import Path

from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    urdf_file = Path('/home/ros/ros2_ws/src/my_robot_description/urdf/my_robot.urdf')
    if not urdf_file.exists():
        raise FileNotFoundError(f'URDF file not found: {urdf_file}')

    robot_description_config = urdf_file.read_text()

    robot_state_publisher_node = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        name='robot_state_publisher',
        output='screen',
        parameters=[
            {'robot_description': robot_description_config},
        ],
    )

    # If your robot has joints and a joint_state_publisher is required, uncomment this node.
    # joint_state_publisher_node = Node(
    #     package='joint_state_publisher',
    #     executable='joint_state_publisher',
    #     name='joint_state_publisher',
    #     output='screen',
    # )

    bridge_node = Node(
        package='esp32_serial_bridge',
        executable='bridge_node',
        name='esp32_serial_bridge',
        output='screen',
    )

    slam_toolbox_node = Node(
        package='slam_toolbox',
        executable='sync_slam_toolbox_node',
        name='slam_toolbox',
        output='screen',
        parameters=[
            {'use_sim_time': False},
            {'map_file_name': ''},
        ],
    )

    rviz_node = Node(
        package='rviz2',
        executable='rviz2',
        name='rviz2',
        output='screen',
        arguments=['-d', str(Path('/home/ros/ros2_ws/src/my_robot_description/urdf/my_robot.rviz'))] if Path('/home/ros/ros2_ws/src/my_robot_description/urdf/my_robot.rviz').exists() else [],
    )

    return LaunchDescription([
        robot_state_publisher_node,
        bridge_node,
        slam_toolbox_node,
        rviz_node,
    ])
