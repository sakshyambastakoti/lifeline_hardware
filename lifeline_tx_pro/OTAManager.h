#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <Arduino.h>

void startOTAMode();
void stopOTAMode();
void handleOTA();
bool isOTAModeActive();
String getOTAIPAddress();
int getOTAProgress();
String getOTAStatusText();

#endif // OTA_MANAGER_H
