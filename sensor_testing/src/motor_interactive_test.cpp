#include "motors_encoders.h"

// Motor control parameters
int motor_speed = 150;  // Default speed (0-255)
char last_command = 's';

void print_menu() {
  Serial.println("\n╔════════════════════════════════════════════╗");
  Serial.println("║      MOTOR DRIVER INTERACTIVE TEST         ║");
  Serial.println("╚════════════════════════════════════════════╝");
  Serial.println("\nCommands:");
  Serial.println("  f - Forward (both motors)");
  Serial.println("  b - Backward (both motors)");
  Serial.println("  r - Right turn (left motor faster)");
  Serial.println("  l - Left turn (right motor faster)");
  Serial.println("  s - Stop");
  Serial.println("  + - Increase speed");
  Serial.println("  - - Decrease speed");
  Serial.println("  ? - Show menu");
  Serial.print("\nCurrent Speed: ");
  Serial.print(motor_speed);
  Serial.println("/255");
}

void execute_command(char cmd) {
  switch(cmd) {
    case 'f':
    case 'F':
      Serial.println("→ FORWARD");
      set_motor1_speed(motor_speed);
      set_motor2_speed(motor_speed);
      last_command = 'f';
      break;
      
    case 'b':
    case 'B':
      Serial.println("← BACKWARD");
      set_motor1_speed(-motor_speed);
      set_motor2_speed(-motor_speed);
      last_command = 'b';
      break;
      
    case 'r':
    case 'R':
      Serial.println("↻ RIGHT TURN");
      set_motor1_speed(motor_speed);
      set_motor2_speed(motor_speed / 2);  // Right motor slower
      last_command = 'r';
      break;
      
    case 'l':
    case 'L':
      Serial.println("↺ LEFT TURN");
      set_motor1_speed(motor_speed / 2);  // Left motor slower
      set_motor2_speed(motor_speed);
      last_command = 'l';
      break;
      
    case 's':
    case 'S':
      Serial.println("⏹ STOP");
      set_motor1_speed(0);
      set_motor2_speed(0);
      last_command = 's';
      break;
      
    case '+':
      if (motor_speed < 255) {
        motor_speed = min(255, motor_speed + 25);
        Serial.print("⬆ Speed increased to: ");
        Serial.println(motor_speed);
      } else {
        Serial.println("⚠ Already at maximum speed (255)");
      }
      break;
      
    case '-':
      if (motor_speed > 25) {
        motor_speed = max(25, motor_speed - 25);
        Serial.print("⬇ Speed decreased to: ");
        Serial.println(motor_speed);
      } else {
        Serial.println("⚠ Already at minimum speed (25)");
      }
      break;
      
    case '?':
      print_menu();
      break;
      
    default:
      Serial.println("❌ Unknown command. Type '?' for help");
      break;
  }
}

void display_status() {
  Serial.print("\n[Speed: ");
  Serial.print(motor_speed);
  Serial.print("/255] Motor 1 RPM: ");
  Serial.print(get_motor1_rpm(), 2);
  Serial.print(" | Motor 2 RPM: ");
  Serial.print(get_motor2_rpm(), 2);
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n\n╔════════════════════════════════════════════╗");
  Serial.println("║   ESP32 MOTOR DRIVER TEST - INTERACTIVE     ║");
  Serial.println("╚════════════════════════════════════════════╝");
  
  // Initialize motors and encoders
  Serial.println("\nInitializing motor driver (L298N)...");
  setup_motors();
  Serial.println("  ✓ Motors initialized");
  
  Serial.println("Initializing encoders (HC-89)...");
  setup_encoders();
  Serial.println("  ✓ Encoders initialized\n");
  
  print_menu();
  Serial.print("\n> ");
}

void loop() {
  // Check for serial input
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    
    // Skip newlines and carriage returns
    if (cmd == '\n' || cmd == '\r') {
      Serial.print("\n> ");
      return;
    }
    
    Serial.print(cmd);  // Echo command
    Serial.print(" ");
    
    execute_command(cmd);
    
    // Repeat current command to maintain motion
    if (cmd == 's' || cmd == 'S' || cmd == '?') {
      Serial.print("\n> ");
    } else {
      display_status();
      Serial.print("> ");
    }
  }
  
  // Update RPM calculations in background
  update_rpm_calculation();
  
  delay(50);  // Small delay for stability
}
