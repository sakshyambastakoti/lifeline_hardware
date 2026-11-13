#ifndef BUZZER_LED_H
#define BUZZER_LED_H

#include "Config.h"

// Initialize Pins for buzzer and LEDs
void initBuzzerLED();

// Alarm & UI feedback tones
void playBootTone();
void playWiFiSuccessTone();
void playWiFiFailTone();
void playCountdownTickTone();
void playPortalOpenTone();
void playSkipConfirmTone();
void playReturnIdleTone();
void playAlertTone(int priority);
void playRxBeep();

// Status/Data LED indicators
void triggerRxBlink();
void updateLEDs();

// Advanced Web-Controllable Buzzer Functions
void setBuzzerState(bool active, uint16_t freq = 2000);
void triggerBuzzer(uint16_t freq, uint32_t durationMs);
void stopBuzzer();
bool isBuzzerActive();
uint16_t getCurrentBuzzerFreq();
void updateBuzzer();
void playSirenTone();
void playSosTone();
void playSuccessTone();
void playErrorTone();

#endif // BUZZER_LED_H
