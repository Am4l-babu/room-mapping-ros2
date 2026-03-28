#!/usr/bin/env python3
"""
=============================================================================
EKF PERFORMANCE VALIDATION
Compare raw vs filtered odometry

Validates Phase 4 EKF sensor fusion effectiveness:
- Noise reduction
- Drift compensation
- Data smoothness
- Accuracy improvement
=============================================================================
"""

import subprocess
import time
import math
import statistics
from collections import deque

class EKFValidator:
    """Validate Extended Kalman Filter performance"""
    
    def __init__(self):
        self.raw_odom_history = deque(maxlen=50)
        self.filtered_odom_history = deque(maxlen=50)
        
    def get_odometry_data(self, topic: str, num_samples: int = 20) -> list:
        """Collect odometry data samples"""
        data = []
        
        print(f"\n>>> Collecting {num_samples} samples from {topic}...\n")
        
        for i in range(num_samples):
            try:
                result = subprocess.run(
                    ['ros2', 'topic', 'echo', topic, '--once'],
                    capture_output=True, timeout=2, text=True
                )
                
                # Parse position
                x, y, theta = 0, 0, 0
                vx, vy, vtheta = 0, 0, 0
                
                for line in result.stdout.split('\n'):
                    if 'x:' in line and any(k in result.stdout[:result.stdout.find(line)] for k in ['position']):
                        try:
                            x = float(line.split(':')[1].strip())
                        except:
                            pass
                    elif 'y:' in line and 'position' in result.stdout[:result.stdout.find(line)]:
                        try:
                            y = float(line.split(':')[1].strip())
                        except:
                            pass
                    elif 'z:' in line and 'linear' in result.stdout[:result.stdout.find(line)]:
                        try:
                            vx = float(line.split(':')[1].strip())
                        except:
                            pass
                
                data.append({'x': x, 'y': y, 'theta': theta, 'vx': vx, 'vy': vy})
                
                if (i + 1) % 5 == 0:
                    print(f"  Collected {i+1}/{num_samples} samples")
                
            except subprocess.TimeoutExpired:
                print(f"  Sample {i+1}: Timeout")
            except Exception as e:
                print(f"  Sample {i+1}: Error")
            
            time.sleep(0.1)
        
        return data
    
    def calculate_noise_level(self, data: list) -> dict:
        """Calculate noise statistics from data stream"""
        if len(data) < 3:
            return {}
        
        # Calculate position deltas
        dx_values = []
        dy_values = []
        
        for i in range(1, len(data)):
            dx = abs(data[i]['x'] - data[i-1]['x'])
            dy = abs(data[i]['y'] - data[i-1]['y'])
            
            if dx < 1 and dy < 1:  # Filter out large jumps
                dx_values.append(dx)
                dy_values.append(dy)
        
        if not dx_values or not dy_values:
            return {}
        
        return {
            'dx_mean': statistics.mean(dx_values),
            'dx_stdev': statistics.stdev(dx_values) if len(dx_values) > 1 else 0,
            'dy_mean': statistics.mean(dy_values),
            'dy_stdev': statistics.stdev(dy_values) if len(dy_values) > 1 else 0,
            'dx_range': max(dx_values) - min(dx_values),
            'dy_range': max(dy_values) - min(dy_values),
        }
    
    def validate_ekf(self):
        """Run complete EKF validation"""
        
        print("=" * 80)
        print("  EKF SENSOR FUSION VALIDATION")
        print("=" * 80)
        
        # Check if EKF is running
        print("\n>>> Checking EKF node status...")
        try:
            result = subprocess.run(['ros2', 'node', 'list'],
                                  capture_output=True, timeout=3, text=True)
            
            if 'ekf' in result.stdout.lower():
                print("  ✓ EKF node is running")
            else:
                print("  ⚠ EKF node not found")
                print("  → Try launching: ros2 launch esp32_serial_bridge micro_ros_bringup_phase4.launch.py")
                return
        except Exception as e:
            print(f"  ✗ Error: {e}")
            return
        
        # Collect data
        print("\n>>> Collecting odometry samples...")
        raw_data = self.get_odometry_data('/odom', 20)
        
        time.sleep(1)
        
        # Try to get filtered data
        try:
            filtered_data = self.get_odometry_data('/odometry/filtered', 20)
        except:
            print("  ✗ filtered topic not available")
            filtered_data = []
        
        # Analyze noise
        if raw_data:
            print("\n>>> Analyzing raw odometry data...")
            raw_noise = self.calculate_noise_level(raw_data)
            
            if raw_noise:
                print(f"\n  Raw Odometry Noise Statistics:")
                print(f"    Position X: μ={raw_noise.get('dx_mean', 0):.6f}, σ={raw_noise.get('dx_stdev', 0):.6f}")
                print(f"    Position Y: μ={raw_noise.get('dy_mean', 0):.6f}, σ={raw_noise.get('dy_stdev', 0):.6f}")
                print(f"    Jitter range: X={raw_noise.get('dx_range', 0):.6f}, Y={raw_noise.get('dy_range', 0):.6f}")
        
        if filtered_data:
            print("\n>>> Analyzing filtered odometry data...")
            filtered_noise = self.calculate_noise_level(filtered_data)
            
            if filtered_noise:
                print(f"\n  Filtered Odometry Noise Statistics:")
                print(f"    Position X: μ={filtered_noise.get('dx_mean', 0):.6f}, σ={filtered_noise.get('dx_stdev', 0):.6f}")
                print(f"    Position Y: μ={filtered_noise.get('dy_mean', 0):.6f}, σ={filtered_noise.get('dy_stdev', 0):.6f}")
                print(f"    Jitter range: X={filtered_noise.get('dx_range', 0):.6f}, Y={filtered_noise.get('dy_range', 0):.6f}")
            
            # Compare improvement
            print("\n>>> EKF Improvement Analysis:")
            
            if raw_noise and filtered_noise:
                dx_reduction = ((raw_noise.get('dx_stdev', 1) - filtered_noise.get('dx_stdev', 0)) 
                               / raw_noise.get('dx_stdev', 1) * 100)
                dy_reduction = ((raw_noise.get('dy_stdev', 1) - filtered_noise.get('dy_stdev', 0)) 
                               / raw_noise.get('dy_stdev', 1) * 100)
                
                print(f"\n  Noise Reduction:")
                print(f"    Position X noise: {dx_reduction:6.1f}% reduction")
                print(f"    Position Y noise: {dy_reduction:6.1f}% reduction")
                
                if dx_reduction > 30 or dy_reduction > 30:
                    print(f"    ✓ EKF is effectively filtering sensor noise")
                else:
                    print(f"    ⚠ EKF filtering effect is subtle - check covariance tuning")
        
        print(f"\n{'='*80}\n")


if __name__ == '__main__':
    validator = EKFValidator()
    validator.validate_ekf()
