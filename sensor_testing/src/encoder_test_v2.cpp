#include <Arduino.h>

// ============= ENCODER CONFIGURATION =============
#define ENCODER1_PIN 34     // Motor 1 encoder - ADC input
#define ENCODER2_PIN 35     // Motor 2 encoder - ADC input
#define ENCODER_SLOTS 20    // 20 slots per revolution

// ============= GLOBAL VARIABLES =============
unsigned long encoder1_count = 0;
unsigned long encoder2_count = 0;
int encoder1_last_state = 0;
int encoder2_last_state = 0;

unsigned long last_sample_time = 0;
unsigned long last_print_time = 0;
float motor1_rpm = 0.0;
float motor2_rpm = 0.0;

// ============= SETUP =============
void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n\n====================================");
    Serial.println("   ENCODER TEST v2 - Polling Mode");
    Serial.println("====================================\n");
    
    // Setup encoder pins as inputs
    pinMode(ENCODER1_PIN, INPUT);
    pinMode(ENCODER2_PIN, INPUT);
    
    Serial.print("Encoder 1: GPIO 34");
    Serial.println();
    Serial.print("Encoder 2: GPIO 35");
    Serial.println();
    
    Serial.println("\nPolling encoder pins every 1ms");
    Serial.println("Waiting for encoder pulses...");
    Serial.println("(Rotate motor wheels or manually trigger encoders)\n");
    
    encoder1_last_state = digitalRead(ENCODER1_PIN);
    encoder2_last_state = digitalRead(ENCODER2_PIN);
    
    last_sample_time = millis();
    last_print_time = millis();
}

// ============= MAIN LOOP =============
void loop() {
    // Poll encoder pins
    int encoder1_state = digitalRead(ENCODER1_PIN);
    int encoder2_state = digitalRead(ENCODER2_PIN);
    
    // Detect rising edge (pulse)
    if (encoder1_state && !encoder1_last_state) {
        encoder1_count++;
    }
    if (encoder2_state && !encoder2_last_state) {
        encoder2_count++;
    }
    
    encoder1_last_state = encoder1_state;
    encoder2_last_state = encoder2_state;
    
    // Calculate RPM every 1 second
    unsigned long current_time = millis();
    if (current_time - last_sample_time >= 1000) {
        motor1_rpm = (encoder1_count * 60.0) / ENCODER_SLOTS;
        motor2_rpm = (encoder2_count * 60.0) / ENCODER_SLOTS;
        
        encoder1_count = 0;
        encoder2_count = 0;
        last_sample_time = current_time;
    }
    
    // Print status every 500ms
    if (current_time - last_print_time >= 500) {
        Serial.println("========================================");
        Serial.print("Motor 1 - RPM: ");
        Serial.println(motor1_rpm, 2);
        Serial.print("Motor 2 - RPM: ");
        Serial.println(motor2_rpm, 2);
        Serial.println("========================================\n");
        last_print_time = current_time;
    }
    
    delay(1);  // Poll every 1ms
}
