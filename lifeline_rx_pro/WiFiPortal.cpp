#include "WiFiPortal.h"
#include "DisplayUI.h"
#include "BuzzerLED.h"
#include "OTAManager.h"
#include <WiFi.h>
#include <Preferences.h>
#include <Update.h>
#include <ArduinoOTA.h>

WebServer wifiServer(80);
DNSServer dnsServer;
Preferences preferences;

// Define variables
bool wifiConnected = false;
bool portalActive = false;
unsigned long portalStartTime = 0;
unsigned long buttonPressStartTime = 0;
bool buttonPressed = false;
int lastBeepSecond = -1;

WiFiNetwork storedNetworks[MAX_WIFI_NETWORKS];
int networkCount = 0;
String activeSSID = "";

String customApiKey = "";
String customApiEndpoint = API_ENDPOINT;

static bool routesConfigured = false;
static bool stationServerStarted = false;

// Forward declarations of server handlers
static void handlePortalRoot();
static void handlePortalSave();
static void handleAPISave();
static void setupServerRoutes();

void loadAPICredentials() {
    preferences.begin("lifeline", true);
    customApiKey = preferences.getString("api_key", "");
    customApiEndpoint = preferences.getString("api_url", API_ENDPOINT);
    preferences.end();
    if (customApiKey.length() > 0) {
        Serial.printf("[API] Loaded Custom API Key: %s...\n", customApiKey.substring(0, min(8, (int)customApiKey.length())).c_str());
    }
    Serial.printf("[API] Active Endpoint: %s\n", customApiEndpoint.c_str());
}

void saveAPICredentials(const String& key, const String& endpoint) {
    preferences.begin("lifeline", false);
    preferences.putString("api_key", key);
    preferences.putString("api_url", endpoint);
    preferences.end();
    customApiKey = key;
    customApiEndpoint = endpoint;
    Serial.println(F("[API] Saved custom API credentials to NVS."));
}

void loadWiFiCredentials() {
    preferences.begin("lifeline", true);
    networkCount = 0;
    
    for (int i = 0; i < MAX_WIFI_NETWORKS; i++) {
        String keySSID = "ssid" + String(i + 1);
        String keyPass = "pass" + String(i + 1);
        String s = preferences.getString(keySSID.c_str(), "");
        String p = preferences.getString(keyPass.c_str(), "");
        
        if (s.length() > 0) {
            storedNetworks[networkCount].ssid = s;
            storedNetworks[networkCount].password = p;
            networkCount++;
        }
    }
    preferences.end();
    
    if (networkCount > 0) {
        activeSSID = storedNetworks[0].ssid;
        Serial.printf("[WIFI] Loaded %d WiFi network(s). Primary: %s\n", networkCount, activeSSID.c_str());
    } else {
        Serial.println(F("[WIFI] No stored WiFi networks found"));
    }
}

void saveWiFiCredentialsList(const WiFiNetwork nets[], int count) {
    preferences.begin("lifeline", false);
    for (int i = 0; i < MAX_WIFI_NETWORKS; i++) {
        String keySSID = "ssid" + String(i + 1);
        String keyPass = "pass" + String(i + 1);
        if (i < count && nets[i].ssid.length() > 0) {
            preferences.putString(keySSID.c_str(), nets[i].ssid);
            preferences.putString(keyPass.c_str(), nets[i].password);
        } else {
            preferences.remove(keySSID.c_str());
            preferences.remove(keyPass.c_str());
        }
    }
    preferences.end();
    
    loadWiFiCredentials();
}

bool connectToWiFiSilent() {
    if (networkCount == 0) {
        Serial.println(F("[WIFI] No networks configured for silent connect"));
        return false;
    }
    
    WiFi.mode(WIFI_STA);
    
    for (int i = 0; i < networkCount; i++) {
        Serial.printf("[WIFI] Silent connect attempt %d/%d to %s...\n", 
                      i + 1, networkCount, storedNetworks[i].ssid.c_str());
        WiFi.begin(storedNetworks[i].ssid.c_str(), storedNetworks[i].password.c_str());
        
        unsigned long startTime = millis();
        while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < WIFI_CONNECT_TIMEOUT) {
            delay(500);
            Serial.print(".");
        }
        Serial.println();
        
        if (WiFi.status() == WL_CONNECTED) {
            wifiConnected = true;
            activeSSID = storedNetworks[i].ssid;
            initNTPTime();
            startStationWebServer();
            Serial.printf("[WIFI] Connected to %s! IP: %s\n", activeSSID.c_str(), WiFi.localIP().toString().c_str());
            return true;
        }
    }
    
    wifiConnected = false;
    Serial.println(F("[WIFI] All network connection attempts failed"));
    return false;
}

bool connectToWiFi() {
    if (networkCount == 0) {
        Serial.println(F("[WIFI] No credentials stored"));
        wifiConnected = false;
        playWiFiFailTone();
        return false;
    }
    
    WiFi.mode(WIFI_STA);
    
    for (int i = 0; i < networkCount; i++) {
        Serial.printf("[WIFI] Connecting to network %d/%d: %s...\n", 
                      i + 1, networkCount, storedNetworks[i].ssid.c_str());
        
        drawWiFiConnectingScreen(storedNetworks[i].ssid, i + 1, networkCount);
        WiFi.begin(storedNetworks[i].ssid.c_str(), storedNetworks[i].password.c_str());
        
        unsigned long startTime = millis();
        while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < WIFI_CONNECT_TIMEOUT) {
            delay(500);
            Serial.print(".");
        }
        Serial.println();
        
        if (WiFi.status() == WL_CONNECTED) {
            wifiConnected = true;
            activeSSID = storedNetworks[i].ssid;
            String ip = WiFi.localIP().toString();
            initNTPTime();
            startStationWebServer();
            Serial.printf("[WIFI] Connected to %s! Web Dashboard: http://%s\n", activeSSID.c_str(), ip.c_str());
            
            currentScreen = SCREEN_WIFI_SPLASH;
            drawWiFiConnectedScreen(ip);
            playWiFiSuccessTone();
            delay(WIFI_SPLASH_TIME);
            return true;
        }
    }
    
    wifiConnected = false;
    Serial.println(F("[WIFI] All network connections failed"));
    playWiFiFailTone();
    return false;
}

void startWiFiPortal() {
    Serial.println(F("[WIFI] Starting Captive WiFi configuration portal..."));
    
    WiFi.disconnect(true);
    delay(100);
    
    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_AP_SSID);
    
    IPAddress apIP = WiFi.softAPIP();
    Serial.printf("[WIFI] Open AP started! SSID: '%s'\n", WIFI_AP_SSID);
    Serial.printf("[WIFI] Portal IP: %s\n", apIP.toString().c_str());
    
    // Start DNS Server for captive portal auto-popup on port 53
    dnsServer.start(53, "*", apIP);
    
    setupServerRoutes();
    if (!stationServerStarted) {
        wifiServer.begin();
        stationServerStarted = true;
    }
    
    portalActive = true;
    portalStartTime = millis();
    currentScreen = SCREEN_PORTAL;
    
    drawWiFiPortalScreen();
}

void stopWiFiPortal() {
    if (!portalActive) return;
    
    Serial.println(F("[WIFI] Stopping AP portal..."));
    dnsServer.stop();
    WiFi.softAPdisconnect(true);
    portalActive = false;
    
    if (networkCount > 0) {
        connectToWiFi();
    }
    
    if (currentScreen != SCREEN_ALERT && currentScreen != SCREEN_CUSTOM_MSG) {
        currentScreen = SCREEN_IDLE;
        drawIdleScreen();
    }
}

static int buttonPressCount = 0;
static unsigned long firstPressTime = 0;

void checkWiFiPortalButton() {
    bool currentButtonState = digitalRead(WIFI_PORTAL_PIN);
    
    // If unit is currently in Local OTA mode, a single press exits OTA mode
    if (isLocalOTAModeActive()) {
        if (currentButtonState == LOW && !buttonPressed) {
            buttonPressed = true;
            buttonPressStartTime = millis();
        } else if (currentButtonState == HIGH && buttonPressed) {
            buttonPressed = false;
            Serial.println(F("[BUTTON] Single Wi-Fi button press -> Exiting Local OTA Mode!"));
            stopLocalOTAMode();
            playSkipConfirmTone();
            currentScreen = SCREEN_IDLE;
            drawIdleScreen();
        }
        return;
    }
    
    if (currentButtonState == LOW) { // Button on GPIO 14 active LOW
        if (!buttonPressed) {
            buttonPressed = true;
            buttonPressStartTime = millis();
            lastBeepSecond = -1;
            previousScreenBeforePress = currentScreen;
            
            // Multi-click detection (3 presses within 1.5 seconds)
            if (buttonPressCount == 0 || millis() - firstPressTime > 1500) {
                buttonPressCount = 1;
                firstPressTime = millis();
            } else {
                buttonPressCount++;
            }
            
            if (buttonPressCount >= 3) {
                buttonPressCount = 0;
                buttonPressed = false;
                Serial.println(F("[BUTTON] 3 Wi-Fi button presses detected -> Starting Local OTA Portal!"));
                playPortalOpenTone();
                startLocalOTAMode();
                return;
            }
        } else {
            unsigned long pressDuration = millis() - buttonPressStartTime;
            int secondsHeld = pressDuration / 1000;
            
            if (secondsHeld != lastBeepSecond && secondsHeld >= 1 && secondsHeld <= 3) {
                lastBeepSecond = secondsHeld;
                playCountdownTickTone();
            }
            
            int minCountdownMs = (previousScreenBeforePress == SCREEN_CUSTOM_MSG || hasActiveChatMessage) ? 450 : 150;
            if (!portalActive && pressDuration >= minCountdownMs) {
                currentScreen = SCREEN_COUNTDOWN;
                drawProgressCountdownScreen(pressDuration, LONG_PRESS_DURATION);
            }
            
            if (pressDuration >= LONG_PRESS_DURATION) {
                buttonPressed = false;
                
                if (portalActive) {
                    stopWiFiPortal();
                } else {
                    playPortalOpenTone();
                    startWiFiPortal();
                }
            }
        }
    } else {
        if (buttonPressed) {
            buttonPressed = false;
            unsigned long totalDuration = millis() - buttonPressStartTime;
            lastBeepSecond = -1;
            
            if (!portalActive) {
                if (totalDuration < 1000) {
                    // Short press
                    if (hasActiveChatMessage || previousScreenBeforePress == SCREEN_CUSTOM_MSG) {
                        currentScreen = SCREEN_CUSTOM_MSG;
                        scrollCurrentMessage();
                    } else if (previousScreenBeforePress == SCREEN_NO_WIFI) {
                        // Skip No WiFi screen
                        playSkipConfirmTone();
                        currentScreen = SCREEN_IDLE;
                        drawIdleScreen();
                    } else if (previousScreenBeforePress == SCREEN_IDLE) {
                        currentScreen = SCREEN_IDLE;
                        drawIdleScreen();
                    } else if (previousScreenBeforePress == SCREEN_ALERT) {
                        currentScreen = SCREEN_ALERT;
                        drawAlertScreen(lastDeviceId, lastAlertIndex, lastRssi);
                    }
                } else {
                    // Pressed between 1s and 3s, released before 3s -> cancel countdown
                    currentScreen = previousScreenBeforePress;
                    if (currentScreen == SCREEN_CUSTOM_MSG) {
                        drawCustomMessageScreen(currentChatDeviceId, currentChatMessage, currentChatRssi, currentChatScrollOffset);
                    } else if (currentScreen == SCREEN_NO_WIFI) {
                        drawNoWiFiScreen();
                    } else if (currentScreen == SCREEN_IDLE) {
                        drawIdleScreen();
                    } else if (currentScreen == SCREEN_ALERT) {
                        drawAlertScreen(lastDeviceId, lastAlertIndex, lastRssi);
                    }
                }
            }
        }
    }
}

void handleWiFiPortal() {
    if (!portalActive) return;
    
    dnsServer.processNextRequest();
    wifiServer.handleClient();
    
    if (millis() - portalStartTime >= WIFI_PORTAL_TIMEOUT) {
        Serial.println(F("[WIFI] Portal timeout, closing..."));
        stopWiFiPortal();
    }
}

// HTML and Endpoint Handlers (Bugatti-Inspired Austere Luxury Design System)
static String getPortalHTML() {
    String currentIP = (wifiConnected && WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
    String currentStatus = (wifiConnected && WiFi.status() == WL_CONNECTED) ? ("ONLINE &bull; " + activeSSID + " (" + String(WiFi.RSSI()) + " dBm)") : "CAPTIVE SOFTAP ACTIVE";
    String freeHeapStr = String(ESP.getFreeHeap() / 1024) + " KB";

    String html = F("<!DOCTYPE html><html lang='en'><head>"
        "<meta charset='UTF-8'>"
        "<meta name='viewport' content='width=device-width,initial-scale=1'>"
        "<title>LIFELINE RX PRO // COMMAND GATEWAY</title>"
        "<style>"
        ":root{--bg-0:#000000;--bg-1:#0d0d0d;--bg-2:#141414;--bg-3:#1f1f1f;--border:#262626;--border-strong:#3a3a3a;--text-0:#ffffff;--text-1:#cccccc;--text-2:#999999;--text-3:#666666;--danger:#ff1e42;--success:#5fa657;--info:#06b6d4;--indigo:#6366f1;--radius:0px;--radius-btn:9999px;}"
        "*{box-sizing:border-box;margin:0;padding:0;border-radius:var(--radius);}"
        "body{background:var(--bg-0);color:var(--text-1);font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;min-height:100vh;display:flex;flex-direction:column;align-items:center;padding:24px 14px 48px;position:relative;-webkit-font-smoothing:antialiased;}"
        ".top-rail{position:fixed;top:0;left:0;width:100%;height:3px;background:linear-gradient(90deg,#6366f1,#06b6d4,#8b5cf6,#f59e0b);z-index:999;box-shadow:0 0 12px rgba(6,182,212,0.6);}"
        ".container{width:100%;max-width:540px;margin:0 auto;position:relative;z-index:1;}"
        ".card{background:rgba(18,18,24,0.92);border:1px solid var(--border-strong);padding:24px 22px;box-shadow:0 18px 36px -12px rgba(0,0,0,0.8);position:relative;margin-bottom:18px;}"
        ".card::before{content:'';position:absolute;top:0;left:0;width:4px;height:100%;background:linear-gradient(180deg,#06b6d4 0%,#6366f1 100%);}"
        ".header{border-bottom:1px solid var(--border);padding-bottom:14px;margin-bottom:16px;}"
        ".meta-row{display:flex;align-items:center;justify-content:space-between;margin-bottom:6px;}"
        ".brand-tag{font-family:Consolas,Monaco,monospace;font-size:11px;font-weight:700;letter-spacing:1.5px;color:var(--info);text-transform:uppercase;}"
        ".badge-mode{font-family:Consolas,Monaco,monospace;font-size:10px;text-transform:uppercase;letter-spacing:1px;padding:3px 9px;background:rgba(95,166,87,0.12);border:1px solid rgba(95,166,87,0.4);color:#86efac;border-radius:var(--radius-btn);}"
        "h1{font-family:'Space Grotesk',sans-serif;font-size:22px;font-weight:700;letter-spacing:-0.5px;color:var(--text-0);margin-bottom:4px;}"
        ".sub{font-size:12px;color:var(--text-2);}"
        ".tele-grid{display:grid;grid-template-columns:repeat(3,1fr);gap:8px;background:var(--bg-1);border:1px solid var(--border);padding:10px 12px;margin-bottom:6px;font-family:Consolas,Monaco,monospace;font-size:11px;}"
        ".tele-cell{display:flex;flex-direction:column;gap:2px;}"
        ".tele-lbl{font-size:9px;color:var(--text-3);text-transform:uppercase;letter-spacing:0.8px;}"
        ".tele-val{font-weight:600;color:var(--text-0);overflow:hidden;text-overflow:ellipsis;white-space:nowrap;}"
        ".tele-val.cyan{color:#67e8f9;}"
        ".sec-head{display:flex;align-items:center;gap:8px;margin-bottom:10px;padding-bottom:8px;border-bottom:1px solid var(--border);}"
        ".sec-tag{font-family:Consolas,Monaco,monospace;font-size:9px;font-weight:700;padding:2px 6px;border:1px solid var(--border-strong);background:var(--bg-3);color:var(--text-1);}"
        ".sec-tag.cyan{border-color:rgba(6,182,212,0.4);color:#67e8f9;background:rgba(6,182,212,0.08);}"
        ".sec-tag.green{border-color:rgba(95,166,87,0.4);color:#86efac;background:rgba(95,166,87,0.08);}"
        ".sec-tag.indigo{border-color:rgba(99,102,241,0.4);color:#a5b4fc;background:rgba(99,102,241,0.08);}"
        ".sec-title{font-size:13px;font-weight:700;text-transform:uppercase;letter-spacing:1px;color:var(--text-0);}"
        ".sec-desc{font-size:11.5px;color:var(--text-2);line-height:1.4;margin-bottom:14px;}"
        ".upload-box{border:1px dashed var(--border-strong);background:var(--bg-1);padding:20px 14px;text-align:center;cursor:pointer;position:relative;margin-bottom:10px;transition:border-color 0.2s;}"
        ".upload-box:hover{border-color:#06b6d4;background:rgba(6,182,212,0.04);}"
        ".upload-box input[type=file]{position:absolute;top:0;left:0;width:100%;height:100%;opacity:0;cursor:pointer;}"
        ".upload-prompt{font-size:12px;color:var(--text-1);font-weight:500;margin-bottom:2px;}"
        ".upload-prompt span{color:#06b6d4;text-decoration:underline;}"
        ".upload-hint{font-family:monospace;font-size:10px;color:var(--text-3);}"
        ".file-sel{display:none;background:var(--bg-2);border:1px solid var(--border-strong);padding:8px 12px;margin-bottom:10px;justify-content:space-between;font-family:monospace;font-size:11px;color:var(--text-0);}"
        ".wifi-slot{background:var(--bg-1);border:1px solid var(--border);padding:12px 12px 4px;margin-bottom:10px;}"
        ".wifi-slot-title{font-family:Consolas,Monaco,monospace;font-size:10px;font-weight:700;color:var(--text-2);text-transform:uppercase;letter-spacing:0.8px;display:flex;justify-content:space-between;margin-bottom:8px;}"
        ".badge-pri{font-size:8px;padding:2px 5px;border-radius:var(--radius-btn);text-transform:uppercase;background:rgba(16,185,129,0.15);border:1px solid rgba(16,185,129,0.4);color:#6ee7b7;}"
        ".badge-sec{font-size:8px;padding:2px 5px;border-radius:var(--radius-btn);text-transform:uppercase;background:var(--bg-3);border:1px solid var(--border-strong);color:var(--text-3);}"
        "label{display:block;font-family:Consolas,Monaco,monospace;font-size:10px;font-weight:700;color:var(--text-2);text-transform:uppercase;letter-spacing:0.8px;margin:8px 0 3px;}"
        "input[type=text],input[type=password]{width:100%;padding:10px 12px;background:var(--bg-1);border:1px solid var(--border);color:var(--text-0);font-family:monospace;font-size:12px;outline:none;transition:border-color 0.2s;margin-bottom:8px;}"
        "input[type=text]:focus,input[type=password]:focus{border-color:var(--text-0);}"
        ".btn{display:inline-flex;align-items:center;justify-content:center;width:100%;padding:12px 18px;font-family:Consolas,Monaco,monospace;font-size:12px;font-weight:700;letter-spacing:1.2px;text-transform:uppercase;border-radius:var(--radius-btn);cursor:pointer;position:relative;overflow:hidden;transition:all 0.2s ease;border:none;margin-top:8px;}"
        ".btn-cyan{background:#06b6d4;color:#000;box-shadow:0 4px 14px rgba(6,182,212,0.3);}"
        ".btn-cyan:hover:not(:disabled){background:#22d3ee;box-shadow:0 6px 20px rgba(6,182,212,0.5);}"
        ".btn-emerald{background:#10b981;color:#000;box-shadow:0 4px 14px rgba(16,185,129,0.3);}"
        ".btn-emerald:hover:not(:disabled){background:#34d399;box-shadow:0 6px 20px rgba(16,185,129,0.5);}"
        ".btn-indigo{background:#6366f1;color:#fff;box-shadow:0 4px 14px rgba(99,102,241,0.3);}"
        ".btn-indigo:hover:not(:disabled){background:#818cf8;box-shadow:0 6px 20px rgba(99,102,241,0.5);}"
        ".btn:disabled{opacity:0.45;cursor:not-allowed;}"
        ".progress-wrap{display:none;margin-top:14px;}"
        ".progress-meta{display:flex;justify-content:space-between;font-family:monospace;font-size:11px;margin-bottom:5px;color:var(--text-2);}"
        ".progress-bar-bg{width:100%;height:8px;background:var(--bg-1);border:1px solid var(--border-strong);overflow:hidden;}"
        ".progress-bar-fill{width:0%;height:100%;background:linear-gradient(90deg,#06b6d4,#6366f1);transition:width 0.1s;}"
        ".msg{margin-top:12px;padding:10px 12px;font-family:monospace;font-size:11px;line-height:1.4;display:none;border-left:3px solid;}"
        ".msg.err{background:rgba(255,30,66,0.12);border-color:var(--danger);color:#fca5a5;}"
        ".msg.ok{background:rgba(95,166,87,0.12);border-color:var(--success);color:#86efac;}"
        ".footer{margin-top:12px;padding-top:14px;border-top:1px solid var(--border);display:flex;justify-content:space-between;font-family:monospace;font-size:10px;color:var(--text-3);}"
        "</style></head><body><div class='top-rail'></div><div class='container'>"

        "<div class='card'><div class='header'><div class='meta-row'><span class='brand-tag'>LIFELINE RX PRO</span>"
        "<span class='badge-mode'>GATEWAY READY</span></div>"
        "<h1>BASE COMMAND CONSOLE</h1><div class='sub'>Disaster Telemetry &amp; LoRa Base Station Gateway</div></div>"
        "<div class='tele-grid'>"
        "<div class='tele-cell'><span class='tele-lbl'>IP Address</span><span class='tele-val cyan'>"
    );
    html += currentIP;
    html += F(
        "</span></div><div class='tele-cell'><span class='tele-lbl'>Firmware</span><span class='tele-val'>v3.1.0 PRO</span></div>"
        "<div class='tele-cell'><span class='tele-lbl'>Free Heap</span><span class='tele-val'>"
    );
    html += freeHeapStr;
    html += F(
        "</span></div><div class='tele-cell'><span class='tele-lbl'>LoRa Uplink</span><span class='tele-val'>433MHz SX1278</span></div>"
        "<div class='tele-cell'><span class='tele-lbl'>Active Link</span><span class='tele-val'>"
    );
    html += currentStatus;
    html += F(
        "</span></div><div class='tele-cell'><span class='tele-lbl'>Flash Safety</span><span class='tele-val'>Dual Partition</span></div>"
        "</div></div>"

        // Module 1: Firmware Flash (OTA)
        "<div class='card'>"
        "<div class='sec-head'><span class='sec-tag cyan'>01</span><span class='sec-title'>Wireless OTA Firmware Flash</span></div>"
        "<div class='sec-desc'>Upload compiled firmware .bin file. Dual-partition safety prevents device bricks.</div>"
        "<form id='ota_form'>"
        "<div class='upload-box'>"
        "<input type='file' id='fwFile' name='update' accept='.bin' required>"
        "<div class='upload-prompt'><span>Browse binary</span> or tap here</div>"
        "<div class='upload-hint'>ESP32 FIRMWARE (*.bin)</div>"
        "</div>"
        "<div class='file-sel' id='rx_file_meta'><span id='rx_file_name'>firmware.bin</span><span id='rx_file_size'>0 KB</span></div>"
        "<button type='submit' class='btn btn-cyan' id='btn_ota'>FLASH BASE STATION FIRMWARE</button>"
        "</form>"
        "<div class='progress-wrap' id='p_box'>"
        "<div class='progress-meta'><span id='rx_p_status'>Flashing firmware...</span><span id='rx_p_pct'>0%</span></div>"
        "<div class='progress-bar-bg'><div class='progress-bar-fill' id='p_bar'></div></div>"
        "</div>"
        "<div class='msg' id='flash_msg'></div>"
        "</div>"

        // Module 2: Tri-Network Wi-Fi Multi-Failover
        "<div class='card'>"
        "<div class='sec-head'><span class='sec-tag green'>02</span><span class='sec-title'>Tri-Network Wi-Fi Failover Setup</span></div>"
        "<div class='sec-desc'>Store up to 3 Wi-Fi networks. The base station auto-reconnects with failover redundancy.</div>"
        "<form action='/save' method='POST'>"
    );

    for (int i = 0; i < 3; i++) {
        String numStr = String(i + 1);
        String currentS = (i < networkCount) ? storedNetworks[i].ssid : "";
        String currentP = (i < networkCount) ? storedNetworks[i].password : "";
        html += "<div class='wifi-slot'>";
        html += "<div class='wifi-slot-title'><span>Network #" + numStr + "</span>";
        html += (i == 0) ? "<span class='badge-pri'>PRIMARY</span>" : "<span class='badge-sec'>BACKUP</span>";
        html += "</div>";
        html += "<label>SSID Name</label>";
        html += "<input type='text' name='ssid" + numStr + "' placeholder='Network SSID' value='" + currentS + "'" + (i == 0 ? " required" : "") + ">";
        html += "<label>WPA2 Password</label>";
        html += "<input type='password' name='pass" + numStr + "' placeholder='Password' value='" + currentP + "'>";
        html += "</div>";
    }

    html += F(
        "<button type='submit' class='btn btn-emerald'>SAVE &amp; RECONNECT WI-FI</button>"
        "</form>"
        "</div>"

        // Module 3: Cloud REST API Setup
        "<div class='card'>"
        "<div class='sec-head'><span class='sec-tag indigo'>03</span><span class='sec-title'>Cloud REST API Webhook</span></div>"
        "<div class='sec-desc'>Centralized server endpoint where emergency distress packets and GPS fixes are forwarded.</div>"
        "<form action='/api-save' method='POST'>"
        "<label>REST API Endpoint URL</label>"
        "<input type='text' name='api_url' value='"
    );
    html += (customApiEndpoint.length() > 0 ? customApiEndpoint : API_ENDPOINT);
    html += F(
        "' placeholder='https://...'>"
        "<label>API Key / Bearer Secret</label>"
        "<input type='password' name='api_key' value='"
    );
    html += customApiKey;
    html += F(
        "' placeholder='Enter API Key (optional)'>"
        "<button type='submit' class='btn btn-indigo'>SAVE API CONFIGURATION</button>"
        "</form>"
        "</div>"

        "<div class='footer'><span>LIFELINE COMMAND // PROTOCOL v3.1</span><span>SYS: ESP32 + SX1278</span></div>"
        "</div>"

        "<script>"
        "var fi=document.getElementById('fwFile');"
        "fi.onchange=function(){"
        "  if(this.files&&this.files[0]){"
        "    document.getElementById('rx_file_name').innerText=this.files[0].name;"
        "    document.getElementById('rx_file_size').innerText=Math.round(this.files[0].size/1024)+' KB';"
        "    document.getElementById('rx_file_meta').style.display='flex';"
        "  }"
        "};"
        "var f=document.getElementById('ota_form');"
        "if(f){f.onsubmit=function(e){"
        "  e.preventDefault();"
        "  if(!fi||!fi.files.length)return false;"
        "  var file=fi.files[0];"
        "  var pb=document.getElementById('p_box');"
        "  var pr=document.getElementById('p_bar');"
        "  var pctLbl=document.getElementById('rx_p_pct');"
        "  var statLbl=document.getElementById('rx_p_status');"
        "  var btn=document.getElementById('btn_ota');"
        "  var msg=document.getElementById('flash_msg');"
        "  pb.style.display='block';pr.style.width='0%';pctLbl.innerText='0%';"
        "  btn.disabled=true;msg.style.display='none';"
        "  statLbl.innerText='Streaming Blocks to Flash Partition...';"
        "  var xhr=new XMLHttpRequest();"
        "  xhr.open('POST','/update?size='+file.size,true);"
        "  xhr.upload.onprogress=function(ev){"
        "    if(ev.lengthComputable){"
        "      var p=Math.round((ev.loaded/ev.total)*100);"
        "      if(p>99)p=99;"
        "      pr.style.width=p+'%';"
        "      pctLbl.innerText=p+'%';"
        "      statLbl.innerText='Writing Flash: '+p+'% ('+Math.round(ev.loaded/1024)+' KB)';"
        "    }"
        "  };"
        "  xhr.onload=function(){"
        "    if(xhr.status>=200&&xhr.status<300){"
        "      pr.style.width='100%';"
        "      pr.style.background='#10b981';"
        "      pctLbl.innerText='100%';"
        "      msg.className='msg ok';msg.style.display='block';"
        "      msg.innerHTML='<strong>UPDATE COMPLETE:</strong> Base Station is rebooting now...';"
        "      setTimeout(function(){location.reload();},6000);"
        "    }else{"
        "      msg.className='msg err';msg.style.display='block';"
        "      msg.innerHTML='<strong>UPDATE FAILED ('+xhr.status+')</strong>';"
        "      btn.disabled=false;"
        "    }"
        "  };"
        "  xhr.onerror=function(){"
        "    pr.style.width='100%';"
        "    pr.style.background='#10b981';"
        "    msg.className='msg ok';msg.style.display='block';"
        "    msg.innerHTML='<strong>TRANSFER COMPLETE:</strong> Base Station rebooting...';"
        "    setTimeout(function(){location.reload();},6000);"
        "  };"
        "  var d=new FormData();d.append('update',file);"
        "  xhr.send(d);"
        "  return false;"
        "};}"
        "</script></body></html>"
    );
    return html;
}

static void handlePortalRoot() {
    wifiServer.send(200, "text/html", getPortalHTML());
}

static void handleAPISave() {
    String newKey = wifiServer.arg("api_key");
    String newUrl = wifiServer.arg("api_url");
    newKey.trim();
    newUrl.trim();
    
    if (newUrl.length() == 0) newUrl = API_ENDPOINT;
    
    saveAPICredentials(newKey, newUrl);
    
    String html = "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'><title>LIFELINE RX // API SAVED</title>";
    html += "<style>:root{--bg-0:#000000;--bg-1:#0d0d0d;--border:#262626;--border-strong:#3a3a3a;--success:#5fa657;--radius-btn:9999px;}"
            "*{box-sizing:border-box;margin:0;padding:0;border-radius:0px;}"
            "body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;background:var(--bg-0);color:#ccc;text-align:center;padding:50px 16px;}"
            ".card{background:rgba(18,18,24,0.92);border:1px solid var(--border-strong);padding:32px 24px;max-width:440px;margin:0 auto;box-shadow:0 20px 40px -15px rgba(0,0,0,0.8);position:relative;}"
            ".card::before{content:'';position:absolute;top:0;left:0;width:4px;height:100%;background:#10b981;}"
            "h2{color:#fff;font-size:18px;margin-bottom:12px;text-transform:uppercase;letter-spacing:1px;} p{color:#999;font-size:12px;line-height:1.5;margin-top:10px;}"
            "code{background:var(--bg-1);padding:6px 10px;border:1px solid var(--border);color:#67e8f9;display:block;margin:6px 0 12px;word-break:break-all;font-family:monospace;font-size:11px;}"
            ".btn{display:inline-block;margin-top:20px;padding:12px 24px;background:#fff;color:#000;text-decoration:none;font-weight:700;font-family:monospace;font-size:11px;letter-spacing:1px;border-radius:var(--radius-btn);text-transform:uppercase;}</style></head><body>";
    html += "<div class='card'><h2>API SETTINGS COMMITTED</h2>";
    html += "<p>Ingestion Endpoint URL:</p><code>" + newUrl + "</code>";
    html += "<p>Authentication Secret:</p><code>" + (newKey.length() > 0 ? newKey : "(None / Cleared)") + "</code>";
    html += "<a class='btn' href='/'>RETURN TO COMMAND CONSOLE</a></div></body></html>";
    
    wifiServer.send(200, "text/html", html);
}

static void handlePortalSave() {
    WiFiNetwork newNets[3];
    int count = 0;
    
    for (int i = 0; i < 3; i++) {
        String s = wifiServer.arg("ssid" + String(i + 1));
        String p = wifiServer.arg("pass" + String(i + 1));
        s.trim();
        p.trim();
        if (s.length() > 0) {
            newNets[count].ssid = s;
            newNets[count].password = p;
            count++;
        }
    }
    
    saveWiFiCredentialsList(newNets, count);
    
    String html = "<!DOCTYPE html><html lang='en'><head>";
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<title>LIFELINE RX // CONFIG SAVED</title>";
    html += "<style>:root{--bg-0:#000000;--bg-1:#0d0d0d;--border-strong:#3a3a3a;--radius-btn:9999px;}"
            "*{box-sizing:border-box;margin:0;padding:0;border-radius:0px;}"
            "body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;background:var(--bg-0);color:#ccc;text-align:center;padding:50px 16px;}"
            ".card{background:rgba(18,18,24,0.92);border:1px solid var(--border-strong);padding:32px 24px;max-width:440px;margin:0 auto;box-shadow:0 20px 40px -15px rgba(0,0,0,0.8);position:relative;}"
            ".card::before{content:'';position:absolute;top:0;left:0;width:4px;height:100%;background:#06b6d4;}"
            "h2{color:#fff;font-size:18px;margin-bottom:10px;text-transform:uppercase;letter-spacing:1px;}"
            "p{color:#999;font-size:12px;line-height:1.6;}</style></head><body>";
    html += "<div class='card'><h2>WI-FI REDUNDANCY SAVED</h2>";
    html += "<p>Successfully stored <strong>" + String(count) + "</strong> failover network profile(s).</p>";
    html += "<p>Base Station is restarting now to connect to primary network...</p></div>";
    html += "</body></html>";
    
    wifiServer.send(200, "text/html", html);
    
    delay(2000);
    ESP.restart();
}

static void setupServerRoutes() {
    if (routesConfigured) return;
    
    wifiServer.on("/", HTTP_GET, handlePortalRoot);
    wifiServer.on("/save", HTTP_POST, handlePortalSave);
    wifiServer.on("/api-save", HTTP_POST, handleAPISave);
    
    static size_t rxPortalExpected = 0;
    static size_t rxPortalAccumulated = 0;
    static int lastPortalPct = -1;
    wifiServer.on("/update", HTTP_POST, []() {
        wifiServer.sendHeader("Connection", "close");
        if (Update.hasError()) {
            wifiServer.send(500, "text/plain", "FAIL: Flash Error");
        } else {
            wifiServer.send(200, "text/plain", "SUCCESS");
        }
        delay(1000);
        ESP.restart();
    }, []() {
        HTTPUpload& upload = wifiServer.upload();
        if (upload.status == UPLOAD_FILE_START) {
            Serial.printf("[WEB OTA] Start: %s\n", upload.filename.c_str());
            currentScreen = SCREEN_OTA;
            lastPortalPct = -1;
            rxPortalExpected = 0;
            rxPortalAccumulated = 0;
            if (wifiServer.hasArg("size")) {
                rxPortalExpected = wifiServer.arg("size").toInt();
            }
            if (rxPortalExpected <= 0) {
                int cl = wifiServer.clientContentLength();
                if (cl > 300) {
                    rxPortalExpected = cl - 200; // Offset multipart boundary overhead
                } else if (cl > 0) {
                    rxPortalExpected = cl;
                } else {
                    rxPortalExpected = 1350000; // Fallback typical firmware size
                }
            }
            Serial.printf("[WEB OTA] Expected size: %u bytes\n", (unsigned int)rxPortalExpected);
            drawOTAProgressScreen(0);
            if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                Update.printError(Serial);
            }
            rxPortalAccumulated += upload.currentSize;
            if (rxPortalExpected > 0) {
                int pct = (rxPortalAccumulated * 100) / rxPortalExpected;
                pct = constrain(pct, 0, 99);
                if (pct != lastPortalPct) {
                    lastPortalPct = pct;
                    drawOTAProgressScreen(pct);
                }
            }
        } else if (upload.status == UPLOAD_FILE_END) {
            if (Update.end(true)) {
                Serial.printf("[WEB OTA] Success: %u bytes\n", (unsigned int)rxPortalAccumulated);
                drawOTAProgressScreen(100);
                drawOTASuccessScreen();
            } else {
                Update.printError(Serial);
                drawOTAFailedScreen("Flash Failed");
            }
        }
    });

    // Captive portal fallback routes
    wifiServer.on("/generate_204", handlePortalRoot);
    wifiServer.on("/redirect", handlePortalRoot);
    wifiServer.on("/hotspot-detect.html", handlePortalRoot);
    wifiServer.on("/canonical.html", handlePortalRoot);
    wifiServer.on("/nconnect.txt", handlePortalRoot);
    wifiServer.onNotFound([]() {
        if (portalActive) {
            wifiServer.sendHeader("Location", "http://192.168.4.1/", true);
            wifiServer.send(302, "text/plain", "");
        } else {
            wifiServer.sendHeader("Location", "/", true);
            wifiServer.send(302, "text/plain", "");
        }
    });

    routesConfigured = true;
}

void startStationWebServer() {
    setupServerRoutes();
    if (!stationServerStarted) {
        wifiServer.begin();
        stationServerStarted = true;
    }
    ArduinoOTA.setHostname("lifeline-rx-base");
    ArduinoOTA.begin();
    Serial.printf("[WEB] Station Web Dashboard active on http://%s\n", WiFi.localIP().toString().c_str());
}

void handleWiFiServer() {
    if (wifiConnected && !portalActive) {
        wifiServer.handleClient();
        ArduinoOTA.handle();
    }
}
