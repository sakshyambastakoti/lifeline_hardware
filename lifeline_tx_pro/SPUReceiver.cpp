#include "SPUReceiver.h"
#include "Config.h"
#include "DisplayUI.h"
#include <WiFi.h>
#include <esp_now.h>

static TelemetryPacket currentTelemetry;
static bool receivedValidData = false;
static unsigned long lastReceiveTime = 0;
static volatile bool newEspNowPacketReceived = false;
static bool espNowInitialized = false;

// ESP-NOW Receive Callback
#if defined(ESP_IDF_VERSION_MAJOR) && ESP_IDF_VERSION_MAJOR >= 5
static void onESPNowDataRecv(const esp_now_recv_info_t * esp_now_info, const uint8_t *incomingData, int len)
#else
static void onESPNowDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len)
#endif
{
    if (len != sizeof(TelemetryPacket)) {
        return;
    }

    const TelemetryPacket* candidate = (const TelemetryPacket*)incomingData;

    // Check magic bytes 'L' and 'F'
    if (candidate->magic1 != PROTOCOL_MAGIC_BYTE1 || candidate->magic2 != PROTOCOL_MAGIC_BYTE2) {
        return;
    }

    // Validate CRC16 checksum
    uint16_t expected_crc = calculate_crc16((const uint8_t*)candidate, sizeof(TelemetryPacket) - sizeof(uint16_t));
    if (candidate->crc16 == expected_crc) {
        memcpy(&currentTelemetry, candidate, sizeof(TelemetryPacket));
        receivedValidData = true;
        lastReceiveTime = millis();
        newEspNowPacketReceived = true;

        #if SERIAL_DEBUG_ENABLED
        Serial.printf("[ESP-NOW RX] SPU Telemetry Received! Code: '%c', Temp: %.1fC, Gas: %u PPM, Health: %u%%\n",
                      currentTelemetry.emergency_code,
                      currentTelemetry.temp_c_x10 / 10.0,
                      currentTelemetry.gas_ppm,
                      currentTelemetry.health_score);
        #endif
    } else {
        #if SERIAL_DEBUG_ENABLED
        Serial.printf("[ESP-NOW RX] CRC Error! Expected 0x%04X, Got 0x%04X\n", expected_crc, candidate->crc16);
        #endif
    }
}

void initSPUReceiver() {
    // Set Wi-Fi STA mode for ESP-NOW listener
    if (WiFi.getMode() == WIFI_OFF) {
        WiFi.mode(WIFI_STA);
    }

    if (esp_now_init() != ESP_OK) {
        Serial.println(F("[SPU RX] Error initializing ESP-NOW Receiver on TX unit!"));
        espNowInitialized = false;
        return;
    }

    esp_now_register_recv_cb(onESPNowDataRecv);
    espNowInitialized = true;
    Serial.println(F("[SPU RX] Initialized ESP-NOW Wireless Telemetry Receiver on TX unit."));
}

bool isESPNowInitialized() {
    return espNowInitialized;
}

bool updateSPUReceiver() {
    if (newEspNowPacketReceived) {
        newEspNowPacketReceived = false;
        return true;
    }
    return false;
}

bool hasSPUTelemetry() {
    // Valid if telemetry received within last 10 seconds
    return receivedValidData && (millis() - lastReceiveTime <= 10000);
}

TelemetryPacket getLatestSPUTelemetry() {
    return currentTelemetry;
}

unsigned long getSPULastReceiveTime() {
    return lastReceiveTime;
}

int mapSPUEmergencyToAlertIndex(char spuCode) {
    switch (spuCode) {
        case EMERGENCY_SOS:         return 0;  // SOS / EMERGENCY
        case EMERGENCY_MEDICAL:     return 1;  // MEDICAL EMERGENCY
        case EMERGENCY_FIRE:        return 0;  // FIRE / HIGH TEMP
        case EMERGENCY_LANDSLIDE:   return 11; // LANDSLIDE
        case EMERGENCY_GAS:         return 0;  // HIGH GAS LEAK
        case EMERGENCY_EARTHQUAKE:  return 0;  // FALL / VIBRATION
        case EMERGENCY_BATTERY:     return 13; // LOW BATTERY / EQUIPMENT FAIL
        default:                    return 0;  // GENERAL EMERGENCY
    }
}
