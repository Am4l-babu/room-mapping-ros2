#include <Arduino.h>

#define ENCODER1_PIN 34     // Motor 1 encoder
#define ENCODER2_PIN 35     // Motor 2 encoder

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n\n====================================");
    Serial.println("   GPIO DEBUG - Raw Pin States");
    Serial.println("====================================\n");
    
    pinMode(ENCODER1_PIN, INPUT);
    pinMode(ENCODER2_PIN, INPUT);
    
    Serial.print("GPIO 34: ");
    Serial.println(ENCODER1_PIN);
    Serial.print("GPIO 35: ");
    Serial.println(ENCODER2_PIN);
    Serial.println("\nReading raw pin states every 100ms...\n");
}

void loop() {
    int pin34 = digitalRead(34);
    int pin35 = digitalRead(35);
    
    Serial.print("GPIO 34: ");
    Serial.print(pin34);
    Serial.print("  |  GPIO 35: ");
    Serial.println(pin35);
    
    delay(100);
}
