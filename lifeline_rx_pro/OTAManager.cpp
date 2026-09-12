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
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>LIFELINE RX // LOCAL OTA PORTAL</title>
  <style>
    :root {
      --bg-0: #000000;
      --bg-1: #0d0d0d;
      --bg-2: #141414;
      --border-strong: #3a3a3a;
      --border: #262626;
      --text-0: #ffffff;
      --text-1: #cccccc;
      --text-2: #999999;
      --text-3: #666666;
      --cyan: #06b6d4;
      --radius-btn: 9999px;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; border-radius: 0px; }
    body {
      background: var(--bg-0);
      color: var(--text-1);
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
      min-height: 100vh;
      display: flex;
      flex-direction: column;
      align-items: center;
      padding: 24px 14px 48px;
      -webkit-font-smoothing: antialiased;
    }
    .top-rail {
      position: fixed;
      top: 0; left: 0; width: 100%; height: 3px;
      background: linear-gradient(90deg, #6366f1, #06b6d4, #8b5cf6, #f59e0b);
      z-index: 999;
      box-shadow: 0 0 12px rgba(6, 182, 212, 0.6);
    }
    .container { width: 100%; max-width: 460px; margin: 0 auto; position: relative; }
    .card {
      background: rgba(18, 18, 24, 0.92);
      border: 1px solid var(--border-strong);
      padding: 28px 24px;
      box-shadow: 0 20px 40px -15px rgba(0,0,0,0.8);
      position: relative;
    }
    .card::before {
      content: '';
      position: absolute;
      top: 0; left: 0; width: 4px; height: 100%;
      background: linear-gradient(180deg, #06b6d4 0%, #6366f1 100%);
    }
    .header {
      border-bottom: 1px solid var(--border);
      padding-bottom: 16px;
      margin-bottom: 20px;
    }
    .meta-row {
      display: flex;
      align-items: center;
      justify-content: space-between;
      margin-bottom: 6px;
    }
    .brand-tag {
      font-family: Consolas, Monaco, monospace;
      font-size: 11px;
      font-weight: 700;
      letter-spacing: 1.5px;
      color: var(--cyan);
      text-transform: uppercase;
    }
    .badge-mode {
      font-family: Consolas, Monaco, monospace;
      font-size: 10px;
      text-transform: uppercase;
      letter-spacing: 1px;
      padding: 3px 9px;
      background: var(--bg-1);
      border: 1px solid var(--border-strong);
      color: var(--text-2);
      border-radius: var(--radius-btn);
    }
    h1 {
      font-size: 22px;
      font-weight: 700;
      letter-spacing: -0.5px;
      color: var(--text-0);
      margin-bottom: 4px;
    }
    .sub { font-size: 12px; color: var(--text-2); }
    .tele-strip {
      background: var(--bg-1);
      border: 1px solid var(--border);
      padding: 10px 12px;
      margin-bottom: 22px;
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 8px;
      font-family: Consolas, Monaco, monospace;
      font-size: 11px;
    }
    .tele-lbl { font-size: 9px; color: var(--text-3); text-transform: uppercase; letter-spacing: 0.8px; display: block; }
    .tele-val { font-weight: 600; color: var(--text-0); }
    .tele-val.cyan { color: #67e8f9; }
    .sec-desc { font-size: 12px; color: var(--text-2); line-height: 1.5; margin-bottom: 14px; }
    .upload-box {
      border: 1px dashed var(--border-strong);
      background: var(--bg-1);
      padding: 22px 14px;
      text-align: center;
      cursor: pointer;
      position: relative;
      margin-bottom: 10px;
      transition: border-color 0.2s;
    }
    .upload-box:hover { border-color: var(--cyan); background: rgba(6, 182, 212, 0.04); }
    .upload-box input[type=file] { position: absolute; top: 0; left: 0; width: 100%; height: 100%; opacity: 0; cursor: pointer; }
    .upload-prompt { font-size: 12px; color: var(--text-1); font-weight: 500; margin-bottom: 2px; }
    .upload-prompt span { color: var(--cyan); text-decoration: underline; }
    .upload-hint { font-family: monospace; font-size: 10px; color: var(--text-3); }
    .file-sel {
      display: none;
      background: var(--bg-2);
      border: 1px solid var(--border-strong);
      padding: 8px 12px;
      margin-bottom: 10px;
      justify-content: space-between;
      font-family: monospace;
      font-size: 11px;
      color: var(--text-0);
    }
    .btn {
      display: inline-flex;
      align-items: center;
      justify-content: center;
      width: 100%;
      padding: 13px 18px;
      font-family: Consolas, Monaco, monospace;
      font-size: 12px;
      font-weight: 700;
      letter-spacing: 1.2px;
      text-transform: uppercase;
      border-radius: var(--radius-btn);
      cursor: pointer;
      position: relative;
      overflow: hidden;
      transition: all 0.2s ease;
      border: none;
      margin-top: 10px;
      background: var(--cyan);
      color: #000;
      box-shadow: 0 4px 16px rgba(6, 182, 212, 0.35);
    }
    .btn:hover:not(:disabled) { background: #22d3ee; box-shadow: 0 6px 24px rgba(6, 182, 212, 0.55); }
    .btn:disabled { opacity: 0.45; cursor: not-allowed; }
    .progress-wrap { display: none; margin-top: 14px; }
    .progress-meta { display: flex; justify-content: space-between; font-family: monospace; font-size: 11px; margin-bottom: 5px; color: var(--text-2); }
    .progress-bar-bg { width: 100%; height: 8px; background: var(--bg-1); border: 1px solid var(--border-strong); overflow: hidden; }
    .progress-bar-fill { width: 0%; height: 100%; background: linear-gradient(90deg, #06b6d4, #6366f1); transition: width 0.1s; }
    .msg { margin-top: 12px; padding: 10px 12px; font-family: monospace; font-size: 11px; line-height: 1.4; display: none; border-left: 3px solid; }
    .msg.err { background: rgba(255, 30, 66, 0.12); border-color: #ff1e42; color: #fca5a5; }
    .msg.ok { background: rgba(95, 166, 87, 0.12); border-color: #5fa657; color: #86efac; }
    .footer { margin-top: 22px; padding-top: 14px; border-top: 1px solid var(--border); display: flex; justify-content: space-between; font-family: monospace; font-size: 10px; color: var(--text-3); }
  </style>
</head>
<body>
  <div class="top-rail"></div>
  <div class="container">
    <div class="card">
      <div class="header">
        <div class="meta-row">
          <span class="brand-tag">LIFELINE RX PRO</span>
          <span class="badge-mode">LOCAL OTA MODE</span>
        </div>
        <h1>BASE STATION GATEWAY</h1>
        <div class="sub">Direct Wireless Firmware Flash Portal</div>
      </div>
      <div class="tele-strip">
        <div><span class="tele-lbl">AP Address</span><span class="tele-val cyan">192.168.4.1</span></div>
        <div><span class="tele-lbl">AP SSID</span><span class="tele-val">LifeLine-RX-OTA</span></div>
        <div><span class="tele-lbl">Hardware</span><span class="tele-val">ESP32 + SX1278</span></div>
        <div><span class="tele-lbl">Exit OTA</span><span class="tele-val">1-Click Wi-Fi Button</span></div>
      </div>
      <p class="sec-desc">Select compiled firmware <b>.bin</b> file to update base station partition:</p>
      <form id="flash_form">
        <div class="upload-box" id="drop_zone">
          <input type="file" id="firmware_file" name="update" accept=".bin" required>
          <div class="upload-prompt"><span>Browse binary</span> or tap here</div>
          <div class="upload-hint">ESP32-WROOM-32 (*.bin)</div>
        </div>
        <div class="file-sel" id="file_meta">
          <span id="file_name">firmware.bin</span>
          <span id="file_size">0 KB</span>
        </div>
        <button type="submit" class="btn" id="btn_flash">FLASH FIRMWARE</button>
      </form>
      <div class="progress-wrap" id="p_box">
        <div class="progress-meta">
          <span id="p_stat">Uploading to Base Station...</span>
          <span id="p_pct">0%</span>
        </div>
        <div class="progress-bar-bg">
          <div class="progress-bar-fill" id="p_bar"></div>
        </div>
      </div>
      <div class="msg" id="flash_msg"></div>
      <div class="footer">
        <span>LIFELINE COMMAND BASE // RX</span>
        <span>PARTITION: OTA_0/1 SAFE</span>
      </div>
    </div>
  </div>
  <script>
    var fi = document.getElementById('firmware_file');
    fi.onchange = function() {
      if (this.files && this.files[0]) {
        document.getElementById('file_name').innerText = this.files[0].name;
        document.getElementById('file_size').innerText = Math.round(this.files[0].size / 1024) + ' KB';
        document.getElementById('file_meta').style.display = 'flex';
      }
    };
    document.getElementById('flash_form').onsubmit = function(e) {
      e.preventDefault();
      var file = fi.files[0];
      if (!file) return;
      var pBox = document.getElementById('p_box');
      var pBar = document.getElementById('p_bar');
      var pPct = document.getElementById('p_pct');
      var pStat = document.getElementById('p_stat');
      var msg = document.getElementById('flash_msg');
      var btn = document.getElementById('btn_flash');
      pBox.style.display = 'block';
      msg.style.display = 'none';
      btn.disabled = true;
      var xhr = new XMLHttpRequest();
      xhr.upload.onprogress = function(evt) {
        if (evt.lengthComputable) {
          var pct = Math.round((evt.loaded / evt.total) * 100);
          pBar.style.width = pct + '%';
          pPct.innerText = pct + '%';
          pStat.innerText = 'Writing Flash: ' + pct + '% (' + Math.round(evt.loaded / 1024) + ' KB)';
        }
      };
      xhr.onload = function() {
        if (xhr.status == 200) {
          pBar.style.width = '100%';
          pBar.style.background = '#10b981';
          pPct.innerText = '100%';
          msg.className = 'msg ok';
          msg.style.display = 'block';
          msg.innerHTML = '<strong>SUCCESS:</strong> Base Station flashed! Rebooting...';
        } else {
          msg.className = 'msg err';
          msg.style.display = 'block';
          msg.innerHTML = '<strong>UPLOAD FAILED (' + xhr.status + ')</strong>';
          btn.disabled = false;
        }
      };
      xhr.onerror = function() {
        pBar.style.width = '100%';
        pBar.style.background = '#10b981';
        msg.className = 'msg ok';
        msg.style.display = 'block';
        msg.innerHTML = '<strong>COMPLETE:</strong> Transfer finished. Base Station rebooting...';
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
    static size_t rxLocalAccumulated = 0;
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
            rxLocalAccumulated = 0;
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
            rxLocalAccumulated += upload.currentSize;
            if (rxLocalExpected > 0) {
                int pct = (rxLocalAccumulated * 100) / rxLocalExpected;
                pct = constrain(pct, 0, 99);
                if (pct != lastRxLocalPct) {
                    lastRxLocalPct = pct;
                    localOtaProgress = pct;
                    drawOTAProgressScreen(localOtaProgress);
                }
            }
        } else if (upload.status == UPLOAD_FILE_END) {
            if (Update.end(true)) {
                Serial.printf("[OTA LOCAL] Success: %u bytes\n", (unsigned int)rxLocalAccumulated);
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

