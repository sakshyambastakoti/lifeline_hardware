#include "APIClient.h"
#include <WiFi.h>
#include <HTTPClient.h>

extern bool wifiConnected;
extern String customApiKey;
extern String customApiEndpoint;

bool pushAlertToAPI(int deviceId, int alertIndex, int rssi) {
    if (!wifiConnected || WiFi.status() != WL_CONNECTED) {
        Serial.println(F("[API] WiFi not connected, skipping API push"));
        return false;
    }
    
    String endpoint = (customApiEndpoint.length() > 0) ? customApiEndpoint : API_ENDPOINT;
    HTTPClient http;
    http.setTimeout(2500); // 2.5 second network timeout
    http.begin(endpoint);
    http.addHeader("Content-Type", "application/json");
    if (customApiKey.length() > 0) {
        http.addHeader("X-API-Key", customApiKey);
        http.addHeader("Authorization", "Bearer " + customApiKey);
    }
    
    String jsonPayload = "{\"DID\":" + String(deviceId) + 
                         ",\"message_code\":" + String(alertIndex) + 
                         ",\"RSSI\":" + String(rssi);
    if (customApiKey.length() > 0) {
        jsonPayload += ",\"api_key\":\"" + customApiKey + "\"";
    }
    jsonPayload += "}";
    
    Serial.printf("[API] Sending to %s: %s\n", endpoint.c_str(), jsonPayload.c_str());
    
    int httpResponseCode = http.POST(jsonPayload);
    bool success = (httpResponseCode >= 200 && httpResponseCode < 300);
    
    if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.printf("[API] Response (%d): %s\n", httpResponseCode, response.c_str());
    } else {
        Serial.printf("[API] Error: %s\n", http.errorToString(httpResponseCode).c_str());
    }
    
    http.end();
    return success;
}

bool pushFullTelemetryToAPI(const FullTelemetryData& data) {
    if (!wifiConnected || WiFi.status() != WL_CONNECTED) {
        Serial.println(F("[API] WiFi not connected, skipping full telemetry push"));
        return false;
    }

    String endpoint = (customApiEndpoint.length() > 0) ? customApiEndpoint : API_ENDPOINT;
    HTTPClient http;
    http.setTimeout(3000); // 3 second network timeout
    http.begin(endpoint);
    http.addHeader("Content-Type", "application/json");
    if (customApiKey.length() > 0) {
        http.addHeader("X-API-Key", customApiKey);
        http.addHeader("Authorization", "Bearer " + customApiKey);
    }

    String jsonPayload = "{";
    if (customApiKey.length() > 0) {
        jsonPayload += "\"api_key\":\"" + customApiKey + "\",";
    }
    jsonPayload += "\"DID\":" + String(data.deviceId) + ",";
    jsonPayload += "\"message_code\":" + String(data.alertIndex) + ",";
    jsonPayload += "\"code\":\"" + String(data.emergencyCode) + "\",";
    jsonPayload += "\"temp\":" + String(data.temperature, 1) + ",";
    jsonPayload += "\"humidity\":" + String(data.humidity, 1) + ",";
    jsonPayload += "\"gas_ppm\":" + String(data.gasPpm) + ",";
    jsonPayload += "\"lat\":" + String(data.latitude, 6) + ",";
    jsonPayload += "\"lon\":" + String(data.longitude, 6) + ",";
    jsonPayload += "\"alt\":" + String(data.altitude) + ",";
    jsonPayload += "\"health\":" + String(data.healthScore) + ",";
    jsonPayload += "\"risk\":" + String(data.riskScore) + ",";
    jsonPayload += "\"RSSI\":" + String(data.rssi);
    jsonPayload += "}";

    Serial.printf("[API EX] Sending rich telemetry JSON: %s\n", jsonPayload.c_str());

    int httpResponseCode = http.POST(jsonPayload);
    bool success = (httpResponseCode >= 200 && httpResponseCode < 300);

    if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.printf("[API EX] Response (%d): %s\n", httpResponseCode, response.c_str());
    } else {
        Serial.printf("[API EX] Error: %s\n", http.errorToString(httpResponseCode).c_str());
    }

    http.end();
    return success;
}
