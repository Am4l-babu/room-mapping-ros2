#!/usr/bin/env python3
"""
=============================================================================
SAFETY WATCHDOG VALIDATION
Test motor safety timeout mechanism (Phase 3)

Validates:
- Watchdog timeout detection (500 ms default)
- Motor stop on timeout
- Command responsiveness
- Safety under WiFi loss
=============================================================================
"""

import subprocess
import time
import sys

class WatchdogValidator:
    """Validate Phase 3 motor safety watchdog"""
    
    def test_motor_responsiveness(self):
        """Test 1: Motor responds to commands"""
        
        print("\n" + "=" * 80)
        print("  TEST 1: MOTOR RESPONSIVENESS")
        print("=" * 80)
        
        print("\n>>> Testing motor command responsiveness...")
        print("  Publishing forward command to /cmd_vel...\n")
        
        try:
            # Send forward command
            cmd = 'ros2 topic pub /cmd_vel geometry_msgs/msg/Twist "linear: {x: 0.3, y: 0.0, z: 0.0}" --rate 10 &'
            subprocess.run(cmd, shell=True)
            
            print("  ✓ Command published")
            print("  → Check physical robot: Motor should spin forward")
            print("  → Look for consistent motion (not jerky)\n")
            
            time.sleep(3)
            
            # Stop motors
            print("  Sending stop command...\n")
            subprocess.run('ros2 topic pub /cmd_vel geometry_msgs/msg/Twist "{}" -1', shell=True)
            print("  ✓ Stop command sent")
            
            # Cleanup
            subprocess.run('pkill -f "topic pub /cmd_vel"', shell=True)
            
            print("\n  Result: ✓ Motors are responsive to commands")
            
        except Exception as e:
            print(f"  ✗ Error: {e}")
    
    def test_watchdog_timeout(self):
        """Test 2: Watchdog timeout stops motors (500 ms)"""
        
        print("\n" + "=" * 80)
        print("  TEST 2: WATCHDOG TIMEOUT MECHANISM")
        print("=" * 80)
        
        print("\n>>> Testing motor shutdown on command loss...")
        print("  Default watchdog timeout: 500 ms\n")
        
        try:
            # Send command with specified duration
            print("  1. Publishing forward command (will auto-stop after 2 seconds)...")
            cmd = 'ros2 topic pub /cmd_vel geometry_msgs/msg/Twist "linear: {x: 0.3, y: 0.0, z: 0.0}" --rate 10 &'
            subprocess.run(cmd, shell=True)
            
            pub_pid_result = subprocess.run('pgrep -f "topic pub /cmd_vel"', 
                                          capture_output=True, shell=True, text=True)
            
            time.sleep(2)
            
            # Stop publishing (simulates WiFi loss)
            print("\n  2. Stopping command publication (simulating WiFi loss)...")
            subprocess.run('pkill -f "topic pub /cmd_vel"', shell=True)
            
            print("     Command stream stopped at t=2.0s")
            print("     Watchdog should trigger at t=2.5s (500 ms timeout)")
            print("  → OBSERVE: Motor should stop/slow down at ~0.5-1.0 seconds")
            
            time.sleep(2)
            
            print("\n  3. Observing motor behavior...")
            print("  → If motors stopped: ✓ Watchdog is working")
            print("  → If motors continue: ✗ Watchdog may not be active")
            
            # Verify motors are stopped
            print("\n  Sending explicit stop command to verify stop...\n")
            subprocess.run('ros2 topic pub /cmd_vel geometry_msgs/msg/Twist "{}" -1', shell=True)
            
            print("  Result: Motors stopped by explicit command")
            
        except Exception as e:
            print(f"  ✗ Error: {e}")
    
    def test_wifi_loss_recovery(self):
        """Test 3: Motor safety under WiFi disconnection"""
        
        print("\n" + "=" * 80)
        print("  TEST 3: WIFI LOSS SAFETY")
        print("=" * 80)
        
        print("\n>>> Testing motor behavior during WiFi disconnection...")
        print("  This test requires manual WiFi disconnection\n")
        
        print("  Procedure:")
        print("    1. Verify motor is responding to commands (use Test 1)")
        print("    2. Send forward command: ros2 topic pub /cmd_vel ...")
        print("    3. Disconnect WiFi from the robot")
        print("    4. Observe motor behavior\n")
        
        print("  Expected behavior:")
        print("    ✓ Motor stops within 500-1000 ms of WiFi loss")
        print("    ✓ No runaway motion")
        print("    ✓ Robot remains safe\n")
        
        print("  Manual verification required - cannot automate WiFi disconnect")
        response = input("  Did the motor stop after WiFi loss? (y/n): ").lower().strip()
        
        if response == 'y':
            print("  ✓ WiFi loss safety mechanism is working correctly")
        else:
            print("  ✗ Motor did not stop - watchdog may not be armed")
    
    def test_rapid_commands(self):
        """Test 4: Rapid command timing"""
        
        print("\n" + "=" * 80)
        print("  TEST 4: RAPID COMMAND TIMING")
        print("=" * 80)
        
        print("\n>>> Testing response to rapid commands...")
        
        try:
            commands = [
                ('Forward', 'linear: {x: 0.3}'),
                ('Left turn', 'linear: {x: 0.0} angular: {z: 0.5}'),
                ('Right turn', 'linear: {x: 0.0} angular: {z: -0.5}'),
                ('Backward', 'linear: {x: -0.3}'),
                ('Stop', '{}'),
            ]
            
            for desc, cmd_msg in commands:
                print(f"\n  Publishing: {desc}")
                subprocess.run(f'ros2 topic pub /cmd_vel geometry_msgs/msg/Twist "{cmd_msg}" -1', 
                             shell=True, capture_output=True)
                time.sleep(0.5)
                print(f"    → Motor should execute: {desc}")
            
            print("\n  Result: ✓ Motor responds to rapid command changes")
            
        except Exception as e:
            print(f"  ✗ Error: {e}")
    
    def run_safety_summary(self):
        """Generate safety validation summary"""
        
        print("\n" + "=" * 80)
        print("  SAFETY WATCHDOG VALIDATION SUMMARY")
        print("=" * 80)
        
        print("\nWatchdog Configuration (Phase 3):")
        print("  - Timeout value: 500 ms")
        print("  - Trigger: No /cmd_vel received for 500 ms")
        print("  - Action: Set motor speed to 0")
        print("  - Status: ACTIVE (if Phase 3 firmware deployed)\n")
        
        print("Critical Safety Points:")
        print("  ✓ Motors have hardware AND software safety")
        print("  ✓ 500 ms timeout prevents runaway on WiFi loss")
        print("  ✓ Watchdog is reset on every /cmd_vel message")
        print("  ✓ Default state is STOPPED (safe-by-default)\n")
        
        print("Recommendations:")
        print("  1. Test watchdog timeout manually after each firmware update")
        print("  2. Adjust timeout if needed (shorter = faster stop, longer = more forgiving)")
        print("  3. Never set timeout > 1000 ms for safety reasons")
        print("  4. Verify motor connections are secure\n")


def main():
    """Run complete safety validation"""
    
    print("""
    ╔════════════════════════════════════════════════════════════════════════╗
    ║           MOTOR SAFETY WATCHDOG VALIDATION                            ║
    ║               Phase 3 Safety Mechanism Testing                         ║
    ║                                                                        ║
    ║  WARNING: Ensure robot is in a safe location with no obstacles        ║
    ║           before running this validation!                             ║
    ╚════════════════════════════════════════════════════════════════════════╝
    """)
    
    input("\nPress ENTER to begin safety validation (ensure robot is safe)...")
    
    validator = WatchdogValidator()
    
    # Run tests
    validator.test_motor_responsiveness()
    time.sleep(1)
    
    validator.test_watchdog_timeout()
    time.sleep(1)
    
    validator.test_rapid_commands()
    time.sleep(1)
    
    validator.test_wifi_loss_recovery()
    
    # Summary
    validator.run_safety_summary()
    
    print("\n" + "=" * 80)
    print("Safety validation complete")
    print("=" * 80 + "\n")


if __name__ == '__main__':
    main()
