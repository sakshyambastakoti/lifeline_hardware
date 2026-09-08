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
    bool isChatMessage;
    String chatMessage;
};

bool initLoRa();
bool parseLoRaPacket(int& deviceId, int& alertIndex, int& rssi);
bool parseLoRaPacketExtended(FullTelemetryData& telemetry);

// Two-way LoRa transmission routines
bool sendDownlinkACK(int targetDeviceId, char emergencyCode, const char* status, const char* message);
bool sendDownlinkCommand(int targetDeviceId, const char* action, const char* message);
bool sendBroadcastEvacuation(const char* message);

#endif // LORA_COMM_H
