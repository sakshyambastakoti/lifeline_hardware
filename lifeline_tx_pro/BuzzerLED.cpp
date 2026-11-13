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

void playSirenTone() {
    // 3 cycles of escalating warble siren
    for (int cycle = 0; cycle < 3; cycle++) {
        for (int f = 1600; f <= 3200; f += 200) {
            executeBeep(f, 25);
        }
        for (int f = 3200; f >= 1600; f -= 200) {
            executeBeep(f, 25);
        }
    }
    stopBuzzer();
}

void playSosTone() {
    // SOS Morse Code: 3 Short (100ms), 3 Long (300ms), 3 Short (100ms)
    const uint16_t freq = 2400;
    for (int i = 0; i < 3; i++) {
        executeBeep(freq, 100);
        delay(80);
    }
    delay(150);
    for (int i = 0; i < 3; i++) {
        executeBeep(freq, 280);
        delay(80);
    }
    delay(150);
    for (int i = 0; i < 3; i++) {
        executeBeep(freq, 100);
        delay(80);
    }
    stopBuzzer();
}

// Advanced Web-Controllable Buzzer Functions
static bool buzzerActive = false;
static bool buzzerTimed = false;
static unsigned long buzzerOffTime = 0;
static uint16_t currentBuzzerFreq = 2000;

void setBuzzerState(bool active, uint16_t freq) {
    if (active) {
        if (freq < 100) freq = 2000;
        buzzerActive = true;
        buzzerTimed = false;
        currentBuzzerFreq = freq;
#if defined(BUZZER_IS_PASSIVE) && !BUZZER_IS_PASSIVE
        digitalWrite(BUZZER_PIN, HIGH);
#else
        tone(BUZZER_PIN, freq);
#endif
        Serial.printf("[BUZZER] State -> ON (Freq: %d Hz)\n", freq);
    } else {
        buzzerActive = false;
        buzzerTimed = false;
#if defined(BUZZER_IS_PASSIVE) && !BUZZER_IS_PASSIVE
        digitalWrite(BUZZER_PIN, LOW);
#else
        noTone(BUZZER_PIN);
        digitalWrite(BUZZER_PIN, LOW);
#endif
        Serial.println(F("[BUZZER] State -> OFF"));
    }
}

void triggerBuzzer(uint16_t freq, uint32_t durationMs) {
    if (durationMs == 0) {
        stopBuzzer();
        return;
    }
    setBuzzerState(true, freq);
    buzzerTimed = true;
    buzzerOffTime = millis() + durationMs;
    Serial.printf("[BUZZER] Triggered -> %d Hz for %u ms\n", freq, (unsigned int)durationMs);
}

void stopBuzzer() {
    setBuzzerState(false, currentBuzzerFreq);
}

bool isBuzzerActive() {
    return buzzerActive;
}

uint16_t getCurrentBuzzerFreq() {
    return currentBuzzerFreq;
}

void updateBuzzer() {
    if (buzzerActive && buzzerTimed) {
        if (millis() >= buzzerOffTime) {
            stopBuzzer();
        }
    }
}
