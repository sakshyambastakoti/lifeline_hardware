#ifndef API_CLIENT_H
#define API_CLIENT_H

#include "Config.h"
#include "LoRaComm.h"

// API Push Functions
bool pushAlertToAPI(int deviceId, int alertIndex, int rssi, float distanceKm = 0.0f, float snr = 0.0f, const String& customMsg = "", const String& source = "LORA");
bool pushFullTelemetryToAPI(const FullTelemetryData& data);
bool pushCustomChatMessageToAPI(int deviceId, const String& message, int rssi, float snr, float distanceKm, const String& source = "LORA");

#endif // API_CLIENT_H
