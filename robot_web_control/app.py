#!/usr/bin/env python3
"""
Standalone Robot Web Control Interface
- Direct ESP32 Serial Communication
- Manual Control (Forward, Backward, Left, Right)
- Obstacle Avoidance Mode
- No ROS 2 Required
"""

from flask import Flask, render_template, jsonify, request
import serial
import threading
import time
from collections import deque
import json

# Import configuration (NEVER commit config.py - it contains credentials!)
# Copy config_template.py to config.py and edit with your actual values
try:
    from config import (
        SERIAL_PORT, BAUD_RATE, FLASK_DEBUG, FLASK_HOST, FLASK_PORT,
        SECRET_KEY, DEFAULT_MOTOR_SPEED, OBSTACLE_THRESHOLD, SAFEGUARD_MARGIN,
        DEVICE_ID, DEVICE_NAME, WIFI_SSID, WIFI_PASSWORD, ESP32_HOST, ESP32_PORT,
        LOG_LEVEL, LOG_FILE
    )
except ImportError:
    print("[ERROR] config.py not found! Please copy config_template.py to config.py and edit it.")
    print("[ERROR] $ cp config_template.py config.py")
    exit(1)

app = Flask(__name__)
app.config['DEBUG'] = FLASK_DEBUG
app.config['SECRET_KEY'] = SECRET_KEY

class RobotController:
    def __init__(self):
        self.ser = None
        self.connected = False
        self.obstacle_distance = 9999  # mm
        self.last_command = None
        self.autonomous_mode = False
        self.autonomous_thread = None
        self.autonomous_stop_event = threading.Event()
        self.scan_state = 'idle'
        self.position_x = 0.0
        self.position_y = 0.0
        self.speed_history = deque(maxlen=10)
        self.lock = threading.Lock()
        
    def connect(self):
        """Connect to ESP32 via serial"""
        try:
            self.ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
            time.sleep(2)  # Wait for ESP32 to initialize
            self.connected = True
            print(f"[INFO] Connected to ESP32 on {SERIAL_PORT}")
            return True
        except Exception as e:
            print(f"[ERROR] Failed to connect: {e}")
            self.connected = False
            return False
    
    def send_command(self, cmd):
        """Send command to ESP32"""
        if not self.connected or not self.ser:
            return False
        
        try:
            with self.lock:
                self.ser.write(f"{cmd}\n".encode())
                self.last_command = cmd
                print(f"[CMD] Sent: {cmd}")
            return True
        except Exception as e:
            print(f"[ERROR] Failed to send command: {e}")
            return False
    
    def read_sensor_data(self):
        """Read sensor data from ESP32 in background thread"""
        while self.connected:
            try:
                if self.ser and self.ser.in_waiting:
                    data = self.ser.read(self.ser.in_waiting).decode('utf-8', errors='ignore')
                    # Parse distance readings
                    for line in data.split('\n'):
                        if 'distance' in line.lower() or 'mm' in line.lower():
                            try:
                                # Extract numeric value
                                distance = int(''.join(filter(str.isdigit, line)))
                                if 0 < distance < 5000:
                                    self.obstacle_distance = distance
                                    print(f"[SENSOR] Distance: {distance}mm")
                            except:
                                pass
            except Exception as e:
                pass
            time.sleep(0.1)
    
    def check_obstacle(self):
        """Check if obstacle detected (distance < 300mm)"""
        return self.obstacle_distance < 300
    
    def move(self, direction, speed=150):
        """Move robot in direction"""
        if self.autonomous_mode and self.check_obstacle() and direction == 'F':
            return False  # Blocked by obstacle
        
        commands = {
            'F': 'F',  # Forward
            'B': 'B',  # Backward
            'L': 'L',  # Left
            'R': 'R',  # Right
            'S': 'S',  # Stop
        }
        
        if direction in commands:
            self.send_command(commands[direction])
            return True
        return False

    def set_autonomous_mode(self, enabled):
        self.autonomous_mode = enabled
        if enabled:
            self.autonomous_stop_event.clear()
            if not self.autonomous_thread or not self.autonomous_thread.is_alive():
                self.autonomous_thread = threading.Thread(target=self._autonomous_loop, daemon=True)
                self.autonomous_thread.start()
            print("[MODE] Autonomous mode enabled")
        else:
            self.autonomous_stop_event.set()
            self.move('S')
            self.scan_state = 'idle'
            print("[MODE] Autonomous mode disabled")

    def _autonomous_loop(self):
        while self.connected and not self.autonomous_stop_event.is_set():
            if not self.autonomous_mode:
                time.sleep(0.1)
                continue

            # Always keep system safer for obstacle avoidance
            if self.check_obstacle():
                self.scan_state = 'obstacle_detected'
                self.move('S')
                time.sleep(0.2)

            # Scan three sectors using turn-based approximation
            self.scan_state = 'scanning_center'
            center_dist = self.obstacle_distance
            time.sleep(0.1)

            self.scan_state = 'scanning_left'
            self.move('L')
            time.sleep(0.3)
            self.move('S')
            left_dist = self.obstacle_distance
            time.sleep(0.2)

            self.scan_state = 'scanning_right'
            self.move('R')
            time.sleep(0.3)
            self.move('S')
            right_dist = self.obstacle_distance
            time.sleep(0.2)

            # Choose direction with max distance
            best = 'center'
            best_val = center_dist
            if left_dist > best_val:
                best = 'left'; best_val = left_dist
            if right_dist > best_val:
                best = 'right'; best_val = right_dist

            self.scan_state = f'resource_max_{best}'
            print(f"[AUTO] Scan results center={center_dist} left={left_dist} right={right_dist}, best={best}")

            if best_val < 300:
                # No safe direction, back off
                self.scan_state = 'backing_off'
                self.move('B')
                time.sleep(0.4)
                self.move('S')
            else:
                if best == 'center':
                    self.move('F')
                elif best == 'left':
                    self.move('L')
                elif best == 'right':
                    self.move('R')
                time.sleep(0.5)
                self.move('S')

            self.scan_state = 'idle'
            time.sleep(0.15)

    def get_status(self):
        """Get current robot status"""
        return {
            'connected': self.connected,
            'obstacle_distance': self.obstacle_distance,
            'obstacle_detected': self.check_obstacle(),
            'autonomous_mode': self.autonomous_mode,
            'scan_state': self.scan_state,
            'last_command': self.last_command,
            'position': {'x': self.position_x, 'y': self.position_y}
        }

# Initialize controller
robot = RobotController()

@app.route('/')
def index():
    """Serve main control page"""
    return render_template('index.html')

@app.route('/api/connect', methods=['POST'])
def connect():
    """Connect to robot"""
    if robot.connect():
        # Start sensor reading thread
        thread = threading.Thread(target=robot.read_sensor_data, daemon=True)
        thread.start()
        return jsonify({'success': True, 'message': 'Connected to robot'})
    return jsonify({'success': False, 'message': 'Failed to connect'}), 500

@app.route('/api/status', methods=['GET'])
def status():
    """Get robot status"""
    return jsonify(robot.get_status())

@app.route('/api/move', methods=['POST'])
def move():
    """Move robot"""
    data = request.json
    direction = data.get('direction', 'S')  # F, B, L, R, S
    
    if robot.move(direction):
        return jsonify({'success': True, 'obstacle_blocked': False})
    else:
        return jsonify({'success': False, 'obstacle_blocked': robot.check_obstacle()}), 409

@app.route('/api/autonomous', methods=['POST'])
def set_autonomous():
    """Toggle autonomous mode"""
    data = request.json
    enabled = data.get('enabled', False)
    robot.set_autonomous_mode(enabled)
    return jsonify({'success': True, 'mode': 'autonomous' if enabled else 'manual'})

@app.route('/api/stop', methods=['POST'])
def stop():
    """Emergency stop"""
    robot.move('S')
    return jsonify({'success': True})

if __name__ == '__main__':
    print("Starting Robot Web Control Interface...")
    print(f"Open browser: http://localhost:5000")
    app.run(host='0.0.0.0', port=5000, debug=False)
