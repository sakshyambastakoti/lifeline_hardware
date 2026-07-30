#ifndef WEB_SERVER_MANAGER_H
#define WEB_SERVER_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include "ConfigSPU.h"
#include "gps_manager.h"
#include "environment_manager.h"
#include "mpu_manager.h"
#include "gas_manager.h"
#include "emergency_detector.h"
#include "health_calculator.h"

class WebServerManager {
public:
    WebServerManager();
    void begin();
    void update();

private:
    WebServer _server;
    
    // Wi-Fi STA & Server Upload Tracking
    bool _wifi_sta_connected;
    String _wifi_sta_ip;
    unsigned long _last_server_upload_time;
    uint32_t _server_upload_count;
    uint32_t _server_upload_fail_count;
    int _last_http_code;
    String _last_server_status;

    void handleRoot();
    void handleApiData();
    void handleNotFound();
    void checkWiFiSTAConnection();
    void uploadTelemetryToServer();
};

extern WebServerManager webServerManager;

#endif // WEB_SERVER_MANAGER_H
