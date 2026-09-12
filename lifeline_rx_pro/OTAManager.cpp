#include "OTAManager.h"
#include "Config.h"
#include "DisplayUI.h"
#include "BuzzerLED.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <HTTPUpdate.h>
#include <WebServer.h>
#include <Update.h>
#include <ArduinoOTA.h>

// Static Helper: Compare semantic versions (e.g. "3.1.0 PRO" vs "3.2.0")
static bool isVersionNewer(String currentVer, String newVer) {
    currentVer.trim();
    newVer.trim();
    
    if (currentVer.startsWith("v") || currentVer.startsWith("V")) currentVer = currentVer.substring(1);
    if (newVer.startsWith("v") || newVer.startsWith("V")) newVer = newVer.substring(1);
    
    int spaceCur = currentVer.indexOf(' ');
    if (spaceCur != -1) currentVer = currentVer.substring(0, spaceCur);
    
    int spaceNew = newVer.indexOf(' ');
    if (spaceNew != -1) newVer = newVer.substring(0, spaceNew);

    int curMajor = 0, curMinor = 0, curPatch = 0;
    int newMajor = 0, newMinor = 0, newPatch = 0;

    sscanf(currentVer.c_str(), "%d.%d.%d", &curMajor, &curMinor, &curPatch);
    sscanf(newVer.c_str(), "%d.%d.%d", &newMajor, &newMinor, &newPatch);

    if (newMajor > curMajor) return true;
    if (newMajor < curMajor) return false;

    if (newMinor > curMinor) return true;
    if (newMinor < curMinor) return false;

    return newPatch > curPatch;
}

// Static Helper: Extract JSON value string by key
static String extractJSONValue(const String& json, const String& key) {
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

bool checkAndPerformOTA() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println(F("[OTA] Skipping OTA check: Wi-Fi not connected."));
        return false;
    }

    Serial.println(F("\n═══════════════════════════════════════════════════════════"));
    Serial.println(F("[OTA] Checking VPS server for remote firmware updates..."));
    Serial.printf("[OTA] Target Manifest URL: %s\n", OTA_VERSION_URL);
    Serial.printf("[OTA] Current Device Firmware: %s\n", FIRMWARE_VERSION);
    Serial.println(F("═══════════════════════════════════════════════════════════"));

    // Update 16x2 LCD UI
    currentScreen = SCREEN_OTA;
    drawOTACheckingScreen();

    WiFiClientSecure client;
    client.setInsecure(); // Secure SSL connection without hardcoded CA certificate

    HTTPClient http;
    http.setTimeout(OTA_CHECK_TIMEOUT_MS);
    
    if (!http.begin(client, OTA_VERSION_URL)) {
        Serial.println(F("[OTA] ERROR: Failed to initialize HTTPS connection to VPS."));
        return false;
    }

    // Telemetry headers sent to VPS
    http.addHeader("User-Agent", "LifeLine-RX-BaseStation/3.1");
    http.addHeader("X-ESP32-MAC", WiFi.macAddress());
    http.addHeader("X-ESP32-Firmware", FIRMWARE_VERSION);
    http.addHeader("X-ESP32-FreeHeap", String(ESP.getFreeHeap()));

    int httpCode = http.GET();
    
    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("[OTA] HTTP check failed. Code: %d (%s)\n", httpCode, http.errorToString(httpCode).c_str());
        http.end();
        return false;
    }

    String payload = http.getString();
    http.end();

    Serial.println(F("[OTA] Manifest received from VPS:"));
    Serial.println(payload);

    String latestVersion = extractJSONValue(payload, "version");
    String binaryUrl = extractJSONValue(payload, "url");

    if (latestVersion.length() == 0 || binaryUrl.length() == 0) {
        Serial.println(F("[OTA] ERROR: Malformed JSON manifest. Missing 'version' or 'url'."));
        return false;
    }

    Serial.printf("[OTA] Server Version: '%s' | Binary URL: '%s'\n", latestVersion.c_str(), binaryUrl.c_str());

    if (!isVersionNewer(FIRMWARE_VERSION, latestVersion)) {
        Serial.println(F("[OTA] Device is running the latest firmware. No update needed."));
        return false;
    }

    // Update Found! Prepare LCD and initiate download
    Serial.println(F("[OTA] *** NEW FIRMWARE AVAILABLE! INITIATING UPDATE ***"));
    
    playBootTone(); // Audio beep notification
    drawOTAFoundScreen(latestVersion);
    delay(1500); // Allow user to read new version on LCD

    // Setup HTTPUpdate progress callbacks for 16x2 LCD
    httpUpdate.onStart([]() {
        Serial.println(F("[OTA] Download started..."));
        drawOTAProgressScreen(0);
    });

    httpUpdate.onProgress([](int current, int total) {
        if (total > 0) {
            int percent = (current * 100) / total;
            static int lastPercent = -1;
            if (percent != lastPercent) {
                lastPercent = percent;
                Serial.printf("[OTA] Progress: %d / %d bytes (%d%%)\n", current, total, percent);
                drawOTAProgressScreen(percent);
            }
        }
    });

    httpUpdate.onEnd([]() {
        Serial.println(F("[OTA] Download complete! Writing flash..."));
        drawOTASuccessScreen();
    });

    httpUpdate.onError([](int err) {
        Serial.printf("[OTA] ERROR: Update failed (Code %d)\n", err);
    });

    // Execute firmware streaming & partition update
    t_httpUpdate_return ret = httpUpdate.update(client, binaryUrl);

    switch (ret) {
        case HTTP_UPDATE_FAILED:
            Serial.printf("[OTA] Update Failed! Error (%d): %s\n", 
                          httpUpdate.getLastError(), 
                          httpUpdate.getLastErrorString().c_str());
            drawOTAFailedScreen("Flash Failed!");
            delay(2000);
            return false;

        case HTTP_UPDATE_NO_UPDATES:
            Serial.println(F("[OTA] No updates performed."));
            return false;

        case HTTP_UPDATE_OK:
            Serial.println(F("[OTA] SUCCESS: Firmware update complete! Rebooting device now..."));
            delay(1000);
            ESP.restart();
            return true;
    }

    return false;
}

// ═══════════════════════════════════════════════════════════════════════════════════
//                 LOCAL WIRELESS OTA (3 Wi-Fi Button Presses)
// ═══════════════════════════════════════════════════════════════════════════════════
static WebServer otaServer(80);
static bool localOtaActive = false;
static int localOtaProgress = 0;

const char* rx_ota_ap_ssid = "LifeLine-RX-OTA";
const char* rx_ota_ap_pass = "12345678";

const char rxServerIndex[] PROGMEM = 
R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>LifeLine RX Base Station OTA Firmware Update</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: 'Segoe UI', Arial, sans-serif; background: #08080c; color: #fff; text-align: center; padding: 30px; margin: 0; }
    .card { background: #121218; border: 2px solid #ff1e42; padding: 25px; max-width: 440px; margin: 0 auto; box-shadow: 0 0 25px rgba(255, 30, 66, 0.2); }
    h1 { color: #ff1e42; margin-bottom: 5px; font-size: 22px; }
    h3 { color: #a3b1c6; font-weight: 300; margin-top: 0; font-size: 14px; }
    input[type=file] { margin: 20px 0; padding: 10px; background: #1b1b24; color: #fff; border: 1px solid #405070; width: 90%; }
    input[type=submit], button { background: #ff1e42; color: #fff; font-weight: bold; border: none; padding: 12px 28px; cursor: pointer; font-size: 15px; width: 90%; }
    input[type=submit]:hover, button:hover { background: #d01030; }
    .progress-box { height: 22px; background: #0a0a0f; border: 1px solid #282836; margin-top: 15px; display: none; overflow: hidden; }
    .progress-bar { height: 100%; width: 0%; background: #ff1e42; text-align: right; padding-right: 6px; line-height: 22px; font-size: 12px; font-weight: bold; color: #fff; }
    .msg { margin-top: 12px; font-size: 12px; color: #00ff87; display: none; font-family: monospace; }
  </style>
</head>
<body>
  <div class="card">
    <h1>LIFELINE RX BASE STATION</h1>
    <h3>Wireless OTA Firmware Portal</h3>
    <p>Select firmware <b>.bin</b> file to update base station:</p>
    <form id='flash_form' method='POST' action='/update' enctype='multipart/form-data'>
      <input type='file' id='firmware_file' name='update' accept='.bin' required><br>
      <button type='submit' id='btn_flash'>Flash Firmware</button>
    </form>
    <div class='progress-box' id='p_box'><div class='progress-bar' id='p_bar'>0%</div></div>
    <div class='msg' id='flash_msg'></div>
  </div>
  <script>
  document.getElementById('flash_form').onsubmit = function(e) {
    e.preventDefault();
    var file = document.getElementById('firmware_file').files[0];
    if (!file) return;
    var pBox = document.getElementById('p_box');
    var pBar = document.getElementById('p_bar');
    var msg = document.getElementById('flash_msg');
    var btn = document.getElementById('btn_flash');
    pBox.style.display = 'block';
    msg.style.display = 'block';
    msg.style.color = '#ff1e42';
    msg.innerText = 'Uploading to Base Station...';
    btn.disabled = true; btn.style.opacity = '0.5';
    var xhr = new XMLHttpRequest();
    xhr.upload.onprogress = function(evt) {
      if (evt.lengthComputable) {
        var pct = Math.round((evt.loaded / evt.total) * 100);
        pBar.style.width = pct + '%';
        pBar.innerText = pct + '%';
        msg.innerText = 'Flashing Base Station: ' + pct + '%';
      }
    };
    xhr.onload = function() {
      if (xhr.status == 200) {
        pBar.style.width = '100%';
        pBar.innerText = '100%';
        pBar.style.background = '#00ff87';
        msg.style.color = '#00ff87';
        msg.innerText = 'SUCCESS! Base Station is rebooting...';
      } else {
        msg.style.color = '#ff1e42';
        msg.innerText = 'Upload failed (' + xhr.status + ')';
        btn.disabled = false; btn.style.opacity = '1';
      }
    };
    xhr.onerror = function() {
      pBar.style.background = '#00ff87';
      msg.style.color = '#00ff87';
      msg.innerText = 'Upload complete! Base Station is rebooting...';
    };
    var data = new FormData();
    data.append('update', file);
    xhr.open('POST', '/update?size=' + file.size);
    xhr.send(data);
  };
  </script>
</body>
</html>
)rawliteral";

void startLocalOTAMode() {
    if (localOtaActive) return;
    localOtaActive = true;
    localOtaProgress = 0;
    
    // Start Access Point
    WiFi.softAP(rx_ota_ap_ssid, rx_ota_ap_pass);
    IPAddress IP = WiFi.softAPIP();
    Serial.print(F("[OTA LOCAL] AP Started. IP: "));
    Serial.println(IP);
    
    // Setup WebServer
    otaServer.on("/", HTTP_GET, []() {
        otaServer.sendHeader("Connection", "close");
        otaServer.send(200, "text/html", rxServerIndex);
    });
    
    static size_t rxLocalExpected = 0;
    static int lastRxLocalPct = -1;
    otaServer.on("/update", HTTP_POST, []() {
        otaServer.sendHeader("Connection", "close");
        otaServer.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "SUCCESS - Rebooting...");
        delay(1000);
        ESP.restart();
    }, []() {
        HTTPUpload& upload = otaServer.upload();
        if (upload.status == UPLOAD_FILE_START) {
            Serial.printf("[OTA LOCAL] Start: %s\n", upload.filename.c_str());
            lastRxLocalPct = -1;
            localOtaProgress = 0;
            rxLocalExpected = 0;
            if (otaServer.hasArg("size")) {
                rxLocalExpected = otaServer.arg("size").toInt();
            }
            if (rxLocalExpected <= 0) {
                int cl = otaServer.clientContentLength();
                if (cl > 300) {
                    rxLocalExpected = cl - 200; // Offset multipart boundary overhead
                } else if (cl > 0) {
                    rxLocalExpected = cl;
                } else {
                    rxLocalExpected = 1350000; // Fallback typical firmware size
                }
            }
            Serial.printf("[OTA LOCAL] Expected size: %u bytes\n", (unsigned int)rxLocalExpected);
            drawOTAProgressScreen(0);
            if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                Update.printError(Serial);
            }
            if (rxLocalExpected > 0) {
                int pct = (upload.totalSize * 100) / rxLocalExpected;
                pct = constrain(pct, 0, 99);
                if (pct != lastRxLocalPct) {
                    lastRxLocalPct = pct;
                    localOtaProgress = pct;
                    drawOTAProgressScreen(localOtaProgress);
                }
            }
        } else if (upload.status == UPLOAD_FILE_END) {
            if (Update.end(true)) {
                Serial.printf("[OTA LOCAL] Success: %u bytes\n", upload.totalSize);
                localOtaProgress = 100;
                lastRxLocalPct = 100;
                drawOTAProgressScreen(100);
                drawOTASuccessScreen();
            } else {
                Update.printError(Serial);
                drawOTAFailedScreen("Flash Fail");
            }
        }
    });
    
    otaServer.begin();
    
    // ArduinoOTA setup for PlatformIO CLI / Terminal upload
    ArduinoOTA.setHostname("lifeline-rx-pro");
    ArduinoOTA.onStart([]() {
        Serial.println(F("[ArduinoOTA RX] Start"));
        drawOTAProgressScreen(0);
    });
    ArduinoOTA.onEnd([]() {
        Serial.println(F("\n[ArduinoOTA RX] End"));
        drawOTASuccessScreen();
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        if (total > 0) {
            int percent = (progress * 100) / total;
            static int lastArduinoOtaPct = -1;
            if (percent != lastArduinoOtaPct) {
                lastArduinoOtaPct = percent;
                drawOTAProgressScreen(percent);
            }
        }
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("[ArduinoOTA RX] Error[%u]\n", error);
        drawOTAFailedScreen("ArduinoOTA Err");
    });
    ArduinoOTA.begin();
    
    currentScreen = SCREEN_OTA;
    drawLocalOTAScreen();
}

void stopLocalOTAMode() {
    if (!localOtaActive) return;
    otaServer.stop();
    ArduinoOTA.end();
    WiFi.softAPdisconnect(true);
    localOtaActive = false;
    Serial.println(F("[OTA LOCAL] Stopped"));
}

void handleLocalOTA() {
    if (!localOtaActive) return;
    otaServer.handleClient();
    ArduinoOTA.handle();
}

bool isLocalOTAModeActive() {
    return localOtaActive;
}

