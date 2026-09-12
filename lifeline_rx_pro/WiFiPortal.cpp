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

// HTML and Endpoint Handlers (Reference Image Exact Style: Light + Dark Mode, 1-Screen Fit)
static String getPortalHTML() {
    String currentIP = (wifiConnected && WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
    String currentStatus = (wifiConnected && WiFi.status() == WL_CONNECTED) ? ("ONLINE &bull; " + activeSSID) : "CAPTIVE SOFTAP ACTIVE";
    String freeHeapStr = String(ESP.getFreeHeap() / 1024) + " KB";

    String html = F(
        "<!DOCTYPE html><html lang='en' data-theme='light'><head><meta charset='UTF-8'>"
        "<meta name='viewport' content='width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no'>"
        "<title>LIFELINE RX PRO // BASE COMMAND CONSOLE</title>"
        "<style>"
        ":root{"
        "--canvas:#f8f9fa;--card-outer:#ededf0;--card-border:#dcdde2;--card-inner:#ffffff;--card-inner-border:#e2e4e9;"
        "--tab-bg:#e5e6eb;--tab-border:#d2d4dc;--tab-text:#4b5563;--tab-active-bg:#000000;--tab-active-text:#ffffff;--tab-active-border:#000000;"
        "--text-main:#000000;--text-sub:#4b5563;--text-muted:#6b7280;--badge-bg:#ffffff;--badge-border:#d1d5db;--badge-text:#111827;"
        "--num-bg:#f3f4f6;--num-border:#e5e7eb;--num-text:#111827;--btn-bg:#ffffff;--btn-text:#000000;--btn-border:#000000;"
        "--btn-hover-bg:#000000;--btn-hover-text:#ffffff;--input-bg:#ffffff;--input-border:#d1d5db;--progress-bg:#e5e7eb;--progress-fill:#000000;"
        "}"
        "[data-theme='dark']{"
        "--canvas:#09090b;--card-outer:#141417;--card-border:#27272a;--card-inner:#1c1c21;--card-inner-border:#2e2e34;"
        "--tab-bg:#1c1c21;--tab-border:#2e2e34;--tab-text:#9ca3af;--tab-active-bg:#ffffff;--tab-active-text:#000000;--tab-active-border:#ffffff;"
        "--text-main:#ffffff;--text-sub:#9ca3af;--text-muted:#6b7280;--badge-bg:#141417;--badge-border:#2e2e34;--badge-text:#e5e7eb;"
        "--num-bg:#27272a;--num-border:#3f3f46;--num-text:#ffffff;--btn-bg:#1c1c21;--btn-text:#ffffff;--btn-border:#ffffff;"
        "--btn-hover-bg:#ffffff;--btn-hover-text:#000000;--input-bg:#121215;--input-border:#3f3f46;--progress-bg:#27272a;--progress-fill:#ffffff;"
        "}"
        "*{box-sizing:border-box;margin:0;padding:0;font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;}"
        "html,body{min-height:100%;min-height:100dvh;overflow-x:hidden;overflow-y:auto;background:var(--canvas);color:var(--text-main);}"
        "body{display:flex;flex-direction:column;justify-content:center;align-items:center;padding:12px 18px;}"
        ".frame{width:100%;max-width:1080px;min-height:94vh;display:flex;flex-direction:column;justify-content:space-between;}"
        ".top-row{display:flex;justify-content:space-between;align-items:center;margin-bottom:8px;}"
        ".badge-pill{display:inline-flex;align-items:center;gap:6px;padding:4px 12px;border-radius:9999px;border:1px solid var(--badge-border);background:var(--badge-bg);color:var(--badge-text);font-family:monospace;font-size:11px;font-weight:700;letter-spacing:0.5px;text-transform:uppercase;}"
        ".theme-btn{display:inline-flex;align-items:center;gap:6px;padding:5px 12px;border-radius:9999px;border:1px solid var(--badge-border);background:var(--badge-bg);color:var(--badge-text);font-size:11px;font-weight:600;cursor:pointer;}"
        ".theme-icon-svg{display:block;width:13px;height:13px;stroke:currentColor;}"
        "h1{font-size:28px;font-weight:800;letter-spacing:-0.8px;color:var(--text-main);line-height:1.15;margin-bottom:2px;}"
        ".sub{font-size:13px;color:var(--text-sub);margin-bottom:12px;}"
        ".tabs{display:flex;gap:8px;margin-bottom:10px;}"
        ".tab{padding:7px 15px;border:1px solid var(--tab-border);background:var(--tab-bg);color:var(--tab-text);font-family:monospace;font-size:11px;font-weight:700;letter-spacing:0.8px;text-transform:uppercase;cursor:pointer;}"
        ".tab.active{background:var(--tab-active-bg);color:var(--tab-active-text);border-color:var(--tab-active-border);}"
        ".card{border:1px solid var(--card-border);background:var(--card-outer);padding:16px 18px;display:grid;grid-template-columns:1.25fr 1fr;gap:16px;box-shadow:0 4px 18px rgba(0,0,0,0.04);}"
        ".panel{display:flex;flex-direction:column;justify-content:space-between;min-height:240px;overflow-y:auto;}"
        ".panel h2{font-size:19px;font-weight:700;color:var(--text-main);margin-bottom:3px;}"
        ".panel p{font-size:12px;color:var(--text-sub);line-height:1.35;margin-bottom:10px;}"
        ".check-grid{display:grid;grid-template-columns:1fr 1fr;gap:5px 12px;margin-bottom:10px;font-size:11px;color:var(--text-sub);}"
        ".check-item{display:flex;align-items:center;gap:6px;}"
        ".upload-box{border:1px dashed var(--card-border);background:var(--card-inner);padding:14px;text-align:center;cursor:pointer;position:relative;margin-bottom:8px;}"
        ".upload-box input[type=file]{position:absolute;top:0;left:0;width:100%;height:100%;opacity:0;cursor:pointer;}"
        ".upload-box span{font-size:12px;font-weight:600;color:var(--text-main);}"
        ".upload-box small{display:block;font-family:monospace;font-size:10px;color:var(--text-muted);margin-top:2px;}"
        ".file-bar{display:none;justify-content:space-between;font-family:monospace;font-size:11px;background:var(--card-inner);border:1px solid var(--card-inner-border);padding:6px 10px;margin-bottom:8px;}"
        ".wifi-slot{background:var(--card-inner);border:1px solid var(--card-inner-border);padding:5px 8px;margin-bottom:4px;}"
        ".wifi-head{display:flex;justify-content:space-between;font-family:monospace;font-size:9.5px;font-weight:700;color:var(--text-muted);margin-bottom:2px;}"
        ".slot-row{display:grid;grid-template-columns:1.2fr 1fr;gap:5px;}"
        "label{display:block;font-family:monospace;font-size:10px;font-weight:700;text-transform:uppercase;color:var(--text-muted);margin:4px 0 2px;}"
        ".input-box{width:100%;padding:5px 8px;border:1px solid var(--input-border);background:var(--input-bg);color:var(--text-main);font-family:monospace;font-size:11px;outline:none;margin-bottom:0px;}"
        ".btn-action{display:inline-flex;align-items:center;gap:8px;padding:8px 20px;border-radius:9999px;border:1.5px solid var(--btn-border);background:var(--btn-bg);color:var(--btn-text);font-family:monospace;font-size:11px;font-weight:700;letter-spacing:0.8px;text-transform:uppercase;cursor:pointer;align-self:flex-start;margin-top:4px;}"
        ".btn-action:hover:not(:disabled){background:var(--btn-hover-bg);color:var(--btn-hover-text);}"
        ".btn-action:disabled{opacity:0.45;cursor:not-allowed;}"
        ".guide{background:var(--card-inner);border:1px solid var(--card-inner-border);padding:14px 16px;display:flex;flex-direction:column;justify-content:space-between;}"
        ".guide-head{font-family:monospace;font-size:10.5px;font-weight:700;color:var(--text-main);text-transform:uppercase;letter-spacing:0.5px;padding-bottom:6px;border-bottom:1px dashed var(--card-inner-border);margin-bottom:8px;}"
        ".steps{display:flex;flex-direction:column;gap:7px;}"
        ".step{display:flex;align-items:flex-start;gap:8px;}"
        ".step-num{width:18px;height:18px;border-radius:50%;background:var(--num-bg);border:1px solid var(--num-border);color:var(--num-text);display:flex;align-items:center;justify-content:center;font-family:monospace;font-size:9px;font-weight:700;flex-shrink:0;margin-top:1px;}"
        ".step-body strong{display:block;font-size:11px;font-weight:700;color:var(--text-main);}"
        ".step-body p{font-size:10px;color:var(--text-muted);line-height:1.3;}"
        ".tele-bar{margin-top:6px;padding-top:6px;border-top:1px dashed var(--card-inner-border);display:flex;justify-content:space-between;font-family:monospace;font-size:9.5px;color:var(--text-muted);}"
        ".tele-bar b{color:var(--text-main);}"
        ".prog{display:none;margin-top:6px;}"
        ".prog-meta{display:flex;justify-content:space-between;font-family:monospace;font-size:10px;color:var(--text-muted);margin-bottom:3px;}"
        ".prog-track{width:100%;height:6px;background:var(--progress-bg);overflow:hidden;}"
        ".prog-fill{height:100%;width:0%;background:var(--progress-fill);transition:width 0.1s;}"
        ".msg{margin-top:6px;padding:6px 10px;font-family:monospace;font-size:10.5px;display:none;background:var(--card-inner);border:1px solid var(--card-inner-border);}"
        ".bot-bar{display:flex;justify-content:space-between;align-items:center;margin-top:6px;font-family:monospace;font-size:10px;color:var(--text-muted);}"
        "@media(max-width:768px){body{padding:8px 10px;}.card{grid-template-columns:1fr;padding:12px;gap:8px;}h1{font-size:20px;}.check-grid{display:none;}.step:nth-child(n+4){display:none;}.btn-action{width:100%;justify-content:center;}}"
        "</style></head><body><div class='frame'>"

        "<div class='top-row'>"
        "<div class='badge-pill'>LIFELINE RX PRO // BASE STATION COMMAND</div>"
        "<button class='theme-btn' id='t_btn'><span id='t_icon'><svg class='theme-icon-svg' width='13' height='13' viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2.2' stroke-linecap='round' stroke-linejoin='round'><circle cx='12' cy='12' r='5'></circle><line x1='12' y1='1' x2='12' y2='3'></line><line x1='12' y1='21' x2='12' y2='23'></line><line x1='4.22' y1='4.22' x2='5.64' y2='5.64'></line><line x1='18.36' y1='18.36' x2='19.78' y2='19.78'></line><line x1='1' y1='12' x2='3' y2='12'></line><line x1='21' y1='12' x2='23' y2='12'></line><line x1='4.22' y1='19.78' x2='5.64' y2='18.36'></line><line x1='18.36' y1='5.64' x2='19.78' y2='4.22'></line></svg></span><span id='t_txt'>LIGHT</span></button>"
        "</div>"

        "<div><h1 id='h_title'>Three services. One base station.</h1><p class='sub'>Select a service below to configure Wi-Fi failover, flash wireless firmware, or set cloud telemetry.</p></div>"

        "<div class='tabs'>"
        "<button class='tab active' id='t_ota' onclick='swTab(\"ota\")'>FIRMWARE OTA</button>"
        "<button class='tab' id='t_wifi' onclick='swTab(\"wifi\")'>TRI-WIFI FAILOVER</button>"
        "<button class='tab' id='t_api' onclick='swTab(\"api\")'>CLOUD REST API</button>"
        "</div>"

        "<div class='card'>"
        "<div class='panel'>"
        "<div><h2 id='p_title'>Wireless OTA Firmware Flash</h2><p id='p_desc'>Wireless firmware flashing with dual-partition roll-back safety &amp; LCD sync.</p>"
        "<div class='check-grid' id='c_grid'>"
        "<div class='check-item'><svg width='12' height='12' viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='3'><polyline points='20 6 9 17 4 12'></polyline></svg> Dual Partition Safe</div>"
        "<div class='check-item'><svg width='12' height='12' viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='3'><polyline points='20 6 9 17 4 12'></polyline></svg> 433MHz LoRa Gateway</div>"
        "<div class='check-item'><svg width='12' height='12' viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='3'><polyline points='20 6 9 17 4 12'></polyline></svg> Tri-WiFi Redundancy</div>"
        "<div class='check-item'><svg width='12' height='12' viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='3'><polyline points='20 6 9 17 4 12'></polyline></svg> 1602 LCD Live Sync</div>"
        "</div></div>"

        "<div id='box_ota'>"
        "<form id='ota_form'>"
        "<div class='upload-box'>"
        "<input type='file' id='fwFile' name='update' accept='.bin' required>"
        "<span>Select or drop firmware.bin</span><small>ESP32-WROOM-32 (*.bin)</small>"
        "</div>"
        "<div class='file-bar' id='rx_meta'><span id='rx_name'>firmware.bin</span><span id='rx_size'>0 KB</span></div>"
        "<button type='submit' class='btn-action' id='btn_ota'>FLASH NOW &rarr;</button>"
        "</form>"
        "<div class='prog' id='p_box'>"
        "<div class='prog-meta'><span id='rx_p_status'>Streaming Blocks...</span><span id='rx_p_pct'>0%</span></div>"
        "<div class='prog-track'><div class='prog-fill' id='p_bar'></div></div>"
        "</div>"
        "<div class='msg' id='flash_msg'></div>"
        "</div>"

        "<div id='box_wifi' style='display:none;'>"
        "<form action='/save' method='POST'>"
    );

    for (int i = 0; i < 3; i++) {
        String numStr = String(i + 1);
        String currentS = (i < networkCount) ? storedNetworks[i].ssid : "";
        String currentP = (i < networkCount) ? storedNetworks[i].password : "";
        html += "<div class='wifi-slot'>";
        html += "<div class='wifi-head'><span>" + String(i + 1) + ". " + (i == 0 ? "PRIMARY" : "BACKUP " + String(i)) + "</span><span>SLOT " + numStr + "</span></div>";
        html += "<div class='slot-row'>";
        html += "<input type='text' class='input-box' name='ssid" + numStr + "' placeholder='SSID' value='" + currentS + "'" + (i == 0 ? " required" : "") + ">";
        html += "<input type='password' class='input-box' name='pass" + numStr + "' placeholder='Password' value='" + currentP + "'>";
        html += "</div></div>";
    }

    html += F(
        "<button type='submit' class='btn-action'>SAVE &amp; RECONNECT &rarr;</button>"
        "</form>"
        "</div>"

        "<div id='box_api' style='display:none;'>"
        "<form action='/api-save' method='POST'>"
        "<label>REST API Endpoint URL</label>"
        "<input type='text' class='input-box' name='api_url' value='"
    );
    html += (customApiEndpoint.length() > 0 ? customApiEndpoint : API_ENDPOINT);
    html += F(
        "' placeholder='https://...'>"
        "<label>API Secret / Key</label>"
        "<input type='password' class='input-box' name='api_key' value='"
    );
    html += customApiKey;
    html += F(
        "' placeholder='Bearer / Token'>"
        "<button type='submit' class='btn-action'>SAVE API CONFIG &rarr;</button>"
        "</form>"
        "</div>"
        "</div>"

        "<div class='guide'>"
        "<div><div class='guide-head'>HOW TO USE THIS SERVICE</div>"
        "<div class='steps' id='s_box'>"
        "<div class='step'><div class='step-num'>1</div><div class='step-body'><strong>1. Select Binary</strong><p>Pick compiled firmware.bin from build output.</p></div></div>"
        "<div class='step'><div class='step-num'>2</div><div class='step-body'><strong>2. Dual-Partition Stream</strong><p>Streams into alternate OTA partition safely.</p></div></div>"
        "<div class='step'><div class='step-num'>3</div><div class='step-body'><strong>3. LCD Progress Sync</strong><p>16x2 I2C display mirrors upload percentage.</p></div></div>"
        "<div class='step'><div class='step-num'>4</div><div class='step-body'><strong>4. Auto-Reboot Cycle</strong><p>ESP32 validates checksum and restarts base station.</p></div></div>"
        "</div></div>"
        "<div class='tele-bar'><span>IP: <b>"
    );
    html += currentIP;
    html += F(
        "</b></span><span>HEAP: <b>"
    );
    html += freeHeapStr;
    html += F(
        "</b></span><span>LINK: <b>"
    );
    html += currentStatus;
    html += F(
        "</b></span></div>"
        "</div>"
        "</div>"

        "<div class='bot-bar'><span>LIFELINE DISASTER COMMAND // PROTOCOL v3.1</span><span>SYS: ESP32 + SX1278</span></div>"
        "</div>"

        "<script>"
        "var curTheme='light';"
        "var sSun=\"<svg class='theme-icon-svg' width='13' height='13' viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2.2' stroke-linecap='round' stroke-linejoin='round'><circle cx='12' cy='12' r='5'></circle><line x1='12' y1='1' x2='12' y2='3'></line><line x1='12' y1='21' x2='12' y2='23'></line><line x1='4.22' y1='4.22' x2='5.64' y2='5.64'></line><line x1='18.36' y1='18.36' x2='19.78' y2='19.78'></line><line x1='1' y1='12' x2='3' y2='12'></line><line x1='21' y1='12' x2='23' y2='12'></line><line x1='4.22' y1='19.78' x2='5.64' y2='18.36'></line><line x1='18.36' y1='5.64' x2='19.78' y2='4.22'></line></svg>\";"
        "var sMoon=\"<svg class='theme-icon-svg' width='13' height='13' viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2.2' stroke-linecap='round' stroke-linejoin='round'><path d='M21 12.79A9 9 0 1 1 11.21 3 7 7 0 0 0 21 12.79z'></path></svg>\";"
        "document.getElementById('t_btn').onclick=function(){"
        "  curTheme=(curTheme==='light'?'dark':'light');"
        "  document.documentElement.setAttribute('data-theme',curTheme);"
        "  document.getElementById('t_icon').innerHTML=(curTheme==='light'?sSun:sMoon);"
        "  document.getElementById('t_txt').innerText=(curTheme==='light'?'LIGHT':'DARK');"
        "};"
        "function swTab(t){"
        "  var tOta=document.getElementById('t_ota'),tWifi=document.getElementById('t_wifi'),tApi=document.getElementById('t_api');"
        "  var bOta=document.getElementById('box_ota'),bWifi=document.getElementById('box_wifi'),bApi=document.getElementById('box_api');"
        "  var pT=document.getElementById('p_title'),pD=document.getElementById('p_desc'),sB=document.getElementById('s_box');"
        "  var cG=document.getElementById('c_grid');"
        "  [tOta,tWifi,tApi].forEach(function(x){x.className='tab';});"
        "  [bOta,bWifi,bApi].forEach(function(x){x.style.display='none';});"
        "  if(cG) cG.style.display=(t==='ota'?'grid':'none');"
        "  if(t==='ota'){"
        "    tOta.className='tab active';bOta.style.display='block';"
        "    pT.innerText='Wireless OTA Firmware Flash';pD.innerText='Wireless firmware flashing with dual-partition roll-back safety & LCD sync.';"
        "    sB.innerHTML='<div class=\"step\"><div class=\"step-num\">1</div><div class=\"step-body\"><strong>1. Select Binary</strong><p>Pick compiled firmware.bin from build folder.</p></div></div><div class=\"step\"><div class=\"step-num\">2</div><div class=\"step-body\"><strong>2. Dual-Partition Stream</strong><p>Streams into alternate OTA partition safely.</p></div></div><div class=\"step\"><div class=\"step-num\">3</div><div class=\"step-body\"><strong>3. LCD Progress Sync</strong><p>16x2 I2C display mirrors upload percentage.</p></div></div><div class=\"step\"><div class=\"step-num\">4</div><div class=\"step-body\"><strong>4. Auto-Reboot Cycle</strong><p>ESP32 validates checksum and restarts base station.</p></div></div>';"
        "  }else if(t==='wifi'){"
        "    tWifi.className='tab active';bWifi.style.display='block';"
        "    pT.innerText='Tri-Network Wi-Fi Failover';pD.innerText='Store up to 3 Wi-Fi network credentials for zero-downtime internet telemetry.';"
        "    sB.innerHTML='<div class=\"step\"><div class=\"step-num\">1</div><div class=\"step-body\"><strong>1. Primary Setup</strong><p>Primary router connection for cloud uplink.</p></div></div><div class=\"step\"><div class=\"step-num\">2</div><div class=\"step-body\"><strong>2. Backup 1 Hotspot</strong><p>Secondary network for field fallback.</p></div></div><div class=\"step\"><div class=\"step-num\">3</div><div class=\"step-body\"><strong>3. Backup 2 Cellular</strong><p>Disaster satellite or cellular modem fallback.</p></div></div><div class=\"step\"><div class=\"step-num\">4</div><div class=\"step-body\"><strong>4. Save to Flash</strong><p>Stored into ESP32 NVS non-volatile flash.</p></div></div>';"
        "  }else{"
        "    tApi.className='tab active';bApi.style.display='block';"
        "    pT.innerText='Cloud REST API Webhook';pD.innerText='Configure centralized server endpoint where incoming LoRa packets are forwarded.';"
        "    sB.innerHTML='<div class=\"step\"><div class=\"step-num\">1</div><div class=\"step-body\"><strong>1. Ingestion Endpoint</strong><p>Specify REST URL for HTTP POST telemetry payloads.</p></div></div><div class=\"step\"><div class=\"step-num\">2</div><div class=\"step-body\"><strong>2. Auth Secret</strong><p>Bearer secret or token for gateway verification.</p></div></div><div class=\"step\"><div class=\"step-num\">3</div><div class=\"step-body\"><strong>3. Dispatch Relay</strong><p>Emergency beacon GPS fixes push to dispatch map.</p></div></div><div class=\"step\"><div class=\"step-num\">4</div><div class=\"step-body\"><strong>4. Save Settings</strong><p>Reboots network service with new endpoint.</p></div></div>';"
        "  }"
        "}"
        "var fi=document.getElementById('fwFile');"
        "fi.onchange=function(){"
        "  if(this.files&&this.files[0]){"
        "    document.getElementById('rx_name').innerText=this.files[0].name;"
        "    document.getElementById('rx_size').innerText=Math.round(this.files[0].size/1024)+' KB';"
        "    document.getElementById('rx_meta').style.display='flex';"
        "  }"
        "};"
        "var f=document.getElementById('ota_form');"
        "if(f){f.onsubmit=function(e){"
        "  e.preventDefault();"
        "  if(!fi||!fi.files.length)return false;"
        "  var file=fi.files[0];"
        "  var pb=document.getElementById('p_box'),pr=document.getElementById('p_bar');"
        "  var pctLbl=document.getElementById('rx_p_pct'),statLbl=document.getElementById('rx_p_status');"
        "  var btn=document.getElementById('btn_ota'),msg=document.getElementById('flash_msg');"
        "  pb.style.display='block';pr.style.width='0%';pctLbl.innerText='0%';"
        "  btn.disabled=true;msg.style.display='none';"
        "  statLbl.innerText='Writing Flash Partition...';"
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
        "      pctLbl.innerText='100%';"
        "      msg.style.display='block';msg.style.color='#10b981';"
        "      msg.innerHTML='<strong>UPDATE COMPLETE:</strong> Base Station is rebooting now...';"
        "      setTimeout(function(){location.reload();},6000);"
        "    }else{"
        "      msg.style.display='block';msg.style.color='#ef4444';"
        "      msg.innerHTML='<strong>UPDATE FAILED ('+xhr.status+')</strong>';"
        "      btn.disabled=false;"
        "    }"
        "  };"
        "  xhr.onerror=function(){"
        "    pr.style.width='100%';"
        "    msg.style.display='block';msg.style.color='#10b981';"
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
