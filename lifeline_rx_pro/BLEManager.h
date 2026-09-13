#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <Arduino.h>
#include "Config.h"

// Nordic UART Service (NUS) UUIDs
#define BLE_SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define BLE_CHAR_RX_UUID           "6E400002-B5A3-F393-E0A9-E50E24DCCA9E" // Mobile -> RX (Write)
#define BLE_CHAR_TX_UUID           "6E400003-B5A3-F393-E0A9-E50E24DCCA9E" // RX -> Mobile (Notify)

void initBLE();
void updateBLE();
bool isBLEConnected();

// Outbound notifications to base station commander smartphone
void sendBLEString(const String& data);
void notifyBLEStatus();
void notifyBLEAlert(int devId, char code, const char* name, int rssi);
void notifyBLEChat(int devId, const char* text, int rssi);
void notifyBLETelemetry(int devId, float temp, float hum, double lat, double lon, int rssi);

// Inbound commands from commander smartphone or USB Serial
void processIncomingBaseCommand(const String& cmd);
bool hasPendingBLEReply();
void getPendingBLEReply(int& devId, String& action, String& message);
bool hasPendingBLEEvac();
String getPendingBLEEvacMessage();
bool hasPendingBLEChat();
String getPendingBLEChatMessage();

#endif // BLE_MANAGER_H
