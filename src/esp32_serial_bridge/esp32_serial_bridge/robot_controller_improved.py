"""
ESP32 Robot Controller Node for ROS 2 - IMPROVED for PID Motors
Phase 2: Closed-Loop Motor Control Integration

Improved Features:
- Sends velocity commands to PID-enabled ESP32 firmware
- Higher odometry update rate (100 Hz compatible)
- Better sensor filtering
- Improved data parsing for performance
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


class RobotControllerPID(Node):
    def __init__(self):
        super().__init__('robot_controller')
        
        # ============= ROBOT PARAMETERS =============
        self.declare_parameter('port', '/dev/ttyACM1')
        self.declare_parameter('baud', 115200)
        self.declare_parameter('wheel_diameter', 0.065)      # meters
        self.declare_parameter('wheel_separation', 0.18)     # meters
        self.declare_parameter('max_speed', 0.5)             # m/s
        self.declare_parameter('encoder_slots', 20)          # slots per revolution
        self.declare_parameter('update_rate', 100)           # Hz
        
        # Get parameters
        port = self.get_parameter('port').value
        baud = self.get_parameter('baud').value
        self.wheel_diameter = self.get_parameter('wheel_diameter').value
        self.wheel_separation = self.get_parameter('wheel_separation').value
        self.max_speed = self.get_parameter('max_speed').value
        self.encoder_slots = self.get_parameter('encoder_slots').value
        self.update_rate = self.get_parameter('update_rate').value
        
        # Calculate constants
        self.wheel_circumference = math.pi * self.wheel_diameter
        self.meters_per_slot = self.wheel_circumference / self.encoder_slots
        self.update_period_sec = 1.0 / self.update_rate
        
        # ============= SERIAL CONNECTION =============
        try:
            self.ser = serial.Serial(port, baud, timeout=0.1)
            self.get_logger().info(f'Connected to ESP32 on {port} @ {baud}')
            time.sleep(2)  # Wait for ESP32 boot
            self.ser.reset_input_buffer()
        except Exception as e:
            self.get_logger().error(f'Failed to connect to Serial: {e}')
            raise SystemExit
        
        # ============= PUBLISHERS =============
        self.odom_pub = self.create_publisher(Odometry, 'odom', 10)
        self.scan_pub = self.create_publisher(LaserScan, 'scan', 10)
        self.imu_pub = self.create_publisher(Imu, 'imu', 10)
        self.tf_broadcaster = TransformBroadcaster(self)
        
        # ============= SUBSCRIBERS =============
        self.cmd_sub = self.create_subscription(Twist, 'cmd_vel', self.cmd_vel_callback, 10)
        
        # ============= ODOMETRY STATE =============
        self.x = 0.0
        self.y = 0.0
        self.theta = 0.0
        self.last_encoder_m1 = 0
        self.last_encoder_m2 = 0
        self.encoder_buffer = deque(maxlen=100)  # For smooth odometry
        
        # Velocity tracking for covariance
        self.last_vel_x = 0.0
        self.last_vel_y = 0.0
        self.last_vel_angular = 0.0
        
        # ============= SENSOR DATA =============
        self.scan_ranges = [float('inf')] * 37
        self.last_imu = {'ax': 0.0, 'ay': 0.0, 'az': 9.81, 
                         'gx': 0.0, 'gy': 0.0, 'gz': 0.0, 'temp': 25.0}
        
        # ============= PID FEEDBACK TRACKING =============
        self.target_rpm_m1 = 0.0
        self.target_rpm_m2 = 0.0
        self.actual_rpm_m1 = 0.0
        self.actual_rpm_m2 = 0.0
        
        # ============= TIMING =============
        self.stamp = self.get_clock().now()
        self.last_odom_time = time.time()
        
        # ============= SERIAL READER THREAD =============
        self.read_thread = threading.Thread(target=self.serial_read_loop, daemon=True)
        self.serial_lock = threading.Lock()
        self.read_thread.start()
        
        self.get_logger().info('Robot Controller (PID) initialized')
        self.get_logger().info(f'Update rate: {self.update_rate} Hz')
        self.get_logger().info(f'Wheel diameter: {self.wheel_diameter*1000:.1f} mm')
        self.get_logger().info(f'Wheel separation: {self.wheel_separation*1000:.1f} mm')
    
    # ============= COMMAND INTERFACE =============
    
    def cmd_vel_callback(self, msg):
        """
        Convert /cmd_vel twist to velocity command for PID motor controller
        
        ESP32 firmware will convert VEL command to RPM targets internally
        """
        try:
            linear_x = msg.linear.x
            angular_z = msg.angular.z
            
            # Clamp to limits
            linear_x = max(min(linear_x, self.max_speed), -self.max_speed)
            angular_z = max(min(angular_z, 2.0), -2.0)  # rad/s limit
            
            # Send velocity command to ESP32
            # Format: VEL:linear,angular
            command = f'VEL:{linear_x:.3f},{angular_z:.3f}\n'
            
            with self.serial_lock:
                self.ser.write(command.encode('utf-8'))
            
            self.get_logger().debug(f'cmd_vel: linear={linear_x:.2f}, angular={angular_z:.2f}')
            
        except Exception as e:
            self.get_logger().warn(f'Error sending cmd_vel: {e}')
    
    # ============= SERIAL COMMUNICATION =============
    
    def serial_read_loop(self):
        """
        High-frequency serial reader thread
        Handles RPM data at up to 100 Hz
        """
        buffer = ""
        error_count = 0
        
        while rclpy.ok():
            try:
                with self.serial_lock:
                    if self.ser.in_waiting > 0:
                        data = self.ser.read(self.ser.in_waiting).decode('utf-8', errors='ignore')
                        buffer += data
                        
                        # Process complete lines
                        while '\n' in buffer:
                            line, buffer = buffer.split('\n', 1)
                            line = line.strip()
                            
                            if line.startswith('RPM:'):
                                self._process_rpm_line(line)
                            elif line.startswith('SCAN:'):
                                self._process_scan_line(line)
                            elif line.startswith('IMU:'):
                                self._process_imu_line(line)
                
                time.sleep(0.001)  # 1ms sleep
                error_count = 0  # Reset error counter
                
            except Exception as e:
                error_count += 1
                if error_count % 100 == 0:
                    self.get_logger().debug(f'Serial read error ({error_count}): {e}')
    
    def _process_rpm_line(self, line):
        """
        Parse RPM data line
        Format: RPM:m1_target,m2_target,m1_actual,m2_actual,pulses_m1,pulses_m2
        
        This is high-frequency feedback (~100 Hz) from motor_control_pid firmware
        """
        try:
            parts = line.replace('RPM:', '').split(',')
            
            if len(parts) < 6:
                return
            
            # Extract data
            target_rpm_m1 = float(parts[0])
            target_rpm_m2 = float(parts[1])
            actual_rpm_m1 = float(parts[2])
            actual_rpm_m2 = float(parts[3])
            pulses_m1 = int(parts[4])
            pulses_m2 = int(parts[5])
            
            # Update feedback
            self.target_rpm_m1 = target_rpm_m1
            self.target_rpm_m2 = target_rpm_m2
            self.actual_rpm_m1 = actual_rpm_m1
            self.actual_rpm_m2 = actual_rpm_m2
            
            # Calculate odometry
            delta_pulses_m1 = pulses_m1 - self.last_encoder_m1
            delta_pulses_m2 = pulses_m2 - self.last_encoder_m2
            
            self.last_encoder_m1 = pulses_m1
            self.last_encoder_m2 = pulses_m2
            
            # If large pulse changes, ignore (likely counter reset)
            if abs(delta_pulses_m1) > 500 or abs(delta_pulses_m2) > 500:
                return
            
            if delta_pulses_m1 == 0 and delta_pulses_m2 == 0:
                return
            
            # Update odometry from encoder deltas
            self._update_odometry(delta_pulses_m1, delta_pulses_m2)
            
        except Exception as e:
            self.get_logger().debug(f'Error parsing RPM line: {e}')
    
    def _process_scan_line(self, line):
        """Process laser scan data (low frequency)"""
        # Not implemented in this version - focus on core control
        pass
    
    def _process_imu_line(self, line):
        """Process IMU data"""
        try:
            parts = line.replace('IMU:', '').split(',')
            
            if len(parts) < 7:
                return
            
            self.last_imu = {
                'ax': float(parts[0]),
                'ay': float(parts[1]),
                'az': float(parts[2]),
                'gx': float(parts[3]),
                'gy': float(parts[4]),
                'gz': float(parts[5]),
                'temp': float(parts[6])
            }
            
            # Publish IMU data
            self._publish_imu()
            
        except Exception as e:
            self.get_logger().debug(f'Error parsing IMU line: {e}')
    
    # ============= ODOMETRY CALCULATION =============
    
    def _update_odometry(self, delta_pulses_m1, delta_pulses_m2):
        """
        Update odometry from encoder pulses
        Uses differential drive kinematics
        """
        # Convert pulses to distances
        delta_dist_m1 = delta_pulses_m1 * self.meters_per_slot
        delta_dist_m2 = delta_pulses_m2 * self.meters_per_slot
        
        # Average distance (forward motion)
        delta_s = (delta_dist_m1 + delta_dist_m2) / 2.0
        
        # Difference in distance (rotation)
        delta_dist_diff = delta_dist_m2 - delta_dist_m1
        delta_theta = delta_dist_diff / self.wheel_separation
        
        # Update pose
        if abs(delta_s) > 0.00001:  # Only if significant motion
            self.x += delta_s * math.cos(self.theta + delta_theta / 2.0)
            self.y += delta_s * math.sin(self.theta + delta_theta / 2.0)
        
        self.theta += delta_theta
        
        # Normalize angle to [-pi, pi]
        while self.theta > math.pi:
            self.theta -= 2 * math.pi
        while self.theta < -math.pi:
            self.theta += 2 * math.pi
        
        # Publish odometry
        self._publish_odometry()
    
    # ============= PUBLISHING =============
    
    def _publish_odometry(self):
        """Publish /odom topic and TF transforms"""
        now = self.get_clock().now()
        
        # Create odometry message
        odom = Odometry()
        odom.header.stamp = now.to_msg()
        odom.header.frame_id = "odom"
        odom.child_frame_id = "base_link"
        
        # Position
        odom.pose.pose.position.x = self.x
        odom.pose.pose.position.y = self.y
        odom.pose.pose.position.z = 0.0
        
        # Orientation (yaw only)
        quat = quaternion_from_euler(0, 0, self.theta)
        odom.pose.pose.orientation = Quaternion(x=quat[0], y=quat[1], 
                                                 z=quat[2], w=quat[3])
        
        # Covariance (small values - encoder odometry is fairly trustworthy short-term)
        odom.pose.covariance[0] = 0.01    # x
        odom.pose.covariance[7] = 0.01    # y
        odom.pose.covariance[35] = 0.1    # theta
        
        # Publish
        self.odom_pub.publish(odom)
        
        # Broadcast TF: odom -> base_link
        transform = TransformStamped()
        transform.header.stamp = now.to_msg()
        transform.header.frame_id = "odom"
        transform.child_frame_id = "base_link"
        
        transform.transform.translation.x = self.x
        transform.transform.translation.y = self.y
        transform.transform.translation.z = 0.0
        
        transform.transform.rotation = odom.pose.pose.orientation
        
        self.tf_broadcaster.sendTransform(transform)
    
    def _publish_imu(self):
        """Publish /imu topic"""
        try:
            now = self.get_clock().now()
            
            imu = Imu()
            imu.header.stamp = now.to_msg()
            imu.header.frame_id = "base_link"
            
            # Accelerometer (convert to m/s²if needed, assuming already scaled)
            imu.linear_acceleration.x = self.last_imu['ax']
            imu.linear_acceleration.y = self.last_imu['ay']
            imu.linear_acceleration.z = self.last_imu['az']
            
            # Gyroscope (convert to rad/s if needed)
            imu.angular_velocity.x = self.last_imu['gx']
            imu.angular_velocity.y = self.last_imu['gy']
            imu.angular_velocity.z = self.last_imu['gz']
            
            # Covariance
            imu.linear_acceleration_covariance = [0.01] * 9
            imu.angular_velocity_covariance = [0.01] * 9
            
            self.imu_pub.publish(imu)
            
        except Exception as e:
            self.get_logger().debug(f'Error publishing IMU: {e}')


def main(args=None):
    rclpy.init(args=args)
    node = RobotControllerPID()
    
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        node.get_logger().info('Shutting down...')
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
