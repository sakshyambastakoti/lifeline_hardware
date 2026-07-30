#include "web_server_manager.h"

WebServerManager webServerManager;

WebServerManager::WebServerManager() : _last_server_upload_time(0) {}

void WebServerManager::begin() {
    wifiPortalSPU.begin();
    #if SPU_DEBUG_ENABLE
    Serial.println(F("[WEB MANAGER] Wi-Fi Portal and Cloud API Client Initialized. (Local browse log removed)"));
    #endif
}

void WebServerManager::update() {
    wifiPortalSPU.update();
}

void WebServerManager::uploadTelemetry() {
    #if ENABLE_SERVER_UPLOAD
    const EnvironmentData& env = envManager.getData();
    const MotionData& motion = mpuManager.getData();
    const GasData& gas = gasManager.getData();
    const GPSData& gps = gpsManager.getData();
    const EmergencyState& emergency = emergencyDetector.getState();
    SystemHealthMetrics health = HealthCalculator::calculate(env, motion, gas, gps);

    pushSPUTelemetryToAPI(env, motion, gas, gps, emergency, health);
    #endif
}
