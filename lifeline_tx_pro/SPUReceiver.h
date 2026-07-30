// SPU Receiver disabled - SPU sends telemetry directly to Cloud Web Server
#ifndef SPU_RECEIVER_H
#define SPU_RECEIVER_H

#include <Arduino.h>
#include "SharedProtocol.h"

inline void initSPUReceiver() {}
inline bool updateSPUReceiver() { return false; }
inline bool hasSPUTelemetry() { return false; }
inline bool hasReceivedSPUTelemetry() { return false; }
inline TelemetryPacket getLatestSPUTelemetry() { TelemetryPacket p; memset(&p, 0, sizeof(p)); return p; }
inline unsigned long getSPULastReceiveTime() { return 0; }
inline int mapSPUEmergencyToAlertIndex(char spuCode) { return 0; }

#endif // SPU_RECEIVER_H
