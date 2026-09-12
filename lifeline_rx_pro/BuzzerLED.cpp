#include "BuzzerLED.h"
#include "DisplayUI.h"
#include <WiFi.h>

extern bool wifiConnected;
extern bool portalActive;
extern ScreenState currentScreen;

static unsigned long rxBlinkEndTime = 0;
static unsigned long lastWiFiBlinkTime = 0;
static bool wifiBlinkState = false;

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
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(LED_WIFI, OUTPUT);
    pinMode(LED_DATA, OUTPUT);
    
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(LED_WIFI, LOW);
    digitalWrite(LED_DATA, LOW);
}

void playBootTone() {
    executeBeep(1500, 80);
}

void playWiFiSuccessTone() {
    executeBeep(1000, 80);
    delay(80);
    executeBeep(1500, 80);
}

void playWiFiFailTone() {
    executeBeep(1500, 80);
    delay(80);
    executeBeep(1000, 80);
}

void playCountdownTickTone() {
    executeBeep(2000, 50);
}

void playPortalOpenTone() {
    executeBeep(1800, 150);
}

void playSkipConfirmTone() {
    executeBeep(1500, 50);
}

void playReturnIdleTone() {
    executeBeep(1200, 80);
}

void playAlertTone(int priority) {
    // Loud, high-visibility 2500 Hz triple alarm beep sequence for all incoming emergency packets
    executeBeep(2500, 120);
    delay(100);
    executeBeep(2500, 120);
    delay(100);
    executeBeep(2500, 140);
}

void playRxBeep() {
    // Crisp, audible confirmation chirp when packet is received
    executeBeep(2400, 80);
    delay(40);
    executeBeep(2700, 80);
}

void triggerRxBlink() {
    uint8_t dataOnState = LED_DATA_ACTIVE_HIGH ? HIGH : LOW;
    digitalWrite(LED_DATA, dataOnState);
    rxBlinkEndTime = millis() + RX_BLINK_DURATION_MS;
    playRxBeep();
}

void updateLEDs() {
    unsigned long now = millis();

    // Strictly sync wifiConnected variable with actual hardware WiFi status
    bool isConnected = (WiFi.status() == WL_CONNECTED);
    wifiConnected = isConnected;

    uint8_t wifiOnState  = LED_WIFI_ACTIVE_HIGH ? HIGH : LOW;
    uint8_t wifiOffState = LED_WIFI_ACTIVE_HIGH ? LOW  : HIGH;
    uint8_t dataOnState  = LED_DATA_ACTIVE_HIGH ? HIGH : LOW;
    uint8_t dataOffState = LED_DATA_ACTIVE_HIGH ? LOW  : HIGH;

    // 1. WiFi LED status update
    if (portalActive || currentScreen == SCREEN_PORTAL) {
        // Fast blink (100 ms interval)
        if (now - lastWiFiBlinkTime >= 100) {
            lastWiFiBlinkTime = now;
            wifiBlinkState = !wifiBlinkState;
            digitalWrite(LED_WIFI, wifiBlinkState ? wifiOnState : wifiOffState);
        }
    } else if (currentScreen == SCREEN_COUNTDOWN) {
        // Slow blink (500 ms interval)
        if (now - lastWiFiBlinkTime >= 500) {
            lastWiFiBlinkTime = now;
            wifiBlinkState = !wifiBlinkState;
            digitalWrite(LED_WIFI, wifiBlinkState ? wifiOnState : wifiOffState);
        }
    } else if (isConnected) {
        digitalWrite(LED_WIFI, wifiOnState);
    } else {
        digitalWrite(LED_WIFI, wifiOffState);
    }

    // 2. Data RX LED blink timeout check
    if (rxBlinkEndTime > 0 && millis() < rxBlinkEndTime) {
        digitalWrite(LED_DATA, dataOnState);
    } else {
        digitalWrite(LED_DATA, dataOffState);
        rxBlinkEndTime = 0;
    }
}
