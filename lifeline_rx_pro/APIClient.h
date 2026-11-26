#ifndef API_CLIENT_H
#define API_CLIENT_H

#include "Config.h"
#include "LoRaComm.h"

// Cloud Downlink Structure (Website -> RX -> TX)
struct DownlinkCommand {
    int commandId;
    int targetDeviceId;
    String action;
    String message;
};

// Background Non-Blocking API Worker Initialization & Status
void initAPIClient();
bool consumeApiUploadResult(bool& sentToWeb);

// API Push Functions (Non-blocking enqueue when WiFi connected)
bool pushAlertToAPI(int deviceId, int alertIndex, int rssi, float distanceKm = 0.0f, float snr = 0.0f, const String& customMsg = "", const String& source = "LORA");
bool pushFullTelemetryToAPI(const FullTelemetryData& data);
bool pushCustomChatMessageToAPI(int deviceId, const String& message, int rssi, float snr, float distanceKm, const String& source = "LORA");

// Cloud Downlink Functions (Non-blocking queue read / enqueue)
bool pollPendingDownlinkFromAPI(DownlinkCommand& cmd);
bool acknowledgeDownlinkToAPI(int commandId, const String& status, bool loraOk);

#endif // API_CLIENT_H
