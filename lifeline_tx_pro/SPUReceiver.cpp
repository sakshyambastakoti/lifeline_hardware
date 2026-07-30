#include "SPUReceiver.h"
#include "Config.h"

#define SPU_UART_RX_PIN 34
#define SPU_UART_TX_PIN 35
#define SPU_UART_BAUD   115200

static HardwareSerial spuSerial(2);
static TelemetryPacket currentTelemetry;
static bool receivedValidData = false;
static unsigned long lastReceiveTime = 0;

static uint8_t rxBuffer[sizeof(TelemetryPacket)];
static size_t rxIndex = 0;

void initSPUReceiver() {
    spuSerial.begin(SPU_UART_BAUD, SERIAL_8N1, SPU_UART_RX_PIN, SPU_UART_TX_PIN);
    Serial.println(F("[SPU RX] Initialized Hardware Serial2 (RX: GPIO 34) for SPU link."));
}

bool updateSPUReceiver() {
    bool packetReceived = false;
    
    while (spuSerial.available() > 0) {
        uint8_t byteIn = spuSerial.read();

        // Looking for start header magic bytes 'L' and 'F'
        if (rxIndex == 0 && byteIn != PROTOCOL_MAGIC_BYTE1) {
            continue;
        }
        if (rxIndex == 1 && byteIn != PROTOCOL_MAGIC_BYTE2) {
            rxIndex = 0;
            if (byteIn == PROTOCOL_MAGIC_BYTE1) {
                rxBuffer[0] = byteIn;
                rxIndex = 1;
            }
            continue;
        }

        rxBuffer[rxIndex++] = byteIn;

        // Received full structure length
        if (rxIndex >= sizeof(TelemetryPacket)) {
            rxIndex = 0; // Reset index for next packet

            TelemetryPacket* candidate = (TelemetryPacket*)rxBuffer;
            
            // Validate CRC16 checksum
            uint16_t expected_crc = calculate_crc16((const uint8_t*)candidate, sizeof(TelemetryPacket) - sizeof(uint16_t));
            if (candidate->crc16 == expected_crc) {
                memcpy(&currentTelemetry, candidate, sizeof(TelemetryPacket));
                receivedValidData = true;
                lastReceiveTime = millis();
                packetReceived = true;
                
                Serial.printf("[SPU RX] Valid Telemetry! Code: '%c', Temp: %.1fC, Gas: %u PPM, Health: %u%%\n",
                              currentTelemetry.emergency_code,
                              currentTelemetry.temp_c_x10 / 10.0,
                              currentTelemetry.gas_ppm,
                              currentTelemetry.health_score);
            } else {
                Serial.printf("[SPU RX] CRC Error! Expected 0x%04X, Got 0x%04X\n", expected_crc, candidate->crc16);
            }
        }
    }
    
    return packetReceived;
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

