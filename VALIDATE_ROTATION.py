#!/usr/bin/env python3
"""
=============================================================================
PHYSICAL VALIDATION: ROTATION TEST
Measure angular accuracy and rotation stability

Procedure:
1. Mark center point on floor
2. Place robot at center, mark initial orientation (e.g., tape)
3. Command pure rotation (angular velocity only, no linear)
4. Rotate 90°, 180°, 270°, 360°
5. Measure final angle vs expected
6. Check for overshoot and oscillation

Rotation errors (in order of likelihood):
- Encoder/IMU integration error
- Gyroscope drift (IMU chip calibration)
- Tire slippage during rotation
- PID overshoot on angular velocity
- Asymmetric angular stiffness (friction varies by angle)
=============================================================================
"""

import subprocess
import time
import math
import statistics
from collections import deque

class RotationValidator:
    """Validate rotation accuracy and stability"""
    
    def __init__(self):
        self.rotation_results = []
        
    def print_header(self, title: str):
        """Print test header"""
        print("\n" + "=" * 80)
        print(f"  {title}")
        print("=" * 80)
    
    def publish_rotation_command(self, angular_velocity: float, 
                                 target_angle: float):
        """
        Publish rotation command for specified angle
        
        Args:
            angular_velocity: rad/s (typically 0.25-0.5 for testing)
            target_angle: degrees to rotate (90, 180, 270, 360)
        """
        angle_rad = math.radians(target_angle)
        expected_time = angle_rad / angular_velocity + 0.5  # Add buffer
        
        print(f"\n>>> Publishing rotation command:")
        print(f"    Angular velocity: {angular_velocity:.2f} rad/s")
        print(f"    Target angle:     {target_angle}°")
        print(f"    Expected time:    {expected_time:.1f}s\n")
        
        print("  IMPORTANT: Center robot and mark current heading!")
        print("  Robot will rotate in place around vertical axis.\n")
        
        input("  Position robot at CENTER POINT and press ENTER...")
        print("  Starting rotation in 2 seconds...")
        time.sleep(2)
        
        # Build ROS command (counterclockwise positive)
        cmd = (f'ros2 topic pub /cmd_vel geometry_msgs/msg/Twist '
               f'"linear: {{x: 0.0, y: 0.0, z: 0.0}} '
               f'angular: {{x: 0.0, y: 0.0, z: {angular_velocity}}}" '
               f'--rate 10 &')
        
        subprocess.run(cmd, shell=True)
        pub_pid = subprocess.run('pgrep -f "topic pub /cmd_vel"', 
                                capture_output=True, shell=True, text=True)
        
        print(f"  ✓ Rotation command published")
        print(f"  ⏱ Rotation started at t=0s\n")
        
        # Run test
        for i in range(int(expected_time)):
            elapsed = i + 1
            remaining = int(expected_time) - elapsed
            print(f"    t={elapsed}s: Rotating... ({remaining}s remaining)", end='\r')
            time.sleep(1)
        
        print(f"\n  ✓ Rotation complete at t={int(expected_time)}s")
        
        # Stop motors
        subprocess.run('pkill -f "topic pub /cmd_vel"', shell=True)
        time.sleep(0.5)
        subprocess.run('ros2 topic pub /cmd_vel geometry_msgs/msg/Twist "{}" -1',
                      shell=True, capture_output=True)
        
        print(f"  ✓ Motors stopped")
    
    def rotation_test(self):
        """Main rotation accuracy test"""
        
        self.print_header("ROTATION ACCURACY TEST")
        
        print("""
        Test Setup:
        ===========
        
        On floor, prepare test location:
        
        1. Mark a CENTER POINT (tape cross)
        2. Draw reference direction line from center
           (e.g., straight ahead)
        3. Mark 90°, 180°, 270°, 360° lines around center circle
        
        Top View:
                      0° (start)
                        ↑
                        |
        270° ← × (0,0) → 90°
                        |
                        ↓
                      180°
        
        Robot placement:
        - Center on center point (wheels won't move much)
        - Initial heading aligned with 0° line
        - Use tape/marker to mark initial heading line on robot
        """)
        
        input("\n  Press ENTER when test area is ready...")
        
        # Test different angular velocities and angles
        test_configs = [
            (0.25, 90),
            (0.25, 180),
            (0.25, 360),
            (0.4, 90),
            (0.4, 180),
        ]
        
        for ang_vel, target_angle in test_configs:
            print(f"\n\n>>> TEST: Angular velocity = {ang_vel} rad/s, Target = {target_angle}°")
            
            self.publish_rotation_command(ang_vel, target_angle)
            
            # Measure actual angle
            print("\n>>> MEASUREMENT PHASE")
            
            while True:
                try:
                    actual_angle = float(input(f"  Actual final angle (degrees, 0-360): "))
                    if 0 <= actual_angle <= 360:
                        break
                    else:
                        print("    ✗ Value must be 0-360 degrees")
                except ValueError:
                    print("    ✗ Invalid input, try again")
            
            # Calculate error
            error_angle = actual_angle - target_angle
            
            # Normalize error to ±180
            if error_angle > 180:
                error_angle = error_angle - 360
            elif error_angle < -180:
                error_angle = error_angle + 360
            
            # Record time (manual estimate from test)
            while True:
                try:
                    settle_time = float(input(f"  Time to settle (seconds, 0-5): "))
                    if 0 <= settle_time <= 5:
                        break
                    else:
                        print("    ✗ Value must be 0-5 seconds")
                except ValueError:
                    print("    ✗ Invalid input, try again")
            
            result = {
                'angular_velocity': ang_vel,
                'target_angle': target_angle,
                'actual_angle': actual_angle,
                'error': error_angle,
                'error_pct': abs(error_angle) / target_angle * 100 if target_angle > 0 else 0,
                'settle_time': settle_time,
            }
            
            self.rotation_results.append(result)
            
            print(f"\n  Results:")
            print(f"    Target:        {target_angle:6.0f}°")
            print(f"    Actual:        {actual_angle:6.1f}°")
            print(f"    Error:         {error_angle:6.1f}° ({result['error_pct']:5.1f}%)")
            print(f"    Settle time:   {settle_time:6.1f}s")
            
            time.sleep(1)
        
        # Analysis
        self.analyze_rotation_performance()
    
    def analyze_rotation_performance(self):
        """Analyze rotation accuracy and identify issues"""
        
        self.print_header("ROTATION PERFORMANCE ANALYSIS")
        
        if not self.rotation_results:
            print("  ✗ No measurements available")
            return
        
        # Calculate averages
        errors = [abs(r['error']) for r in self.rotation_results]
        settle_times = [r['settle_time'] for r in self.rotation_results]
        
        avg_error = statistics.mean(errors)
        max_error = max(errors)
        avg_settle_time = statistics.mean(settle_times)
        max_settle_time = max(settle_times)
        
        print(f"\nRotation Performance Summary:")
        print(f"  Tests performed: {len(self.rotation_results)}")
        print(f"  Average error:   {avg_error:6.1f}°")
        print(f"  Max error:       {max_error:6.1f}°")
        print(f"  Avg settle time: {avg_settle_time:6.1f}s")
        print(f"  Max settle time: {max_settle_time:6.1f}s")
        
        # Analyze error patterns
        print(f"\nDetailed Results:")
        print(f"  {'Vel':>5} {'Target':>7} {'Actual':>7} {'Error':>7} {'Settle':>7}")
        print(f"  {'-'*5} {'-'*7} {'-'*7} {'-'*7} {'-'*7}")
        
        for r in self.rotation_results:
            print(f"  {r['angular_velocity']:>5.2f} "
                  f"{r['target_angle']:>7.0f}° "
                  f"{r['actual_angle']:>7.1f}° "
                  f"{r['error']:>7.1f}° "
                  f"{r['settle_time']:>7.2f}s")
        
        # Diagnosis
        print(f"\n>>> Diagnosis:")
        
        # Check if error systematic
        positive_errors = [r['error'] for r in self.rotation_results if r['error'] > 0]
        negative_errors = [r['error'] for r in self.rotation_results if r['error'] < 0]
        
        if len(positive_errors) > len(negative_errors) * 1.5:
            print(f"  Pattern: Systematic OVERSHOOT")
            print(f"  Likely cause: PID gain too high or gyro bias")
            print(f"  Fix: DECREASE angular PID gains (Kp) or recalibrate IMU")
        elif len(negative_errors) > len(positive_errors) * 1.5:
            print(f"  Pattern: Systematic UNDERSHOOT")
            print(f"  Likely cause: PID gain too low")
            print(f"  Fix: INCREASE angular PID gains (Kp)")
        else:
            print(f"  Pattern: Random error")
            print(f"  Likely cause: Gyroscope drift or random slippage")
        
        # Severity assessment
        print(f"\n>>> Severity Assessment:")
        
        if max_error < 3:
            print(f"  ✓ EXCELLENT: Max error <3° (very accurate rotation)")
        elif max_error < 5:
            print(f"  ✓ GOOD: Max error <5° (acceptable for most tasks)")
        elif max_error < 10:
            print(f"  ⚠ ACCEPTABLE: Max error <10° (needs monitoring)")
        else:
            print(f"  ✗ POOR: Max error >{max_error}° (critical, requires tuning)")
        
        # Overshoot check
        if avg_settle_time > 1.5:
            print(f"\n  ⚠ SLOW SETTLING: {avg_settle_time:.1f}s average")
            print(f"    May indicate PID oscillation or high friction")
        
        # Look for velocity dependence
        print(f"\n>>> Velocity Dependence:")
        
        vel_025_errors = [abs(r['error']) for r in self.rotation_results 
                         if r['angular_velocity'] == 0.25]
        vel_040_errors = [abs(r['error']) for r in self.rotation_results 
                         if r['angular_velocity'] == 0.4]
        
        if vel_025_errors and vel_040_errors:
            avg_025 = statistics.mean(vel_025_errors)
            avg_040 = statistics.mean(vel_040_errors)
            
            print(f"  0.25 rad/s: {avg_025:.2f}° error")
            print(f"  0.40 rad/s: {avg_040:.2f}° error")
            
            if avg_040 > avg_025 * 1.5:
                print(f"  → Error increases with speed: May indicate gyro saturation")
            elif avg_040 < avg_025 * 0.5:
                print(f"  → Error decreases with speed: May indicate static friction")
    
    def get_tuning_recommendations(self):
        """Provide rotation tuning recommendations"""
        
        print("\n" + "=" * 80)
        print("  ROTATION TUNING RECOMMENDATIONS")
        print("=" * 80)
        
        print("""
        === GYROSCOPE-BASED ROTATION ===
        
        If using IMU gyroscope for rotation control:
        
        1. Gyro Calibration:
           - Ensure IMU is calibrated at startup
           - Leave robot stationary for 2-3 seconds at power-on
           - Gyro bias should be near zero after calibration
        
        2. Gyro Drift:
           - Gyros drift over time (typically 1°-5° per minute)
           - Not caused by PID - inherent to MEMS sensors
           - Mitigation: Use EKF to fuse gyro with other sensors
           - Reset yaw periodically if not using SLAM
        
        === ENCODER-BASED ROTATION ===
        
        If using motor encoders (differential drive):
        
        1. Encoder Count Mismatch:
           - Rotate in place, measure encoder counts (left vs right)
           - If counts differ: Recalibrate or adjust motor PWM
           - Formula: Target_PWM_right = (counts_left/counts_right) * current_PWM
        
        2. Wheel Slip:
           - Tires may slip during high-speed rotation
           - Causes undershoot
           - Fix: Lower rotation speed limit
        
        === PID TUNING FOR ROTATION ===
        
        Angular velocity controller:
        
        #define ANGULAR_KP  0.5  // Proportional gain
        #define ANGULAR_KI  0.05 // Integral gain
        #define ANGULAR_KD  0.1  // Derivative gain
        
        Tuning steps:
        
        1. Start with conservative gains:
           Set KP=0.1, KI=0, KD=0
        
        2. Increase KP until oscillation starts:
           Increase by 0.1 increments
           Run 90° rotation test
           When oscillation appears, back off 20%
        
        3. Add derivative to reduce oscillation:
           Start with KD = 0.1
           Increase by 0.05 if still oscillating
        
        4. Verify no steady-state error:
           If consistently undershoots/overshoots:
           Increase KI to 0.01 or 0.05
        
        Symptoms & Fixes:
        
        Oscillation (overshoot):
        ├─ Too slow to detect: INCREASE KP
        ├─ Overshoots then corrects: INCREASE KD
        └─ Steady oscillation: DECREASE KP
        
        Undershoot (stops before target):
        ├─ Not enough gain: INCREASE KP
        ├─ Friction too high: Use higher velocity setpoint
        └─ Encoder mismatch: Recalibrate encoder counts
        
        Slow settling:
        ├─ High friction: Check wheel condition
        ├─ Low gain: INCREASE KP
        └─ Damping too high: DECREASE KD
        
        === TEST PROCEDURE AFTER TUNING ===
        
        1. Run 90° rotation test with new gains
        2. Measure overshoot and settle time
        3. Run 180° and 360° tests for consistency
        4. Check if error depends on angular velocity
        5. Verify no oscillation in final heading
        
        === MECHANICAL CHECKS ===
        
        1. Tire Condition:
           - Inspect for wear, debris
           - Check pressure is equal both wheels
           - Test by pushing robot: should roll straight
        
        2. Bearing Friction:
           - Spin each wheel freely
           - Should coast smoothly
           - If sticky: Check for debris, may need lubrication
        
        3. Chassis Alignment:
           - Both wheels should be same radius ±1mm
           - Axles should be parallel and perpendicular to chassis
           - Measure diagonals: should be equal within 2mm
        """)


def main():
    """Run rotation validation"""
    
    print("""
    ╔════════════════════════════════════════════════════════════════════════╗
    ║              ROTATION ACCURACY VALIDATION                             ║
    ║                  Angular Motion Testing                               ║
    ║                                                                        ║
    ║  WARNING: Ensure robot is centered and won't collide!                 ║
    ║           Keep emergency stop ready during testing.                   ║
    ╚════════════════════════════════════════════════════════════════════════╝
    """)
    
    validator = RotationValidator()
    validator.rotation_test()
    validator.get_tuning_recommendations()
    
    print(f"\n{'='*80}\nRotation validation complete\n{'='*80}\n")


if __name__ == '__main__':
    main()
