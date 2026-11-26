#include "APIClient.h"
#include "OTAManager.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

extern bool wifiConnected;
extern bool portalActive;
extern String customApiKey;
extern String customApiEndpoint;

// ═══════════════════════════════════════════════════════════════════════════════════
//               NON-BLOCKING BACKGROUND API QUEUE & WORKER TASK
// ═══════════════════════════════════════════════════════════════════════════════════
enum ApiJobType {
    JOB_ALERT = 0,
    JOB_FULL_TELEMETRY,
    JOB_CHAT,
    JOB_DOWNLINK_ACK
};

struct ApiJob {
    ApiJobType type;
    int deviceId;
    int alertIndex;
    char emergencyCode;
    float temperature;
    float humidity;
    int gasPpm;
    double latitude;
    double longitude;
    int altitude;
    int healthScore;
    int riskScore;
    int rssi;
    float snr;
    float distanceKm;
    bool isFullTelemetry;
    bool isChatMessage;
    bool loraOk;
    bool notifyAlertIcon;
    int commandId;
    char message[160];
    char source[16];
    char status[32];
};

struct DownlinkCommandPOD {
    int commandId;
    int targetDeviceId;
    char action[24];
    char message[160];
};

static QueueHandle_t apiTxQueue = nullptr;
static QueueHandle_t downlinkRxQueue = nullptr;
static TaskHandle_t apiWorkerHandle = nullptr;

static volatile bool apiUploadResultReady = false;
static volatile bool apiUploadResultSuccess = false;

static bool isNetworkReadyForCloud() {
    return (wifiConnected && WiFi.status() == WL_CONNECTED && !portalActive && !isLocalOTAModeActive());
}

static String escapeJsonStr(const char* input) {
    String s = (input != nullptr) ? String(input) : String("");
    s.replace("\\", "\\\\");
    s.replace("\"", "\\\"");
    s.replace("\n", " ");
    s.replace("\r", "");
    return s;
}

static void addAuthHeaders(HTTPClient& http) {
    if (customApiKey.length() > 0) {
        http.addHeader("X-API-Key", customApiKey);
        http.addHeader("Authorization", "Bearer " + customApiKey);
    }
}

static bool executeHttpPost(const String& endpoint, const String& jsonPayload, const char* tag, int timeoutMs) {
    if (!isNetworkReadyForCloud()) {
        return false;
    }

    HTTPClient http;
    http.setConnectTimeout(2000); // Fast 2s connect timeout so dead links fail quickly
    http.setTimeout(timeoutMs);
    if (!http.begin(endpoint)) {
        Serial.printf("[%s] Failed to init HTTP client\n", tag);
        return false;
    }

    http.addHeader("Content-Type", "application/json");
    addAuthHeaders(http);

    Serial.printf("[%s] Sending to %s: %s\n", tag, endpoint.c_str(), jsonPayload.c_str());

    int httpResponseCode = http.POST(jsonPayload);
    bool success = (httpResponseCode >= 200 && httpResponseCode < 300);

    if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.printf("[%s] Response (%d): %s\n", tag, httpResponseCode, response.c_str());
    } else {
        Serial.printf("[%s] Error: %s\n", tag, http.errorToString(httpResponseCode).c_str());
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

static bool executeDownlinkPoll(DownlinkCommandPOD& outCmd, bool& networkError) {
    networkError = false;
    if (!isNetworkReadyForCloud()) {
        return false;
    }

    String pollUrl = API_DOWNLINK_POLL_ENDPOINT;
    if (customApiEndpoint.indexOf("/API/Create/message.php") >= 0) {
        String base = customApiEndpoint;
        base.replace("/API/Create/message.php", "/API/Read/pending_commands.php");
        pollUrl = base;
    }

    pollUrl += (pollUrl.indexOf("?") < 0 ? "?rx_id=" : "&rx_id=") + String(DEVICE_ID);

    HTTPClient http;
    http.setConnectTimeout(2000);
    http.setTimeout(2000);
    if (!http.begin(pollUrl)) {
        networkError = true;
        return false;
    }
    addAuthHeaders(http);

    int httpCode = http.GET();
    if (httpCode == 200) {
        String payload = http.getString();
        http.end();

        bool hasCommand = extractJsonBoolVal(payload, "has_command", false);
        if (!hasCommand) {
            int cId = extractJsonIntVal(payload, "command_id", 0);
            if (cId <= 0) return false;
        }

        int cmdId = extractJsonIntVal(payload, "command_id", 0);
        int targetDid = extractJsonIntVal(payload, "target_did", 0);
        String action = extractJsonStringVal(payload, "action");
        if (action.length() == 0) action = "MSG";
        String message = extractJsonStringVal(payload, "message");

        if (cmdId > 0 && message.length() > 0) {
            outCmd.commandId = cmdId;
            outCmd.targetDeviceId = targetDid;
            strlcpy(outCmd.action, action.c_str(), sizeof(outCmd.action));
            strlcpy(outCmd.message, message.c_str(), sizeof(outCmd.message));
            Serial.printf("[API DOWNLINK] Polled pending cmd #%d for DEV %d: %s\n",
                          outCmd.commandId, outCmd.targetDeviceId, outCmd.message);
            return true;
        }
    } else {
        if (httpCode < 0) {
            networkError = true;
        }
        http.end();
    }
    return false;
}

static bool processApiJob(const ApiJob& job) {
    String endpoint = (customApiEndpoint.length() > 0) ? customApiEndpoint : API_ENDPOINT;

    if (job.type == JOB_ALERT) {
        String safeMsg = escapeJsonStr(job.message);
        String json = "{";
        if (customApiKey.length() > 0) json += "\"api_key\":\"" + customApiKey + "\",";
        json += "\"DID\":" + String(job.deviceId) + ",";
        json += "\"message_code\":" + String(job.alertIndex) + ",";
        json += "\"RSSI\":" + String(job.rssi) + ",";
        json += "\"snr\":" + String(job.snr, 1) + ",";
        json += "\"distance_km\":" + String(job.distanceKm, 2) + ",";
        json += "\"is_chat\":" + String(safeMsg.length() > 0 ? "true" : "false") + ",";
        json += "\"custom_msg\":\"" + safeMsg + "\",";
        json += "\"source\":\"" + String(job.source) + "\"";
        json += "}";
        return executeHttpPost(endpoint, json, "API", 2500);
    }

    if (job.type == JOB_FULL_TELEMETRY) {
        String safeMsg = escapeJsonStr(job.message);
        String json = "{";
        if (customApiKey.length() > 0) json += "\"api_key\":\"" + customApiKey + "\",";
        json += "\"DID\":" + String(job.deviceId) + ",";
        json += "\"message_code\":" + String(job.alertIndex) + ",";
        json += "\"code\":\"" + String(job.emergencyCode) + "\",";
        json += "\"temp\":" + String(job.temperature, 1) + ",";
        json += "\"humidity\":" + String(job.humidity, 1) + ",";
        json += "\"gas_ppm\":" + String(job.gasPpm) + ",";
        json += "\"lat\":" + String(job.latitude, 6) + ",";
        json += "\"lon\":" + String(job.longitude, 6) + ",";
        json += "\"alt\":" + String(job.altitude) + ",";
        json += "\"health\":" + String(job.healthScore) + ",";
        json += "\"risk\":" + String(job.riskScore) + ",";
        json += "\"RSSI\":" + String(job.rssi) + ",";
        json += "\"snr\":" + String(job.snr, 1) + ",";
        json += "\"distance_km\":" + String(job.distanceKm, 2) + ",";
        json += "\"is_chat\":" + String(job.isChatMessage ? "true" : "false") + ",";
        json += "\"custom_msg\":\"" + safeMsg + "\",";
        json += "\"source\":\"LORA\"";
        json += "}";
        return executeHttpPost(endpoint, json, "API EX", 2500);
    }

    if (job.type == JOB_CHAT) {
        String safeMsg = escapeJsonStr(job.message);
        String json = "{";
        if (customApiKey.length() > 0) json += "\"api_key\":\"" + customApiKey + "\",";
        json += "\"DID\":" + String(job.deviceId) + ",";
        json += "\"message_code\":0,";
        json += "\"code\":\"M\",";
        json += "\"RSSI\":" + String(job.rssi) + ",";
        json += "\"snr\":" + String(job.snr, 1) + ",";
        json += "\"distance_km\":" + String(job.distanceKm, 2) + ",";
        json += "\"is_chat\":true,";
        json += "\"custom_msg\":\"" + safeMsg + "\",";
        json += "\"source\":\"" + String(job.source) + "\"";
        json += "}";
        return executeHttpPost(endpoint, json, "API CHAT", 2500);
    }

    if (job.type == JOB_DOWNLINK_ACK) {
        String ackUrl = API_DOWNLINK_ACK_ENDPOINT;
        if (customApiEndpoint.indexOf("/API/Create/message.php") >= 0) {
            String base = customApiEndpoint;
            base.replace("/API/Create/message.php", "/API/Update/command_status.php");
            ackUrl = base;
        }
        String json = "{";
        if (customApiKey.length() > 0) json += "\"api_key\":\"" + customApiKey + "\",";
        json += "\"command_id\":" + String(job.commandId) + ",";
        json += "\"rx_id\":" + String(DEVICE_ID) + ",";
        json += "\"status\":\"" + String(job.status) + "\",";
        json += "\"lora_tx_ok\":" + String(job.loraOk ? "true" : "false");
        json += "}";
        return executeHttpPost(ackUrl, json, "API DOWNLINK ACK", 2500);
    }

    return false;
}

static void apiWorkerTask(void* pvParameters) {
    (void)pvParameters;
    unsigned long lastPollMs = 0;
    unsigned long pollIntervalMs = DOWNLINK_POLL_INTERVAL_MS;

    for (;;) {
        ApiJob job;
        if (xQueueReceive(apiTxQueue, &job, pdMS_TO_TICKS(50)) == pdTRUE) {
            bool ok = false;
            if (isNetworkReadyForCloud()) {
                ok = processApiJob(job);
            } else {
                Serial.println(F("[API] WiFi offline, skipping queued cloud push"));
            }

            if (job.notifyAlertIcon) {
                apiUploadResultSuccess = ok;
                apiUploadResultReady = true;
            }
            continue;
        }

        if (isNetworkReadyForCloud()) {
            unsigned long now = millis();
            if (lastPollMs == 0 || (now - lastPollMs >= pollIntervalMs)) {
                lastPollMs = now;
                DownlinkCommandPOD podCmd;
                bool netErr = false;
                if (executeDownlinkPoll(podCmd, netErr)) {
                    xQueueSend(downlinkRxQueue, &podCmd, 0);
                }
                if (netErr) {
                    pollIntervalMs = min((unsigned long)10000, pollIntervalMs + 2000);
                } else {
                    pollIntervalMs = DOWNLINK_POLL_INTERVAL_MS;
                }
            }
        } else {
            lastPollMs = 0;
            pollIntervalMs = DOWNLINK_POLL_INTERVAL_MS;
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

void initAPIClient() {
    if (apiTxQueue == nullptr) {
        apiTxQueue = xQueueCreate(8, sizeof(ApiJob));
    }
    if (downlinkRxQueue == nullptr) {
        downlinkRxQueue = xQueueCreate(4, sizeof(DownlinkCommandPOD));
    }
    if (apiWorkerHandle == nullptr && apiTxQueue != nullptr && downlinkRxQueue != nullptr) {
        xTaskCreatePinnedToCore(
            apiWorkerTask,
            "api_worker",
            10240,
            nullptr,
            1,
            &apiWorkerHandle,
            0 // Run on Core 0 so Core 1 loop() (LoRa + LCD clock) never blocks on HTTP/DNS
        );
        Serial.println(F("[API] Non-blocking background cloud worker started"));
    }
}

bool consumeApiUploadResult(bool& sentToWeb) {
    if (!apiUploadResultReady) return false;
    sentToWeb = apiUploadResultSuccess;
    apiUploadResultReady = false;
    return true;
}

static bool enqueueApiJob(const ApiJob& job) {
    if (!isNetworkReadyForCloud()) {
        return false;
    }
    if (apiTxQueue == nullptr) {
        return false;
    }
    if (xQueueSend(apiTxQueue, &job, 0) != pdTRUE) {
        Serial.println(F("[API] Upload queue full, dropping oldest/new job"));
        return false;
    }
    return true;
}

bool pushAlertToAPI(int deviceId, int alertIndex, int rssi, float distanceKm, float snr, const String& customMsg, const String& source) {
    if (!isNetworkReadyForCloud()) {
        Serial.println(F("[API] WiFi not connected, skipping API push"));
        return false;
    }

    ApiJob job = {};
    job.type = JOB_ALERT;
    job.deviceId = deviceId;
    job.alertIndex = alertIndex;
    job.rssi = rssi;
    job.distanceKm = distanceKm;
    job.snr = snr;
    job.notifyAlertIcon = true;
    strlcpy(job.message, customMsg.c_str(), sizeof(job.message));
    strlcpy(job.source, source.c_str(), sizeof(job.source));
    return enqueueApiJob(job);
}

bool pushFullTelemetryToAPI(const FullTelemetryData& data) {
    if (!isNetworkReadyForCloud()) {
        Serial.println(F("[API] WiFi not connected, skipping full telemetry push"));
        return false;
    }

    ApiJob job = {};
    job.type = JOB_FULL_TELEMETRY;
    job.deviceId = data.deviceId;
    job.alertIndex = data.alertIndex;
    job.emergencyCode = data.emergencyCode;
    job.temperature = data.temperature;
    job.humidity = data.humidity;
    job.gasPpm = data.gasPpm;
    job.latitude = data.latitude;
    job.longitude = data.longitude;
    job.altitude = data.altitude;
    job.healthScore = data.healthScore;
    job.riskScore = data.riskScore;
    job.rssi = data.rssi;
    job.snr = data.snr;
    job.distanceKm = data.distanceKm;
    job.isFullTelemetry = data.isFullTelemetry;
    job.isChatMessage = data.isChatMessage;
    job.notifyAlertIcon = (data.emergencyCode != 'N' && !data.isChatMessage);
    strlcpy(job.message, data.chatMessage.c_str(), sizeof(job.message));
    strlcpy(job.source, "LORA", sizeof(job.source));
    return enqueueApiJob(job);
}

bool pushCustomChatMessageToAPI(int deviceId, const String& message, int rssi, float snr, float distanceKm, const String& source) {
    if (!isNetworkReadyForCloud()) {
        Serial.println(F("[API] WiFi not connected, skipping custom chat push"));
        return false;
    }

    ApiJob job = {};
    job.type = JOB_CHAT;
    job.deviceId = deviceId;
    job.rssi = rssi;
    job.snr = snr;
    job.distanceKm = distanceKm;
    job.isChatMessage = true;
    job.notifyAlertIcon = false;
    strlcpy(job.message, message.c_str(), sizeof(job.message));
    strlcpy(job.source, source.c_str(), sizeof(job.source));
    return enqueueApiJob(job);
}

bool pollPendingDownlinkFromAPI(DownlinkCommand& cmd) {
    if (downlinkRxQueue == nullptr) return false;
    DownlinkCommandPOD podCmd;
    if (xQueueReceive(downlinkRxQueue, &podCmd, 0) == pdTRUE) {
        cmd.commandId = podCmd.commandId;
        cmd.targetDeviceId = podCmd.targetDeviceId;
        cmd.action = String(podCmd.action);
        cmd.message = String(podCmd.message);
        return true;
    }
    return false;
}

bool acknowledgeDownlinkToAPI(int commandId, const String& status, bool loraOk) {
    if (!isNetworkReadyForCloud()) {
        return false;
    }

    ApiJob job = {};
    job.type = JOB_DOWNLINK_ACK;
    job.commandId = commandId;
    job.loraOk = loraOk;
    job.notifyAlertIcon = false;
    strlcpy(job.status, status.c_str(), sizeof(job.status));
    return enqueueApiJob(job);
}
