/*
 * PID Motor Control Demo with Serial Interface
 * 
 * IMPROVEMENTS OVER motor_interactive_test.cpp:
 * - Closed-loop RPM control (stable speeds)
 * - 100 Hz control loop (responsive)
 * - Smooth acceleration (no jerky motion)
 * - Compatible with ROS 2 robot_controller.py
 * 
 * Serial Protocol: RPM:m1_target,m2_target,m1_actual,m2_actual
 */

#include "motor_control_pid.h"

// Command parser state
float cmd_linear_velocity = 0.0;  // m/s
float cmd_angular_velocity = 0.0; // rad/s

// Robot parameters
const float wheel_diameter = 0.065;     // 65mm wheels
const float wheel_separation = 0.18;    // 180mm axle
const float max_linear_speed = 0.5;     // m/s
const float max_angular_speed = 2.0;    // rad/s
const float encoder_slots = 20.0;
const float wheel_circumference = M_PI * wheel_diameter;
const float mm_per_slot = (wheel_circumference * 1000.0) / encoder_slots; // ~10.2mm

// Kinematics: Convert velocity to target RPM
void velocity_to_rpm_targets() {
    // Differential drive kinematics
    // v_left = linear - (angular * wheel_separation/2)
    // v_right = linear + (angular * wheel_separation/2)
    
    float v_left = cmd_linear_velocity - (cmd_angular_velocity * wheel_separation / 2.0);
    float v_right = cmd_linear_velocity + (cmd_angular_velocity * wheel_separation / 2.0);
    
    // Clamp velocities to max
    v_left = constrain(v_left, -max_linear_speed, max_linear_speed);
    v_right = constrain(v_right, -max_linear_speed, max_linear_speed);
    
    // Convert m/s to RPM
    // RPM = (v_m_per_s / circumference_m) * 60 sec
    float target_rpm_1 = (v_left / wheel_circumference) * 60.0;
    float target_rpm_2 = (v_right / wheel_circumference) * 60.0;
    
    set_motor1_target_rpm(target_rpm_1);
    set_motor2_target_rpm(target_rpm_2);
}

void send_odom_data_fast() {
    // Send compact format: RPM:m1_tgt,m2_tgt,m1_act,m2_act,p1,p2
    Serial.print("RPM:");
    Serial.print(get_motor1_target_rpm(), 1);
    Serial.print(",");
    Serial.print(get_motor2_target_rpm(), 1);
    Serial.print(",");
    Serial.print(get_motor1_rpm(), 1);
    Serial.print(",");
    Serial.print(get_motor2_rpm(), 1);
    Serial.print(",");
    Serial.print(get_motor1_pulses());
    Serial.print(",");
    Serial.println(get_motor2_pulses());
}

void parse_velocity_command(const char* line) {
    // Format: VEL:linear,angular
    // Example: VEL:0.3,0.5
    
    if (sscanf(line, "VEL:%f,%f", &cmd_linear_velocity, &cmd_angular_velocity) == 2) {
        // Clamp to limits
        cmd_linear_velocity = constrain(cmd_linear_velocity, -max_linear_speed, max_linear_speed);
        cmd_angular_velocity = constrain(cmd_angular_velocity, -max_angular_speed, max_angular_speed);
        
        velocity_to_rpm_targets();
        
        Serial.print("[VEL] Linear: ");
        Serial.print(cmd_linear_velocity, 2);
        Serial.print(" m/s | Angular: ");
        Serial.print(cmd_angular_velocity, 2);
        Serial.println(" rad/s");
    }
}

void parse_rpm_command(const char* line) {
    // Format: RPM_SET:m1_target,m2_target
    // Example: RPM_SET:30,30
    
    float rpm1, rpm2;
    if (sscanf(line, "RPM_SET:%f,%f", &rpm1, &rpm2) == 2) {
        set_motor1_target_rpm(rpm1);
        set_motor2_target_rpm(rpm2);
        
        Serial.print("[RPM_SET] M1: ");
        Serial.print(rpm1, 1);
        Serial.print(" | M2: ");
        Serial.print(rpm2, 1);
        Serial.println(" RPM");
    }
}

void parse_serial_command(const String& cmd) {
    if (cmd.startsWith("VEL:")) {
        parse_velocity_command(cmd.c_str());
    } else if (cmd.startsWith("RPM_SET:")) {
        parse_rpm_command(cmd.c_str());
    } else if (cmd == "STOP") {
        stop_motors_smooth();
        Serial.println("[STOP] Motors stopped smoothly");
    } else if (cmd == "ESTOP") {
        stop_motors_emergency();
        Serial.println("[ESTOP] Emergency stop activated!");
    } else if (cmd == "STATUS") {
        print_motor_control_status();
    } else if (cmd == "HELP") {
        Serial.println("\n╔════════════════ PID MOTOR CONTROL COMMANDS ════════════════╗");
        Serial.println("║ VEL:linear,angular   - Set velocity (m/s, rad/s)              ║");
        Serial.println("║                        Example: VEL:0.3,0.5                  ║");
        Serial.println("║ RPM_SET:m1,m2        - Set target RPM for each motor         ║");
        Serial.println("║                        Example: RPM_SET:30,30                ║");
        Serial.println("║ STOP                 - Smooth deceleration stop             ║");
        Serial.println("║ ESTOP                - Emergency stop (immediate)           ║");
        Serial.println("║ STATUS               - Print control status                 ║");
        Serial.println("║ HELP                 - Show this menu                       ║");
        Serial.println("╚════════════════════════════════════════════════════════════════╝");
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n╔══════════════════════════════════════════════════╗");
    Serial.println("║  ESP32 PID MOTOR CONTROL SYSTEM (IMPROVED)       ║");
    Serial.println("║  Phase 2: Closed-Loop Feedback Control          ║");
    Serial.println("╚══════════════════════════════════════════════════╝");
    
    setup_motor_control_pid();
    
    Serial.println("\nSystem ready. Type 'HELP' for commands.");
}

unsigned long last_odom_send = 0;
const unsigned long ODOM_SEND_INTERVAL = 10; // Send every 10ms (~100 Hz)

void loop() {
    // Core control loop (100 Hz)
    motor_control_update();
    
    // Send odometry data at high frequency
    unsigned long now = millis();
    if (now - last_odom_send >= ODOM_SEND_INTERVAL) {
        last_odom_send = now;
        send_odom_data_fast();
    }
    
    // Handle serial input
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        
        if (cmd.length() > 0) {
            parse_serial_command(cmd);
        }
    }
    
    // Very brief delay to prevent watchdog trigger
    delayMicroseconds(100);
}
