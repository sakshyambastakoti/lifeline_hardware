#ifndef LORA_COMM_H
#define LORA_COMM_H

#include "Config.h"

extern bool loraInitialized;

struct FullTelemetryData {
    int deviceId;
    char emergencyCode;    // 'N', 'F', 'L', 'Q', 'S', etc.
    int alertIndex;        // Mapped 0-14 for LCD UI
    float temperature;
    float humidity;
    int gasPpm;
    double latitude;
    double longitude;
    int altitude;
    int healthScore;
    int riskScore;
    int rssi;
    bool isFullTelemetry;
};

bool initLoRa();
bool parseLoRaPacket(int& deviceId, int& alertIndex, int& rssi);
bool parseLoRaPacketExtended(FullTelemetryData& telemetry);

#endif // LORA_COMM_H
