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

bool stateGreen = false;
bool stateRed = false;

void setLED(uint8_t pin, bool state) {
    if (pin == LED_GREEN) {
        stateGreen = state;
        pinMode(LED_GREEN, OUTPUT);
        uint8_t lvl = (LED_GREEN_ACTIVE_HIGH ? (state ? HIGH : LOW) : (state ? LOW : HIGH));
        digitalWrite(LED_GREEN, lvl);
    }
    if (pin == LED_RED) {
        stateRed = state;
        pinMode(LED_RED, OUTPUT);
        uint8_t lvl = (LED_RED_ACTIVE_HIGH ? (state ? HIGH : LOW) : (state ? LOW : HIGH));
        digitalWrite(LED_RED, lvl);
        Serial.printf("[LED] Red LED (GPIO %d) set to %s (level: %s)\n", LED_RED, state ? "ON" : "OFF", lvl == HIGH ? "HIGH" : "LOW");
    }
}

void clearAllLEDs() {
    stateGreen = false;
    stateRed = false;
    pinMode(LED_GREEN, OUTPUT);
    digitalWrite(LED_GREEN, LED_GREEN_ACTIVE_HIGH ? LOW : HIGH);
    pinMode(LED_RED, OUTPUT);
    digitalWrite(LED_RED, LED_RED_ACTIVE_HIGH ? LOW : HIGH);
}

void updateLEDs() {
    // Continuously enforce desired LED output state
    // Prevents matrix keypad scanning on shared/adjacent GPIOs from clearing or floating the LEDs
    if (stateGreen) {
        pinMode(LED_GREEN, OUTPUT);
        digitalWrite(LED_GREEN, LED_GREEN_ACTIVE_HIGH ? HIGH : LOW);
    }
    if (stateRed) {
        pinMode(LED_RED, OUTPUT);
        digitalWrite(LED_RED, LED_RED_ACTIVE_HIGH ? HIGH : LOW);
    }
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
