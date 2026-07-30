#include "esp_now_manager.h"

ESPNowManager espNowManager;

// Callback when ESP-NOW packet is sent
#if defined(ESP_IDF_VERSION_MAJOR) && ESP_IDF_VERSION_MAJOR >= 5
static void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
#else
static void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
#endif
{
    #if SPU_DEBUG_ENABLE
    if (status == ESP_NOW_SEND_SUCCESS) {
        Serial.println(F("[ESP-NOW] Telemetry Packet Broadcast Sent Successfully"));
    } else {
        Serial.println(F("[ESP-NOW] Telemetry Packet Delivery Failed"));
    }
    #endif
}

ESPNowManager::ESPNowManager() : _initialized(false) {
    // Broadcast MAC address FF:FF:FF:FF:FF:FF
    memset(_broadcastMac, 0xFF, 6);
}

void ESPNowManager::begin() {
    // Ensure Wi-Fi mode supports ESP-NOW (WIFI_STA or WIFI_AP_STA)
    if (WiFi.getMode() == WIFI_OFF) {
        WiFi.mode(WIFI_AP_STA);
    }

    if (esp_now_init() != ESP_OK) {
        Serial.println(F("[ESP-NOW] Error initializing ESP-NOW!"));
        _initialized = false;
        return;
    }

    esp_now_register_send_cb(onDataSent);

    // Register broadcast peer
    esp_now_peer_info_t peerInfo;
    memset(&peerInfo, 0, sizeof(peerInfo));
    memcpy(peerInfo.peer_addr, _broadcastMac, 6);
    peerInfo.channel = ESPNOW_CHANNEL;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println(F("[ESP-NOW] Failed to add broadcast peer!"));
    } else {
        Serial.println(F("[ESP-NOW] Broadcast Peer Added (FF:FF:FF:FF:FF:FF). ESP-NOW Ready!"));
    }

    _initialized = true;
}

bool ESPNowManager::sendTelemetry(const TelemetryPacket& pkt) {
    if (!_initialized) {
        #if SPU_DEBUG_ENABLE
        Serial.println(F("[ESP-NOW] Cannot send: ESP-NOW not initialized"));
        #endif
        return false;
    }

    esp_err_t result = esp_now_send(_broadcastMac, (const uint8_t *)&pkt, sizeof(TelemetryPacket));
    
    if (result == ESP_OK) {
        #if SPU_DEBUG_ENABLE
        Serial.printf("[ESP-NOW] Transmitted TelemetryPacket (Size: %u bytes, Code: '%c')\n", sizeof(TelemetryPacket), pkt.emergency_code);
        #endif
        return true;
    } else {
        #if SPU_DEBUG_ENABLE
        Serial.printf("[ESP-NOW] Send error code: %d\n", result);
        #endif
        return false;
    }
}
