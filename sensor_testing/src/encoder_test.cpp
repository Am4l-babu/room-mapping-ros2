/*
 * Minimal Encoder Test Program
 * Tests HC-89 encoders (encoder pulses and RPM calculation)
 * No motor control - focus on encoder reading only
 */

#include <Arduino.h>

// ============= ENCODER CONFIGURATION =============
#define ENCODER1_PIN 4      // Motor 1 encoder (GPIO 4)
#define ENCODER2_PIN 5      // Motor 2 encoder (GPIO 5)
#define ENCODER_SLOTS 20    // 20 slots per revolution
#define SAMPLE_TIME 1000    // Sample every 1 second

// ============= GLOBAL VARIABLES =============
volatile unsigned long encoder1_count = 0;
volatile unsigned long encoder2_count = 0;
volatile unsigned long encoder1_count_prev = 0;
volatile unsigned long encoder2_count_prev = 0;

unsigned long last_sample_time = 0;
float motor1_rpm = 0.0;
float motor2_rpm = 0.0;

// ============= INTERRUPT SERVICE ROUTINES =============
void IRAM_ATTR encoder1_isr() {
    encoder1_count++;
}

void IRAM_ATTR encoder2_isr() {
    encoder2_count++;
}

// ============= FUNCTION PROTOTYPES =============
void update_rpm_calculation();
void print_status();

// ============= SETUP =============
void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n\n====================================");
    Serial.println("   ENCODER TEST - HC-89 Sensors");
    Serial.println("====================================\n");
    
    // Setup encoder pins
    pinMode(ENCODER1_PIN, INPUT);
    pinMode(ENCODER2_PIN, INPUT);
    
    // Attach interrupts
    attachInterrupt(digitalPinToInterrupt(ENCODER1_PIN), encoder1_isr, RISING);
    attachInterrupt(digitalPinToInterrupt(ENCODER2_PIN), encoder2_isr, RISING);
    
    Serial.println("✓ Encoder 1: GPIO 4 - Ready");
    Serial.println("✓ Encoder 2: GPIO 5 - Ready");
    Serial.println("\nWaiting for encoder pulses...");
    Serial.println("(Rotate motor wheels or manually trigger encoders)\n");
    
    last_sample_time = millis();
}

// ============= RPM CALCULATION =============
void update_rpm_calculation() {
    unsigned long current_time = millis();
    unsigned long time_elapsed = current_time - last_sample_time;
    
    if (time_elapsed >= SAMPLE_TIME) {
        // Calculate Motor 1 RPM
        unsigned long motor1_pulses = encoder1_count - encoder1_count_prev;
        motor1_rpm = (motor1_pulses * 60000.0) / (time_elapsed * ENCODER_SLOTS);
        
        // Calculate Motor 2 RPM
        unsigned long motor2_pulses = encoder2_count - encoder2_count_prev;
        motor2_rpm = (motor2_pulses * 60000.0) / (time_elapsed * ENCODER_SLOTS);
        
        // Update previous counts
        encoder1_count_prev = encoder1_count;
        encoder2_count_prev = encoder2_count;
        last_sample_time = current_time;
    }
}

// ============= MAIN LOOP =============
void loop() {
    // Calculate RPM
    update_rpm_calculation();
    
    // Print status every 500ms
    static unsigned long last_print = 0;
    if (millis() - last_print >= 500) {
        print_status();
        last_print = millis();
    }
    
    delay(10);
}

// ============= DEBUG OUTPUT =============
void print_status() {
    Serial.println("========================================");
    
    // Encoder 1 Status
    Serial.print("Encoder 1 (GPIO 4):");
    Serial.print(" | Pulses: ");
    Serial.print(encoder1_count);
    Serial.print(" | RPM: ");
    Serial.print(motor1_rpm, 2);
    Serial.println();
    
    // Encoder 2 Status
    Serial.print("Encoder 2 (GPIO 5):");
    Serial.print(" | Pulses: ");
    Serial.print(encoder2_count);
    Serial.print(" | RPM: ");
    Serial.print(motor2_rpm, 2);
    Serial.println();
    
    Serial.println("========================================\n");
}

/*
 * USAGE:
 * 1. Upload this code to ESP32
 * 2. Open serial monitor at 115200 baud
 * 3. Manually rotate motors or trigger encoders
 * 4. Watch pulse counts increment
 * 5. RPM calculated every 1 second
 * 
 * EXPECTED OUTPUT:
 * ========================================
 * Encoder 1 (GPIO 4): | Pulses: 245 | RPM: 735.00
 * Encoder 2 (GPIO 5): | Pulses: 238 | RPM: 714.00
 * ========================================
 * 
 * TROUBLESHOOTING:
 * - Pulses not incrementing? Check GPIO connections
 * - RPM shows 0? Rotate motor faster
 * - Wrong RPM? Adjust ENCODER_SLOTS (20 for 20-slot wheel)
 */
