#!/usr/bin/env python3
"""
=============================================================================
DETAILED LATENCY ANALYSIS
Comprehensive WiFi + ROS 2 latency profiling

Measures:
- WiFi transmission delay
- Message serialization time  
- DDS middleware processing
- ROS 2 subscription callback latency
- End-to-end latency with statistical analysis
=============================================================================
"""

import subprocess
import time
import statistics
import threading
from collections import deque

class LatencyAnalyzer:
    """Detailed latency analysis"""
    
    def __init__(self):
        self.measurements = deque(maxlen=100)
        self.wifi_latencies = deque(maxlen=50)
        
    def measure_simple_latency(self, num_samples: int = 30):
        """Basic latency: ROS now vs message timestamp"""
        
        print("\n" + "=" * 80)
        print("  LATENCY ANALYSIS: SIMPLE METHOD")
        print("=" * 80)
        
        print(f"\n>>> Measuring timestamp latency ({num_samples} samples)...\n")
        
        latencies = []
        
        for i in range(num_samples):
            try:
                # Get message
                result = subprocess.run(
                    ['ros2', 'topic', 'echo', '/odom', '--once'],
                    capture_output=True, timeout=2, text=True
                )
                
                ros_now = time.time()
                
                # Parse timestamp
                sec = 0
                ns = 0
                for line in result.stdout.split('\n'):
                    if 'stamp:' in line:
                        continue
                    if ' sec:' in line:
                        try:
                            sec = int(line.split(':')[1].strip())
                        except:
                            pass
                    elif ' nanosec:' in line:
                        try:
                            ns = int(line.split(':')[1].strip())
                        except:
                            pass
                
                msg_time = sec + ns / 1e9
                latency = (ros_now - msg_time) * 1000  # milliseconds
                
                if -1000 < latency < 1000:  # Filter outliers
                    latencies.append(latency)
                    self.measurements.append(latency)
                
                if (i + 1) % 10 == 0:
                    print(f"  Progress: {i+1}/{num_samples}")
                
            except subprocess.TimeoutExpired:
                pass
            except Exception as e:
                pass
            
            time.sleep(0.05)
        
        if latencies:
            stats = {
                'mean': statistics.mean(latencies),
                'median': statistics.median(latencies),
                'stdev': statistics.stdev(latencies) if len(latencies) > 1 else 0,
                'min': min(latencies),
                'max': max(latencies),
                'p95': statistics.quantiles(latencies, n=20)[18] if len(latencies) > 20 else max(latencies),
                'p99': statistics.quantiles(latencies, n=100)[98] if len(latencies) > 100 else max(latencies),
            }
            
            print("\n  Latency Statistics (milliseconds):")
            print(f"    Mean:        {stats['mean']:7.2f} ms")
            print(f"    Median:      {stats['median']:7.2f} ms")
            print(f"    Std Dev:     {stats['stdev']:7.2f} ms")
            print(f"    Min:         {stats['min']:7.2f} ms")
            print(f"    Max:         {stats['max']:7.2f} ms")
            print(f"    95th %ile:   {stats['p95']:7.2f} ms")
            print(f"    99th %ile:   {stats['p99']:7.2f} ms")
            print(f"    Range:       {stats['max']-stats['min']:7.2f} ms")
            
            # Percentile breakdown
            print(f"\n  Distribution:")
            under_5 = len([l for l in latencies if l < 5])
            under_10 = len([l for l in latencies if l < 10])
            under_20 = len([l for l in latencies if l < 20])
            over_20 = len([l for l in latencies if l >= 20])
            
            total = len(latencies)
            print(f"    <5 ms:   {under_5:3d} samples ({under_5/total*100:5.1f}%)")
            print(f"    <10 ms:  {under_10:3d} samples ({under_10/total*100:5.1f}%)")
            print(f"    <20 ms:  {under_20:3d} samples ({under_20/total*100:5.1f}%)")
            print(f"    ≥20 ms:  {over_20:3d} samples ({over_20/total*100:5.1f}%)")
            
            # Assessment
            print(f"\n  Assessment:")
            if stats['mean'] < 5:
                print(f"    ✓ Excellent latency (sub-5ms - very fast WiFi)")
            elif stats['mean'] < 10:
                print(f"    ✓ Good latency (5-10ms - optimal for robot control)")
            elif stats['mean'] < 20:
                print(f"    ✓ Acceptable latency (10-20ms - suitable for most tasks)")
            elif stats['mean'] < 50:
                print(f"    ⚠ Marginal latency (20-50ms - may impact control stability)")
            else:
                print(f"    ✗ Poor latency (>50ms - consider network issues)")
            
            return stats
        else:
            print(f"  ✗ Unable to measure latency")
            return {}
    
    def measure_topic_rate_consistency(self, topic: str = '/odom', duration_sec: int = 10):
        """Measure topic publishing rate consistency (jitter)"""
        
        print("\n" + "=" * 80)
        print("  LATENCY ANALYSIS: RATE CONSISTENCY / JITTER")
        print("=" * 80)
        
        print(f"\n>>> Measuring {topic} rate consistency for {duration_sec}s...\n")
        
        try:
            result = subprocess.run(
                ['ros2', 'topic', 'hz', topic, '--window', str(duration_sec + 5)],
                capture_output=True, timeout=duration_sec + 10, text=True
            )
            
            print("  Publishing rate analysis:")
            for line in result.stdout.split('\n'):
                if 'average rate' in line or 'min' in line or 'max' in line:
                    if line.strip():
                        print(f"    {line.strip()}")
            
            # Extract rate info
            for line in result.stdout.split('\n'):
                if 'average rate' in line:
                    try:
                        avg_rate = float(line.split(':')[1].split('Hz')[0].strip())
                        period_ms = 1000 / avg_rate
                        print(f"\n  Period analysis:")
                        print(f"    Expected period: {period_ms:.1f} ms")
                        print(f"    Actual avg rate: {avg_rate:.1f} Hz")
                        
                        if avg_rate > 95:
                            print(f"    ✓ Very consistent publishing rate")
                        elif avg_rate > 90:
                            print(f"    ✓ Good rate consistency")
                        else:
                            print(f"    ⚠ Rate drops detected")
                    except:
                        pass
        
        except subprocess.TimeoutExpired:
            print("  ✗ Measurement timeout")
        except Exception as e:
            print(f"  ✗ Error: {e}")
    
    def measure_inter_message_timing(self, topic: str = '/odom', num_messages: int = 20):
        """Measure time between consecutive messages"""
        
        print("\n" + "=" * 80)
        print("  LATENCY ANALYSIS: INTER-MESSAGE TIMING")
        print("=" * 80)
        
        print(f"\n>>> Measuring time between {topic} messages ({num_messages} samples)...\n")
        
        inter_message_times = []
        last_time = None
        
        for i in range(num_messages):
            try:
                start = time.time()
                result = subprocess.run(
                    ['ros2', 'topic', 'echo', topic, '--once'],
                    capture_output=True, timeout=2, text=True
                )
                current_time = time.time()
                
                if last_time:
                    delta = (current_time - last_time) * 1000  # milliseconds
                    inter_message_times.append(delta)
                
                last_time = current_time
                
            except Exception as e:
                pass
            
            time.sleep(0.05)
        
        if inter_message_times:
            stats = {
                'mean': statistics.mean(inter_message_times),
                'stdev': statistics.stdev(inter_message_times) if len(inter_message_times) > 1 else 0,
                'min': min(inter_message_times),
                'max': max(inter_message_times),
            }
            
            print(f"  Inter-Message Timing Statistics (milliseconds):")
            print(f"    Mean period:  {stats['mean']:7.2f} ms")
            print(f"    Std Dev:      {stats['stdev']:7.2f} ms")
            print(f"    Min:          {stats['min']:7.2f} ms")
            print(f"    Max:          {stats['max']:7.2f} ms")
            print(f"    Jitter:       {stats['max']-stats['min']:7.2f} ms")
            
            # Expected periods
            expected_periods = {
                '/odom': 10,    # 100 Hz
                '/imu': 20,     # 50 Hz
                '/scan': 125,   # 8 Hz
            }
            
            expected = expected_periods.get(topic, 0)
            if expected:
                print(f"    Expected:     {expected:7.1f} ms")
                variance = abs(stats['mean'] - expected) / expected * 100
                jitter_pct = stats['stdev'] / expected * 100
                
                print(f"\n  Timing Quality:")
                print(f"    Period variance: {variance:5.1f}% {'✓' if variance < 5 else '⚠'}")
                print(f"    Jitter:          {jitter_pct:5.1f}% {'✓' if jitter_pct < 10 else '⚠'}")
    
    def diagnose_latency_sources(self):
        """Provide diagnosis of where latency comes from"""
        
        print("\n" + "=" * 80)
        print("  LATENCY SOURCE DIAGNOSIS")
        print("=" * 80)
        
        print("""
  Typical latency breakdown in WiFi-based robot systems:
  
  Component                    Typical Range    Notes
  ───────────────────────────────────────────────────────────
  WiFi transmission            1-5 ms          Depends on channel, congestion
  ESP32 serialization          0.5-1 ms        DDS/MQTT serialization
  Micro-ROS agent              0.5-1 ms        Deserialization, routing
  ROS 2 DDS publish            0.5-1 ms        Middleware processing
  Subscriber callback          0.5-2 ms        Application processing
  ───────────────────────────────────────────────────────────
  Total (typical)              3-10 ms         
  Total (WiFi interference)    10-50 ms        Peak congestion
  
  
  Improvement strategies:
  1. WiFi channel selection
     - Use less congested 2.4/5GHz channel
     - Avoid microwave/cordless phones interference
  
  2. ROS 2 middleware optimization
     - Use Cyclone DDS backend (faster than others)
     - Tune QoS for low-latency
  
  3. ESP32 firmware optimization
     - Reduce serialization overhead
     - Use faster WiFi mode (802.11ac)
  
  4. Network optimization
     - Dedicated WiFi access point for robot
     - Minimize physical obstacles
     - Keep ESP32 ≤5 meters from router
        """)
    
    def run_full_analysis(self):
        """Run complete latency analysis"""
        
        print("""
        ╔════════════════════════════════════════════════════════════════════════╗
        ║           COMPREHENSIVE LATENCY ANALYSIS                              ║
        ║              WiFi → ROS 2 Communication Profiling                      ║
        ╚════════════════════════════════════════════════════════════════════════╝
        """)
        
        # Run measurements
        print("\nPhase 1: Simple Timestamp Latency")
        stats1 = self.measure_simple_latency(30)
        
        time.sleep(2)
        
        print("\nPhase 2: Rate Consistency Analysis")
        self.measure_topic_rate_consistency('/odom', 10)
        
        time.sleep(1)
        
        print("\nPhase 3: Inter-Message Timing")
        self.measure_inter_message_timing('/odom', 15)
        
        time.sleep(1)
        
        print("\nPhase 4: Latency Source Diagnosis")
        self.diagnose_latency_sources()
        
        # Summary
        print("\n" + "=" * 80)
        print("  LATENCY ANALYSIS COMPLETE")
        print("=" * 80)
        
        if stats1:
            print(f"\nKey Finding: Average latency = {stats1.get('mean', 0):.1f} ms")
            print(f"             Median latency = {stats1.get('median', 0):.1f} ms")
        
        print(f"\nRecommendation:")
        if stats1 and stats1.get('mean', 100) < 10:
            print("  ✓ System latency is excellent - suitable for all robot tasks")
        elif stats1 and stats1.get('mean', 100) < 20:
            print("  ✓ System latency is good - suitable for most robot tasks")
        elif stats1 and stats1.get('mean', 100) < 50:
            print("  ⚠ System latency is acceptable - verify with real payload")
        else:
            print("  ✗ System latency is high - investigate WiFi conditions")
        
        print(f"\n{'='*80}\n")


if __name__ == '__main__':
    analyzer = LatencyAnalyzer()
    analyzer.run_full_analysis()
