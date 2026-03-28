#!/usr/bin/env python3
"""
Synchronized Teleoperation Node
Controls both real and simulated robot from keyboard input
Ensures LiDAR scanning is synchronized (same commands = same motion)

Usage:
  ros2 run my_robot_controllers sync_teleop_node --ros-args -p enable_sim:=true
"""

import rclpy
from rclpy.node import Node
from geometry_msgs.msg import Twist
from std_msgs.msg import String
import sys
import select
import termios
import tty

class SyncTeleopNode(Node):
    """Synchronized teleoperation for real + simulated robot"""
    
    def __init__(self):
        super().__init__('sync_teleop_node')
        
        # Parameters
        self.declare_parameter('enable_sim', False)
        self.declare_parameter('max_linear_speed', 0.5)
        self.declare_parameter('max_angular_speed', 1.0)
        self.declare_parameter('linear_step', 0.05)
        self.declare_parameter('angular_step', 0.1)
        
        self.enable_sim = self.get_parameter('enable_sim').value
        self.max_linear = self.get_parameter('max_linear_speed').value
        self.max_angular = self.get_parameter('max_angular_speed').value
        self.linear_step = self.get_parameter('linear_step').value
        self.angular_step = self.get_parameter('angular_step').value
        
        # Current velocities
        self.linear_x = 0.0
        self.angular_z = 0.0
        
        # Publishers
        self.real_cmd_pub = self.create_publisher(Twist, '/real/cmd_vel', 10)
        self.sim_cmd_pub = self.create_publisher(Twist, '/sim/cmd_vel', 10) if self.enable_sim else None
        
        # Status publisher
        self.status_pub = self.create_publisher(String, '/teleop/status', 10)
        
        # Create timer for publishing
        self.create_timer(0.1, self.publish_commands)
        
        # Terminal setup
        self.settings = termios.tcgetattr(sys.stdin)
        
        self.get_logger().info('=== Synchronized Teleoperation Node ===')
        self.get_logger().info(f'Real robot: ENABLED')
        self.get_logger().info(f'Simulated robot: {"ENABLED" if self.enable_sim else "DISABLED"}')
        self.get_logger().info('')
        self.print_help()
    
    def print_help(self):
        """Print keyboard controls"""
        help_text = """
╔════════════════════════════════════════════╗
║       KEYBOARD TELEOPERATION CONTROLS      ║
╠════════════════════════════════════════════╣
║                                            ║
║                  w/W                       ║
║                  ↑ ↑                       ║
║              a/A ← + → d/D                 ║
║                  ↓ ↓                       ║
║                  s/S                       ║
║                                            ║
║  w/W : increase forward speed              ║
║  s/S : increase backward speed             ║
║  a/A : increase left turn                  ║
║  d/D : increase right turn                 ║
║  SPACE : full stop                         ║
║  ? : show this help                        ║
║  q : quit                                  ║
║                                            ║
╚════════════════════════════════════════════╝
"""
        self.get_logger().info(help_text)
    
    def get_key(self):
        """Get non-blocking keyboard input"""
        tty.setraw(sys.stdin.fileno())
        rlist, _, _ = select.select([sys.stdin], [], [], 0.1)
        if rlist:
            key = sys.stdin.read(1)
        else:
            key = ''
        termios.tcsetattr(sys.stdin, termios.TCSADRAIN, self.settings)
        return key
    
    def handle_input(self):
        """Handle keyboard input"""
        key = self.get_key()
        
        if key == 'w':
            # Increase forward speed
            self.linear_x = min(self.max_linear, self.linear_x + self.linear_step)
            self.status_pub.publish(String(data=f'FWD: {self.linear_x:.2f} m/s'))
            
        elif key == 's':
            # Increase backward speed
            self.linear_x = max(-self.max_linear, self.linear_x - self.linear_step)
            self.status_pub.publish(String(data=f'BWD: {self.linear_x:.2f} m/s'))
            
        elif key == 'a':
            # Turn left
            self.angular_z = min(self.max_angular, self.angular_z + self.angular_step)
            self.status_pub.publish(String(data=f'LEFT: {self.angular_z:.2f} rad/s'))
            
        elif key == 'd':
            # Turn right
            self.angular_z = max(-self.max_angular, self.angular_z - self.angular_step)
            self.status_pub.publish(String(data=f'RIGHT: {self.angular_z:.2f} rad/s'))
            
        elif key == ' ':
            # Full stop
            self.linear_x = 0.0
            self.angular_z = 0.0
            self.status_pub.publish(String(data='STOP'))
            
        elif key == '?':
            self.print_help()
            
        elif key == 'q':
            self.get_logger().info('Shutting down...')
            self.linear_x = 0.0
            self.angular_z = 0.0
            rclpy.shutdown()
            return False
        
        return True
    
    def publish_commands(self):
        """Publish velocity commands to both robots"""
        # Create twist message
        twist = Twist()
        twist.linear.x = self.linear_x
        twist.linear.y = 0.0
        twist.linear.z = 0.0
        twist.angular.x = 0.0
        twist.angular.y = 0.0
        twist.angular.z = self.angular_z
        
        # Publish to real robot
        self.real_cmd_pub.publish(twist)
        
        # Publish to simulated robot if enabled
        if self.enable_sim and self.sim_cmd_pub:
            self.sim_cmd_pub.publish(twist)
        
        # Handle keyboard input
        if not self.handle_input():
            self.destroy_node()


def main(args=None):
    rclpy.init(args=args)
    
    node = SyncTeleopNode()
    
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        termios.tcsetattr(sys.stdin, termios.TCSADRAIN, node.settings)
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
