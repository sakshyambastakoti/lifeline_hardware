#ifndef API_CLIENT_H
#define API_CLIENT_H

#include "Config.h"
#include "LoRaComm.h"

void pushAlertToAPI(int deviceId, int alertIndex, int rssi);
void pushFullTelemetryToAPI(const FullTelemetryData& data);

#endif // API_CLIENT_H
