#include "BuzzerLED.h"

// Universal Beep Driver: Compatible with both Active and Passive Buzzers
static void executeBeep(uint16_t freq, uint16_t durationMs) {
    // 1. Frequency driver for Passive Buzzers
    tone(BUZZER_PIN, freq, durationMs);
    
    // 2. High-level pulse driver for Active Buzzers
    digitalWrite(BUZZER_PIN, HIGH);
    delay(durationMs);
    digitalWrite(BUZZER_PIN, LOW);
    
    noTone(BUZZER_PIN);
}

void initBuzzerLED() {
    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_RED, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
    clearAllLEDs();
    
    // Startup Power-on Beep Sequence (2 short beeps)
    executeBeep(2000, 50);
    delay(50);
    executeBeep(2500, 50);
}

void setLED(uint8_t pin, bool state) {
    digitalWrite(pin, state ? HIGH : LOW);
}

void clearAllLEDs() {
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED, LOW);
}

void playSuccessTone() {
    executeBeep(2200, 80);
    delay(40);
    executeBeep(2700, 100);
}

void playErrorTone() {
    executeBeep(1200, 100);
    delay(40);
    executeBeep(800, 150);
}

void playConfirmTone() {
    executeBeep(2500, 80);
}

void playClickTone() {
    executeBeep(2200, 40);
}

void playNavigateTone() {
    executeBeep(1800, 30);
}
