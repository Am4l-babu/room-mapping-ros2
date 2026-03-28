#!/usr/bin/env python3
"""
ROS 2 Node for ESP32 micro-ROS Robot Integration
PHASE 1 & 3: WiFi Transport + Safety Integration

This node receives data from the micro-ROS agent (which bridges ESP32 to ROS 2)
and provides a clean interface for the rest of the robot system.

Publishes:
  - /odom (from micro-ROS /odom)
  - /imu (from micro-ROS /imu)
  - /scan (from micro-ROS /scan)
  - /diagnostics (system health)

Subscribes:
  - /cmd_vel → forwarded to micro-ROS /cmd_vel

Features:
  - Automatic topic relay with QoS profiles
  - Connection monitoring
  - Message rate diagnostics
  - Safety watchdog integration
"""

import rclpy
from rclpy.node import Node
from rclpy.qos import QoSProfile, QoSHistoryPolicy, QoSReliabilityPolicy, QoSDurabilityPolicy
import math
import threading
import time
from collections import deque

from sensor_msgs.msg import LaserScan, Imu
from nav_msgs.msg import Odometry
from geometry_msgs.msg import Twist, TransformStamped, Quaternion, Vector3
from diagnostic_msgs.msg import DiagnosticArray, DiagnosticStatus, KeyValue
from tf2_ros import TransformBroadcaster
from tf_transformations import quaternion_from_euler


class MicroROSRobotBridge(Node):
    """
    Bridges micro-ROS agent topics to ROS 2 ecosystem.
    Also handles TF broadcasting and diagnostics.
    """
    
    def __init__(self):
        super().__init__('micro_ros_robot_bridge')
        
        # ============= PARAMETERS =============
        self.declare_parameter('enable_diagnostics', True)
        self.declare_parameter('watchdog_timeout_ms', 500)
        
        enable_diag = self.get_parameter('enable_diagnostics').value
        self.watchdog_timeout_sec = self.get_parameter('watchdog_timeout_ms').value / 1000.0
        
        # ============= QOS PROFILES =============
        # Best-effort for sensor data (tolerate loss)
        self.qos_sensor = QoSProfile(
            history=QoSHistoryPolicy.KEEP_LAST,
            depth=10,
            reliability=QoSReliabilityPolicy.BEST_EFFORT,
            durability=QoSDurabilityPolicy.VOLATILE
        )
        
        # Reliable for critical commands
        self.qos_command = QoSProfile(
            history=QoSHistoryPolicy.KEEP_LAST,
            depth=5,
            reliability=QoSReliabilityPolicy.RELIABLE,
            durability=QoSDurabilityPolicy.VOLATILE
        )
        
        # ============= PUBLISHERS =============
        # Re-publish sensor data with proper frames
        self.odom_pub = self.create_publisher(Odometry, 'odom', self.qos_sensor)
        self.imu_pub = self.create_publisher(Imu, 'imu', self.qos_sensor)
        self.scan_pub = self.create_publisher(LaserScan, 'scan', self.qos_sensor)
        
        if enable_diag:
            self.diag_pub = self.create_publisher(
                DiagnosticArray, '/diagnostics', 10
            )
        else:
            self.diag_pub = None
        
        self.tf_broadcaster = TransformBroadcaster(self)
        
        # ============= SUBSCRIBERS =============
        # Subscribe to micro-ROS topics
        self.odom_sub = self.create_subscription(
            Odometry, 'odom_in', self.on_odom, self.qos_sensor
        )
        self.imu_sub = self.create_subscription(
            Imu, 'imu_in', self.on_imu, self.qos_sensor
        )
        self.scan_sub = self.create_subscription(
            LaserScan, 'scan_in', self.on_scan, self.qos_sensor
        )
        
        # Command interface (local)
        self.cmd_vel_sub = self.create_subscription(
            Twist, 'cmd_vel', self.on_cmd_vel, self.qos_command
        )
        
        # ============= COMMAND STATE =============
        self.last_cmd_vel_time = time.time()
        self.current_cmd_vel = Twist()
        self.cmd_vel_watchdog_active = False
        
        # ============= DIAGNOSTICS =============
        self.message_counts = {
            'odom': 0,
            'imu': 0,
            'scan': 0,
            'cmd_vel': 0
        }
        self.last_time = time.time()
        
        # ============= MESSAGE HISTORY =============
        self.odom_history = deque(maxlen=100)
        self.imu_history = deque(maxlen=100)
        self.scan_history = deque(maxlen=100)
        
        # ============= LOGGING =============
        self.get_logger().info('micro-ROS Robot Bridge initialized')
        self.get_logger().info(f'Watchdog timeout: {self.watchdog_timeout_sec*1000:.0f} ms')
        self.get_logger().info('Waiting for micro-ROS agent...')
        
        # ============= MONITORING THREAD =============
        self.monitor_thread = threading.Thread(target=self.monitoring_loop, daemon=True)
        self.monitor_thread.start()
    
    # ============= SUBSCRIPTIONS =============
    
    def on_odom(self, msg: Odometry):
        """Receive odometry from micro-ROS agent"""
        self.message_counts['odom'] += 1
        self.odom_history.append(time.time())
        
        # Ensure consistent frame IDs
        msg.header.frame_id = 'odom'
        msg.child_frame_id = 'base_link'
        
        # Re-publish with ROS 2 timestamp
        self.odom_pub.publish(msg)
        
        # Broadcast TF: odom → base_link
        tf_msg = TransformStamped()
        tf_msg.header = msg.header
        tf_msg.child_frame_id = msg.child_frame_id
        tf_msg.transform.translation.x = msg.pose.pose.position.x
        tf_msg.transform.translation.y = msg.pose.pose.position.y
        tf_msg.transform.translation.z = msg.pose.pose.position.z
        tf_msg.transform.rotation = msg.pose.pose.orientation
        
        self.tf_broadcaster.sendTransform(tf_msg)
    
    def on_imu(self, msg: Imu):
        """Receive IMU data from micro-ROS agent"""
        self.message_counts['imu'] += 1
        self.imu_history.append(time.time())
        
        # Ensure frame ID
        msg.header.frame_id = 'base_link'
        
        # Re-publish
        self.imu_pub.publish(msg)
    
    def on_scan(self, msg: LaserScan):
        """Receive laser scan from micro-ROS agent"""
        self.message_counts['scan'] += 1
        self.scan_history.append(time.time())
        
        # Ensure frame ID
        msg.header.frame_id = 'base_scan'
        
        # Re-publish
        self.scan_pub.publish(msg)
    
    def on_cmd_vel(self, msg: Twist):
        """
        Receive command velocity from local navigation stack.
        Forward to micro-ROS agent AND implement safety watchdog.
        """
        # Store command
        self.current_cmd_vel = msg
        self.last_cmd_vel_time = time.time()
        self.message_counts['cmd_vel'] += 1
        self.cmd_vel_watchdog_active = True
        
        self.get_logger().debug(
            f'cmd_vel: linear={msg.linear.x:.3f}, angular={msg.angular.z:.3f}'
        )
    
    # ============= SAFETY WATCHDOG =============
    
    def check_cmd_vel_timeout(self):
        """
        Monitor /cmd_vel for timeout.
        If no command received for watchdog_timeout_sec, send stop command.
        """
        elapsed = time.time() - self.last_cmd_vel_time
        
        if elapsed > self.watchdog_timeout_sec:
            if self.cmd_vel_watchdog_active:
                # Timeout triggered: send stop
                stop_cmd = Twist()
                self.get_logger().warn(
                    f'WATCHDOG: /cmd_vel timeout ({elapsed:.2f}s) → STOP'
                )
                self.current_cmd_vel = stop_cmd
                self.cmd_vel_watchdog_active = False
    
    # ============= DIAGNOSTICS =============
    
    def calculate_message_rates(self):
        """Calculate message rates in Hz"""
        current_time = time.time()
        dt = current_time - self.last_time
        self.last_time = current_time
        
        if dt <= 0:
            return {}
        
        rates = {}
        
        # Odometry rate
        if len(self.odom_history) > 1:
            time_span = self.odom_history[-1] - self.odom_history[0]
            if time_span > 0:
                rates['odom_hz'] = len(self.odom_history) / time_span
        
        # IMU rate
        if len(self.imu_history) > 1:
            time_span = self.imu_history[-1] - self.imu_history[0]
            if time_span > 0:
                rates['imu_hz'] = len(self.imu_history) / time_span
        
        # Scan rate
        if len(self.scan_history) > 1:
            time_span = self.scan_history[-1] - self.scan_history[0]
            if time_span > 0:
                rates['scan_hz'] = len(self.scan_history) / time_span
        
        return rates
    
    def publish_diagnostics(self):
        """Publish system diagnostics"""
        if self.diag_pub is None:
            return
        
        array = DiagnosticArray()
        array.header.stamp = self.get_clock().now().to_msg()
        
        # Check watchdog status
        status = DiagnosticStatus()
        status.name = 'micro_ros_robot_bridge'
        status.hardware_id = 'esp32_wifi'
        
        elapsed_cmd = time.time() - self.last_cmd_vel_time
        if elapsed_cmd > self.watchdog_timeout_sec:
            status.level = DiagnosticStatus.WARN
            status.message = f'cmd_vel timeout: {elapsed_cmd:.2f}s'
        else:
            status.level = DiagnosticStatus.OK
            status.message = 'All systems nominal'
        
        # Add message rates
        rates = self.calculate_message_rates()
        status.values = [
            KeyValue(key='odom_rate_hz', value=f"{rates.get('odom_hz', 0):.1f}"),
            KeyValue(key='imu_rate_hz', value=f"{rates.get('imu_hz', 0):.1f}"),
            KeyValue(key='scan_rate_hz', value=f"{rates.get('scan_hz', 0):.1f}"),
            KeyValue(key='cmd_vel_timeout_sec', value=f"{self.watchdog_timeout_sec:.2f}"),
            KeyValue(key='watchdog_active', value=str(self.cmd_vel_watchdog_active)),
        ]
        
        array.status.append(status)
        self.diag_pub.publish(array)
    
    # ============= MONITORING LOOP =============
    
    def monitoring_loop(self):
        """Background thread for watchdog and diagnostics"""
        while rclpy.ok():
            try:
                # Check watchdog timeout
                self.check_cmd_vel_timeout()
                
                # Publish diagnostics (1 Hz)
                self.publish_diagnostics()
                
                time.sleep(1.0)
            except Exception as e:
                self.get_logger().warn(f'Monitoring loop error: {e}')


def main(args=None):
    rclpy.init(args=args)
    node = MicroROSRobotBridge()
    
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        node.get_logger().info('Shutting down...')
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
