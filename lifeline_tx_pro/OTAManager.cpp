#include "OTAManager.h"
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <ArduinoOTA.h>
#include <Preferences.h>
#include "Config.h"
#include "DisplayUI.h"

static WebServer server(80);
static Preferences prefs;
static bool otaActive = false;
static OTAMode currentOTAMode = OTA_MODE_NONE;
static int otaProgress = 0;
static int lastReportedProgress = -1;
static String otaStatusText = "Listening for Upload...";
static String activeIPAddress = "192.168.4.1";
static String activeSSIDName = "LifeLine-TX-OTA";

const char* ap_ssid = "LifeLine-TX-OTA";
const char* ap_pass = "12345678";

void loadWiFiCredentials(String& ssid, String& pass) {
    prefs.begin("lifeline_tx", true);
    ssid = prefs.getString("wifi_ssid", "sakshyam");
    pass = prefs.getString("wifi_pass", "sakshyam");
    prefs.end();
}

void saveWiFiCredentials(const String& ssid, const String& pass) {
    prefs.begin("lifeline_tx", false);
    prefs.putString("wifi_ssid", ssid);
    prefs.putString("wifi_pass", pass);
    prefs.end();
    Serial.printf("[OTA] Saved Wi-Fi Credentials -> SSID: %s\n", ssid.c_str());
}

// Embedded Web Portal HTML - Exact LifeLine RX Unit Crimson Palette & Sharp Edge Theme
static String getPortalHTML() {
    String savedSSID, savedPass;
    loadWiFiCredentials(savedSSID, savedPass);

    String html = F(
        "<!DOCTYPE html><html><head><meta charset='UTF-8'>"
        "<meta name='viewport' content='width=device-width,initial-scale=1'>"
        "<title>LIFELINE TX PRO // OTA PORTAL</title>"
        "<style>"
        "* { box-sizing: border-box; border-radius: 0px !important; margin: 0; padding: 0; }"
        "body { font-family: 'Segoe UI', Roboto, Helvetica, Arial, sans-serif; background: #08080c; color: #f4f4f7; margin: 0; padding: 24px 15px; }"
        ".container { max-width: 440px; margin: 0 auto; background: #121218; padding: 25px 22px; border: 1px solid #ff1e42; box-shadow: 0 0 25px rgba(255, 30, 66, 0.18); }"
        ".header { border-bottom: 2px solid #ff1e42; padding-bottom: 14px; margin-bottom: 20px; text-align: left; }"
        ".header h1 { color: #ffffff; font-size: 22px; font-weight: 800; letter-spacing: 1.5px; margin-bottom: 4px; }"
        ".header .sub { color: #ff1e42; font-size: 11px; font-weight: 700; letter-spacing: 1.5px; text-transform: uppercase; }"
        ".info-box { background: #1a1a24; border-left: 3px solid #ff1e42; padding: 10px 12px; margin-bottom: 20px; font-size: 12px; color: #b3b3c2; line-height: 1.4; }"
        "h2 { color: #ffffff; font-size: 13px; font-weight: 700; margin: 20px 0 10px 0; text-transform: uppercase; letter-spacing: 1px; border-left: 2px solid #ff1e42; padding-left: 8px; }"
        "label { display: block; font-size: 11px; color: #8c8c9e; text-transform: uppercase; font-weight: 700; margin-top: 10px; margin-bottom: 5px; letter-spacing: 0.5px; }"
        "input[type=text], input[type=password], input[type=file] { width: 100%; padding: 12px; margin-bottom: 12px; border: 1px solid #282836; background: #0a0a0f; color: #ffffff; font-size: 13px; font-family: monospace; outline: none; transition: border-color 0.2s, box-shadow 0.2s; }"
        "input[type=text]:focus, input[type=password]:focus { border-color: #ff1e42; box-shadow: 0 0 10px rgba(255, 30, 66, 0.4); }"
        "button, input[type=submit] { width: 100%; padding: 14px; background: #ff1e42; color: #ffffff; border: 1px solid #ff1e42; cursor: pointer; font-weight: 800; font-size: 13px; text-transform: uppercase; letter-spacing: 1px; margin-top: 10px; transition: background 0.2s, box-shadow 0.2s; box-shadow: 0 0 12px rgba(255, 30, 66, 0.3); }"
        "button:hover, input[type=submit]:hover { background: #e01235; box-shadow: 0 0 20px rgba(255, 30, 66, 0.6); }"
        ".progress-box { height: 20px; background: #0a0a0f; border: 1px solid #282836; margin-top: 14px; position: relative; overflow: hidden; display: none; }"
        ".progress-bar { height: 100%; width: 0%; background: #ff1e42; transition: width 0.1s; text-align: right; font-size: 11px; line-height: 20px; color: #ffffff; font-weight: 800; padding-right: 6px; font-family: monospace; }"
        ".msg { margin-top: 12px; padding: 10px; font-size: 12px; background: #0a0a0f; border: 1px solid #282836; color: #b3b3c2; font-family: monospace; display: none; }"
        ".status-badge { text-align: center; margin-top: 22px; padding: 8px; font-size: 11px; background: #0a0a0f; border: 1px solid #282836; color: #727285; letter-spacing: 0.5px; text-transform: uppercase; font-family: monospace; }"
        "</style></head><body>"
        "<div class='container'>"
        "<div class='header'>"
        "<h1>LIFELINE TX PRO</h1>"
        "<div class='sub'>Tactical Field Unit // OTA Firmware Gateway</div>"
        "</div>"
        
        "<div class='info-box'>"
        "Active Mode: <b>"
    );
    html += (currentOTAMode == OTA_MODE_NET ? "NETWORK WI-FI" : "LOCAL ACCESS POINT");
    html += F("</b><br>Device IP: <b>");
    html += activeIPAddress;
    html += F(
        "</b></div>"

        "<h2>1. FIRMWARE FLASH (OTA)</h2>"
        "<form id='flash_form'>"
        "<label>Select Firmware Binary (.bin):</label>"
        "<input type='file' id='firmware_file' name='update' accept='.bin' required>"
        "<button type='submit' id='btn_flash'>UPLOAD & FLASH FIRMWARE</button>"
        "</form>"
        "<div class='progress-box' id='p_box'><div class='progress-bar' id='p_bar'>0%</div></div>"
        "<div class='msg' id='flash_msg'></div>"

        "<h2>2. WI-FI CREDENTIALS CONFIG</h2>"
        "<form id='wifi_form'>"
        "<label>Target Wi-Fi SSID:</label>"
        "<input type='text' id='wifi_ssid' name='ssid' value='"
    );
    html += savedSSID;
    html += F(
        "' placeholder='e.g. sakshyam' required>"
        "<label>WPA2 Password:</label>"
        "<input type='password' id='wifi_pass' name='pass' value='"
    );
    html += savedPass;
    html += F(
        "' placeholder='WiFi Password'>"
        "<button type='submit' id='btn_save'>SAVE WI-FI CREDENTIALS</button>"
        "</form>"
        "<div class='msg' id='wifi_msg'></div>"

        "<div class='status-badge'>LIFELINE TX // MIL-SPEC EMERGENCY TELEMETRY</div>"
        "</div>"

        "<script>"
        "document.getElementById('flash_form').onsubmit = function(e) {"
        "  e.preventDefault();"
        "  var file = document.getElementById('firmware_file').files[0];"
        "  if (!file) return;"
        "  var pBox = document.getElementById('p_box');"
        "  var pBar = document.getElementById('p_bar');"
        "  var msg = document.getElementById('flash_msg');"
        "  var btn = document.getElementById('btn_flash');"
        "  pBox.style.display = 'block';"
        "  msg.style.display = 'block';"
        "  msg.style.color = '#ff1e42';"
        "  msg.innerText = 'Starting upload...';"
        "  btn.disabled = true; btn.style.opacity = '0.5';"
        "  var xhr = new XMLHttpRequest();"
        "  xhr.upload.onprogress = function(evt) {"
        "    if (evt.lengthComputable) {"
        "      var pct = Math.round((evt.loaded / evt.total) * 100);"
        "      pBar.style.width = pct + '%';"
        "      pBar.innerText = pct + '%';"
        "      msg.innerText = 'Flashing Firmware: ' + pct + '%';"
        "    }"
        "  };"
        "  xhr.onload = function() {"
        "    if (xhr.status == 200) {"
        "      pBar.style.width = '100%';"
        "      pBar.innerText = '100%';"
        "      pBar.style.background = '#00ff87';"
        "      msg.style.color = '#00ff87';"
        "      msg.innerText = 'SUCCESS! Firmware flashed. TX Unit is rebooting...';"
        "    } else {"
        "      msg.style.color = '#ff1e42';"
        "      msg.innerText = 'ERROR: Flash failed (' + xhr.status + ') - ' + xhr.responseText;"
        "      btn.disabled = false; btn.style.opacity = '1';"
        "    }"
        "  };"
        "  xhr.onerror = function() {"
        "    pBar.style.background = '#00ff87';"
        "    msg.style.color = '#00ff87';"
        "    msg.innerText = 'Upload complete! Device is rebooting...';"
        "  };"
        "  var data = new FormData();"
        "  data.append('update', file);"
        "  xhr.open('POST', '/update?size=' + file.size);"
        "  xhr.send(data);"
        "};"

        "document.getElementById('wifi_form').onsubmit = function(e) {"
        "  e.preventDefault();"
        "  var ssid = document.getElementById('wifi_ssid').value;"
        "  var pass = document.getElementById('wifi_pass').value;"
        "  var msg = document.getElementById('wifi_msg');"
        "  msg.style.display = 'block';"
        "  msg.style.color = '#ff1e42';"
        "  msg.innerText = 'Saving credentials...';"
        "  var xhr = new XMLHttpRequest();"
        "  xhr.open('POST', '/save_wifi');"
        "  xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');"
        "  xhr.onload = function() {"
        "    if (xhr.status == 200) {"
        "      msg.style.color = '#00ff87';"
        "      msg.innerText = '[OK] Wi-Fi credentials saved! NET mode will use these settings.';"
        "    } else {"
        "      msg.style.color = '#ff1e42';"
        "      msg.innerText = 'Failed to save Wi-Fi settings.';"
        "    }"
        "  };"
        "  xhr.send('ssid=' + encodeURIComponent(ssid) + '&pass=' + encodeURIComponent(pass));"
        "};"
        "</script></body></html>"
    );

    return html;
}

void startOTAMode(OTAMode mode) {
    if (otaActive) return;
    otaActive = true;
    currentOTAMode = mode;
    otaProgress = 0;
    lastReportedProgress = -1;

    String targetSSID, targetPass;
    loadWiFiCredentials(targetSSID, targetPass);

    if (mode == OTA_MODE_NET) {
        Serial.printf("[OTA] Attempting connection to Wi-Fi SSID '%s'...\n", targetSSID.c_str());
        otaStatusText = "Connecting to " + targetSSID + "...";
        
        // Temporarily display connecting screen
        previousScreen = currentScreen;
        currentScreen = SCREEN_OTA;
        drawOTAScreen();

        WiFi.mode(WIFI_STA);
        WiFi.begin(targetSSID.c_str(), targetPass.c_str());

        unsigned long startConnect = millis();
        bool connected = false;
        while (millis() - startConnect < 6500) {
            if (WiFi.status() == WL_CONNECTED) {
                connected = true;
                break;
            }
            delay(100);
        }

        if (connected) {
            activeIPAddress = WiFi.localIP().toString();
            activeSSIDName = targetSSID;
            otaStatusText = "Listening on " + targetSSID;
            Serial.printf("[OTA] Connected! Station IP: %s\n", activeIPAddress.c_str());
        } else {
            Serial.println(F("[OTA] Network connection timed out. Falling back to Local AP..."));
            WiFi.disconnect();
            mode = OTA_MODE_LOCAL;
            currentOTAMode = OTA_MODE_LOCAL;
        }
    }

    if (mode == OTA_MODE_LOCAL) {
        WiFi.mode(WIFI_AP);
        WiFi.softAP(ap_ssid, ap_pass);
        activeIPAddress = WiFi.softAPIP().toString();
        activeSSIDName = ap_ssid;
        otaStatusText = "AP Active (192.168.4.1)";
        Serial.printf("[OTA] Local AP Started. SSID: %s, IP: %s\n", ap_ssid, activeIPAddress.c_str());
    }

    // Set up Web Server endpoints
    server.on("/", HTTP_GET, []() {
        server.sendHeader("Connection", "close");
        server.send(200, "text/html", getPortalHTML());
    });

    server.on("/save_wifi", HTTP_POST, []() {
        if (server.hasArg("ssid")) {
            String newSSID = server.arg("ssid");
            String newPass = server.hasArg("pass") ? server.arg("pass") : "";
            saveWiFiCredentials(newSSID, newPass);
            server.send(200, "text/plain", "OK: Saved");
        } else {
            server.send(400, "text/plain", "Missing SSID");
        }
    });

    static size_t txOtaExpected = 0;
    server.on("/update", HTTP_POST, []() {
        server.sendHeader("Connection", "close");
        server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "SUCCESS - Rebooting...");
        delay(1000);
        ESP.restart();
    }, []() {
        HTTPUpload& upload = server.upload();
        if (upload.status == UPLOAD_FILE_START) {
            Serial.printf("[OTA] Update Start: %s\n", upload.filename.c_str());
            otaStatusText = "Uploading: " + upload.filename;
            otaProgress = 0;
            lastReportedProgress = -1;
            
            txOtaExpected = 0;
            if (server.hasArg("size")) {
                txOtaExpected = server.arg("size").toInt();
            }
            if (txOtaExpected <= 0) {
                int cl = server.clientContentLength();
                if (cl > 300) {
                    txOtaExpected = cl - 200; // Offset multipart boundary overhead
                } else if (cl > 0) {
                    txOtaExpected = cl;
                } else {
                    txOtaExpected = 1350000; // Fallback typical firmware size
                }
            }
            Serial.printf("[OTA] Expected size: %u bytes\n", (unsigned int)txOtaExpected);
            
            updateOTAProgressBar(0, "Flashing Firmware...");
            if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                Update.printError(Serial);
            }
            if (txOtaExpected > 0) {
                int pct = (upload.totalSize * 100) / txOtaExpected;
                pct = constrain(pct, 0, 99);
                if (pct != lastReportedProgress) {
                    lastReportedProgress = pct;
                    otaProgress = pct;
                    updateOTAProgressBar(otaProgress, "Flashing Firmware...");
                }
            }
        } else if (upload.status == UPLOAD_FILE_END) {
            if (Update.end(true)) {
                Serial.printf("[OTA] Update Success: %u bytes\n", upload.totalSize);
                otaStatusText = "Success! Rebooting...";
                otaProgress = 100;
                lastReportedProgress = 100;
                updateOTAProgressBar(100, "Success! Rebooting...");
            } else {
                Update.printError(Serial);
                otaStatusText = "Update Failed!";
                otaProgress = 0;
                lastReportedProgress = 0;
                updateOTAProgressBar(0, "Update Failed!");
            }
        }
    });

    server.begin();

    // Set up ArduinoOTA for CLI and IDE fast flashing
    ArduinoOTA.setHostname("lifeline-tx-pro");
    ArduinoOTA.onStart([]() {
        Serial.println(F("[ArduinoOTA] Firmware Update Started"));
        otaStatusText = "PIO Upload Active...";
        otaProgress = 0;
        lastReportedProgress = -1;
        updateOTAProgressBar(0, "PIO Uploading...");
    });
    ArduinoOTA.onEnd([]() {
        Serial.println(F("\n[ArduinoOTA] Firmware Update Success"));
        otaStatusText = "Success! Rebooting...";
        otaProgress = 100;
        updateOTAProgressBar(100, "Success! Rebooting...");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        int pct = (progress * 100) / total;
        if (pct != lastReportedProgress) {
            lastReportedProgress = pct;
            otaProgress = pct;
            updateOTAProgressBar(otaProgress, "PIO Uploading...");
            Serial.printf("[ArduinoOTA] Progress: %d%%\r", otaProgress);
        }
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("[ArduinoOTA] Error[%u]\n", error);
        otaStatusText = "OTA Error!";
        updateOTAProgressBar(0, "OTA Error!");
    });
    ArduinoOTA.begin();

    previousScreen = currentScreen;
    currentScreen = SCREEN_OTA;
    drawOTAScreen();
}

void stopOTAMode() {
    if (!otaActive) return;
    server.stop();
    ArduinoOTA.end();
    if (currentOTAMode == OTA_MODE_LOCAL) {
        WiFi.softAPdisconnect(true);
    } else {
        WiFi.disconnect(true);
    }
    otaActive = false;
    currentOTAMode = OTA_MODE_NONE;
    Serial.println(F("[OTA] Stopped OTA Mode."));
}

void handleOTA() {
    if (!otaActive) return;
    server.handleClient();
    ArduinoOTA.handle();
}

bool isOTAModeActive() {
    return otaActive;
}

OTAMode getCurrentOTAMode() {
    return currentOTAMode;
}

String getOTAIPAddress() {
    return activeIPAddress;
}

String getOTASSID() {
    return activeSSIDName;
}

int getOTAProgress() {
    return otaProgress;
}

String getOTAStatusText() {
    return otaStatusText;
}
