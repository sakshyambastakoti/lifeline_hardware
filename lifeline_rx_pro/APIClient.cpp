#include "APIClient.h"
#include <WiFi.h>
#include <HTTPClient.h>

extern bool wifiConnected;
extern String customApiKey;
extern String customApiEndpoint;

bool pushAlertToAPI(int deviceId, int alertIndex, int rssi, float distanceKm, float snr, const String& customMsg, const String& source) {
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
    
    String safeMsg = customMsg;
    safeMsg.replace("\"", "\\\"");
    safeMsg.replace("\n", " ");
    safeMsg.replace("\r", "");

    String jsonPayload = "{";
    if (customApiKey.length() > 0) {
        jsonPayload += "\"api_key\":\"" + customApiKey + "\",";
    }
    jsonPayload += "\"DID\":" + String(deviceId) + ",";
    jsonPayload += "\"message_code\":" + String(alertIndex) + ",";
    jsonPayload += "\"RSSI\":" + String(rssi) + ",";
    jsonPayload += "\"snr\":" + String(snr, 1) + ",";
    jsonPayload += "\"distance_km\":" + String(distanceKm, 2) + ",";
    jsonPayload += "\"is_chat\":" + String(customMsg.length() > 0 ? "true" : "false") + ",";
    jsonPayload += "\"custom_msg\":\"" + safeMsg + "\",";
    jsonPayload += "\"source\":\"" + source + "\"";
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

    String safeMsg = data.chatMessage;
    safeMsg.replace("\"", "\\\"");
    safeMsg.replace("\n", " ");
    safeMsg.replace("\r", "");

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
    jsonPayload += "\"RSSI\":" + String(data.rssi) + ",";
    jsonPayload += "\"snr\":" + String(data.snr, 1) + ",";
    jsonPayload += "\"distance_km\":" + String(data.distanceKm, 2) + ",";
    jsonPayload += "\"is_chat\":" + String(data.isChatMessage ? "true" : "false") + ",";
    jsonPayload += "\"custom_msg\":\"" + safeMsg + "\",";
    jsonPayload += "\"source\":\"LORA\"";
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

bool pushCustomChatMessageToAPI(int deviceId, const String& message, int rssi, float snr, float distanceKm, const String& source) {
    if (!wifiConnected || WiFi.status() != WL_CONNECTED) {
        Serial.println(F("[API] WiFi not connected, skipping custom chat push"));
        return false;
    }

    String endpoint = (customApiEndpoint.length() > 0) ? customApiEndpoint : API_ENDPOINT;
    HTTPClient http;
    http.setTimeout(3000);
    http.begin(endpoint);
    http.addHeader("Content-Type", "application/json");
    if (customApiKey.length() > 0) {
        http.addHeader("X-API-Key", customApiKey);
        http.addHeader("Authorization", "Bearer " + customApiKey);
    }

    String safeMsg = message;
    safeMsg.replace("\"", "\\\"");
    safeMsg.replace("\n", " ");
    safeMsg.replace("\r", "");

    String jsonPayload = "{";
    if (customApiKey.length() > 0) {
        jsonPayload += "\"api_key\":\"" + customApiKey + "\",";
    }
    jsonPayload += "\"DID\":" + String(deviceId) + ",";
    jsonPayload += "\"message_code\":0,";
    jsonPayload += "\"code\":\"M\",";
    jsonPayload += "\"RSSI\":" + String(rssi) + ",";
    jsonPayload += "\"snr\":" + String(snr, 1) + ",";
    jsonPayload += "\"distance_km\":" + String(distanceKm, 2) + ",";
    jsonPayload += "\"is_chat\":true,";
    jsonPayload += "\"custom_msg\":\"" + safeMsg + "\",";
    jsonPayload += "\"source\":\"" + source + "\"";
    jsonPayload += "}";

    Serial.printf("[API CHAT] Sending custom message JSON to %s: %s\n", endpoint.c_str(), jsonPayload.c_str());

    int httpResponseCode = http.POST(jsonPayload);
    bool success = (httpResponseCode >= 200 && httpResponseCode < 300);

    if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.printf("[API CHAT] Response (%d): %s\n", httpResponseCode, response.c_str());
    } else {
        Serial.printf("[API CHAT] Error: %s\n", http.errorToString(httpResponseCode).c_str());
    }

    http.end();
    return success;
}

// ═══════════════════════════════════════════════════════════════════════════════════
//                      CLOUD DOWNLINK JSON PARSING HELPERS
// ═══════════════════════════════════════════════════════════════════════════════════
static String extractJsonStringVal(const String& json, const String& key) {
    String searchKey = "\"" + key + "\":";
    int keyIdx = json.indexOf(searchKey);
    if (keyIdx == -1) {
        searchKey = "\"" + key + "\" :";
        keyIdx = json.indexOf(searchKey);
    }
    if (keyIdx == -1) return "";

    int valStart = json.indexOf('"', keyIdx + searchKey.length());
    if (valStart == -1) return "";
    valStart += 1;

    int valEnd = json.indexOf('"', valStart);
    if (valEnd == -1) return "";

    return json.substring(valStart, valEnd);
}

static int extractJsonIntVal(const String& json, const String& key, int defaultVal = 0) {
    String searchKey = "\"" + key + "\":";
    int keyIdx = json.indexOf(searchKey);
    if (keyIdx == -1) {
        searchKey = "\"" + key + "\" :";
        keyIdx = json.indexOf(searchKey);
    }
    if (keyIdx == -1) return defaultVal;

    int idx = keyIdx + searchKey.length();
    while (idx < (int)json.length() && (json[idx] == ' ' || json[idx] == '\t' || json[idx] == '"')) {
        idx++;
    }

    String numStr = "";
    while (idx < (int)json.length() && (isdigit(json[idx]) || json[idx] == '-')) {
        numStr += json[idx];
        idx++;
    }

    return (numStr.length() > 0) ? numStr.toInt() : defaultVal;
}

static bool extractJsonBoolVal(const String& json, const String& key, bool defaultVal = false) {
    String searchKey = "\"" + key + "\":";
    int keyIdx = json.indexOf(searchKey);
    if (keyIdx == -1) {
        searchKey = "\"" + key + "\" :";
        keyIdx = json.indexOf(searchKey);
    }
    if (keyIdx == -1) return defaultVal;

    int idx = keyIdx + searchKey.length();
    while (idx < (int)json.length() && (json[idx] == ' ' || json[idx] == '\t')) idx++;

    if (json.substring(idx).startsWith("true") || json.substring(idx).startsWith("\"true\"") || json.substring(idx).startsWith("1")) {
        return true;
    }
    return false;
}

// ═══════════════════════════════════════════════════════════════════════════════════
//                 TWO-WAY CLOUD DOWNLINK INGESTION & ACKNOWLEDGEMENT
// ═══════════════════════════════════════════════════════════════════════════════════
bool pollPendingDownlinkFromAPI(DownlinkCommand& cmd) {
    if (!wifiConnected || WiFi.status() != WL_CONNECTED) {
        return false;
    }

    String pollUrl = API_DOWNLINK_POLL_ENDPOINT;
    if (customApiEndpoint.indexOf("/API/Create/message.php") >= 0) {
        String base = customApiEndpoint;
        base.replace("/API/Create/message.php", "/API/Read/pending_commands.php");
        pollUrl = base;
    }

    if (pollUrl.indexOf("?") < 0) {
        pollUrl += "?rx_id=" + String(DEVICE_ID);
    } else {
        pollUrl += "&rx_id=" + String(DEVICE_ID);
    }

    HTTPClient http;
    http.setTimeout(2500);
    http.begin(pollUrl);
    if (customApiKey.length() > 0) {
        http.addHeader("X-API-Key", customApiKey);
        http.addHeader("Authorization", "Bearer " + customApiKey);
    }

    int httpCode = http.GET();
    if (httpCode == 200) {
        String payload = http.getString();
        http.end();

        bool hasCommand = extractJsonBoolVal(payload, "has_command", false);
        if (!hasCommand) {
            int cId = extractJsonIntVal(payload, "command_id", 0);
            if (cId <= 0) return false;
            hasCommand = true;
        }

        cmd.commandId = extractJsonIntVal(payload, "command_id", 0);
        cmd.targetDeviceId = extractJsonIntVal(payload, "target_did", 0);
        cmd.action = extractJsonStringVal(payload, "action");
        if (cmd.action.length() == 0) cmd.action = "MSG";
        cmd.message = extractJsonStringVal(payload, "message");

        if (cmd.commandId > 0 && cmd.message.length() > 0) {
            Serial.printf("[API DOWNLINK] Polled pending cmd #%d for DEV %d: %s\n",
                          cmd.commandId, cmd.targetDeviceId, cmd.message.c_str());
            return true;
        }
    } else {
        http.end();
    }
    return false;
}

bool acknowledgeDownlinkToAPI(int commandId, const String& status, bool loraOk) {
    if (!wifiConnected || WiFi.status() != WL_CONNECTED) {
        return false;
    }

    String ackUrl = API_DOWNLINK_ACK_ENDPOINT;
    if (customApiEndpoint.indexOf("/API/Create/message.php") >= 0) {
        String base = customApiEndpoint;
        base.replace("/API/Create/message.php", "/API/Update/command_status.php");
        ackUrl = base;
    }

    HTTPClient http;
    http.setTimeout(2500);
    http.begin(ackUrl);
    http.addHeader("Content-Type", "application/json");
    if (customApiKey.length() > 0) {
        http.addHeader("X-API-Key", customApiKey);
        http.addHeader("Authorization", "Bearer " + customApiKey);
    }

    String json = "{";
    if (customApiKey.length() > 0) {
        json += "\"api_key\":\"" + customApiKey + "\",";
    }
    json += "\"command_id\":" + String(commandId) + ",";
    json += "\"rx_id\":" + String(DEVICE_ID) + ",";
    json += "\"status\":\"" + status + "\",";
    json += "\"lora_tx_ok\":" + String(loraOk ? "true" : "false");
    json += "}";

    int httpCode = http.POST(json);
    bool ok = (httpCode >= 200 && httpCode < 300);
    if (httpCode > 0) {
        Serial.printf("[API DOWNLINK ACK] Cmd #%d status updated (%d): %s\n", commandId, httpCode, status.c_str());
    } else {
        Serial.printf("[API DOWNLINK ACK] Error acknowledging cmd #%d: %s\n", commandId, http.errorToString(httpCode).c_str());
    }
    http.end();
    return ok;
}

