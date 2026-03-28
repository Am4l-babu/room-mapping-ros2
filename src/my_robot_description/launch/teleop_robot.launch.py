#!/usr/bin/env python3
"""
Keyboard Teleoperation Launch File
Supports real robot + keyboard control in RViz2
"""

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
import os


def generate_launch_description():
    
    # Arguments
    use_sim_time_arg = DeclareLaunchArgument(
        'use_sim_time',
        default_value='false',
        description='Use simulation time'
    )
    
    enable_rviz = DeclareLaunchArgument(
        'enable_rviz',
        default_value='true',
        description='Enable RViz2'
    )
    
    enable_teleop = DeclareLaunchArgument(
        'enable_teleop',
        default_value='true',
        description='Enable keyboard teleoperation'
    )
    
    # Get robot description package path
    try:
        robot_desc_pkg = FindPackageShare(package='my_robot_description').find('my_robot_description')
    except:
        robot_desc_pkg = '/home/ros/ros2_ws/src/my_robot_description'
    
    # ============================================================
    # CORE ROBOT NODES (always run)
    # ============================================================
    
    # 1. Robot State Publisher (publishes TF)
    robot_state_pub = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        name='robot_state_publisher',
        output='screen',
        parameters=[{
            'use_sim_time': LaunchConfiguration('use_sim_time'),
            'robot_description': open(
                os.path.join(robot_desc_pkg, 'urdf', 'robot.urdf'),
                'r'
            ).read() if os.path.exists(os.path.join(robot_desc_pkg, 'urdf', 'robot.urdf')) else 
            '''<?xml version="1.0" ?>
<robot name="robot">
    <link name="base_link">
        <inertial>
            <mass value="5.0"/>
            <inertia ixx="0.1" ixy="0" ixz="0" iyy="0.1" iyz="0" izz="0.1"/>
        </inertial>
    </link>
    <link name="laser">
        <inertial>
            <mass value="0.5"/>
            <inertia ixx="0.01" ixy="0" ixz="0" iyy="0.01" iyz="0" izz="0.01"/>
        </inertial>
    </link>
    <joint name="laser_joint" type="fixed">
        <parent link="base_link"/>
        <child link="laser"/>
        <origin xyz="0 0 0.1" rpy="0 0 0"/>
    </joint>
</robot>''',
        }]
    )
    
    # 2. Joint State Publisher (publishes joint states) - optional for static robots
    joint_state_pub = Node(
        package='joint_state_publisher',
        executable='joint_state_publisher',
        name='joint_state_publisher',
        output='screen',
        parameters=[{
            'use_sim_time': LaunchConfiguration('use_sim_time'),
        }]
    )
    
    # 3. TF Broadcaster for ESP32 odometry
    # This transforms /odom to /base_link
    tf_broadcaster = Node(
        package='tf2_ros',
        executable='static_transform_publisher',
        arguments=['0', '0', '0', '0', '0', '0', 'map', 'odom'],
        name='static_tf_map_odom'
    )
    
    # ============================================================
    # REAL ROBOT CONTROL
    # ============================================================
    
    # 4. Motor Controller (controls ESP32 via serial)
    motor_controller = Node(
        package='esp32_serial_bridge',
        executable='esp32_bridge_node',
        name='motor_controller',
        output='screen',
        parameters=[{
            'port': '/dev/ttyUSB0',           # Adjust to your ESP32 port
            'baudrate': 115200,
            'use_sim_time': LaunchConfiguration('use_sim_time'),
        }],
        remappings=[
            ('/cmd_vel', 'cmd_vel'),
            ('/odom', 'odom'),
            ('/imu', 'imu'),
            ('/scan', 'scan'),
        ]
    )
    
    # Optional: IMU IMF (if separate IMU node needed)
    imu_node = Node(
        package='imu_complementary_filter',
        executable='imu_complementary_filter_node',
        name='imu_filter',
        output='screen',
        parameters=[{
            'use_mag': False,
            'publish_tf': True,
            'frame_in': 'imu_link',
            'frame_out': 'base_link',
        }],
        remappings=[
            ('/imu/data_raw', 'imu'),
            ('/imu/data', 'imu_data'),
        ]
    )
    
    # ============================================================
    # TELEOP - KEYBOARD CONTROL
    # ============================================================
    
    teleop_node = Node(
        package='teleop_twist_keyboard',
        executable='teleop_twist_keyboard',
        name='teleop_keyboard',
        prefix='xterm -e',  # Opens in separate terminal window
        parameters=[{
            'repeat_rate': 10.0,        # Hz
            'frame_id': 'cmd_vel',
            'stamped': False,
            'use_sim_time': LaunchConfiguration('use_sim_time'),
        }],
        remappings=[
            ('/cmd_vel', 'cmd_vel'),
        ],
        condition=IfCondition(LaunchConfiguration('enable_teleop'))
    )
    
    # ============================================================
    # VISUALIZATION - RViz2
    # ============================================================
    
    rviz_config_path = os.path.join(
        robot_desc_pkg, 'rviz', 'robot.rviz'
    )
    
    # Create default RViz config if it doesn't exist
    if not os.path.exists(rviz_config_path):
        os.makedirs(os.path.dirname(rviz_config_path), exist_ok=True)
        with open(rviz_config_path, 'w') as f:
            f.write('''Panels:
  - Class: rviz_common/Displays
    Help Height: 78
    Name: Displays
    Property Tree Widget:
      Expanded:
        - /TF1
        - /LaserScan1
      Splitter Ratio: 0.5
    Tree Height: 387
  - Class: rviz_common/Selection
    Name: Selection
  - Class: rviz_common/Tool Properties
    Expanded:
      - /2D Goal Pose1
      - /Publish Point1
    Name: Tool Properties
    Splitter Ratio: 0.588679
  - Class: rviz_common/Views
    Expanded:
      - /Current View1
    Name: Views
    Splitter Ratio: 0.5
Toolbars:
  toolbarArea: UI
Visualization Manager:
  Class: ''
  Displays:
    - Alpha: 0.5
      Cell Size: 1
      Class: rviz_common/Grid
      Color: 160; 160; 164
      Enabled: true
      Line Style:
        Line Width: 0.03
        Value: Line
      Name: Grid
      Normal Cell Count: 0
      Offset:
        X: 0
        Y: 0
      Plane: XY
      Plane Cell Count: 10
      Reference Frame: <Fixed Frame>
      Value: true
    - Class: rviz_common/TF
      Enabled: true
      Frame Timeout: 15
      Frames:
        All Enabled: true
        base_link:
          Value: true
        map:
          Value: true
        odom:
          Value: true
      Marker Scale: 1
      Name: TF
      Show Arrows: true
      Show Axes: true
      Value: true
    - Alpha: 1
      Color Transformer: Z Axis
      Class: rviz_common/LaserScan
      Decay Time: 0
      Enabled: true
      Invert Rainbow: false
      Max Color: 255; 255; 255
      Min Color: 0; 0; 0
      Name: LaserScan
      Position Transformer: XYZ
      Selectable: true
      Size (Pixels): 3
      Size (m): 0.05
      Style: Points
      Topic:
        Depth: 5
        Durability Policy: Volatile
        History Policy: Keep Last
        Reliability Policy: Best Effort
        Value: /scan
      Use Fixed Frame: true
      Use rainbow: true
      Value: true
    - Alpha: 0.7
      Class: rviz_common/Map
      Color Scheme: map
      Draw Behind: false
      Enabled: false
      Name: Map
      Topic:
        Depth: 5
        Durability Policy: Transient Local
        History Policy: Keep Last
        Reliability Policy: Reliable
        Value: /map
      Utility Property:
        Alpha: 1
      Value: false
    - Angle Tolerance: 0.05
      Class: rviz_common/Odometry
      Covariance:
        Orientation:
          Alpha: 0.5
          Color: 255; 255; 127
          Color Style: RGB
          Frame: Local
          Offset: 1
          Scale: 1
          Value: true
        Position:
          Alpha: 0.3
          Color: 204; 51; 204
          Scale: 1
          Value: true
        Value: true
      Enabled: true
      Keep (0): 0
      Name: Odometry
      Position Tolerance: 0.1
      Topic:
        Depth: 5
        Durability Policy: Volatile
        History Policy: Keep Last
        Reliability Policy: Best Effort
        Value: /odom
      Value: true
  Enabled: true
  Global Options:
    Background Color: 48; 48; 48
    Fixed Frame: map
    Frame Rate: 30
  Name: root
  Tools:
    - Class: rviz_common/Interact
      Hide Inactive Objects: true
    - Class: rviz_common/MoveCamera
    - Class: rviz_common/Select
    - Class: rviz_common/FocusCamera
    - Class: rviz_common/Measure
      Class Name: rviz_common/Measure
    - Class: rviz_rendering/SelectionTool
      Class Name: rviz_rendering/SelectionTool
    - Autostarting: false
      Class: rviz_common/PublishPoint
      Class Name: rviz_common/PublishPoint
      Invert Z Axis: false
      Topic: clicked_point
  Value: true
  Views:
    Current:
      Angle: 0
      Class: rviz_common/TopDownOrtho
      Enable Stereo Rendering:
        Stereo Eye Separation: 0.06
        Stereo Focal Distance: 1
        Swap Stereo Eyes: false
        Value: false
      Invert Z Axis: false
      Name: Current View
      Near Clip Distance: 0.01
      Scale: 50
      Target Frame: <Fixed Frame>
      X: 0
      Y: 0
    Saved Views: {}
  Wigets: {}
Window Geometry:
  Displays:
    collapsed: false
    height: 846
    width: 1456
  Height: 1016
  Hide Left Dock: false
  Hide Right Dock: false
  QMainWindow State: 000000ff00000000fd000000040000000000000283000003dcfc020000000afb0000001200530065006c0065006300740069006f006e00000000000000011d0000005c00fffffffb0000001e0054006f006f006c002000500072006f0070006500720074006900650073020000011d000001e90000011d00fffffffb0000000a00560069006500770073000000011d000002760000010d00fffffffb00000012004600690065006c0064004f0066005600690065007700000025a0000001200000000000fffffffb0000001200530065006c0065006300740069006f006e010000011d0000005c0000000000fffffffb000000120044006900730070006c006100790073000000011d000002320000010d00fffffffb0000002000450078007000610063007400200054006f006f006c0043006f006e00660069006700000000000000011d0000011d00fffffffc0000000300000005ae00000108fc0100000001fb0000000c0041006c0069006700680074000000000000010800000000000000000000000e3000003dc00000001000f00000298fc020000000afb0000001200530065006c0065006300740069006f006e00000000000000011d0000005c00fffffffb0000001e0054006f006f006c002000500072006f0070006500720074006900650073020000011d000001e90000011d00fffffffb0000000a00560069006500770073000000011d000002760000010d00fffffffb00000012004600690065006c0064004f0066005600690065007700000025a0000001200000000000fffffffb0000001200530065006c0065006300740069006f006e010000011d0000005c0000000000fffffffb000000120044006900730070006c006100790073000000011d000002320000010d00fffffffb0000002000450078007000610063007400200054006f006f006c0043006f006e00660069006700000000000000011d0000011d00fffffffb0000001800500075006200690073006800500069006e0074000000011d0000013b0000000000fffffffb00000014005600690065007700200043006f006e00740072006f006c0073000000011d0000013b00000000000000000000008d000003dc0000000000000004000000040000000800000008fc00000001000000020000000bfb0000001200530065006c0065006300740069006f006e00000000000000011d0000005c00fffffffb0000001e0054006f006f006c002000500072006f0070006500720074006900650073000000011d000001e90000011d00fffffffb0000000a00560069006500770073010000011d000002760000010d00fffffffb00000012004600690065006c0064004f0066005600690065007700000025a0000001200000000000fffffffb0000001200530065006c0065006300740069006f006e040000011d0000005c0000000000fffffffb000000120044006900730070006c006100790073000000011d000002320000010d00fffffffb0000002000450078007000610063007400200054006f006f006c0043006f006e00660069006700000000000000011d0000011d00fffffffc0000000300000005ae00000108fc0100000001fb0000000c0041006c0069006700680074000000000000010800000000000000000000000e3000003dc00000000000000
Selection:
  collapsed: false
Tool Properties:
  collapsed: false
Views:
  collapsed: true
Width: 1456
Height: 1016
''')
    
    rviz_node = Node(
        package='rviz2',
        executable='rviz2',
        name='rviz2',
        arguments=['-d', rviz_config_path],
        parameters=[{
            'use_sim_time': LaunchConfiguration('use_sim_time'),
        }],
        output='screen',
        condition=IfCondition(LaunchConfiguration('enable_rviz'))
    )
    
    # ============================================================
    # LAUNCH DESCRIPTION
    # ============================================================
    
    return LaunchDescription([
        use_sim_time_arg,
        enable_rviz,
        enable_teleop,
        
        # Core robot nodes
        robot_state_pub,
        joint_state_pub,
        tf_broadcaster,
        
        # Real robot control
        motor_controller,
        # imu_node,  # Uncomment if needed
        
        # Teleop
        teleop_node,
        
        # Visualization
        rviz_node,
    ])
