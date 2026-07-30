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
#include "SPUReceiver.h"

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    delay(100);
    
    #if SERIAL_DEBUG_ENABLED
    printDebugHeader();
    #endif
    
    Serial.println(F("[INIT] Starting LifeLine TX..."));
    
    // 1. Immediately deselect SPI Chip Selects to prevent bus contention
    pinMode(LORA_CS, OUTPUT);
    digitalWrite(LORA_CS, HIGH);
    pinMode(TFT_CS, OUTPUT);
    digitalWrite(TFT_CS, HIGH);
    
    // 2. Initialize shared SPI bus with custom pins (SCK: 5, MISO: 17, MOSI: 27)
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
    
    // 3. Initialize Subsystems & SPU Receiver (UART RX: GPIO 34)
    initBuzzerLED();
    initKeypad();
    initDisplay();
    initLoRa();
    initSPUReceiver();
    
    // Boot Screen
    currentScreen = SCREEN_BOOT;
    drawBootScreen();
    
    // LED flash
    setLED(LED_GREEN, true);
    setLED(LED_RED, true);
    clearAllLEDs();
    
    Serial.println(F("[INIT] Ready"));
    Serial.printf("[INIT] Device: TX #%03d\n", DEVICE_ID);
}

static bool bootupTelemetrySent = false;
static unsigned long lastPeriodicLoraTx = 0;
#define LORA_PERIODIC_INTERVAL 3600000UL // 1 Hour (3,600,000 ms)

void loop() {
    handleOTA();
    
    // Process incoming SPU telemetry & automatic emergencies
    if (updateSPUReceiver()) {
        TelemetryPacket spuPkt = getLatestSPUTelemetry();
        unsigned long now = millis();
        
        // 1. Initial Power-up Boot Log (SPU -> TX -> RX -> Cloud)
        if (!bootupTelemetrySent) {
            bootupTelemetrySent = true;
            lastPeriodicLoraTx = now;
            Serial.println(F("[BOOT LORA] Power-up telemetry log transmitted over LoRa -> RX -> Cloud!"));
            transmitSPUTelemetry(spuPkt);
        }
        // 2. Automatic Emergency Triggered by SPU
        else if (spuPkt.emergency_code != EMERGENCY_NONE && spuPkt.emergency_code != 0) {
            Serial.printf("[AUTO ALERT] Automatic emergency received from SPU: '%c'\n", spuPkt.emergency_code);
            lastPeriodicLoraTx = now;
            
            selectedAlertIndex = mapSPUEmergencyToAlertIndex(spuPkt.emergency_code);
            currentScreen = SCREEN_SENDING;
            drawSendingScreen();
            
            playErrorTone();
            lastTransmitSuccess = transmitSPUTelemetry(spuPkt);
            
            currentScreen = SCREEN_RESULT;
            drawResultScreen();
        }
        // 3. Periodic 1-Hour Routine LoRa Telemetry Heartbeat
        else if (now - lastPeriodicLoraTx >= LORA_PERIODIC_INTERVAL) {
            lastPeriodicLoraTx = now;
            Serial.println(F("[1-HR LORA] Transmitting 1-hour routine telemetry log over LoRa -> RX -> Cloud..."));
            transmitSPUTelemetry(spuPkt);
        }
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
        case SCREEN_SENSOR_LOG:
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
