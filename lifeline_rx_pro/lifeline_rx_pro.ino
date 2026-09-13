/*
 * ═══════════════════════════════════════════════════════════════════════════════════
 *                        ╔═════════════════════════════════════╗
 *                        ║    LIFELINE EMERGENCY RECEIVER      ║
 *                        ║   Professional Base Station v3.1    ║
 *                        ║ (16x2 LCD, Multi-WiFi & OTA Web)    ║
 *                        ╚═════════════════════════════════════╝
 * ═══════════════════════════════════════════════════════════════════════════════════
 */

#include "Config.h"
#include "BuzzerLED.h"
#include "DisplayUI.h"
#include "LoRaComm.h"
#include "WiFiPortal.h"
#include "APIClient.h"
#include "OTAManager.h"
#include "BLEManager.h"

// Static serial debugger variables
static String serialInputBuffer = "";

// Forward declarations for Serial Debug Menu
void printSerialDebugMenu();
bool checkSerialSimulatedPacket(FullTelemetryData& telemetry);

bool handleIncomingLoRaTelemetry() {
    FullTelemetryData telemetry;
    bool packetReceived = parseLoRaPacketExtended(telemetry);

    if (!packetReceived) return false;

    // Immediately trigger Red Data LED and loud buzzer alert simultaneously on data reception
    triggerRxBlink();

    if (telemetry.isChatMessage) {
        Serial.printf("[RX CHAT LOG] Dev #%d: '%s' (Dist: %.2fkm)\n", telemetry.deviceId, telemetry.chatMessage.c_str(), telemetry.distanceKm);
        notifyBLEChat(telemetry.deviceId, telemetry.chatMessage.c_str(), telemetry.rssi);
        sendDownlinkACK(telemetry.deviceId, 'M', "LOGGED", "Base received chat");
        hasActiveChatMessage = true;
        currentChatMessage = telemetry.chatMessage;
        currentChatDeviceId = telemetry.deviceId;
        currentChatRssi = telemetry.rssi;
        currentChatScrollOffset = 0;
        currentScreen = SCREEN_CUSTOM_MSG;
        drawCustomMessageScreen(currentChatDeviceId, currentChatMessage, currentChatRssi, currentChatScrollOffset);
        
        // Push custom chat message to Cloud API with distance and SNR for website display
        pushCustomChatMessageToAPI(telemetry.deviceId, telemetry.chatMessage, telemetry.rssi, telemetry.snr, telemetry.distanceKm, "LORA");
        return true;
    }

    if (telemetry.emergencyCode == 'N') {
        // Normal 1-Hour Periodic Telemetry Heartbeat Log
        Serial.printf("[RX NORMAL LOG] Device=%d, Temp=%.1f, Lat=%.6f, Lon=%.6f\n",
                      telemetry.deviceId, telemetry.temperature, telemetry.latitude, telemetry.longitude);
        notifyBLETelemetry(telemetry.deviceId, telemetry.temperature, telemetry.humidity,
                           telemetry.latitude, telemetry.longitude, telemetry.rssi);
        // Immediate Downlink ACK BEFORE cloud HTTP request!
        sendDownlinkACK(telemetry.deviceId, 'N', "LOGGED", "Heartbeat OK");
        pushFullTelemetryToAPI(telemetry);
    } else {
        // Active Emergency Alert (Delivery, Heli Rescue, Medical Shortage, Oxygen, etc.)
        Serial.printf("[RX ALERT] Device=%d, Code=%c (%s), RSSI=%ddB, SNR=%.1fdB, Dist=%.2fkm\n",
                      telemetry.deviceId, telemetry.emergencyCode, alertNames[telemetry.alertIndex], 
                      telemetry.rssi, telemetry.snr, telemetry.distanceKm);

        // 1. Render LCD UI IMMEDIATELY upon packet arrival (3ms latency, zero delay!)
        currentScreen = SCREEN_ALERT;
        drawAlertScreen(telemetry.alertIndex, telemetry.rssi, telemetry.snr, telemetry.distanceKm, false, false);

        // 2. Notify BLE smartphone companion app
        notifyBLEAlert(telemetry.deviceId, telemetry.emergencyCode, alertNames[telemetry.alertIndex], telemetry.rssi);

        // 3. Send LoRa Downlink ACK to TX unit (buzzer and LED already active without delay)
        sendDownlinkACK(telemetry.deviceId, telemetry.emergencyCode, "LOGGED", "Base confirmed");

        // 4. Upload to Cloud Web API
        bool sentToWeb = pushFullTelemetryToAPI(telemetry);

        // 5. Dynamically update Wi-Fi icon on LCD to reflect real cloud push status
        updateAlertWebStatus(sentToWeb);
    }

    return true;
}

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    delay(100);
    
    Serial.println(F("\n╔═══════════════════════════════════════════════════════════╗"));
    Serial.println(F("║      LIFELINE EMERGENCY RECEIVER v3.1 PRO                 ║"));
    Serial.println(F("║     16x2 LCD & Multi-WiFi (WiFi Button + OTA Web)         ║"));
    Serial.println(F("╚═══════════════════════════════════════════════════════════╝\n"));
    
    // Initialize IOs
    initBuzzerLED();
    
    pinMode(LORA_CS, OUTPUT);
    pinMode(LORA_RST, OUTPUT);
    pinMode(WIFI_PORTAL_PIN, INPUT_PULLUP);
    
    digitalWrite(LORA_CS, HIGH);
    
    // LoRa module hardware reset
    digitalWrite(LORA_RST, LOW);
    delay(10);
    digitalWrite(LORA_RST, HIGH);
    delay(10);
    
    // Setup sub-systems
    initDisplay();
    
    currentScreen = SCREEN_BOOT;
    drawBootScreen();
    playBootTone();
    
    Serial.println(F("[OK] Boot screen displayed"));
    
    initLoRa();
    initBLE();
    
    Serial.println(F("=== Ready to receive emergency alerts & BLE Commander ===\n"));
    
    #if SERIAL_DEBUG_ENABLED
    printSerialDebugMenu();
    #endif
}

void loop() {
    updateLEDs();
    updateBLE();
    handleLocalOTA();
    handleWiFiServer();
    
    #if SERIAL_DEBUG_ENABLED
    // Process Serial commands & simulated packets (from Web Serial companion app or terminal)
    if (Serial.available()) {
        FullTelemetryData simTelem;
        if (checkSerialSimulatedPacket(simTelem)) {
            triggerRxBlink();
            if (simTelem.isChatMessage) {
                Serial.printf("[RX CHAT LOG] Dev #%d: '%s'\n", simTelem.deviceId, simTelem.chatMessage.c_str());
                notifyBLEChat(simTelem.deviceId, simTelem.chatMessage.c_str(), simTelem.rssi);
                sendDownlinkACK(simTelem.deviceId, 'M', "LOGGED", "Base received chat");
                hasActiveChatMessage = true;
                currentChatMessage = simTelem.chatMessage;
                currentChatDeviceId = simTelem.deviceId;
                currentChatRssi = simTelem.rssi;
                currentChatScrollOffset = 0;
                currentScreen = SCREEN_CUSTOM_MSG;
                drawCustomMessageScreen(currentChatDeviceId, currentChatMessage, currentChatRssi, currentChatScrollOffset);
            } else if (simTelem.emergencyCode == 'N') {
                Serial.printf("[RX NORMAL LOG] Device=%d, Temp=%.1f, Lat=%.6f, Lon=%.6f\n",
                              simTelem.deviceId, simTelem.temperature, simTelem.latitude, simTelem.longitude);
                notifyBLETelemetry(simTelem.deviceId, simTelem.temperature, simTelem.humidity,
                                   simTelem.latitude, simTelem.longitude, simTelem.rssi);
                sendDownlinkACK(simTelem.deviceId, 'N', "LOGGED", "Heartbeat OK");
                pushFullTelemetryToAPI(simTelem);
            } else {
                Serial.printf("[RX ALERT] Device=%d, Code=%c (%s), RSSI=%ddB, SNR=%.1fdB, Dist=%.2fkm\n",
                              simTelem.deviceId, simTelem.emergencyCode, alertNames[simTelem.alertIndex], 
                              simTelem.rssi, simTelem.snr, simTelem.distanceKm);
                currentScreen = SCREEN_ALERT;
                drawAlertScreen(simTelem.alertIndex, simTelem.rssi, simTelem.snr, simTelem.distanceKm, false, false);
                notifyBLEAlert(simTelem.deviceId, simTelem.emergencyCode, alertNames[simTelem.alertIndex], simTelem.rssi);
                sendDownlinkACK(simTelem.deviceId, simTelem.emergencyCode, "LOGGED", "Base confirmed");
                bool sentToWeb = pushFullTelemetryToAPI(simTelem);
                updateAlertWebStatus(sentToWeb);
            }
        }
    }
    #endif

    // Process Commander BLE / Serial dispatch commands
    if (hasPendingBLEReply()) {
        int devId;
        String action, msg;
        getPendingBLEReply(devId, action, msg);
        Serial.printf("[BASE COMMAND] Sending Downlink CMD to #%d: %s (%s)\n", devId, action.c_str(), msg.c_str());
        bool sentOk = sendDownlinkCommand(devId, action.c_str(), msg.c_str());
        char confirm[128];
        snprintf(confirm, sizeof(confirm), "CMD_SENT:DEV=%03d,ACTION=%s,OK=%d", devId, action.c_str(), sentOk ? 1 : 0);
        sendBLEString(String(confirm));
        Serial.println(confirm);
    }
    
    // Process Commander BLE / Serial evacuation broadcast
    if (hasPendingBLEEvac()) {
        String evacMsg = getPendingBLEEvacMessage();
        Serial.printf("[BASE EVAC] Broadcasting Evacuation: '%s'\n", evacMsg.c_str());
        bool sentOk = sendBroadcastEvacuation(evacMsg.c_str());
        char confirm[128];
        snprintf(confirm, sizeof(confirm), "EVAC_SENT:OK=%d", sentOk ? 1 : 0);
        sendBLEString(String(confirm));
        Serial.println(confirm);
    }

    // Process Commander BLE custom message (MSG: or CHAT:)
    if (hasPendingBLEChat()) {
        String chat = getPendingBLEChatMessage();
        Serial.printf("[BASE BLE MSG] Incoming Custom BLE Message on Base: '%s'\n", chat.c_str());
        hasActiveChatMessage = true;
        currentChatMessage = chat;
        currentChatDeviceId = 0;
        currentChatRssi = -50;
        currentChatScrollOffset = 0;
        currentScreen = SCREEN_CUSTOM_MSG;
        playAlertTone(0);
        drawCustomMessageScreen(currentChatDeviceId, currentChatMessage, currentChatRssi, currentChatScrollOffset);
        
        // Push local commander message to Cloud API with BLE source tag
        pushCustomChatMessageToAPI(0, chat, -50, 10.0f, 0.01f, "BLE");
    }
    
    if (isLocalOTAModeActive()) {
        checkWiFiPortalButton();
        return;
    }
    
    if (portalActive) {
        checkWiFiPortalButton();
        handleWiFiPortal();
        return;
    }
    
    switch (currentScreen) {
        
        case SCREEN_BOOT:
            if (updateBootAnimation()) {
                loadWiFiCredentials();
                loadAPICredentials();
                if (networkCount > 0) {
                    Serial.printf("[WIFI] Auto-connecting to %d stored network(s). Primary: %s\n", networkCount, activeSSID.c_str());
                    bool connected = connectToWiFi();
                    if (connected) {
                        // Check for Remote OTA updates from VPS immediately on boot
                        checkAndPerformOTA();
                        
                        currentScreen = SCREEN_IDLE;
                        drawIdleScreen();
                        Serial.println(F("[STATE] WiFi connected -> Switched to IDLE"));
                    } else {
                        currentScreen = SCREEN_NO_WIFI;
                        drawNoWiFiScreen();
                        Serial.println(F("[STATE] WiFi failed -> Switched to NO_WIFI screen"));
                    }
                } else {
                    Serial.println(F("[WIFI] No stored credentials -> Switched to NO_WIFI screen"));
                    playWiFiFailTone();
                    currentScreen = SCREEN_NO_WIFI;
                    drawNoWiFiScreen();
                }
            }
            break;
            
        case SCREEN_NO_WIFI:
            checkWiFiPortalButton();
            
            // Auto timeout 60 seconds -> switch to IDLE
            if (millis() - noWiFiStartTime >= NO_WIFI_TIMEOUT) {
                playReturnIdleTone();
                currentScreen = SCREEN_IDLE;
                drawIdleScreen();
                Serial.println(F("[STATE] No WiFi screen timeout (60s) -> Switched to IDLE"));
            }
            
            // Check for incoming telemetry / alerts
            handleIncomingLoRaTelemetry();
            break;
            
        case SCREEN_COUNTDOWN:
            checkWiFiPortalButton();
            break;
            
        case SCREEN_PORTAL:
            checkWiFiPortalButton();
            handleWiFiPortal();
            break;
            
        case SCREEN_IDLE:
            checkWiFiPortalButton();
            updateIdleAnimation();
            
            handleIncomingLoRaTelemetry();
            break;
            
        case SCREEN_ALERT:
            checkWiFiPortalButton();
            
            handleIncomingLoRaTelemetry();
            
            if (shouldReturnToIdle()) {
                playReturnIdleTone();
                currentScreen = SCREEN_IDLE;
                drawIdleScreen();
                Serial.println(F("[STATE] Auto-returned to IDLE from ALERT (15s timeout)"));
            }
            break;

        case SCREEN_CUSTOM_MSG:
            checkWiFiPortalButton();
            
            handleIncomingLoRaTelemetry();

            // Auto-advance multi-page custom message every 4 seconds if longer than fits in one screen
            {
                static unsigned long lastAutoMsgScroll = 0;
                if (currentChatMessage.length() > 28) {
                    if (millis() - lastAutoMsgScroll >= 4000) {
                        lastAutoMsgScroll = millis();
                        scrollCurrentMessage(false);
                    }
                }
            }
            
            if (shouldReturnToIdle()) {
                hasActiveChatMessage = false;
                playReturnIdleTone();
                currentScreen = SCREEN_IDLE;
                drawIdleScreen();
                Serial.println(F("[STATE] Auto-returned to IDLE from CUSTOM_MSG (15s timeout)"));
            }
            break;
            
        case SCREEN_WIFI_SPLASH:
        case SCREEN_HISTORY:
        case SCREEN_SYSTEM_INFO:
            break;
    }
    
    delay(10);
}

// ═══════════════════════════════════════════════════════════════════════════════════
//                          SERIAL DEBUG CONFIGURATION
// ═══════════════════════════════════════════════════════════════════════════════════
bool checkSerialSimulatedPacket(FullTelemetryData& telemetry) {
    if (!Serial.available()) return false;
    
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n' || c == '\r') {
            if (serialInputBuffer.length() > 0) break;
        } else {
            serialInputBuffer += c;
        }
    }
    
    if (serialInputBuffer.length() == 0) return false;
    
    String input = serialInputBuffer;
    serialInputBuffer = "";
    input.trim();
    
    if (input.length() == 0) return false;
    
    if (input == "h" || input == "H" || input == "help" || input == "?") {
        printSerialDebugMenu();
        return false;
    }

    // Intercept Base Station Downlink & Status commands (from Web Companion App or Terminal)
    if (input.startsWith("REPLY:") || input.startsWith("CMD:") ||
        input.startsWith("EVAC:")  || input.equalsIgnoreCase("STATUS") ||
        input.equalsIgnoreCase("PING")) {
        processIncomingBaseCommand(input);
        return false;
    }
    
    // 1. Chat simulation: "CHAT:<message>" or "<devId>,CHAT,<message>"
    if (input.startsWith("CHAT:") || input.startsWith("chat:")) {
        telemetry.deviceId = 1;
        telemetry.emergencyCode = 'M';
        telemetry.alertIndex = 0;
        telemetry.isFullTelemetry = false;
        telemetry.isChatMessage = true;
        telemetry.chatMessage = input.substring(5);
        telemetry.chatMessage.trim();
        telemetry.rssi = -65;
        Serial.printf("[SERIAL DEBUG] Simulated Chat: Dev=1, Msg='%s'\n", telemetry.chatMessage.c_str());
        return true;
    }
    if (input.indexOf(",CHAT,") != -1 || input.indexOf(",chat,") != -1) {
        int chatComma = (input.indexOf(",CHAT,") != -1) ? input.indexOf(",CHAT,") : input.indexOf(",chat,");
        telemetry.deviceId = input.substring(0, chatComma).toInt();
        telemetry.emergencyCode = 'M';
        telemetry.alertIndex = 0;
        telemetry.isFullTelemetry = false;
        telemetry.isChatMessage = true;
        telemetry.chatMessage = input.substring(chatComma + 6);
        telemetry.chatMessage.trim();
        telemetry.rssi = -65;
        Serial.printf("[SERIAL DEBUG] Simulated Chat: Dev=%d, Msg='%s'\n", telemetry.deviceId, telemetry.chatMessage.c_str());
        return true;
    }

    // 2. Quick alert by single digit
    if (input.length() == 1 && ((input[0] >= '0' && input[0] <= '9'))) {
        telemetry.deviceId = 1;
        telemetry.alertIndex = (input[0] == '0') ? 9 : input[0] - '1';
        telemetry.emergencyCode = (telemetry.alertIndex == 14) ? 'N' : 'E';
        telemetry.isFullTelemetry = false;
        telemetry.isChatMessage = false;
        telemetry.rssi = -65;
        Serial.printf("[SERIAL DEBUG] Quick alert: Device=%d, Alert=%d (%s)\n", 
                      telemetry.deviceId, telemetry.alertIndex, alertNames[telemetry.alertIndex]);
        return true;
    }
    
    // 3. Quick alert by single letter
    if (input.length() == 1 && ((input[0] >= 'A' && input[0] <= 'O') || (input[0] >= 'a' && input[0] <= 'o'))) {
        telemetry.deviceId = 1;
        char code = (input[0] >= 'a') ? (input[0] - 'a' + 'A') : input[0];
        telemetry.alertIndex = code - 'A';
        telemetry.emergencyCode = code;
        telemetry.isFullTelemetry = false;
        telemetry.isChatMessage = false;
        telemetry.rssi = -65;
        Serial.printf("[SERIAL DEBUG] Quick alert: Device=%d, Alert=%c (%s)\n", 
                      telemetry.deviceId, code, alertNames[telemetry.alertIndex]);
        return true;
    }
    
    // 4. Standard format: DEVICE_ID,ALERT_CODE
    int comma = input.indexOf(',');
    if (comma <= 0) {
        Serial.println("[SERIAL DEBUG] Invalid format. Use: DEVICE_ID,ALERT_CODE or CHAT:text");
        return false;
    }
    
    telemetry.deviceId = input.substring(0, comma).toInt();
    String alertPart = input.substring(comma + 1);
    alertPart.trim();
    
    if (alertPart.length() == 1 && alertPart[0] >= 'A' && alertPart[0] <= 'O') {
        telemetry.alertIndex = alertPart[0] - 'A';
        telemetry.emergencyCode = alertPart[0];
    } else if (alertPart.length() == 1 && alertPart[0] >= 'a' && alertPart[0] <= 'o') {
        telemetry.alertIndex = alertPart[0] - 'a';
        telemetry.emergencyCode = alertPart[0] - 'a' + 'A';
    } else {
        telemetry.alertIndex = alertPart.toInt();
        telemetry.emergencyCode = (telemetry.alertIndex == 14) ? 'N' : 'E';
    }
    
    if (telemetry.alertIndex < 0 || telemetry.alertIndex >= ALERT_COUNT) {
        Serial.printf("[SERIAL DEBUG] Invalid alert index: %d\n", telemetry.alertIndex);
        return false;
    }
    
    telemetry.isFullTelemetry = false;
    telemetry.isChatMessage = false;
    telemetry.rssi = -65;
    telemetry.snr = 8.5f;
    telemetry.distanceKm = calculateDistanceKm(telemetry.rssi, 0.0, 0.0);
    Serial.printf("[SERIAL DEBUG] Simulated packet: Device=%d, Alert=%d (%s)\n", 
                  telemetry.deviceId, telemetry.alertIndex, alertNames[telemetry.alertIndex]);
    
    return true;
}

void printSerialDebugMenu() {
    Serial.println(F("\n╔════════════════════════════════════════════════════════════╗"));
    Serial.println(F("║       LIFELINE RECEIVER v3.1 PRO - Serial Debug            ║"));
    Serial.println(F("╠════════════════════════════════════════════════════════════╣"));
    Serial.println(F("║ QUICK COMMANDS:                                            ║"));
    Serial.println(F("║   1-9  : Simulate alert 1-9 from device 1                  ║"));
    Serial.println(F("║   0    : Simulate alert 10 from device 1                   ║"));
    Serial.println(F("║   A-O  : Simulate alert by code from device 1              ║"));
    Serial.println(F("║                                                            ║"));
    Serial.println(F("║ FULL FORMAT:                                               ║"));
    Serial.println(F("║   DEVICE_ID,ALERT_CODE  (e.g., '3,A' or '3,5')             ║"));
    Serial.println(F("║                                                            ║"));
    Serial.println(F("║ ALERT CODES:                                               ║"));
    Serial.println(F("║   A(0)=EMERGENCY       B(1)=MEDICAL      C(2)=MEDICINE     ║"));
    Serial.println(F("║   D(3)=EVACUATION      E(4)=STATUS OK    F(5)=INJURY       ║"));
    Serial.println(F("║   G(6)=FOOD            H(7)=WATER        I(8)=WEATHER      ║"));
    Serial.println(F("║   J(9)=LOST PERSON     K(10)=ANIMAL      L(11)=LANDSLIDE   ║"));
    Serial.println(F("║   M(12)=SNOW STORM     N(13)=EQUIPMENT   O(14)=OTHER       ║"));
    Serial.println(F("╚════════════════════════════════════════════════════════════╝\n"));
}
