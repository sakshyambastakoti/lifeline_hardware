#ifndef WEB_SERVER_MANAGER_H
#define WEB_SERVER_MANAGER_H

#include <Arduino.h>
#include "ConfigSPU.h"
#include "wifi_portal_spu.h"
#include "api_client_spu.h"

class WebServerManager {
public:
    WebServerManager();
    void begin();
    void update();
    void uploadTelemetry();

private:
    unsigned long _last_server_upload_time;
};

extern WebServerManager webServerManager;

#endif // WEB_SERVER_MANAGER_H
