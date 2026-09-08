#include "api_client_spu.h"
#include "wifi_portal_spu.h"

static int mapEmergencyToMessageCode(char code) {
    switch (code) {
        case EMERGENCY_SOS:         return 0;  // CRITICAL SOS
        case EMERGENCY_MEDICAL:     return 5;  // SEVERE INJURY / MEDICAL
        case EMERGENCY_FIRE:        return 0;  // FIRE / CRITICAL SOS
        case EMERGENCY_LANDSLIDE:   return 12; // LANDSLIDE / HAZARD
        case EMERGENCY_GAS:         return 0;  // GAS / HAZARD
        case EMERGENCY_EARTHQUAKE:  return 0;  // SEISMIC VIBRATION / SOS
        case EMERGENCY_BATTERY:     return 3;  // LOW BATTERY / SUPPLY
        default:                    return 14; // STATUS OK / ALL SAFE
    }
}

void pushSPUTelemetryToAPI(const EnvironmentData& env,
                           const MotionData& motion,
                           const GasData& gas,
                           const GPSData& gps,
                           const EmergencyState& emergency,
                           const SystemHealthMetrics& health) {
    if (WiFi.status() != WL_CONNECTED) {
        #if SPU_DEBUG_ENABLE
        Serial.println(F("[API SPU] Wi-Fi not connected. Skipping remote API telemetry upload."));
        #endif
        return;
    }

    HTTPClient http;
    http.setTimeout(3000); // 3 second network timeout
    http.begin(API_ENDPOINT);
    http.addHeader("Content-Type", "application/json");

    int msgCode = mapEmergencyToMessageCode(emergency.code);
    int wifiRssi = WiFi.RSSI();

    char payload[1024];
    snprintf(payload, sizeof(payload),
        "{"
        "\"DID\":%d,"
        "\"device_name\":\"%s\","
        "\"firmware\":\"%s\","
        "\"uptime_sec\":%lu,"
        "\"message_code\":%d,"
        "\"code\":\"%c\","
        "\"emergency_desc\":\"%s\","
        "\"priority\":%u,"
        "\"temp\":%.2f,"
        "\"humidity\":%.2f,"
        "\"pressure\":%.2f,"
        "\"gas_ppm\":%u,"
        "\"accel_x\":%.3f,\"accel_y\":%.3f,\"accel_z\":%.3f,"
        "\"gyro_x\":%.2f,\"gyro_y\":%.2f,\"gyro_z\":%.2f,"
        "\"total_g\":%.3f,\"tilt_deg\":%.2f,"
        "\"is_moving\":%s,\"sudden_impact\":%s,\"free_fall\":%s,\"seismic_vibe\":%s,"
        "\"gps_fix\":%s,\"satellites\":%u,"
        "\"lat\":%.6f,\"lon\":%.6f,\"alt\":%.1f,"
        "\"health\":%u,\"risk\":%u,\"battery\":100,"
        "\"RSSI\":%d"
        "}",
        SPU_DEVICE_ID,
        SPU_DEVICE_NAME,
        SPU_FIRMWARE_VERSION,
        millis() / 1000,
        msgCode,
        emergency.code,
        emergency.description,
        emergency.priority,
        env.temperature_c,
        env.humidity_pct,
        env.pressure_hpa,
        gas.ppm_estimate,
        motion.accel_x, motion.accel_y, motion.accel_z,
        motion.gyro_x, motion.gyro_y, motion.gyro_z,
        motion.total_accel_g, motion.tilt_deg,
        motion.is_motion_detected ? "true" : "false",
        motion.is_sudden_impact ? "true" : "false",
        motion.is_free_fall ? "true" : "false",
        motion.is_earthquake_vibration ? "true" : "false",
        gps.fix_valid ? "true" : "false",
        gps.satellites,
        gps.latitude, gps.longitude, gps.altitude_m,
        health.node_health_score,
        health.environmental_risk_score,
        wifiRssi
    );

    #if SPU_DEBUG_ENABLE
    Serial.printf("[API SPU] Sending Full Sensor Telemetry JSON to %s:\n%s\n", API_ENDPOINT, payload);
    #endif

    int httpResponseCode = http.POST(payload);

    if (httpResponseCode > 0) {
        String response = http.getString();
        #if SPU_DEBUG_ENABLE
        Serial.printf("[API SPU] HTTP Response (%d): %s\n", httpResponseCode, response.c_str());
        #endif
    } else {
        #if SPU_DEBUG_ENABLE
        Serial.printf("[API SPU] HTTP POST Error: %s\n", http.errorToString(httpResponseCode).c_str());
        #endif
    }

    http.end();
}
