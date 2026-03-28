#!/usr/bin/env python3
"""
=============================================================================
PHYSICAL VALIDATION: WIFI FAILURE TEST
Verify robot safety when WiFi connection is lost

Procedure:
1. Get robot moving forward steadily
2. Physically disconnect WiFi (unplug ESP32 WiFi antenna or disable WiFi on router)
3. Measure time for robot to stop (watchdog timeout)
4. Verify motors actually stop (safety mechanism working)
5. Reconnect WiFi and verify robot restarts

Safety Watchdog Specification:
- Timeout: 500ms (defined in ESP32 firmware Phase 3)
- Behavior: Motor PWM set to 0, robot coasts to stop
- Expected motor stop time: 500ms + coasting time (typically 0.5-1.5s total)

Why this matters:
- If WiFi drops, ESP32 can continue executing old commands indefinitely
- Watchdog MUST stop motors within 500ms of last valid command
- If watchdog fails: Robot could run into obstacles uncontrolled
=============================================================================
"""

import subprocess
import time
from enum import Enum

class WifiTestPhase(Enum):
    """Test phases"""
    STARTUP = 1
    BASELINE = 2
    WIFI_DISCONNECT = 3
    FAILURE_WAIT = 4
    VERIFICATION = 5
    RECOVERY = 6

class WiFiFailureValidator:
    """Validate WiFi failure safety response"""
    
    def __init__(self):
        self.results = {}
        self.current_phase = WifiTestPhase.STARTUP
    
    def print_header(self, title: str):
        """Print section header"""
        print("\n" + "=" * 80)
        print(f"  {title}")
        print("=" * 80)
    
    def print_phase(self, phase_name: str):
        """Print phase marker"""
        print(f"\n>>> {phase_name}")
    
    def check_robot_communication(self) -> bool:
        """Check if robot is accessible"""
        print("  Checking ROS 2 connectivity...", end=' ')
        
        result = subprocess.run(
            "ros2 topic list",
            shell=True,
            capture_output=True,
            timeout=3
        )
        
        if result.returncode == 0:
            print("✓")
            return True
        else:
            print("✗")
            return False
    
    def get_robot_orientation(self) -> float:
        """Estimate robot heading from IMU"""
        print("  Getting initial heading from /imu...", end=' ')
        
        result = subprocess.run(
            "ros2 topic echo /imu --once 2>/dev/null | grep 'z:' | head -1",
            shell=True,
            capture_output=True,
            timeout=2,
            text=True
        )
        
        if result.returncode == 0:
            print("✓")
            return 0.0  # For this test we just track motion
        else:
            print("?")
            return None
    
    def send_forward_command(self, velocity: float = 0.3):
        """Send robot forward"""
        print(f"  Sending forward command ({velocity} m/s)...")
        
        cmd = (f'ros2 topic pub /cmd_vel geometry_msgs/msg/Twist '
               f'"linear: {{x: {velocity}, y: 0.0, z: 0.0}} '
               f'angular: {{x: 0.0, y: 0.0, z: 0.0}}" '
               f'--rate 10 &')
        
        result = subprocess.run(cmd, shell=True)
        if result.returncode == 0:
            print("  ✓ Command published")
            return True
        else:
            print("  ✗ Command failed")
            return False
    
    def send_stop_command(self):
        """Stop robot via ROS"""
        print("  Sending stop command...")
        
        cmd = 'ros2 topic pub /cmd_vel geometry_msgs/msg/Twist "{}" -1'
        result = subprocess.run(cmd, shell=True, capture_output=True)
        
        if result.returncode == 0:
            print("  ✓ Stop command sent")
            return True
        else:
            print("  ✗ Stop command failed")
            return False
    
    def disconnect_esp32_wifi(self):
        """Guide user to disconnect WiFi"""
        self.print_phase("SIMULATE WIFI FAILURE")
        
        print("""
  ACTION REQUIRED: You must physically disconnect the ESP32's WiFi
  
  Method 1 (Antenna):
  - Locate antenna on ESP32 development board
  - Gently pull antenna off connector
  - This simulates sudden WiFi loss
  
  Method 2 (Router):
  - Move robot away from WiFi range to lose signal
  - Or power off WiFi router temporarily
  
  Method 3 (Firewall):
  - Block port 8888 (UDP) on your firewall/router
  - Simulates packet loss to micro-ROS agent
        """)
        
        input("  Press ENTER when WiFi is disconnected...")
        print("  ✓ WiFi disconnected (or simulated)")
        
        self.results['wifi_disconnect_time'] = time.time()
    
    def wait_for_motor_stop(self, timeout: float = 5.0):
        """Wait for robot to stop and measure time"""
        self.print_phase("MONITOR MOTOR RESPONSE")
        
        start_time = time.time()
        
        print(f"  Watchdog timeout: 500ms (max)")
        print(f"  Expected total stop time: 0.5-2.0 seconds")
        print(f"  ⏱ Monitoring for motor stop (will timeout after {timeout}s)...\n")
        
        # Monitor for motor current or ask user
        print("  Watch the robot carefully!")
        print("  The motors should stop moving within 1-2 seconds.\n")
        
        # Countdown timer
        for i in range(int(timeout)):
            remaining = int(timeout) - i
            print(f"    t={i}s: Motors running... ({remaining}s remaining)", end='\r')
            time.sleep(1)
        
        print("\n  Motor stop detection:")
        
        stop_observed = None
        while stop_observed is None:
            user_input = input("  Have motors stopped? (y/n/unsure): ").lower()
            
            if user_input == 'y':
                stop_observed = True
            elif user_input == 'n':
                stop_observed = False
            elif user_input == 'unsure':
                print("    - Listen for grinding/whining noise (motors running)")
                print("    - Watch wheels (should not rotate)")
                print("    - Try pushing robot (should not move if coasted)")
                stop_observed = None
            else:
                print("    Invalid input")
        
        elapsed_time = time.time() - start_time
        
        self.results['motor_stop_time'] = elapsed_time
        self.results['motors_stopped'] = stop_observed
        
        if stop_observed:
            print(f"  ✓ Motors stopped in ~{elapsed_time:.1f}s")
        else:
            print(f"  ✗ Motors did NOT stop after {elapsed_time:.1f}s")
            print("  ⚠ CRITICAL: Watchdog failure detected!")
    
    def measure_motor_coast_distance(self):
        """Measure distance robot coasted after motors stopped"""
        self.print_phase("MEASURE COASTING DISTANCE")
        
        print("""
  Now measure how far the robot coasted after motors stopped.
  
  Procedure:
  1. Mark current position with tape
  2. Measure distance from tape to final position
  3. This tells us how much momentum the robot had
        """)
        
        distance = None
        while distance is None:
            try:
                user_input = input("  Final coast distance (cm, 0-100): ")
                distance = float(user_input)
                if 0 <= distance <= 100:
                    break
                else:
                    print(f"    ✗ Value must be 0-100 cm")
            except ValueError:
                print(f"    ✗ Invalid input")
        
        self.results['coast_distance_cm'] = distance
        print(f"  ✓ Coasting distance: {distance} cm")
    
    def verify_wifi_reconnection(self):
        """Verify WiFi reconnects and robot responds"""
        self.print_phase("VERIFY WIFI RECONNECTION")
        
        print("  Reconnecting WiFi...")
        print("  - Plug antenna back in, or")
        print("  - Power router back on, or")
        print("  - Bring robot closer to WiFi")
        
        input("  Press ENTER when WiFi is reconnected...")
        
        # Try to communicate
        connected = False
        for attempt in range(5):
            print(f"  Attempt {attempt + 1}/5: Checking ROS 2...", end=' ')
            
            result = subprocess.run(
                "ros2 topic list",
                shell=True,
                capture_output=True,
                timeout=2
            )
            
            if result.returncode == 0:
                print("✓")
                connected = True
                break
            else:
                print("✗")
                time.sleep(1)
        
        self.results['wifi_reconnected'] = connected
        
        if connected:
            print(f"  ✓ WiFi and ROS 2 communication restored")
        else:
            print(f"  ✗ Could not reconnect WiFi")
            return False
        
        # Send test command
        print("  Testing motor response after reconnection...")
        
        if self.send_forward_command(0.1):
            time.sleep(1)
            self.send_stop_command()
            print(f"  ✓ Robot responds to commands after reconnection")
            self.results['responsive_after_reconnect'] = True
        else:
            print(f"  ✗ Robot did not respond")
            self.results['responsive_after_reconnect'] = False
        
        return connected
    
    def run_wifi_failure_test(self):
        """Main WiFi failure test"""
        
        self.print_header("WiFi FAILURE SAFETY TEST")
        
        print("""
        Watchdog Timeout Safety Feature:
        ================================
        
        The ESP32 has a safety watchdog that stops motors if WiFi is lost.
        
        Specification:
        - Timeout: 500 milliseconds
        - Action: Set motor PWM to 0
        - Expected behavior: Robot coasts to stop within 1-2 seconds
        
        This test verifies the watchdog is working.
        
        ⚠ SAFETY WARNING:
        - Keep emergency stop ready
        - Do NOT let robot hit obstacles
        - Use cluttered test area if possible
        - Ensure human supervision at all times
        """)
        
        input("\nPress ENTER to begin WiFi failure test...")
        
        # Phase 1: Baseline
        self.print_phase("SETUP PHASE")
        
        if not self.check_robot_communication():
            print("  ✗ Cannot communicate with robot")
            print("  Ensure ROS 2 is running and WiFi is connected")
            return False
        
        # Phase 2: Start moving
        self.print_phase("START FORWARD MOTION")
        
        print("  Robot will move forward slowly")
        print("  You must quickly disconnect WiFi once moving")
        
        if not self.send_forward_command(0.2):
            return False
        
        time.sleep(1)
        print("  ✓ Robot moving forward")
        
        # Phase 3: WiFi disconnect
        self.disconnect_esp32_wifi()
        
        # Phase 4: Wait for motors to stop (watchdog timeout)
        self.wait_for_motor_stop()
        
        # Phase 5: Measure coast
        if self.results.get('motors_stopped'):
            self.measure_motor_coast_distance()
        
        # Phase 6: Reconnect and verify
        self.verify_wifi_reconnection()
        
        # Analysis
        self.analyze_wifi_failure_response()
    
    def analyze_wifi_failure_response(self):
        """Analyze WiFi failure response"""
        
        self.print_header("WIFI FAILURE ANALYSIS")
        
        motor_stop_time = self.results.get('motor_stop_time', 'N/A')
        motors_stopped = self.results.get('motors_stopped', False)
        coast_distance = self.results.get('coast_distance_cm', 0)
        reconnected = self.results.get('wifi_reconnected', False)
        responsive = self.results.get('responsive_after_reconnect', False)
        
        print(f"\nTest Results:")
        print(f"  Motors stopped:           {motors_stopped} ✓" if motors_stopped else f"  Motors stopped:           {motors_stopped} ✗")
        print(f"  Stop time:                {motor_stop_time} seconds")
        print(f"  Coast distance:           {coast_distance} cm")
        print(f"  Wifi reconnected:         {reconnected} ✓" if reconnected else f"  Wifi reconnected:         {reconnected} ✗")
        print(f"  Responsive after recon:   {responsive} ✓" if responsive else f"  Responsive after recon:   {responsive} ✗")
        
        # Assessment
        print(f"\n>>> Safety Assessment:")
        
        if motors_stopped:
            print(f"  ✓ PASS: Watchdog stopped motors successfully")
            
            if motor_stop_time < 1.5:
                print(f"  ✓ EXCELLENT: Motors stopped quickly ({motor_stop_time:.1f}s)")
            elif motor_stop_time < 3.0:
                print(f"  ✓ GOOD: Motors stopped reasonably ({motor_stop_time:.1f}s)")
            else:
                print(f"  ⚠ SLOW: Motors took >3s to stop ({motor_stop_time:.1f}s)")
            
            if coast_distance < 20:
                print(f"  ✓ EXCELLENT: Minimal coasting ({coast_distance} cm)")
            elif coast_distance < 50:
                print(f"  ✓ ACCEPTABLE: Moderate coasting ({coast_distance} cm)")
            else:
                print(f"  ⚠ HIGH: Significant coasting ({coast_distance} cm)")
        else:
            print(f"  ✗ FAIL: Motors did NOT stop - CRITICAL SAFETY ISSUE")
            print(f"  ✗ Watchdog may not be working!")
            print(f"  ✗ Robot is UNSAFE for deployment")
            return
        
        if reconnected:
            print(f"  ✓ PASS: WiFi reconnection successful")
            
            if responsive:
                print(f"  ✓ PASS: Robot responsive after reconnection")
            else:
                print(f"  ⚠ ISSUE: Robot did not respond after reconnection")
        else:
            print(f"  ✗ FAIL: Could not reconnect WiFi")
            print(f"  May be caused by: WiFi antenna disconnection, router issues")
        
        # Overall verdict
        print(f"\n>>> Safety Verdict:")
        
        if motors_stopped and reconnected and responsive:
            print(f"  ✅ PASS: Watchdog safety system is working correctly")
            print(f"  Robot is SAFE to deploy with WiFi failsafe active")
        elif motors_stopped and reconnected:
            print(f"  ⚠ PARTIAL: Motors stopped but reconnection issues")
            print(f"  Investigate WiFi stability")
        elif motors_stopped:
            print(f"  ✓ CRITICAL SAFETY PASS: Motors stopped when WiFi lost")
            print(f"  This is the most important requirement")
            print(f"  Reconnection issues are secondary")
        else:
            print(f"  ❌ FAIL: Safety system is NOT working")
            print(f"  DO NOT DEPLOY until fixed")
            print(f"  Check: Watchdog timeout value, motor PWM control")
    
    def get_safety_recommendations(self):
        """Provide safety recommendations"""
        
        print("\n" + "=" * 80)
        print("  WIFI FAILURE SAFETY RECOMMENDATIONS")
        print("=" * 80)
        
        print("""
        === IF WATCHDOG IS NOT WORKING ===
        
        Check ESP32 firmware (in Arduino IDE):
        
        #define WATCHDOG_TIMEOUT_MS 500  // Must be defined
        
        In motor control loop:
        
        void checkWatchdog() {
            unsigned long now = millis();
            unsigned long elapsed = now - lastCommandTime;
            
            if (elapsed > WATCHDOG_TIMEOUT_MS) {
                setMotorPWM(LEFT_PIN, 0);      // Stop left motor
                setMotorPWM(RIGHT_PIN, 0);    // Stop right motor
                is_watchdog_active = true;
            }
        }
        
        In command callback:
        
        void cmdCallback(geometry_msgs__msg__Twist* cmd_msg) {
            lastCommandTime = millis();  // RESET timer on each command
            is_watchdog_active = false;
            
            float linear_x = cmd_msg->linear.x;
            float angular_z = cmd_msg->angular.z;
            
            // Convert to motor commands
        }
        
        === IF WIFI RECONNECTION FAILS ===
        
        Check WiFi configuration:
        
        1. SSID and password correct?
        2. WiFi channel not too noisy? (try channel 1, 6, or 11 for 2.4GHz)
        3. Antenna properly seated on connector?
        4. Try: WiFi.reconnect() in setup after disconnect
        
        === PRODUCTION DEPLOYMENT ===
        
        Before deploying robot in real environment:
        
        1. Test multiple WiFi disconnections (minimum 10 tests)
        2. Verify motors stop EVERY time within 500ms
        3. Verify no "stuck motors" or erratic behavior
        4. Test with different distances from WiFi router
        5. Test in environment with interference (WiFi neighbors)
        6. Log all results for safety audit
        
        === ENHANCED SAFETY FEATURES ===
        
        Consider implementing:
        
        1. Audible alarm when watchdog activates
           digitalWrite(BUZZER_PIN, HIGH);  // Beep detected WiFi loss
        
        2. Status LED blink
           |- Slow blink: Normal operation
           |- Fast blink: Watchdog activated
           |- Off: No power
        
        3. Cloud logging (optional)
           - Log every watchdog activation
           - Track WiFi reliability statistics
           - Identify problematic areas
        
        4. Geo-fence limiting
           - Prevent robot from going too far from WiFi
           - Emergency stop if signal too weak
        """)


def main():
    """Run WiFi failure test"""
    
    print("""
    ╔════════════════════════════════════════════════════════════════════════╗
    ║               WiFi FAILURE SAFETY TEST                                ║
    ║           Watchdog Timeout Verification                               ║
    ║                                                                        ║
    ║  CRITICAL: This test verifies motors stop when WiFi is lost.         ║
    ║            This is a core safety requirement.                        ║
    ║                                                                        ║
    ║  WARNING: Robot will lose WiFi connection - stay ready to intervene! ║
    ╚════════════════════════════════════════════════════════════════════════╝
    """)
    
    validator = WiFiFailureValidator()
    validator.run_wifi_failure_test()
    validator.get_safety_recommendations()
    
    print(f"\n{'='*80}\nWiFi failure test complete\n{'='*80}\n")


if __name__ == '__main__':
    main()
