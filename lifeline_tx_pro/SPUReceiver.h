#ifndef SPU_RECEIVER_H
#define SPU_RECEIVER_H

#include <Arduino.h>
#include "SharedProtocol.h"

void initSPUReceiver();
bool updateSPUReceiver();
bool hasSPUTelemetry();
bool hasReceivedSPUTelemetry();
TelemetryPacket getLatestSPUTelemetry();
unsigned long getSPULastReceiveTime();
int mapSPUEmergencyToAlertIndex(char spuCode);

#endif // SPU_RECEIVER_H
