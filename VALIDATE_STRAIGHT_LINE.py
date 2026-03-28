#!/usr/bin/env python3
"""
=============================================================================
PHYSICAL VALIDATION: STRAIGHT-LINE MOTION TEST
Measure lateral drift and identify causes

Procedure:
1. Mark 3-meter straight line on floor (tape)
2. Place robot at start, aligned with line
3. Command constant forward velocity
4. Measure lateral deviation every 0.5m
5. Repeat 3 times for statistical analysis

Drift causes (in order of likelihood):
- Wheel/encoder mismatch (differential motor responses)
- Uneven tire pressure or wear
- Mechanical misalignment (chassis not square)
- PID gain imbalance (left/right motor gains differ)
- Encoder calibration error
=============================================================================
"""

import subprocess
import time
import math
import statistics
from collections import deque

class StraightLineValidator:
    """Validate straight-line motion accuracy"""
    
    def __init__(self):
        self.drift_measurements = []
        self.test_results = {}
        
    def print_header(self, title: str):
        """Print test header"""
        print("\n" + "=" * 80)
        print(f"  {title}")
        print("=" * 80)
    
    def publish_motor_command(self, linear_velocity: float, duration_sec: float):
        """
        Publish constant velocity command for specified duration
        
        Args:
            linear_velocity: m/s (typically 0.2-0.3 for testing)
            duration_sec: how long to run test
        """
        print(f"\n>>> Publishing forward command: {linear_velocity} m/s for {duration_sec}s\n")
        print("  IMPORTANT: Ensure test area is clear of obstacles!")
        print("  Robot will move forward in a straight line.\n")
        
        # Give user time to position robot
        input("  Position robot at START LINE and press ENTER...")
        print("  Starting motion in 2 seconds...")
        time.sleep(2)
        
        # Build ROS command
        cmd = (f'ros2 topic pub /cmd_vel geometry_msgs/msg/Twist '
               f'"linear: {{x: {linear_velocity}, y: 0.0, z: 0.0}}" '
               f'--rate 10 &')
        
        subprocess.run(cmd, shell=True)
        pub_pid = subprocess.run('pgrep -f "topic pub /cmd_vel"', 
                                capture_output=True, shell=True, text=True)
        
        print(f"  ✓ Motor command published (PID: {pub_pid.stdout.strip()})")
        print(f"  ⏱ Motion started at t=0s\n")
        
        # Run test
        for i in range(int(duration_sec)):
            elapsed = i + 1
            remaining = int(duration_sec) - elapsed
            print(f"    t={elapsed}s: Still moving... ({remaining}s remaining)", end='\r')
            time.sleep(1)
        
        print(f"\n  ✓ Motion complete at t={int(duration_sec)}s")
        
        # Stop motors
        subprocess.run('pkill -f "topic pub /cmd_vel"', shell=True)
        time.sleep(0.5)
        subprocess.run('ros2 topic pub /cmd_vel geometry_msgs/msg/Twist "{}" -1',
                      shell=True, capture_output=True)
        
        print(f"  ✓ Motors stopped")
    
    def straight_line_test(self):
        """
        Main straight-line motion test
        
        Setup:
        - Mark 3-meter line on floor with tape
        - Lines every 0.5m for measurement points
        """
        self.print_header("STRAIGHT-LINE MOTION TEST")
        
        print("""
        Test Setup:
        ===========
        
        On floor, create test track:
        
        START → | 0.5m | 1.0m | 1.5m | 2.0m | 2.5m | 3.0m | END
                 ↓      ↓      ↓      ↓      ↓      ↓      ↓
        
        Use tape to mark:
        1. Straight center line (3 meters)
        2. Perpendicular lines at 0.5m intervals for measuring lateral offset
        
        Robot placement:
        - Back wheels aligned with START line
        - Robot body centered on center line
        - Facing END
        
        Test execution:
        - Command forward motion at controlled velocity
        - Measure lateral offset at each 0.5m mark
        - Record along with encoder/IMU data
        """)
        
        input("\n  Press ENTER when test track is ready...")
        
        # Run test at multiple velocities
        velocities = [0.2, 0.3, 0.4]
        
        for velocity in velocities:
            print(f"\n\n>>> TEST: Forward velocity = {velocity} m/s")
            print(f"    Distance = 3 meters")
            print(f"    Expected time = {3.0/velocity:.1f} seconds\n")
            
            # Calculate expected time and run
            expected_time = 3.0 / velocity + 1  # Add 1s buffer
            self.publish_motor_command(velocity, expected_time)
            
            # Measure drift
            print("\n>>> MEASUREMENT PHASE")
            print("  Stand at each line marker and measure lateral offset:\n")
            
            offsets = []
            for distance in [0.5, 1.0, 1.5, 2.0, 2.5, 3.0]:
                while True:
                    try:
                        offset_cm = float(input(f"  Lateral offset at {distance}m (cm): "))
                        if -30 < offset_cm < 30:  # Sanity check
                            offsets.append(offset_cm / 100)  # Convert to meters
                            break
                        else:
                            print("    ✗ Value outside reasonable range (-30 to +30 cm)")
                    except ValueError:
                        print("    ✗ Invalid input, try again")
            
            # Analyze drift
            if offsets:
                stats = {
                    'velocity': velocity,
                    'offsets': offsets,
                    'max_drift': max(offsets),
                    'mean_drift': statistics.mean(offsets),
                    'std_dev': statistics.stdev(offsets) if len(offsets) > 1 else 0,
                }
                
                # Calculate drift rate (cm per meter)
                drift_rate = stats['max_drift'] * 100 / 3.0  # cm per meter traveled
                
                print(f"\n  Results for {velocity} m/s:")
                print(f"    Max lateral drift:   {stats['max_drift']*100:6.1f} cm")
                print(f"    Mean lateral drift:  {stats['mean_drift']*100:6.1f} cm")
                print(f"    Std deviation:       {stats['std_dev']*100:6.1f} cm")
                print(f"    Drift rate:          {drift_rate:6.2f} cm/meter")
                
                self.test_results[velocity] = stats
                self.drift_measurements.append(stats)
            
            time.sleep(1)  # Rest between tests
        
        # Summary analysis
        self.analyze_drift()
    
    def analyze_drift(self):
        """Analyze drift patterns and identify root cause"""
        
        print("\n" + "=" * 80)
        print("  DRIFT ANALYSIS & ROOT CAUSE IDENTIFICATION")
        print("=" * 80)
        
        if not self.drift_measurements:
            print("  ✗ No measurements available")
            return
        
        print(f"\n>>> Drift Summary:")
        print(f"    Tests performed: {len(self.drift_measurements)}")
        
        all_drifts = []
        for result in self.drift_measurements:
            all_drifts.extend(result['offsets'])
        
        overall_max = max(all_drifts) * 100  # cm
        overall_mean = statistics.mean(all_drifts) * 100
        
        print(f"    Maximum drift (3m):  {overall_max:6.1f} cm")
        print(f"    Mean drift (3m):     {overall_mean:6.1f} cm")
        print(f"    Drift rate:          {overall_max/3.0:6.2f} cm per meter")
        
        # Diagnose root cause
        print(f"\n>>> Root Cause Analysis:")
        
        # Pattern analysis
        drifts_by_distance = {}
        for result in self.drift_measurements:
            for i, dist in enumerate([0.5, 1.0, 1.5, 2.0, 2.5, 3.0]):
                if dist not in drifts_by_distance:
                    drifts_by_distance[dist] = []
                drifts_by_distance[dist].append(result['offsets'][i])
        
        # Check if drift increases linearly (encoder mismatch) or accelerates
        drift_growth = []
        for dist in sorted(drifts_by_distance.keys()):
            avg_drift = statistics.mean(drifts_by_distance[dist])
            drift_growth.append((dist, avg_drift))
        
        print(f"\n  Drift Growth Pattern:")
        for dist, drift in drift_growth:
            print(f"    {dist:3.1f}m: {drift*100:6.1f} cm")
        
        # Determine if linear or accelerating
        if len(drift_growth) >= 2:
            growth_rates = []
            for i in range(1, len(drift_growth)):
                rate = (drift_growth[i][1] - drift_growth[i-1][1]) / \
                       (drift_growth[i][0] - drift_growth[i-1][0])
                growth_rates.append(rate)
            
            avg_growth_rate = statistics.mean(growth_rates)
            
            print(f"\n  Growth Pattern Analysis:")
            print(f"    Average growth rate: {avg_growth_rate*100:.2f} cm/meter")
            
            if abs(avg_growth_rate) < 0.02:  # Less than 2cm/meter²
                print(f"    Pattern: STEADY/LINEAR drift")
                print(f"    Likely cause: Encoder mismatch or PID imbalance")
                print(f"    Severity: ONE motor consistently faster than other")
            elif avg_growth_rate > 0.02:
                print(f"    Pattern: ACCELERATING drift")
                print(f"    Likely cause: Mechanical issue (tire wear, alignment)")
                print(f"    Severity: Problem gets worse with distance")
            else:
                print(f"    Pattern: CORRECTIVE drift")
                print(f"    Likely cause: IMU-based or encoder-based correction")
        
        # Severity assessment
        print(f"\n>>> Severity Assessment:")
        
        if overall_max < 5:
            print(f"    ✓ EXCELLENT: Drift <5cm over 3m (negligible)")
            print(f"      Action: No tuning needed")
        elif overall_max < 15:
            print(f"    ✓ GOOD: Drift {overall_max:.0f}cm over 3m (acceptable)")
            print(f"      Action: Monitor, tune if worse")
        elif overall_max < 30:
            print(f"    ⚠ ACCEPTABLE: Drift {overall_max:.0f}cm over 3m (needs tuning)")
            print(f"      Action: PID tuning recommended")
        else:
            print(f"    ✗ POOR: Drift {overall_max:.0f}cm over 3m (critical)")
            print(f"      Action: Mechanical inspection + PID tuning required")
    
    def get_correction_recommendations(self):
        """Provide specific correction actions"""
        
        print("\n" + "=" * 80)
        print("  CORRECTIVE ACTIONS")
        print("=" * 80)
        
        print("""
        === MECHANICAL CHECKS ===
        
        1. Wheel Alignment:
           - Check if wheels are parallel to chassis midline
           - Misalignment causes steady drift
           - Fix: Adjust mechanical steering linkage
        
        2. Tire Condition:
           - Inspect for uneven wear, low pressure
           - Uneven tires cause accelerating drift
           - Fix: Replace/inflate and rebalance wheels
        
        3. Encoder Calibration:
           - Verify encoders read same distance in free motion
           - Mismatch causes proportional drift
           - Fix: Recalibrate encoder counts per rotation
        
        === PID TUNING ===
        
        1. Motor Speed Mismatch:
           - If left motor faster: DECREASE LEFT_KP or INCREASE RIGHT_KP
           - If right motor faster: INCREASE LEFT_KP or DECREASE RIGHT_KP
           - Adjustment: Start with ±5% changes, test after each
        
        2. PID Parameters (in firmware):
           
           // Current values
           #define LEFT_KP  0.8
           #define LEFT_KI  0.1
           #define LEFT_KD  0.05
           
           #define RIGHT_KP  0.8
           #define RIGHT_KI  0.1
           #define RIGHT_KD  0.05
           
           // Tuning guide:
           - If motor oscillates: DECREASE KP or increase KD
           - If motor slow to respond: INCREASE KP
           - If steady-state error exists: INCREASE KI
        
        3. Velocity Command Filtering:
           - Add exponential moving average to smooth commands
           - Reduces sudden acceleration changes
        
        === EKF TUNING ===
        
        1. Odometry Covariance:
           - Higher covariance = trust less encoder data
           - If drift is still present, EKF can't help
           - Fix mechanical first, then tune covariance
        
        2. IMU Covariance:
           - IMU can detect rotation from accelerometer
           - If yaw drifts, increase IMU gyro covariance
        
        === SYSTEMATIC TUNING APPROACH ===
        
        Test 1: Motor Gain Mismatch
        ├─ Run straight line at 0.3 m/s
        ├─ Measure drift direction (left or right)
        ├─ Adjust KP of faster motor down by 5%
        └─ Repeat until drift is balanced
        
        Test 2: PID Responsiveness
        ├─ Increase setpoint demand (motor speed command)
        ├─ Observe overshoot and settling time
        ├─ Adjust KP/KD to minimize ringing
        └─ Target: Quick response, <10% overshoot
        
        Test 3: Mechanical Alignment
        ├─ Run forward, then backward on same line
        ├─ Compare forward vs backward drift
        ├─ If very different: wheel alignment issue
        └─ If similar: likely motor/PID issue
        
        === MEASUREMENT PROCEDURES ===
        
        To determine motor speed ratio:
        1. Manually run encoders (rotate wheels by hand)
        2. Count encoder ticks for same wheel rotation
        3. Calculate ratio: left_ticks / right_ticks
        4. If not 1.0, adjust motor PWM gain by ratio
        
        To verify encoder calibration:
        1. Push robot forward 1 meter (manually)
        2. Record total encoder counts (left + right)
        3. Calculate ticks_per_meter = total_counts / 1.0
        4. Compare to firmware constant
        5. Update if different by >5%
        """)


def main():
    """Run straight-line motion validation"""
    
    print("""
    ╔════════════════════════════════════════════════════════════════════════╗
    ║           STRAIGHT-LINE MOTION VALIDATION                             ║
    ║                 Drift and Accuracy Testing                            ║
    ║                                                                        ║
    ║  WARNING: Ensure test area is clear and robot won't collide!          ║
    ║           Keep emergency stop ready during testing.                   ║
    ╚════════════════════════════════════════════════════════════════════════╝
    """)
    
    validator = StraightLineValidator()
    validator.straight_line_test()
    validator.get_correction_recommendations()
    
    print(f"\n{'='*80}\nPhysical validation complete\n{'='*80}\n")


if __name__ == '__main__':
    main()
