import rclpy
from rclpy.node import Node
import serial
import math
import threading

from sensor_msgs.msg import LaserScan
from nav_msgs.msg import Odometry
from geometry_msgs.msg import Twist, TransformStamped, Quaternion
from tf2_ros import TransformBroadcaster


class Esp32Bridge(Node):
    def __init__(self):
        super().__init__('esp32_bridge_node')

        try:
            self.ser = serial.Serial('/dev/ttyUSB0', 115200, timeout=0.1)
            self.get_logger().info('Connected to ESP32 on /dev/ttyUSB0')
        except Exception as e:
            self.get_logger().error(f'Failed to connect to Serial: {e}')
            raise SystemExit

        self.odom_pub = self.create_publisher(Odometry, '/odom', 10)
        self.scan_pub = self.create_publisher(LaserScan, '/tof_scan', 10)
        self.cmd_sub = self.create_subscription(Twist, '/cmd_vel', self.cmd_vel_callback, 10)
        self.tf_broadcaster = TransformBroadcaster(self)

        self.scan_ranges = [float('inf')] * 37
        self.current_sweep_dir = 1
        self.last_angle = 0

        self.read_thread = threading.Thread(target=self.serial_read_loop)
        self.read_thread.daemon = True
        self.read_thread.start()

    @staticmethod
    def euler_to_quaternion(yaw):
        q = Quaternion()
        q.x = 0.0
        q.y = 0.0
        q.z = math.sin(yaw * 0.5)
        q.w = math.cos(yaw * 0.5)
        return q

    def cmd_vel_callback(self, msg):
        linear_x = msg.linear.x
        angular_z = msg.angular.z
        wheel_separation = 0.18

        v_left = linear_x - (angular_z * wheel_separation / 2.0)
        v_right = linear_x + (angular_z * wheel_separation / 2.0)

        pwm_left = int(max(min((v_left / 0.5) * 255, 255), -255))
        pwm_right = int(max(min((v_right / 0.5) * 255, 255), -255))

        command_str = f'CMD:{pwm_left},{pwm_right}\n'
        self.ser.write(command_str.encode('utf-8'))

    def serial_read_loop(self):
        while rclpy.ok():
            if self.ser.in_waiting > 0:
                try:
                    line = self.ser.readline().decode('utf-8').strip()
                    if line.startswith('ODOM:'):
                        self.process_odom(line)
                    elif line.startswith('SCAN:'):
                        self.process_scan(line)
                except Exception:
                    pass

    def process_odom(self, data):
        parts = data.replace('ODOM:', '').split(',')
        if len(parts) != 3:
            return

        x = float(parts[0])
        y = float(parts[1])
        theta = float(parts[2])

        now = self.get_clock().now().to_msg()

        t = TransformStamped()
        t.header.stamp = now
        t.header.frame_id = 'odom'
        t.child_frame_id = 'base_footprint'
        t.transform.translation.x = x
        t.transform.translation.y = y
        t.transform.translation.z = 0.0
        t.transform.rotation = self.euler_to_quaternion(theta)
        self.tf_broadcaster.sendTransform(t)

        odom = Odometry()
        odom.header.stamp = now
        odom.header.frame_id = 'odom'
        odom.child_frame_id = 'base_footprint'
        odom.pose.pose.position.x = x
        odom.pose.pose.position.y = y
        odom.pose.pose.orientation = t.transform.rotation
        self.odom_pub.publish(odom)

    def process_scan(self, data):
        parts = data.replace('SCAN:', '').split(',')
        if len(parts) != 2:
            return

        angle_deg = int(parts[0])
        distance_mm = int(parts[1])
        distance_m = distance_mm / 1000.0 if distance_mm != -1 else float('inf')

        idx = int(angle_deg / 5)
        if 0 <= idx < 37:
            self.scan_ranges[idx] = distance_m

        if (self.current_sweep_dir == 1 and angle_deg < self.last_angle) or \
           (self.current_sweep_dir == -1 and angle_deg > self.last_angle):
            self.current_sweep_dir *= -1

            scan = LaserScan()
            scan.header.stamp = self.get_clock().now().to_msg()
            scan.header.frame_id = 'tof_sensor'
            scan.angle_min = -1.5708
            scan.angle_max = 1.5708
            scan.angle_increment = 0.0872665
            scan.time_increment = 0.015
            scan.range_min = 0.03
            scan.range_max = 2.0
            scan.ranges = self.scan_ranges.copy()
            self.scan_pub.publish(scan)
            self.scan_ranges = [float('inf')] * 37

        self.last_angle = angle_deg


def main(args=None):
    rclpy.init(args=args)
    node = Esp32Bridge()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()
