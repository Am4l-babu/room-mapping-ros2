"""
ESP32 Robot Controller Node for ROS 2
Handles motor control and sensor interfacing
"""

import rclpy
from rclpy.node import Node
import serial
import math
import threading
import time
from collections import deque

from sensor_msgs.msg import LaserScan, Imu
from nav_msgs.msg import Odometry
from geometry_msgs.msg import Twist, TransformStamped, Quaternion, Vector3
from tf2_ros import TransformBroadcaster
from tf_transformations import quaternion_from_euler


class RobotController(Node):
    def __init__(self):
        super().__init__('robot_controller')
        
        # Robot parameters
        self.declare_parameter('port', '/dev/ttyACM1')
        self.declare_parameter('baud', 115200)
        self.declare_parameter('wheel_diameter', 0.065)  # meters
        self.declare_parameter('wheel_separation', 0.18)  # meters
        self.declare_parameter('max_speed', 0.5)  # m/s
        
        port = self.get_parameter('port').value
        baud = self.get_parameter('baud').value
        self.wheel_diameter = self.get_parameter('wheel_diameter').value
        self.wheel_separation = self.get_parameter('wheel_separation').value
        self.max_speed = self.get_parameter('max_speed').value
        
        # Serial connection
        try:
            self.ser = serial.Serial(port, baud, timeout=0.1)
            self.get_logger().info(f'Connected to ESP32 on {port} @ {baud}')
            time.sleep(2)  # Wait for ESP32 boot
            # Clear buffer
            self.ser.reset_input_buffer()
        except Exception as e:
            self.get_logger().error(f'Failed to connect to Serial: {e}')
            raise SystemExit
        
        # Publishers
        self.odom_pub = self.create_publisher(Odometry, 'odom', 10)
        self.scan_pub = self.create_publisher(LaserScan, 'scan', 10)
        self.imu_pub = self.create_publisher(Imu, 'imu', 10)
        self.tf_broadcaster = TransformBroadcaster(self)
        
        # Subscribers
        self.cmd_sub = self.create_subscription(Twist, 'cmd_vel', self.cmd_vel_callback, 10)
        
        # Odometry tracking
        self.x = 0.0
        self.y = 0.0
        self.theta = 0.0
        self.last_encoder_m1 = 0
        self.last_encoder_m2 = 0
        self.encoder_history = deque(maxlen=100)
        
        # Laser scan data
        self.scan_ranges = [float('inf')] * 37
        self.last_angle = 0
        self.current_sweep_dir = 1
        
        # IMU data
        self.last_accel = Vector3(x=0.0, y=0.0, z=9.81)
        self.last_gyro = Vector3(x=0.0, y=0.0, z=0.0)
        
        # Serial reading thread
        self.read_thread = threading.Thread(target=self.serial_read_loop, daemon=True)
        self.read_thread.start()
        
        self.get_logger().info('Robot Controller initialized')
    
    def cmd_vel_callback(self, msg):
        """Convert cmd_vel to motor commands"""
        try:
            linear_x = msg.linear.x
            angular_z = msg.angular.z
            
            # Differential drive kinematics
            v_left = linear_x - (angular_z * self.wheel_separation / 2.0)
            v_right = linear_x + (angular_z * self.wheel_separation / 2.0)
            
            # Convert to PWM (-255 to 255)
            pwm_left = int(max(min((v_left / self.max_speed) * 255, 255), -255))
            pwm_right = int(max(min((v_right / self.max_speed) * 255, 255), -255))
            
            # Send to ESP32
            # We'll send custom format that ESP32 can parse
            command = f'{pwm_left},{pwm_right},{int(pwm_left > 0)},{int(pwm_right > 0)}\n'
            self.ser.write(command.encode('utf-8'))
            
        except Exception as e:
            self.get_logger().warn(f'Error sending command: {e}')
    
    def serial_read_loop(self):
        """Read data from ESP32 serial port"""
        buffer = ""
        while rclpy.ok():
            try:
                if self.ser.in_waiting > 0:
                    data = self.ser.read(self.ser.in_waiting).decode('utf-8', errors='ignore')
                    buffer += data
                    
                    # Process complete lines
                    while '\n' in buffer:
                        line, buffer = buffer.split('\n', 1)
                        line = line.strip()
                        
                        if line.startswith('RPM:'):
                            self.process_rpm_data(line)
                        elif line.startswith('SCAN:'):
                            self.process_scan_data(line)
                        elif line.startswith('IMU:'):
                            self.process_imu_data(line)
                
                time.sleep(0.01)
            except Exception as e:
                self.get_logger().debug(f'Serial error: {e}')
    
    def process_rpm_data(self, data):
        """Process: RPM:m1_rpm,m2_rpm,m1_pulses,m2_pulses"""
        try:
            parts = data.replace('RPM:', '').split(',')
            if len(parts) < 4:
                return
            
            m1_rpm = float(parts[0])
            m2_rpm = float(parts[1])
            m1_pulses = int(parts[2])
            m2_pulses = int(parts[3])
            
            # Calculate distances
            encoder_slots = 20
            wheel_circumference = math.pi * self.wheel_diameter
            
            # Distance per pulse
            dist_per_pulse = wheel_circumference / encoder_slots
            
            # Calculate delta distances
            delta_m1 = (m1_pulses - self.last_encoder_m1) * dist_per_pulse
            delta_m2 = (m2_pulses - self.last_encoder_m2) * dist_per_pulse
            
            self.last_encoder_m1 = m1_pulses
            self.last_encoder_m2 = m2_pulses
            
            # Update odometry
            delta_dist = (delta_m1 + delta_m2) / 2.0
            delta_theta = (delta_m2 - delta_m1) / self.wheel_separation
            
            self.theta += delta_theta
            self.x += delta_dist * math.cos(self.theta)
            self.y += delta_dist * math.sin(self.theta)
            
            # Publish odometry
            self.publish_odometry()
            
        except Exception as e:
            self.get_logger().debug(f'Error processing RPM data: {e}')
    
    def process_scan_data(self, data):
        """Process: SCAN:angle,distance_mm"""
        try:
            parts = data.replace('SCAN:', '').split(',')
            if len(parts) != 2:
                return
            
            angle_deg = int(parts[0])
            distance_mm = int(parts[1])
            distance_m = distance_mm / 1000.0 if distance_mm > 0 else float('inf')
            
            # Map angle to scan array index (0-180 degrees = -π/2 to π/2 radians)
            # Assuming 5 degree increments for 37 elements
            idx = int((angle_deg + 90) / 5)  # Offset by 90 to center at 0
            if 0 <= idx < 37:
                self.scan_ranges[idx] = distance_m
            
            # Check if sweep completed
            if (self.current_sweep_dir == 1 and angle_deg < self.last_angle) or \
               (self.current_sweep_dir == -1 and angle_deg > self.last_angle):
                self.current_sweep_dir *= -1
                self.publish_scan()
                self.scan_ranges = [float('inf')] * 37
            
            self.last_angle = angle_deg
            
        except Exception as e:
            self.get_logger().debug(f'Error processing scan data: {e}')
    
    def process_imu_data(self, data):
        """Process: IMU:accel_x,accel_y,accel_z,gyro_x,gyro_y,gyro_z,temp"""
        try:
            parts = data.replace('IMU:', '').split(',')
            if len(parts) < 7:
                return
            
            accel_x = float(parts[0])
            accel_y = float(parts[1])
            accel_z = float(parts[2])
            gyro_x = float(parts[3])
            gyro_y = float(parts[4])
            gyro_z = float(parts[5])
            
            self.last_accel = Vector3(x=accel_x, y=accel_y, z=accel_z)
            self.last_gyro = Vector3(x=gyro_x, y=gyro_y, z=gyro_z)
            
        except Exception as e:
            self.get_logger().debug(f'Error processing IMU data: {e}')
    
    def publish_odometry(self):
        """Publish odometry message and TF"""
        now = self.get_clock().now().to_msg()
        
        # Odometry message
        odom = Odometry()
        odom.header.stamp = now
        odom.header.frame_id = 'odom'
        odom.child_frame_id = 'base_footprint'
        
        odom.pose.pose.position.x = self.x
        odom.pose.pose.position.y = self.y
        odom.pose.pose.position.z = 0.0
        
        q = quaternion_from_euler(0, 0, self.theta)
        odom.pose.pose.orientation = Quaternion(x=q[0], y=q[1], z=q[2], w=q[3])
        
        odom.twist.twist.linear.x = 0.0  # Would need velocities from encoder data
        odom.twist.twist.angular.z = 0.0
        
        self.odom_pub.publish(odom)
        
        # TF broadcast
        t = TransformStamped()
        t.header.stamp = now
        t.header.frame_id = 'odom'
        t.child_frame_id = 'base_footprint'
        t.transform.translation.x = self.x
        t.transform.translation.y = self.y
        t.transform.translation.z = 0.0
        t.transform.rotation = odom.pose.pose.orientation
        
        self.tf_broadcaster.sendTransform(t)
    
    def publish_scan(self):
        """Publish laser scan message"""
        scan = LaserScan()
        scan.header.stamp = self.get_clock().now().to_msg()
        scan.header.frame_id = 'tof_sensor'
        
        scan.angle_min = -math.pi / 2
        scan.angle_max = math.pi / 2
        scan.angle_increment = math.pi / 36  # 180/36 = 5 degrees
        scan.time_increment = 0.01
        scan.range_min = 0.03
        scan.range_max = 2.0
        scan.ranges = self.scan_ranges.copy()
        
        self.scan_pub.publish(scan)


def main(args=None):
    rclpy.init(args=args)
    node = RobotController()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()
