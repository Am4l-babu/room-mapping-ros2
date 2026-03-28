================================================================================
PID TUNING GUIDE FOR ESP32 MOTOR CONTROL
Systematic approach to tuning closed-loop motor performance
================================================================================

BACKGROUND: PID CONTROL
=======================

PID (Proportional-Integral-Derivative) controller maintains a variable
at a target setpoint by adjusting an actuator.

In our system:
  Input (Setpoint):    Target RPM from ROS 2 (via kinematics)
  Feedback:            Actual RPM from encoders (100 Hz)
  Actuator:            PWM to motor (0-255)
  Process:             DC motor + mechanical load

PID Terms Explained:
┌─────────────────────────────────────────────────────────────────┐
│ P (Proportional Term)                                           │
│ ─────────────────────                                           │
│ Output ∝ Current Error                                          │
│ Effect: Responds immediately to error, but can overshoot       │
│ Too high (Kp): System oscillates, unstable                     │
│ Too low:       Slow response, takes long to reach target       │
│                                                                 │
│ I (Integral Term)                                              │
│ ─────────────────                                              │
│ Output ∝ Sum of all past errors                                │
│ Effect: Eliminates steady-state error                          │
│ Too high (Ki): Oscillations, slow settling                     │
│ Too low:       Doesn't reach target (steady-state error)       │
│                                                                 │
│ D (Derivative Term)                                            │
│ ──────────────────                                             │
│ Output ∝ Rate of change of error (damping)                     │
│ Effect: Prevents overshoot, stabilizes                         │
│ Too high (Kd): Sluggish, jerky motion                          │
│ Too low:       Overshoots, oscillates                          │
└─────────────────────────────────────────────────────────────────┘

================================================================================
PHASE 1: CALIBRATION & BASELINE TESTING
========================================

1.1 Build and Upload Firmware
─────────────────────────────

cd /home/ros/ros2_ws/sensor_testing
pio run -e motor_control_pid -t upload --upload-port /dev/ttyACM1
pio device monitor --port /dev/ttyACM1 --baud 115200

Expected output:
  [PID] Motor control system initialized
  System ready. Type 'HELP' for commands.

1.2 Test Motor Manually (Serial Commands)
──────────────────────────────────────────

Without ROS 2 running, test basic motor response:

Command 1: Check Current State
  Input:  STATUS
  Output: Prints current RPM_target, RPM_actual, error, pulses
  
  Example output:
    Motor 1 | Target: 0.0 RPM | Ramped: 0.0 | Actual: 0.0 | Error: 0.00 RPM
    Motor 2 | Target: 0.0 RPM | Ramped: 0.0 | Actual: 0.0 | Error: 0.00 RPM
    Encoder Counts | M1: 0 | M2: 0

Command 2: Gentle Acceleration Test
  Input:  RPM_SET:20,20
  Wait 5 seconds
  Input:  STATUS
  
  Expected trend:
    t=0s:   Target: 20.0  Actual: 0.0   Error: 20.0 RPM
    t=1s:   Target: 20.0  Actual: 8.3   Error: 11.7 RPM    [P term working]
    t=2s:   Target: 20.0  Actual: 15.2  Error: 4.8 RPM     [I term kicking in]
    t=3s:   Target: 20.0  Actual: 19.1  Error: 0.9 RPM     [Converging]
    t=5s:   Target: 20.0  Actual: 20.0  Error: 0.0 RPM ✓   [Settled]

Command 3: Overshoot Test
  Input:  RPM_SET:50,50
  Observe serial output
  
  Check:
    Does actual ever exceed target by >5 RPM? (overshoot is bad)
    Does it settle in <2 seconds?
    Does error stay <1 RPM steady-state?

1.3 Observe Encoder Pulses
──────────────────────────

Enable high-frequency printing (edit motor_control_pid.cpp):

```cpp
// After publish line, add:
if (counter % 10 == 0) {  // Print every 100ms
    Serial.print("Pulses: ");
    Serial.println(encoder1_count);
}
```

Expected:
  At 20 RPM: ~6.7 pulses per 100ms (20/60 * 20 = 6.67)
  At 50 RPM: ~16.7 pulses per 100ms

If pulses are zero or very low:
  - Check encoder connections (GPIO 4, 5)
  - Check for ISR conflicts
  - Verify motor is actually spinning

================================================================================
PHASE 2: INITIAL TUNING (Ziegler-Nichols Manual Method)
===============================================

Goal: Find P-only settings first, then add I and D

2.1 P-Only Tuning (Find Critical Gain)
──────────────────────────────────────

Edit motor_control_pid.h:
  #define PID_KP <value>
  #define PID_KI 0.0    // Disable I
  #define PID_KD 0.0    // Disable D

Start with Kp = 0.5 (conservative)

Recompile and test:
  pio run -e motor_control_pid -t upload --upload-port /dev/ttyACM1

Test sequence:
  1. RPM_SET:30,30
  2. After 3 seconds, capture STATUS
  3. Look at error and overshoot

Adjust and repeat:
  If error large (>5 RPM):    Increase Kp → 0.7
  If overshoots small (1-2):  OK, proceed
  If overshoots large (>10):  Reduce Kp → 0.3

Continue until:
  ✓ Reaches target within 2 seconds
  ✓ Overshoot < 5 RPM (25% max for 20 RPM)
  ✓ Steady-state error < 2 RPM

Record this as Kp_critical

2.2 Add Integral (I-Term)
─────────────────────────

Ki helps eliminate final steady-state error.

Set:
  #define PID_KI 0.1 * Kp_critical   (start conservative)

Test:
  RPM_SET:30,30
  Wait 5 seconds
  STATUS
  
  Expected:
    Error should approach 0 over time
    But response may be oscillatory first
    
  If oscillates:  Reduce Ki
  If error stays: Increase Ki

Final goal: Error < 1 RPM at steady state

2.3 Add Derivative (D-Term)
──────────────────────────

Kd damps overshoot and provides stability.

Start conservative:
  #define PID_KD 0.05*Kp_critical

Test:
  RPM_SET:50,50  (higher speed, reveals overshoot better)
  COUNT: how far does actual exceed target?
  
  Expected: Overshoot should smooth out
  
  If overshoots increase: Reduce Kd
  If undershoots (sluggish): Increase Kd

================================================================================
PHASE 3: ADVANCED TUNING (Frequency Response)
=============================================

3.1 Step Response Test
─────────────────────

Goal: Respond quickly without oscillating

Test sequence:
  1. Set RPM_SET:0,0
  2. Wait for complete stop
  3. Rapidly send: RPM_SET:60,60
  4. Watch output for 3 seconds
  5. Record: rise time, overshoot, settling time

Ideal response:
  Rise time: < 0.5 seconds
  Overshoot: < 10%
  Settling: < 1 second

If rise time too slow:
  → Increase Kp (more aggressive)
  → Increase Ki (eliminate error faster)

If overshoot too large:
  → Increase Kd (more damping)
  → Decrease Kp (less aggressive)

3.2 Frequency Response (Optional - Advanced)
─────────────────────────────────────────────

For precise tuning, test sine wave response:

Create firmware test mode that sends:
  RPM_target = 30 + 10*sin(2π*0.5*t)  // 0.5 Hz oscillation

Measure amplitude and phase lag of actual response.
Better tracking = better controller tuning.

(This is optional - step response usually sufficient)

================================================================================
PHASE 4: MOTION PROFILE TESTING
===============================

4.1 Two-Point Performance
─────────────────────────

Test moving between two RPM levels:

Step 1: Ramp up
  RPM_SET:0,0  → wait 2s → RPM_SET:40,40

Step 2: Hold
  Observe at 40 RPM for 3 seconds (should stay at 40)

Step 3: Ramp down
  RPM_SET:0,0  → should decel smoothly

Check smoother acceleration:
  Edit motor_control_pid.h:
    #define MAX_ACCELERATION 50   // Start at 50 RPM/sec
  
  This limits target ramp to prevent jerky motion
  Adjust if needed:
    Too fast: Increase to 100
    Too slow: Decrease to 25

4.2 Load Test
─────────────

Apply resistance to wheel and retest:

With wheel free:
  RPM_SET:30,30 → error ≈ 0 after 2s

With hand resistance (moderate):
  RPM_SET:30,30 → should still reach 30
  May take slightly longer
  Error should stay < 3 RPM

This tests integral term effectiveness.

================================================================================
TUNING REFERENCE TABLE
======================

Scenario              Kp Change    Ki Change    Kd Change    Why
────────────────────────────────────────────────────────────────────
Rise time too slow    Increase     Increase     -            More aggression
Too much overshoot    Decrease     -            Increase     Add damping
Oscillating           Decrease     Decrease     Increase     Reduce gain, damp
Steady-state error    -            Increase     -            Eliminate error
Jerky/noisy           Decrease     Decrease     Increase     Smooth, filter
Response too sluggish Increase     Increase     Decrease     More responsive

Typical Good Values:
  Small motors (similar to yours):
    Kp: 0.8 - 1.5
    Ki: 0.2 - 0.5
    Kd: 0.05 - 0.2

================================================================================
IMPLEMENTATION: UPDATING KERNEL GAINS
====================================

Method 1: Recompile and Upload (Full)
───────────────────────────────────────

1. Edit motor_control_pid.h:
   #define PID_KP 1.2
   #define PID_KI 0.3
   #define PID_KD 0.1

2. Rebuild and upload:
   pio run -e motor_control_pid -t upload --upload-port /dev/ttyACM1

3. Restart firmware

Time: ~10 seconds

Method 2: Runtime Recalibration (Future Feature)
──────────────────────────────────────────────────

Will add RECONFIG command:
  Input:  RECONFIG:1.5,0.4,0.15
  Output: [PID] Gains updated: Kp=1.5 Ki=0.4 Kd=0.15

(Requires adding serial parsing - TODO for future)

================================================================================
DIAGNOSTIC COMMANDS
===================

Check current tuning performance:

// Every second, get control status
for i in {1..30}; do
  echo "STATUS" > /dev/ttyACM1
  sleep 1.0
done

// Parse and plot error over time
pio device monitor --port /dev/ttyACM1 --baud 115200 | \
  grep "Error:" | awk '{print $NF}' > error_log.txt

Then analyze in spreadsheet:
  Plot error vs time
  Should trend toward zero
  Should not oscillate

================================================================================
PID TUNING CHECKLIST
===================

Before tuning:
  ☐ Firmware compiles and uploads
  ☐ Motor spins when commanded (manual test)
  ☐ Encoder pulses count up when spinning
  ☐ Serial port responds to commands

During tuning:
  ☐ Document baseline values (Kp, Ki, Kd)
  ☐ Record response for each gain change
  ☐ Test multiple RPM levels (20, 50, 100 RPM)
  ☐ Check both motors (may have slight differences)

Final validation:
  ☐ Rise time < 1 second
  ☐ Overshoot < 10%
  ☐ Steady-state error < 1%
  ☐ No oscillation
  ☐ Smooth acceleration/deceleration
  ☐ Stable under light load
  ☐ Performance consistent for 10+ test cycles

================================================================================
TYPICAL TUNING SESSION TIMELINE
===============================

Time Required: 20-30 minutes for complete system

1. Setup & Testing (5 min)
   - Upload firmware
   - Verify basic motor response
   
2. P-Only Tuning (8 min)
   - Find initial Kp (conservative start)
   - Increase until see overshoot
   - Back off for stable response
   
3. I-Term Tuning (5 min)
   - Add Ki to eliminate steady-state error
   - Adjust for stability
   
4. D-Term Tuning (5 min)
   - Add Kd to smooth transient response
   - Adjust overshoot vs. rise time trade-off
   
5. Verification (5 min)
   - Test multiple setpoints
   - Test under load
   - Document final values

SUCCESS CRITERIA: Motors respond smoothly to /cmd_vel, odometry stable,
no overshoot or oscillation, steady-state error ≤ 1%

================================================================================
