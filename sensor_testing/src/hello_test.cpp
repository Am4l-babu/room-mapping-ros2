#include <Arduino.h>

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n\n=== HELLO WORLD TEST ===");
    Serial.println("Serial communication is working!");
}

void loop() {
    Serial.print(".");
    delay(1000);
}
