#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <Arduino.h>
#include "Config.h"

// Nordic UART Service (NUS) UUIDs
#define BLE_SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define BLE_CHAR_RX_UUID           "6E400002-B5A3-F393-E0A9-E50E24DCCA9E" // Mobile -> TX (Write)
#define BLE_CHAR_TX_UUID           "6E400003-B5A3-F393-E0A9-E50E24DCCA9E" // TX -> Mobile (Notify)

void initBLE();
void updateBLE();
bool isBLEConnected();

// Radio Power Control (ON / OFF)
bool isBLERadioEnabled();
void setBLERadioEnabled(bool enable);
void toggleBLERadio();
String getConnectedClientInfo();

// Outbound notifications to paired smartphone
void sendBLEString(const String& data);
void notifyBLEStatus();
void notifyBLEAlertSent(char code, const char* name);
void notifyBLEAck(const char* status, const char* baseId, const char* note, int rssi, int snr);
void notifyBLEAckTimeout();

// Inbound commands from paired smartphone
bool hasPendingBLEChatMessage();
String getPendingBLEChatMessage();
bool hasPendingBLEAlert();
char getPendingBLEAlert();

#endif // BLE_MANAGER_H
