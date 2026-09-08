/*
 * ═══════════════════════════════════════════════════════════════════════════════════
 *                        ╔═════════════════════════════════════╗
 *                        ║   LIFELINE EMERGENCY TRANSMITTER    ║
 *                        ║   Professional Field Unit v3.1      ║
 *                        ╚═════════════════════════════════════╝
 * ═══════════════════════════════════════════════════════════════════════════════════
 */

#include <SPI.h>
#include "Config.h"
#include "BuzzerLED.h"
#include "DisplayUI.h"
#include "LoRaComm.h"
#include "KeypadInput.h"
#include "OTAManager.h"
#include "BLEManager.h"

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    delay(100);
    
    #if SERIAL_DEBUG_ENABLED
    printDebugHeader();
    #endif
    
    Serial.println(F("[INIT] Starting LifeLine TX (Manual SOS Unit)..."));
    
    // 1. Immediately deselect SPI Chip Selects to prevent bus contention
    pinMode(LORA_CS, OUTPUT);
    digitalWrite(LORA_CS, HIGH);
    pinMode(TFT_CS, OUTPUT);
    digitalWrite(TFT_CS, HIGH);
    
    // 2. Initialize shared SPI bus with custom pins (SCK: 5, MISO: 17, MOSI: 27)
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
    
    // 3. Initialize Subsystems (Buzzer/LED, Keypad, TFT Display, LoRa, BLE)
    initBuzzerLED();
    initKeypad();
    initDisplay();
    initLoRa();
    initBLE();
    
    // Boot Screen
    currentScreen = SCREEN_BOOT;
    drawBootScreen();
    
    // LED flash
    setLED(LED_GREEN, true);
    setLED(LED_RED, true);
    clearAllLEDs();
    
    Serial.println(F("[INIT] Ready"));
    Serial.printf("[INIT] Device: TX #%03d (Manual SOS + BLE Companion Mode)\n", DEVICE_ID);
}

void loop() {
    handleOTA();
    updateBLE();
    
    // Process incoming BLE mobile chat message
    if (hasPendingBLEChatMessage()) {
        String chat = getPendingBLEChatMessage();
        Serial.printf("[BLE CHAT TRIGGER] Transmitting: '%s'\n", chat.c_str());
        currentScreen = SCREEN_SENDING;
        drawSendingScreen();
        lastTransmitSuccess = transmitChatMessage(chat);
        currentScreen = SCREEN_RESULT;
        drawResultScreen();
    }
    
    // Process incoming BLE mobile alert trigger
    if (hasPendingBLEAlert()) {
        char code = getPendingBLEAlert();
        Serial.printf("[BLE ALERT TRIGGER] Code: '%c'\n", code);
        int targetIdx = 0;
        for (int i = 0; i < ALERT_COUNT; i++) {
            if (getAlertCode(i) == code) {
                targetIdx = i;
                break;
            }
        }
        selectedAlertIndex = targetIdx;
        currentScreen = SCREEN_SENDING;
        drawSendingScreen();
        lastTransmitSuccess = transmitAlertWithAck(selectedAlertIndex);
        currentScreen = SCREEN_RESULT;
        drawResultScreen();
    }
    
    switch (currentScreen) {
        case SCREEN_BOOT:
            if (millis() - bootStartTime >= BOOT_DISPLAY_TIME) {
                currentScreen = SCREEN_MENU;
                drawMenuScreen();
                Serial.println(F("[STATE] -> MENU"));
            }
            break;
            
        case SCREEN_MENU:
        case SCREEN_CONFIRM:
        case SCREEN_SYSTEM_INFO:
        case SCREEN_USER_MANUAL:
        case SCREEN_OTA:
        case SCREEN_OTA_SELECT:
            {
                char key = keypad.getKey();
                #if SERIAL_DEBUG_ENABLED
                if (!key) {
                    key = readSerialKey();
                    if (key) Serial.printf("[SERIAL] Key: %c\n", key);
                }
                #endif
                if (key) handleKeyPress(key);
            }
            break;

        case SCREEN_SENSOR_LOG:
            {
                static unsigned long lastLogRefresh = 0;
                if (millis() - lastLogRefresh >= 1000) {
                    lastLogRefresh = millis();
                    drawSensorLogScreen();
                }

                char key = keypad.getKey();
                #if SERIAL_DEBUG_ENABLED
                if (!key) {
                    key = readSerialKey();
                    if (key) Serial.printf("[SERIAL] Key: %c\n", key);
                }
                #endif
                if (key) handleKeyPress(key);
            }
            break;
            
        case SCREEN_SENDING:
            break;
            
        case SCREEN_RESULT:
            {
                if (lastTransmitSuccess && RESULT_SUCCESS_TIME > 0) {
                    if (millis() - resultStartTime >= RESULT_SUCCESS_TIME) {
                        clearAllLEDs();
                        currentScreen = SCREEN_MENU;
                        drawMenuScreen();
                        Serial.println(F("[STATE] Auto -> MENU"));
                    }
                }
                
                char key = keypad.getKey();
                #if SERIAL_DEBUG_ENABLED
                if (!key) {
                    key = readSerialKey();
                    if (key) Serial.printf("[SERIAL] Key: %c\n", key);
                }
                #endif
                if (key) handleKeyPress(key);
            }
            break;
    }
    
    delay(10);
}
