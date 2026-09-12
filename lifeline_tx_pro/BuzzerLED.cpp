#include "BuzzerLED.h"

// Universal Beep Driver: Compatible with both Active and Passive Buzzers
static void executeBeep(uint16_t freq, uint16_t durationMs) {
#if defined(BUZZER_IS_PASSIVE) && !BUZZER_IS_PASSIVE
    digitalWrite(BUZZER_PIN, HIGH);
    delay(durationMs);
    digitalWrite(BUZZER_PIN, LOW);
#else
    tone(BUZZER_PIN, freq);
    delay(durationMs);
    noTone(BUZZER_PIN);
    digitalWrite(BUZZER_PIN, LOW);
#endif
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
