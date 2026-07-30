#ifndef ESP_NOW_MANAGER_H
#define ESP_NOW_MANAGER_H

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include "ConfigSPU.h"
#include "SharedProtocol.h"

class ESPNowManager {
public:
    ESPNowManager();
    void begin();
    bool sendTelemetry(const TelemetryPacket& pkt);
    
private:
    bool _initialized;
    uint8_t _broadcastMac[6];
};

extern ESPNowManager espNowManager;

#endif // ESP_NOW_MANAGER_H
