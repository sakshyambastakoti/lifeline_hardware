#ifndef SPU_RECEIVER_H
#define SPU_RECEIVER_H

#include <Arduino.h>
#include "SharedProtocol.h"

void initSPUReceiver();
bool isESPNowInitialized();
bool updateSPUReceiver();
bool hasSPUTelemetry();
TelemetryPacket getLatestSPUTelemetry();
unsigned long getSPULastReceiveTime();
int mapSPUEmergencyToAlertIndex(char spuCode);

#endif // SPU_RECEIVER_H
