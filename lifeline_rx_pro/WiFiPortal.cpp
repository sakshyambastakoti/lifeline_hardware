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

// HTML and Endpoint Handlers (Dark Theme, Sharp Edges, Crimson Red & Cyan Aesthetic)
static String getPortalHTML() {
    String currentIP = (wifiConnected && WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
    String currentStatus = (wifiConnected && WiFi.status() == WL_CONNECTED) ? ("ONLINE &bull; " + activeSSID + " (" + String(WiFi.RSSI()) + " dBm)") : "STANDALONE AP SETUP";
    String freeHeapStr = String(ESP.getFreeHeap() / 1024) + " KB";

    String html = F("<!DOCTYPE html><html><head>"
        "<meta charset='UTF-8'>"
        "<meta name='viewport' content='width=device-width,initial-scale=1'>"
        "<title>LIFELINE RX // BASE COMMAND DASHBOARD</title>"
        "<style>"
        "* { box-sizing: border-box; border-radius: 0px !important; margin: 0; padding: 0; }"
        "body { font-family: 'Segoe UI', Roboto, Helvetica, Arial, sans-serif; background: #08080c; color: #f4f4f7; padding: 20px 14px; }"
        ".container { max-width: 480px; margin: 0 auto; background: #121218; padding: 24px 20px; border: 1px solid #ff1e42; box-shadow: 0 0 25px rgba(255, 30, 66, 0.18); }"
        ".header { border-bottom: 2px solid #ff1e42; padding-bottom: 12px; margin-bottom: 18px; }"
        "h1 { color: #ffffff; font-size: 21px; font-weight: 800; letter-spacing: 1px; }"
        ".brand-sub { color: #ff1e42; font-size: 11px; font-weight: 700; letter-spacing: 1.5px; text-transform: uppercase; margin-top: 3px; }"
        ".status-badge { background: #1a1a24; border-left: 3px solid #00ff87; padding: 10px 12px; margin-bottom: 20px; font-size: 11.5px; color: #d0d0dc; line-height: 1.5; font-family: monospace; }"
        ".card { background: #171722; border: 1px solid #28283a; padding: 16px 14px; margin-bottom: 20px; }"
        "h2 { color: #ffffff; font-size: 13px; font-weight: 700; margin-bottom: 12px; text-transform: uppercase; letter-spacing: 1px; border-left: 3px solid #ff1e42; padding-left: 8px; }"
        "h2.ota { border-left-color: #00d4ff; }"
        "h2.api { border-left-color: #00ff87; }"
        "label { display: block; font-size: 11px; color: #9c9cb0; text-transform: uppercase; font-weight: 700; margin: 8px 0 4px 0; letter-spacing: 0.5px; }"
        "input[type=text], input[type=password], input[type=file] { width: 100%; padding: 11px; margin-bottom: 10px; border: 1px solid #2e2e42; background: #0a0a0f; color: #ffffff; font-size: 13px; font-family: monospace; outline: none; transition: border-color 0.2s; }"
        "input[type=text]:focus, input[type=password]:focus { border-color: #ff1e42; }"
        "input[type=file] { padding: 8px; }"
        "input[type=submit] { width: 100%; padding: 13px; background: #ff1e42; color: #ffffff; border: 1px solid #ff1e42; cursor: pointer; font-weight: 800; font-size: 13px; text-transform: uppercase; letter-spacing: 1px; margin-top: 6px; transition: background 0.2s, box-shadow 0.2s; }"
        "input[type=submit]:hover { background: #e01235; box-shadow: 0 0 15px rgba(255, 30, 66, 0.5); }"
        "input.btn-ota { background: #00a8cc; border-color: #00d4ff; }"
        "input.btn-ota:hover { background: #00c4ec; box-shadow: 0 0 15px rgba(0, 212, 255, 0.5); }"
        "input.btn-api { background: #008744; border-color: #00ff87; }"
        "input.btn-api:hover { background: #00a855; box-shadow: 0 0 15px rgba(0, 255, 135, 0.5); }"
        ".desc { font-size: 11.5px; color: #a4a4b8; line-height: 1.4; margin-bottom: 12px; }"
        "</style></head><body><div class='container'>");

    html += "<div class='header'>";
    html += "<h1>LIFELINE RX PRO</h1>";
    html += "<div class='brand-sub'>Base Station Gateway // Web Command Dashboard</div>";
    html += "</div>";

    html += "<div class='status-badge'>";
    html += "IP: " + currentIP + "<br>";
    html += "LINK: " + currentStatus + "<br>";
    html += "FREE HEAP: " + freeHeapStr + " | FW: v3.1.0 PRO";
    html += "</div>";

    // 1. API Configuration Section
    html += "<div class='card'>";
    html += "<h2 class='api'>1. Cloud REST API Configuration</h2>";
    html += "<div class='desc'>Configure cloud endpoint & API key for forwarding emergency alerts & LoRa telemetry to your dashboard or server.</div>";
    html += "<form action='/api-save' method='POST'>";
    html += "<label>REST API Endpoint URL:</label>";
    html += "<input type='text' name='api_url' value='" + (customApiEndpoint.length() > 0 ? customApiEndpoint : API_ENDPOINT) + "' placeholder='https://...'>";
    html += "<label>API Key (X-API-Key / Bearer Authentication):</label>";
    html += "<input type='text' name='api_key' value='" + customApiKey + "' placeholder='Enter API Key (or leave blank if none)'>";
    html += "<input class='btn-api' type='submit' value='SAVE API CONFIGURATION'>";
    html += "</form>";
    html += "</div>";

    // 2. Wireless OTA Firmware Upgrade Section
    html += "<div class='card'>";
    html += "<h2 class='ota'>2. Wireless OTA Firmware Upgrade</h2>";
    html += "<div class='desc'>Upload a freshly compiled <code>firmware.bin</code> over Wi-Fi. The 16x2 LCD shows live progress and restarts the base station on completion.</div>";
    html += "<form action='/update' method='POST' enctype='multipart/form-data'>";
    html += "<label>Select Firmware Binary (.bin):</label>";
    html += "<input type='file' name='update' accept='.bin' required>";
    html += "<input class='btn-ota' type='submit' value='FLASH FIRMWARE (OTA)'>";
    html += "</form>";
    html += "</div>";

    // 3. Wi-Fi Multi-Network Setup Section
    html += "<div class='card'>";
    html += "<h2>3. Wi-Fi Multi-Network Setup</h2>";
    html += "<div class='desc'>Configure up to 3 local Wi-Fi networks for failover internet connectivity.</div>";
    html += "<form action='/save' method='POST'>";
    for (int i = 0; i < 3; i++) {
        String numStr = String(i + 1);
        String currentS = (i < networkCount) ? storedNetworks[i].ssid : "";
        String currentP = (i < networkCount) ? storedNetworks[i].password : "";
        html += "<label>WiFi #" + numStr + " SSID" + (i == 0 ? " (Primary)" : " (Backup)") + ":</label>";
        html += "<input type='text' name='ssid" + numStr + "' placeholder='Network SSID' value='" + currentS + "'" + (i == 0 ? " required" : "") + ">";
        html += "<label>WiFi #" + numStr + " Password:</label>";
        html += "<input type='password' name='pass" + numStr + "' placeholder='Password' value='" + currentP + "'>";
    }
    html += "<input type='submit' value='SAVE & RECONNECT WI-FI'>";
    html += "</form>";
    html += "</div>";

    html += "</div></body></html>";
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
    
    String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'><title>LIFELINE RX - API SAVED</title>";
    html += "<style>* { box-sizing: border-box; border-radius: 0px !important; } body{font-family:'Segoe UI',sans-serif;background:#08080c;color:#fff;text-align:center;padding:50px 15px;}";
    html += ".card{background:#121218;border:1px solid #00ff87;padding:30px 20px;max-width:440px;margin:0 auto;box-shadow:0 0 25px rgba(0,255,135,0.2);}";
    html += "h2{color:#00ff87;font-size:20px;margin-bottom:10px;text-transform:uppercase;letter-spacing:1px;} p{color:#b3b3c2;font-size:13px;line-height:1.6;}";
    html += "code{background:#0a0a0f;padding:4px 8px;border:1px solid #282836;color:#00d4ff;display:block;margin:10px 0;word-break:break-all;}";
    html += "a{display:inline-block;margin-top:20px;padding:12px 24px;background:#ff1e42;color:#fff;text-decoration:none;font-weight:bold;letter-spacing:1px;}</style></head><body>";
    html += "<div class='card'><h2>API SETTINGS SAVED</h2>";
    html += "<p>API Key:</p><code>" + (newKey.length() > 0 ? newKey : "(None / Cleared)") + "</code>";
    html += "<p>Endpoint URL:</p><code>" + newUrl + "</code>";
    html += "<a href='/'>RETURN TO DASHBOARD</a></div></body></html>";
    
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
    
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<title>LIFELINE RX - SAVED</title>";
    html += "<style>* { box-sizing: border-box; border-radius: 0px !important; } body{font-family:'Segoe UI',sans-serif;background:#08080c;color:#fff;text-align:center;padding:50px 15px;}";
    html += ".card{background:#121218;border:1px solid #ff1e42;padding:30px 20px;max-width:420px;margin:0 auto;box-shadow:0 0 25px rgba(255,30,66,0.2);}";
    html += "h2{color:#ff1e42;font-size:20px;margin-bottom:10px;text-transform:uppercase;letter-spacing:1px;} p{color:#b3b3c2;font-size:13px;}</style></head><body>";
    html += "<div class='card'><h2>CONFIG SAVED</h2>";
    html += "<p>Successfully saved " + String(count) + " network(s).</p>";
    html += "<p>Device is restarting and connecting...</p></div>";
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
    
    // Direct OTA Firmware Flash endpoint
    wifiServer.on("/update", HTTP_POST, []() {
        wifiServer.sendHeader("Connection", "close");
        String res = (Update.hasError()) ? 
            "<!DOCTYPE html><html><body style='background:#08080c;color:#ff1e42;font-family:sans-serif;text-align:center;padding:50px;'><h2>OTA UPDATE FAILED</h2><p style='color:#bbb;'>An error occurred during flashing.</p><br><a href='/' style='color:#fff;background:#ff1e42;padding:10px 20px;text-decoration:none;'>RETURN</a></body></html>" : 
            "<!DOCTYPE html><html><body style='background:#08080c;color:#00ff87;font-family:sans-serif;text-align:center;padding:50px;'><h2>OTA UPDATE SUCCESSFUL!</h2><p style='color:#b3b3c2;'>Base station is rebooting with new firmware...</p></body></html>";
        wifiServer.send(200, "text/html", res);
        delay(1000);
        ESP.restart();
    }, []() {
        HTTPUpload& upload = wifiServer.upload();
        if (upload.status == UPLOAD_FILE_START) {
            Serial.printf("[WEB OTA] Start: %s\n", upload.filename.c_str());
            currentScreen = SCREEN_OTA;
            drawOTAProgressScreen(0);
            if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                Update.printError(Serial);
            }
            if (upload.totalSize > 0) {
                int pct = (upload.currentSize * 100) / upload.totalSize;
                drawOTAProgressScreen(pct);
            }
        } else if (upload.status == UPLOAD_FILE_END) {
            if (Update.end(true)) {
                Serial.printf("[WEB OTA] Success: %u bytes\n", upload.totalSize);
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
