#include "wifi_portal_spu.h"

WiFiPortalSPU wifiPortalSPU;

WiFiPortalSPU::WiFiPortalSPU()
    : _server(80),
      _wifiConnected(false),
      _portalActive(false),
      _portalStartTime(0),
      _buttonPressStartTime(0),
      _buttonPressed(false),
      _lastBeepSecond(-1),
      _networkCount(0),
      _activeSSID("") {}

void WiFiPortalSPU::begin() {
    pinMode(WIFI_PORTAL_PIN, INPUT_PULLUP);
    loadWiFiCredentials();

    if (_networkCount > 0) {
        connectToWiFiSilent();
    } else {
        Serial.println(F("[WIFI SPU] No stored Wi-Fi networks found."));
    }
}

void WiFiPortalSPU::loadWiFiCredentials() {
    _preferences.begin("lifeline", true);
    _networkCount = 0;

    for (int i = 0; i < MAX_WIFI_NETWORKS; i++) {
        String keySSID = "ssid" + String(i + 1);
        String keyPass = "pass" + String(i + 1);
        String s = _preferences.getString(keySSID.c_str(), "");
        String p = _preferences.getString(keyPass.c_str(), "");

        if (s.length() > 0) {
            _storedNetworks[_networkCount].ssid = s;
            _storedNetworks[_networkCount].password = p;
            _networkCount++;
        }
    }
    _preferences.end();

    if (_networkCount > 0) {
        _activeSSID = _storedNetworks[0].ssid;
        Serial.printf("[WIFI SPU] Loaded %d Wi-Fi network(s). Primary: %s\n", _networkCount, _activeSSID.c_str());
    }
}

void WiFiPortalSPU::saveWiFiCredentialsList(const WiFiNetworkSPU nets[], int count) {
    _preferences.begin("lifeline", false);
    for (int i = 0; i < MAX_WIFI_NETWORKS; i++) {
        String keySSID = "ssid" + String(i + 1);
        String keyPass = "pass" + String(i + 1);
        if (i < count && nets[i].ssid.length() > 0) {
            _preferences.putString(keySSID.c_str(), nets[i].ssid);
            _preferences.putString(keyPass.c_str(), nets[i].password);
        } else {
            _preferences.remove(keySSID.c_str());
            _preferences.remove(keyPass.c_str());
        }
    }
    _preferences.end();

    loadWiFiCredentials();
}

bool WiFiPortalSPU::connectToWiFiSilent() {
    if (_networkCount == 0) return false;

    // Coexistence mode for ESP-NOW and Wi-Fi STA
    WiFi.mode(WIFI_AP_STA);

    for (int i = 0; i < _networkCount; i++) {
        Serial.printf("[WIFI SPU] Silent connect attempt %d/%d to %s...\n",
                      i + 1, _networkCount, _storedNetworks[i].ssid.c_str());
        WiFi.begin(_storedNetworks[i].ssid.c_str(), _storedNetworks[i].password.c_str());

        unsigned long startTime = millis();
        while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < WIFI_CONNECT_TIMEOUT) {
            delay(500);
            Serial.print(".");
        }
        Serial.println();

        if (WiFi.status() == WL_CONNECTED) {
            _wifiConnected = true;
            _activeSSID = _storedNetworks[i].ssid;
            Serial.printf("[WIFI SPU] Connected to %s! IP: %s\n", _activeSSID.c_str(), WiFi.localIP().toString().c_str());
            return true;
        }
    }

    _wifiConnected = false;
    Serial.println(F("[WIFI SPU] All network connection attempts failed"));
    return false;
}

bool WiFiPortalSPU::connectToWiFi() {
    return connectToWiFiSilent();
}

void WiFiPortalSPU::startWiFiPortal() {
    Serial.println(F("[WIFI SPU] Starting Captive Wi-Fi configuration portal..."));

    WiFi.disconnect(true);
    delay(100);

    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(WIFI_AP_SSID);

    IPAddress apIP = WiFi.softAPIP();
    Serial.printf("[WIFI SPU] Open AP started! SSID: '%s'\n", WIFI_AP_SSID);
    Serial.printf("[WIFI SPU] Portal IP: %s\n", apIP.toString().c_str());

    _dnsServer.start(53, "*", apIP);

    _server.on("/", std::bind(&WiFiPortalSPU::handlePortalRoot, this));
    _server.on("/save", HTTP_POST, std::bind(&WiFiPortalSPU::handlePortalSave, this));

    // Captive portal detection endpoints
    _server.on("/generate_204", std::bind(&WiFiPortalSPU::handlePortalRoot, this));
    _server.on("/redirect", std::bind(&WiFiPortalSPU::handlePortalRoot, this));
    _server.on("/hotspot-detect.html", std::bind(&WiFiPortalSPU::handlePortalRoot, this));
    _server.on("/canonical.html", std::bind(&WiFiPortalSPU::handlePortalRoot, this));
    _server.on("/nconnect.txt", std::bind(&WiFiPortalSPU::handlePortalRoot, this));
    _server.onNotFound([this]() {
        _server.sendHeader("Location", "http://192.168.4.1/", true);
        _server.send(302, "text/plain", "");
    });

    _server.begin();

    _portalActive = true;
    _portalStartTime = millis();
}

void WiFiPortalSPU::stopWiFiPortal() {
    if (!_portalActive) return;

    Serial.println(F("[WIFI SPU] Stopping portal..."));
    _dnsServer.stop();
    _server.stop();
    WiFi.softAPdisconnect(true);
    _portalActive = false;

    if (_networkCount > 0) {
        connectToWiFiSilent();
    }
}

void WiFiPortalSPU::checkWiFiPortalButton() {
    bool currentButtonState = digitalRead(WIFI_PORTAL_PIN);

    if (currentButtonState == LOW) { // Button on GPIO 14 active LOW
        if (!_buttonPressed) {
            _buttonPressed = true;
            _buttonPressStartTime = millis();
            _lastBeepSecond = -1;
        } else {
            unsigned long pressDuration = millis() - _buttonPressStartTime;
            if (pressDuration >= 3000) { // 3 seconds hold
                _buttonPressed = false;
                if (_portalActive) {
                    stopWiFiPortal();
                } else {
                    startWiFiPortal();
                }
            }
        }
    } else {
        _buttonPressed = false;
    }
}

void WiFiPortalSPU::update() {
    checkWiFiPortalButton();

    if (_portalActive) {
        _dnsServer.processNextRequest();
        _server.handleClient();

        if (millis() - _portalStartTime >= WIFI_PORTAL_TIMEOUT) {
            Serial.println(F("[WIFI SPU] Portal timeout, closing..."));
            stopWiFiPortal();
        }
    }
}

String WiFiPortalSPU::getPortalHTML() {
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<title>LIFELINE SPU - CAPTIVE PORTAL</title>";
    html += "<style>";
    html += "* { box-sizing: border-box; border-radius: 0px !important; }";
    html += "body { font-family: 'Segoe UI', Roboto, Helvetica, Arial, sans-serif; background: #08080c; color: #f4f4f7; margin: 0; padding: 20px 15px; }";
    html += ".container { max-width: 440px; margin: 0 auto; background: #121218; padding: 25px 22px; border: 1px solid #ff1e42; box-shadow: 0 0 25px rgba(255, 30, 66, 0.15); }";
    html += ".header { border-bottom: 2px solid #ff1e42; padding-bottom: 12px; margin-bottom: 20px; text-align: left; }";
    html += "h1 { color: #ffffff; font-size: 22px; margin: 0 0 5px 0; font-weight: 800; letter-spacing: 1px; }";
    html += ".brand-sub { color: #ff1e42; font-size: 11px; font-weight: 700; letter-spacing: 1.5px; text-transform: uppercase; }";
    html += ".info-box { background: #1a1a24; border-left: 3px solid #ff1e42; padding: 10px 12px; margin-bottom: 22px; font-size: 12px; color: #b3b3c2; line-height: 1.4; }";
    html += "h2 { color: #ffffff; font-size: 13px; font-weight: 700; margin: 18px 0 8px 0; text-transform: uppercase; letter-spacing: 1px; border-left: 2px solid #ff1e42; padding-left: 8px; }";
    html += "label { display: block; font-size: 11px; color: #8c8c9e; text-transform: uppercase; font-weight: 700; margin-top: 8px; margin-bottom: 4px; letter-spacing: 0.5px; }";
    html += "input[type=text], input[type=password] { width: 100%; padding: 12px; margin-bottom: 12px; border: 1px solid #282836; background: #0a0a0f; color: #ffffff; font-size: 14px; font-family: monospace; outline: none; transition: border-color 0.2s, box-shadow 0.2s; }";
    html += "input[type=text]:focus, input[type=password]:focus { border-color: #ff1e42; box-shadow: 0 0 10px rgba(255, 30, 66, 0.4); }";
    html += "input[type=submit] { width: 100%; padding: 14px; background: #ff1e42; color: #ffffff; border: 1px solid #ff1e42; cursor: pointer; font-weight: 800; font-size: 14px; text-transform: uppercase; letter-spacing: 1px; margin-top: 15px; transition: background 0.2s, box-shadow 0.2s; box-shadow: 0 0 12px rgba(255, 30, 66, 0.3); }";
    html += "input[type=submit]:hover { background: #e01235; box-shadow: 0 0 20px rgba(255, 30, 66, 0.6); }";
    html += ".status { text-align: center; margin-top: 20px; padding: 8px; font-size: 11px; background: #0a0a0f; border: 1px solid #282836; color: #727285; letter-spacing: 0.5px; }";
    html += "</style></head><body>";
    html += "<div class='container'>";
    html += "<div class='header'>";
    html += "<h1>LIFELINE SPU</h1>";
    html += "<div class='brand-sub'>Sensor Node - Captive WiFi Setup</div>";
    html += "</div>";
    html += "<div class='info-box'>Configure 1, 2, or 3 WiFi networks for SPU remote API transmission. Network 1 is primary.</div>";
    html += "<form action='/save' method='POST'>";

    for (int i = 0; i < 3; i++) {
        String numStr = String(i + 1);
        String currentS = (i < _networkCount) ? _storedNetworks[i].ssid : "";
        String currentP = (i < _networkCount) ? _storedNetworks[i].password : "";

        html += "<h2>WiFi Network #" + numStr + (i == 0 ? " (Primary Required)" : " (Optional Backup)") + "</h2>";
        html += "<label>SSID (Network Name):</label>";
        html += "<input type='text' name='ssid" + numStr + "' placeholder='Network SSID' value='" + currentS + "'" + (i == 0 ? " required" : "") + ">";
        html += "<label>WPA2 Password:</label>";
        html += "<input type='password' name='pass" + numStr + "' placeholder='WiFi Password' value='" + currentP + "'>";
    }

    html += "<input type='submit' value='SAVE & CONNECT WI-FI'>";
    html += "</form>";
    html += "<div class='status'>STORED NETWORKS: " + String(_networkCount) + " / 3</div>";
    html += "</div></body></html>";
    return html;
}

void WiFiPortalSPU::handlePortalRoot() {
    _server.send(200, "text/html", getPortalHTML());
}

void WiFiPortalSPU::handlePortalSave() {
    WiFiNetworkSPU newNets[3];
    int count = 0;

    for (int i = 0; i < 3; i++) {
        String s = _server.arg("ssid" + String(i + 1));
        String p = _server.arg("pass" + String(i + 1));
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
    html += "<title>LIFELINE SPU - SAVED</title>";
    html += "<style>* { box-sizing: border-box; border-radius: 0px !important; } body{font-family:'Segoe UI',sans-serif;background:#08080c;color:#fff;text-align:center;padding:50px 15px;}";
    html += ".card{background:#121218;border:1px solid #ff1e42;padding:30px 20px;max-width:420px;margin:0 auto;box-shadow:0 0 25px rgba(255,30,66,0.2);}";
    html += "h2{color:#ff1e42;font-size:20px;margin-bottom:10px;text-transform:uppercase;letter-spacing:1px;} p{color:#b3b3c2;font-size:13px;}</style></head><body>";
    html += "<div class='card'><h2>CONFIG SAVED</h2>";
    html += "<p>Successfully saved " + String(count) + " network(s).</p>";
    html += "<p>SPU is restarting and connecting...</p></div>";
    html += "</body></html>";

    _server.send(200, "text/html", html);

    delay(2000);
    ESP.restart();
}
