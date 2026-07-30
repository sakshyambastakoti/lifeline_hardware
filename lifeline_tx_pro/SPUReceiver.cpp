#include "SPUReceiver.h"
#include "Config.h"
#include "DisplayUI.h"

static HardwareSerial spuSerial(2);
static TelemetryPacket currentTelemetry;
static bool receivedValidData = false;
static unsigned long lastReceiveTime = 0;

void initSPUReceiver() {
    spuSerial.begin(115200, SERIAL_8N1, SPU_RX_PIN, -1);
    Serial.printf("[SPU RX] Initialized Hardware Serial2 Telemetry Receiver (GPIO %d @ 115200 baud).\n", SPU_RX_PIN);
}

bool updateSPUReceiver() {
    bool newPacketFound = false;

    while (spuSerial.available() >= sizeof(TelemetryPacket)) {
        if (spuSerial.peek() != PROTOCOL_MAGIC_BYTE1) {
            spuSerial.read(); // Align to magic byte
            continue;
        }

        TelemetryPacket candidate;
        spuSerial.readBytes((uint8_t*)&candidate, sizeof(TelemetryPacket));

        if (candidate.magic1 == PROTOCOL_MAGIC_BYTE1 && candidate.magic2 == PROTOCOL_MAGIC_BYTE2) {
            uint16_t expected_crc = calculate_crc16((const uint8_t*)&candidate, sizeof(TelemetryPacket) - sizeof(uint16_t));
            if (candidate.crc16 == expected_crc) {
                memcpy(&currentTelemetry, &candidate, sizeof(TelemetryPacket));
                receivedValidData = true;
                lastReceiveTime = millis();
                newPacketFound = true;

                #if SERIAL_DEBUG_ENABLED
                Serial.printf("[UART RX] SPU Telemetry Received! Code: '%c', Temp: %.1fC, Gas: %u PPM, Health: %u%%\n",
                              currentTelemetry.emergency_code,
                              currentTelemetry.temp_c_x10 / 10.0,
                              currentTelemetry.gas_ppm,
                              currentTelemetry.health_score);
                #endif
            } else {
                #if SERIAL_DEBUG_ENABLED
                Serial.printf("[UART RX] CRC Error! Expected 0x%04X, Got 0x%04X\n", expected_crc, candidate.crc16);
                #endif
            }
        }
    }

    return newPacketFound;
}

bool hasSPUTelemetry() {
    // Valid if telemetry received within last 10 seconds
    return receivedValidData && (millis() - lastReceiveTime <= 10000);
}

bool hasReceivedSPUTelemetry() {
    return receivedValidData;
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
