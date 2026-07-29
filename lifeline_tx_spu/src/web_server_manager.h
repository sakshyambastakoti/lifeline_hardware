#ifndef WEB_SERVER_MANAGER_H
#define WEB_SERVER_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
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
    void handleRoot();
    void handleApiData();
    void handleNotFound();
};

extern WebServerManager webServerManager;

#endif // WEB_SERVER_MANAGER_H
