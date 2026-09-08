#ifndef API_CLIENT_H
#define API_CLIENT_H

#include "Config.h"
#include "LoRaComm.h"

bool pushAlertToAPI(int deviceId, int alertIndex, int rssi);
bool pushFullTelemetryToAPI(const FullTelemetryData& data);

#endif // API_CLIENT_H
