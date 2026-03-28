#!/usr/bin/env python3
"""
Simple Keyboard Teleoperation for ROS 2
No external dependencies required - uses only rclpy
"""

import rclpy
from rclpy.node import Node
from geometry_msgs.msg import Twist
import sys
import select
import termios
import tty
from threading import Thread


class KeyboardTeleop(Node):
    """Simple keyboard teleoperation node"""
    
    def __init__(self):
        super().__init__('keyboard_teleop')
        
        # Parameters
        self.declare_parameter('linear_scale', 0.1)  # m/s per key press
        self.declare_parameter('angular_scale', 0.2)  # rad/s per key press
        self.declare_parameter('max_linear_speed', 0.5)
        self.declare_parameter('max_angular_speed', 1.0)
        self.declare_parameter('topic', '/cmd_vel')
        
        # Get parameters
        self.linear_scale = self.get_parameter('linear_scale').value
        self.angular_scale = self.get_parameter('angular_scale').value
        self.max_linear = self.get_parameter('max_linear_speed').value
        self.max_angular = self.get_parameter('max_angular_speed').value
        topic = self.get_parameter('topic').value
        
        # Current command
        self.linear_x = 0.0
        self.angular_z = 0.0
        self.stopped = False
        
        # Publisher
        self.cmd_pub = self.create_publisher(Twist, topic, 10)
        
        # Timer for publishing
        self.create_timer(0.1, self.timer_callback)
        
        # Terminal setup
        self.settings = termios.tcgetattr(sys.stdin)
        tty.setraw(sys.stdin.fileno())
        
        # Print instructions
        self.print_instructions()
        
        # Start input thread
        self.input_thread = Thread(target=self.input_loop, daemon=True)
        self.input_thread.start()
    
    def print_instructions(self):
        """Print help text"""
        help_text = """
╔════════════════════════════════════════════════════════╗
║            KEYBOARD TELEOPERATION                     ║
╠════════════════════════════════════════════════════════╣
║                                                        ║
║                    w                                   ║
║                    ↑ (forward)                         ║
║              a ← + → d  (turn)                         ║
║                    ↓ (backward)                        ║
║                    s                                   ║
║                                                        ║
║  w    : increase forward speed                         ║
║  s    : increase backward speed                        ║
║  a    : rotate counter-clockwise (left)               ║
║  d    : rotate clockwise (right)                       ║
║                                                        ║
║  SPACE : FULL STOP                                     ║
║  c    : clear (reset all)                              ║
║  ?    : show this help                                 ║
║  q    : QUIT                                           ║
║                                                        ║
║  Current Command:                                      ║
║    Linear:  {:.2f} m/s                                 ║
║    Angular: {:.2f} rad/s                               ║
║                                                        ║
╚════════════════════════════════════════════════════════╝
""".format(self.linear_x, self.angular_z)
        print(help_text)
    
    def get_key(self):
        """Non-blocking key input"""
        rlist, _, _ = select.select([sys.stdin], [], [], 0.1)
        if rlist:
            return sys.stdin.read(1)
        return ''
    
    def input_loop(self):
        """Main input processing loop"""
        while rclpy.ok():
            key = self.get_key()
            
            if not key:
                continue
            
            old_linear = self.linear_x
            old_angular = self.angular_z
            
            if key == 'w':
                # Forward
                self.linear_x = min(
                    self.max_linear,
                    self.linear_x + self.linear_scale
                )
            
            elif key == 's':
                # Backward
                self.linear_x = max(
                    -self.max_linear,
                    self.linear_x - self.linear_scale
                )
            
            elif key == 'a':
                # Turn left (counter-clockwise)
                self.angular_z = min(
                    self.max_angular,
                    self.angular_z + self.angular_scale
                )
            
            elif key == 'd':
                # Turn right (clockwise)
                self.angular_z = max(
                    -self.max_angular,
                    self.angular_z - self.angular_scale
                )
            
            elif key == ' ':
                # Space = FULL STOP
                self.linear_x = 0.0
                self.angular_z = 0.0
                self.stopped = True
            
            elif key == 'c':
                # Clear
                self.linear_x = 0.0
                self.angular_z = 0.0
            
            elif key == '?':
                self.print_instructions()
            
            elif key == 'q':
                # Quit
                self.get_logger().info("Shutting down...")
                self.linear_x = 0.0
                self.angular_z = 0.0
                self.stopped = True
                termios.tcsetattr(sys.stdin, termios.TCSADRAIN, self.settings)
                rclpy.shutdown()
                return
            
            # Print status if changed
            if old_linear != self.linear_x or old_angular != self.angular_z:
                status = f"Linear: {self.linear_x:+.2f} m/s  |  Angular: {self.angular_z:+.2f} rad/s"
                print(f"\r{status}", end='', flush=True)
    
    def timer_callback(self):
        """Publish velocity command"""
        twist = Twist()
        twist.linear.x = self.linear_x
        twist.linear.y = 0.0
        twist.linear.z = 0.0
        twist.angular.x = 0.0
        twist.angular.y = 0.0
        twist.angular.z = self.angular_z
        
        self.cmd_pub.publish(twist)


def main(args=None):
    rclpy.init(args=args)
    
    try:
        node = KeyboardTeleop()
        rclpy.spin(node)
    except KeyboardInterrupt:
        print("\n\nKeyboard interrupt received")
    except Exception as e:
        print(f"\nError: {e}")
    finally:
        if rclpy.ok():
            rclpy.shutdown()


if __name__ == '__main__':
    main()
