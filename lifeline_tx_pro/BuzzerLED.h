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

#endif // BUZZER_LED_H
