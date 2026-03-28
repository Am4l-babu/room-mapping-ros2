#!/usr/bin/env python3
"""
=============================================================================
PHYSICAL VALIDATION: SLAM QUALITY TEST
Verify SLAM mapping, loop closure, and map accuracy

Procedure:
1. Start fresh SLAM session
2. Teleoperate robot through test environment
3. Observe if loop closures are detected
4. Measure map distortion and accuracy
5. Verify SLAM toolbox is working correctly

SLAM Performance Metrics:
- Loop closure success: Detects when returning to known area %
- Map accuracy: Estimated distance errors in map <5-10%
- Consistency: Map remains stable when revisiting areas
- CPU usage: SLAM shouldn't exceed 30% CPU

Common SLAM Issues:
- No loop closures: Scan resolution too coarse, features not distinct enough
- Map distortion: Too few loop closures, drift accumulation
- CPU overload: Processing every scan, consider downsampling
- Oscillation: Multiple loop closures conflicting, too aggressive optimization
=============================================================================
"""

import subprocess
import time
import math
from enum import Enum

class SLAMPhase(Enum):
    """SLAM test phases"""
    STARTUP = 1
    BASELINE = 2
    MAPPING = 3
    LOOP_CLOSURE = 4
    MAP_ANALYSIS = 5

class SLAMValidator:
    """Validate SLAM mapping quality"""
    
    def __init__(self):
        self.results = {
            'slam_running': False,
            'loop_closures': 0,
            'estimated_map_size_m': 0,
            'distortion_observed': False,
        }
    
    def print_header(self, title: str):
        """Print section header"""
        print("\n" + "=" * 80)
        print(f"  {title}")
        print("=" * 80)
    
    def print_phase(self, phase_name: str):
        """Print phase marker"""
        print(f"\n>>> {phase_name}")
    
    def check_slam_toolbox_running(self) -> bool:
        """Check if SLAM Toolbox is running"""
        print("  Checking SLAM Toolbox status...", end=' ')
        
        result = subprocess.run(
            "ros2 node list | grep -i slam",
            shell=True,
            capture_output=True
        )
        
        running = result.returncode == 0
        print("✓ Running" if running else "✗ Not running")
        return running
    
    def start_slam_mapping(self):
        """Start fresh SLAM mapping session"""
        self.print_phase("START SLAM MAPPING")
        
        print("""
  SLAM Toolbox will now start recording scans and building a map.
  
  Procedure:
  1. Robot will start at origin (0, 0)
  2. Slowly teleoperate through environment
  3. Cover area with varied features (walls, obstacles, corners)
  4. When returning to known areas, SLAM should detect loop closures
  5. Total mapping time: 10-15 minutes recommended
        """)
        
        if not self.check_slam_toolbox_running():
            print("\n  ✗ SLAM Toolbox not running!")
            print("  Start with: ros2 launch slam_toolbox online_async.launch.py")
            return False
        
        print("\n  Starting fresh mapping session...")
        
        # Save current map if exists
        save_result = subprocess.run(
            "ros2 service call /slam_toolbox/save_map slam_toolbox/srv/SaveMap '{filename: \"map_pre_test\"}'",
            shell=True,
            capture_output=True,
            timeout=2
        )
        
        print("  ✓ Ready for mapping")
        print("  ⏱ Mapping session started\n")
        
        self.results['slam_running'] = True
        return True
    
    def teleoperate_mapping_session(self):
        """Guide user through teleoperation session"""
        self.print_phase("TELEOPERATE ROBOT")
        
        print("""
  Key principles for good SLAM mapping:
  
  1. VARY THE ROUTES:
     - Don't just drive back-and-forth in straight lines
     - Make turns, figure-8 patterns, spiral patterns
     - Visit all areas of the environment multiple times
  
  2. WATCH FOR LOOP CLOSURES:
     - When returning to familiar area, watch /tf
     - Loop closure should correct accumulated drift
     - May see jumps in map coordinates (constrained optimization)
  
  3. DISTINCTIVE FEATURES REQUIRED:
     - Flat empty warehouse: SLAM will fail (no features to track)
     - Structured environment with walls/obstacles: Good
     - Mixed features (walls, clutter, furniture): Excellent
  
  Recommended mapping route:
  
       [Start]
         |
         v
      ┌─────────┐
      │         │  1. Spiral outward from start
      │  [S]    │  2. Drive perimeter/walls
      │         │  3. Make loop back to start
      └─────────┘  4. Drive cross-patterns
         
         MAP QUALITY BY COVERAGE:
         - Partial coverage (1/3 of area): Fair
         - Medium coverage (2/3 of area): Good
         - Full coverage (entire area): Excellent
        """)
        
        input("  Press ENTER when ready to start teleoperation...")
        
        duration_minutes = None
        while duration_minutes is None:
            try:
                duration_str = input("  How long to map? (minutes, 5-30): ")
                duration_minutes = int(duration_str)
                if 5 <= duration_minutes <= 30:
                    break
                else:
                    print(f"    ✗ Use 5-30 minutes")
                    duration_minutes = None
            except ValueError:
                print(f"    ✗ Invalid input")
        
        print(f"\n  Mapping session: {duration_minutes} minutes")
        print(f"  ⏱ Starting... (monitor /tf and /map topics)\n")
        
        # Timer
        start_time = time.time()
        for i in range(duration_minutes * 60):
            elapsed_min = i // 60
            elapsed_sec = i % 60
            remaining_min = (duration_minutes * 60 - i - 1) // 60
            remaining_sec = (duration_minutes * 60 - i - 1) % 60
            
            print(f"    Mapping: {elapsed_min:02d}:{elapsed_sec:02d} " 
                  f"({remaining_min:02d}:{remaining_sec:02d} remaining)", 
                  end='\r')
            time.sleep(1)
        
        print(f"\n  ✓ Mapping session complete")
        
        self.results['mapping_duration_min'] = duration_minutes
    
    def observe_loop_closures(self):
        """Check if loop closures were detected"""
        self.print_phase("LOOP CLOSURE DETECTION")
        
        print("""
  Loop Closure Signs:
  
  1. Visual (RViz):
     - Red arrows jumping (pose constraint applied)
     - Map sections snapping into alignment
     - Normally gradual drift corrected suddenly
  
  2. Topic monitoring:
     ros2 topic echo /scan_matched_points2
     - Should continuously receive scans
     - Volume might suddenly expand on loop closure
  
  3. TF monitoring:
     ros2 run rqt_graph rqt_graph
     - Should show map <-> odom link
     - Loop closure applies constraint between distant poses
        """)
        
        num_closures = None
        while num_closures is None:
            try:
                num_str = input("  How many loop closures detected? (0-50): ")
                num_closures = int(num_str)
                if 0 <= num_closures <= 50:
                    break
                else:
                    print(f"    ✗ Use 0-50")
                    num_closures = None
            except ValueError:
                print(f"    ✗ Invalid input")
        
        self.results['loop_closures'] = num_closures
        
        if num_closures == 0:
            print(f"\n  ⚠ No loop closures detected")
            print(f"    Possible causes:")
            print(f"    - Environment lacks distinctive features")
            print(f"    - LiDAR resolution too coarse (Phase 2 37 points)")
            print(f"    - Loop closure search parameters not tuned")
        elif num_closures < 3:
            print(f"\n  ⚠ Few loop closures ({num_closures})")
            print(f"    Map may accumulate significant drift")
        else:
            print(f"\n  ✓ Good loop closure detection ({num_closures})")
    
    def assess_map_distortion(self):
        """Assess map quality and distortion"""
        self.print_phase("MAP QUALITY ASSESSMENT")
        
        print("""
  Visual Map Quality Checks:
  
  1. CONSISTENCY:
     - Walls should be straight lines (not wavy)
     - Parallel features should match both passes
     - Repeated areas should align closely
  
  2. DISTORTION TYPES:
     - Wavy walls: Excessive drift before loop closure
     - Doubled features: Loop closure too aggressive
     - Gaps: No scans in some areas
     - Rotation: Map rotated incorrectly
  
  3. SCALE VERIFICATION:
     - Measure known distance in environment
     - Compare to same distance in map
     - Should match within 5-10%
        """)
        
        distortion = None
        while distortion is None:
            user_input = input("  Visual distortion observed? (none/minor/moderate/severe): ").lower()
            
            valid = ['none', 'minor', 'moderate', 'severe']
            if user_input in valid:
                distortion = user_input
            else:
                print(f"    ✗ Use: {', '.join(valid)}")
        
        severity_map = {
            'none': 0,
            'minor': 1,
            'moderate': 2,
            'severe': 3
        }
        
        self.results['distortion_level'] = severity_map[distortion]
        self.results['distortion_description'] = distortion
        
        # Scale verification
        print(f"\n  Scale Verification:")
        print(f"  Measure a known distance in your environment")
        print(f"  (e.g., 5 meters down a wall)")
        
        real_distance = None
        while real_distance is None:
            try:
                real_str = input("  Known real-world distance (meters, 1-20): ")
                real_distance = float(real_str)
                if 1 <= real_distance <= 20:
                    break
                else:
                    print(f"    ✗ Use 1-20 meters")
                    real_distance = None
            except ValueError:
                print(f"    ✗ Invalid input")
        
        map_distance = None
        while map_distance is None:
            try:
                map_str = input("  Same distance measured in RViz map (meters): ")
                map_distance = float(map_str)
                if 0.5 <= map_distance <= 30:
                    break
                else:
                    print(f"    ✗ Use reasonable value")
                    map_distance = None
            except ValueError:
                print(f"    ✗ Invalid input")
        
        scale_error = abs(map_distance - real_distance) / real_distance * 100
        
        self.results['scale_error_percent'] = scale_error
        
        print(f"\n  Scale Check:")
        print(f"    Real distance: {real_distance:.1f}m")
        print(f"    Map distance: {map_distance:.1f}m")
        print(f"    Error: {scale_error:.1f}%")
        
        if scale_error < 5:
            print(f"  ✓ EXCELLENT: Scale accurate (<5%)")
        elif scale_error < 10:
            print(f"  ✓ GOOD: Scale acceptable (<10%)")
        elif scale_error < 15:
            print(f"  ⚠ MODERATE: Scale slightly off (<15%)")
        else:
            print(f"  ✗ POOR: Scale significantly off (>{scale_error:.1f}%)")
    
    def save_and_measure_map(self):
        """Save map and get measurements"""
        self.print_phase("SAVE AND ANALYZE MAP")
        
        print("  Saving SLAM map...")
        
        save_result = subprocess.run(
            "ros2 service call /slam_toolbox/save_map slam_toolbox/srv/SaveMap '{filename: \"map_validation_test\"}'",
            shell=True,
            capture_output=True,
            timeout=2
        )
        
        if save_result.returncode == 0:
            print("  ✓ Map saved: ~/map_validation_test.pgm")
        else:
            print("  ⚠ Could not save map file")
        
        # Ask for map dimensions
        map_size = None
        while map_size is None:
            try:
                size_str = input("  Estimated map coverage (meters²): ")
                map_size = float(size_str)
                if 1 <= map_size <= 1000:
                    break
                else:
                    print(f"    ✗ Use 1-1000 m²")
                    map_size = None
            except ValueError:
                print(f"    ✗ Invalid input")
        
        self.results['estimated_map_size_m2'] = map_size
    
    def check_cpu_usage(self):
        """Monitor SLAM CPU usage"""
        self.print_phase("CPU USAGE MONITORING")
        
        print("  Checking SLAM process CPU usage...")
        
        result = subprocess.run(
            "ps aux | grep slam | grep -v grep | awk '{print $3}'",
            shell=True,
            capture_output=True,
            text=True
        )
        
        try:
            if result.stdout.strip():
                cpu_usage = float(result.stdout.strip().split('\n')[0])
                self.results['slam_cpu_percent'] = cpu_usage
                
                print(f"  Estimated CPU usage: {cpu_usage:.1f}%")
                
                if cpu_usage < 10:
                    print(f"  ✓ EXCELLENT: Very efficient")
                elif cpu_usage < 20:
                    print(f"  ✓ GOOD: Reasonable CPU load")
                elif cpu_usage < 30:
                    print(f"  ⚠ MODERATE: Using significant CPU")
                else:
                    print(f"  ✗ HIGH: CPU overloaded, may affect performance")
            else:
                print(f"  ? Could not determine CPU usage")
        except:
            print(f"  ? Error parsing CPU usage")
    
    def run_slam_quality_test(self):
        """Main SLAM quality test"""
        
        self.print_header("SLAM QUALITY VALIDATION TEST")
        
        print("""
        SLAM Toolbox Performance Validation
        ===================================
        
        This test verifies:
        1. SLAM mapping is working correctly
        2. Loop closures are detected
        3. Map quality is acceptable
        4. No CPU overload
        
        Expected outcomes (good):
        - Loop closures: 3-10 in typical env
        - Map distortion: None to minor
        - Scale error: <10%
        - CPU usage: 10-20%
        
        Expected outcomes (poor):
        - No loop closures and wavy map: Features not distinct
        - High CPU usage: Processing too many scans
        - Large scale errors (>15%): Drift accumulation
        """)
        
        input("\nPress ENTER to begin SLAM test...")
        
        # Phase 1: Check SLAM status
        self.print_phase("VERIFY SLAM TOOLBOX")
        
        if not self.start_slam_mapping():
            return False
        
        # Phase 2: Teleoperation
        self.teleoperate_mapping_session()
        
        # Phase 3: Loop closures
        self.observe_loop_closures()
        
        # Phase 4: Map quality
        self.assess_map_distortion()
        
        # Phase 5: Save and measure
        self.save_and_measure_map()
        
        # Phase 6: CPU monitoring
        self.check_cpu_usage()
        
        # Analysis
        self.analyze_slam_performance()
    
    def analyze_slam_performance(self):
        """Analyze SLAM performance"""
        
        self.print_header("SLAM PERFORMANCE ANALYSIS")
        
        print(f"\nTest Results Summary:")
        print(f"  Loop closures detected: {self.results.get('loop_closures', 0)}")
        print(f"  Map distortion: {self.results.get('distortion_description', 'unknown')}")
        print(f"  Scale error: {self.results.get('scale_error_percent', -1):.1f}%")
        print(f"  Map size: {self.results.get('estimated_map_size_m2', 0):.1f} m²")
        print(f"  SLAM CPU: {self.results.get('slam_cpu_percent', -1):.1f}%")
        
        # Detailed analysis
        print(f"\n>>> Detailed Assessment:")
        
        # Loop closure analysis
        lc = self.results.get('loop_closures', 0)
        if lc == 0:
            print(f"  ⚠ No loop closures detected")
            print(f"    Possible issues:")
            print(f"    - LiDAR too coarse (Phase 2 has only 37 points)")
            print(f"      Solution: Upgrade to Phase 5 (90 points)")
            print(f"    - Environment lacks features (empty hallway)")
            print(f"      Solution: More complex test environment")
            print(f"    - Scan matcher parameters need tuning")
        elif lc < 3:
            print(f"  ⚠ Few loop closures ({lc}): Map may accumulate drift")
        else:
            print(f"  ✓ Good loop closure detection ({lc})")
        
        # Distortion analysis
        distortion = self.results.get('distortion_level', 0)
        if distortion == 0:
            print(f"  ✓ Excellent map quality: No distortion")
        elif distortion == 1:
            print(f"  ✓ Good map quality: Minor distortion")
        elif distortion == 2:
            print(f"  ⚠ Moderate map quality: Noticeable distortion")
            print(f"    Likely cause: Insufficient loop closures")
        else:
            print(f"  ✗ Poor map quality: Severe distortion")
        
        # Scale error analysis
        scale_error = self.results.get('scale_error_percent', -1)
        if 0 <= scale_error < 5:
            print(f"  ✓ Excellent scale accuracy: {scale_error:.1f}% error")
        elif scale_error < 10:
            print(f"  ✓ Good scale accuracy: {scale_error:.1f}% error")
        elif scale_error < 15:
            print(f"  ⚠ Moderate scale error: {scale_error:.1f}%")
        else:
            print(f"  ✗ Poor scale accuracy: {scale_error:.1f}% error")
        
        # Overall verdict
        print(f"\n>>> Overall SLAM Status:")
        
        critical_issues = []
        warnings = []
        
        if lc == 0:
            critical_issues.append("No loop closures detected")
        if distortion >= 2:
            critical_issues.append("Significant map distortion")
        
        if scale_error > 0 and scale_error >= 15:
            critical_issues.append(f"Poor scale ({scale_error:.1f}%)")
        if scale_error > 0 and scale_error >= 10:
            warnings.append(f"Moderate scale error ({scale_error:.1f}%)")
        
        if critical_issues:
            print(f"  ✗ ISSUES FOUND:")
            for issue in critical_issues:
                print(f"     - {issue}")
        
        if warnings:
            print(f"  ⚠ WARNINGS:")
            for warning in warnings:
                print(f"     - {warning}")
        
        if not critical_issues:
            print(f"  ✅ SLAM performance acceptable")
            print(f"  Map is ready for navigation tasks")
    
    def get_slam_optimization_guide(self):
        """Provide SLAM optimization guidance"""
        
        print("\n" + "=" * 80)
        print("  SLAM OPTIMIZATION GUIDE")
        print("=" * 80)
        
        print("""
        === IF NO LOOP CLOSURES ===
        
        Problem likely: LiDAR resolution insufficient
        
        Phase 2 scan: Only 37 points (very coarse)
        Phase 5 scan: 90 points with filtering (much better)
        
        Solution: Deploy Phase 5 enhanced scanning
        
        Location in code:
        - micro-ROS-Agent/src/scan_publisher.cpp
        - Change: SCAN_POINTS_PHASE2 (37) → SCAN_POINTS_PHASE5 (90)
        - Add: Feature-based filtering to keep only edges
        - Rebuild and redeploy to ESP32
        
        === IF MAP DISTORTION ===
        
        Problem: Accumulated drift from odometry
        
        Solutions:
        
        1. More frequent loop closures:
           - Reduce search window (scan further ahead)
           - Increase feature importance
           - More distinctive test environment
        
        2. Better odometry (reduce drift):
           - Calibrate motor encoders precisely
           - Verify EKF is working (Phase 4)
           - Check wheel alignment (no toe-in)
        
        3. SLAM parameter tuning:
           - Location: /opt/ros/[distro]/share/slam_toolbox/params/
           - LoopSearchMaximumDistance: Larger = find loops further away
           - DistanceVarianceThreshold: Smaller = stricter loop verification
           - AngleVarianceThreshold: Smaller = stricter angle verification
        
        === IF SCALE ERRORS ===
        
        Problem: Systematic error in distance measurement
        
        Likely causes:
        1. Encoder calibration off (affects odom distance)
           → Recalibrate encoder counts per wheel rotation
        
        2. Wheel radius incorrect in code
           → Measure actual wheel diameter
           → Update WHEEL_RADIUS in firmware
        
        3. EKF parameters tuned for wrong noise levels
           → Verify IMU accelerometer range
           → Check gyro scale setting (typically ±250°/s)
        
        === HIGH CPU USAGE ===
        
        If CPU >30%:
        
        1. Downsampling:
           - SLAM doesn't need 8 Hz scans
           - Can process every 2nd scan (4 Hz)
           - Saves 50% CPU while keeping good features
        
        2. Resolution reduction:
           - If Phase 5 still expensive, use Phase 2
           - 37 points uses ~25% CPU of 90 points
        
        3. Async processing:
           - Use async.launch.py instead of sync
           - Processes scans in background
           - Doesn't block main controller loop
        """)


def main():
    """Run SLAM quality test"""
    
    print("""
    ╔════════════════════════════════════════════════════════════════════════╗
    ║                    SLAM QUALITY VALIDATION                            ║
    ║              Mapping Performance and Loop Closure Test                 ║
    ║                                                                        ║
    ║  This test requires: 10-15 minutes of manual teleoperation           ║
    ║                     Complex environment with features                 ║
    ║                     SLAM Toolbox running                              ║
    ╚════════════════════════════════════════════════════════════════════════╝
    """)
    
    validator = SLAMValidator()
    validator.run_slam_quality_test()
    validator.get_slam_optimization_guide()
    
    print(f"\n{'='*80}\nSLAM quality test complete\n{'='*80}\n")


if __name__ == '__main__':
    main()
