#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <Arduino.h>

enum OTAMode {
    OTA_MODE_NONE = 0,
    OTA_MODE_LOCAL = 1,
    OTA_MODE_NET = 2
};

void startOTAMode(OTAMode mode = OTA_MODE_LOCAL);
void stopOTAMode();
void handleOTA();
bool isOTAModeActive();
OTAMode getCurrentOTAMode();
String getOTAIPAddress();
String getOTASSID();
int getOTAProgress();
String getOTAStatusText();

void loadWiFiCredentials(String& ssid, String& pass);
void saveWiFiCredentials(const String& ssid, const String& pass);

#endif // OTA_MANAGER_H
