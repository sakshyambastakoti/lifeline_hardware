#ifndef API_CLIENT_SPU_H
#define API_CLIENT_SPU_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "ConfigSPU.h"
#include "environment_manager.h"
#include "mpu_manager.h"
#include "gas_manager.h"
#include "gps_manager.h"
#include "emergency_detector.h"
#include "health_calculator.h"

void pushSPUTelemetryToAPI(const EnvironmentData& env,
                           const MotionData& motion,
                           const GasData& gas,
                           const GPSData& gps,
                           const EmergencyState& emergency,
                           const SystemHealthMetrics& health);

#endif // API_CLIENT_SPU_H
