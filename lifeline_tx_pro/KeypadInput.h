#ifndef KEYPAD_INPUT_H
#define KEYPAD_INPUT_H

#include "Config.h"
#include "BuzzerLED.h"
#include "DisplayUI.h"
#include "LoRaComm.h"
#include <Keypad.h>

// Custom Keypad driver that protects shared GPIO 13 (LED_RED) from being floated or suppressed when active
class SharedKeypad : public Keypad {
public:
    SharedKeypad(char *userKeymap, byte *row, byte *col, byte numRows, byte numCols)
        : Keypad(userKeymap, row, col, numRows, numCols) {}
        
    void pin_mode(byte pinNum, byte mode) override {
        // If Red LED is actively ON, prevent keypad scan from leaving GPIO 13 in high-impedance INPUT
        if (pinNum == LED_RED && stateRed) {
            pinMode(LED_RED, OUTPUT);
            return;
        }
        pinMode(pinNum, mode);
    }
    
    void pin_write(byte pinNum, boolean level) override {
        if (pinNum == LED_RED && stateRed) {
            if (level == LOW) {
                // Brief pulse to sample row inputs
                digitalWrite(LED_RED, LOW);
            } else {
                uint8_t activeLvl = LED_RED_ACTIVE_HIGH ? HIGH : LOW;
                digitalWrite(LED_RED, activeLvl);
                pinMode(LED_RED, OUTPUT);
            }
            return;
        }
        digitalWrite(pinNum, level);
    }
};

extern SharedKeypad keypad;

void initKeypad();
char getKeyWithRepeat();
char readSerialKey();
void printDebugHeader();

void handleKeyPress(char key);
void handleMenuInput(char key);
void handleConfirmInput(char key);
void handleResultInput(char key);
void handleSystemInfoInput(char key);
void handleUserManualInput(char key);
void handleBLEPortalInput(char key);
void handleMessagePopupInput(char key);

#endif // KEYPAD_INPUT_H
