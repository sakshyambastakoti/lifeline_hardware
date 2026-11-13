#ifndef BUZZER_LED_H
#define BUZZER_LED_H

#include "Config.h"

extern bool stateGreen;
extern bool stateRed;

void initBuzzerLED();
void setLED(uint8_t pin, bool state);
void clearAllLEDs();
void updateLEDs();

void playSuccessTone();
void playErrorTone();
void playConfirmTone();
void playClickTone();
void playNavigateTone();
void playSirenTone();
void playSosTone();

// Advanced Web-Controllable Buzzer Functions
void setBuzzerState(bool active, uint16_t freq = 2000);
void triggerBuzzer(uint16_t freq, uint32_t durationMs);
void stopBuzzer();
bool isBuzzerActive();
uint16_t getCurrentBuzzerFreq();
void updateBuzzer();

#endif // BUZZER_LED_H
