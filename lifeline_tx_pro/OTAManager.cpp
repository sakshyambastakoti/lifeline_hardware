#include "OTAManager.h"
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <ArduinoOTA.h>
#include "Config.h"
#include "DisplayUI.h"

static WebServer server(80);
static bool otaActive = false;
static int otaProgress = 0;
static String otaStatusText = "Listening for Upload...";

const char* ap_ssid = "LifeLine-TX-OTA";
const char* ap_pass = "12345678";

// Embedded HTML web page for drag-and-drop / file browser upload
const char serverIndex[] PROGMEM = 
R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>LifeLine TX OTA Firmware Update</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background: #0d0d0f; color: #fff; text-align: center; padding: 40px 10px; margin: 0; }
    .card { background: #1a1a2e; border: 2px solid #00d4ff; border-radius: 16px; padding: 30px; max-width: 480px; margin: 0 auto; box-shadow: 0 0 30px rgba(0,212,255,0.25); }
    h1 { color: #00ff87; margin-bottom: 5px; font-size: 24px; }
    h3 { color: #a3b1c6; font-weight: 400; margin-top: 0; font-size: 15px; }
    .badge { background: #252542; border: 1px solid #00d4ff; padding: 8px 14px; border-radius: 8px; display: inline-block; margin: 10px 0; color: #00d4ff; font-weight: bold; }
    input[type=file] { margin: 20px 0; padding: 12px; background: #2d2d44; color: #fff; border-radius: 8px; border: 1px solid #405070; width: 85%; }
    input[type=submit] { background: linear-gradient(90deg, #00d4ff, #00ff87); color: #000; font-weight: bold; border: none; padding: 14px 32px; border-radius: 8px; cursor: pointer; font-size: 16px; width: 90%; transition: all 0.2s; }
    input[type=submit]:hover { opacity: 0.9; transform: scale(1.02); }
    .footer { margin-top: 20px; font-size: 12px; color: #64748b; }
  </style>
</head>
<body>
  <div class="card">
    <h1>LifeLine Emergency TX</h1>
    <h3>Wireless OTA Firmware Portal</h3>
    <div class="badge">Board: ESP32 Field Transmitter</div>
    <p>Select firmware <b>.bin</b> file to flash to device:</p>
    <form method='POST' action='/update' enctype='multipart/form-data'>
      <input type='file' name='update' accept='.bin' required><br>
      <input type='submit' value='Upload & Flash Firmware'>
    </form>
    <div class="footer">Compatible with PlatformIO IDE & Terminal Upload</div>
  </div>
</body>
</html>
)rawliteral";

void startOTAMode() {
    if (otaActive) return;
    otaActive = true;
    otaProgress = 0;
    otaStatusText = "Listening for Upload...";
    
    // Start Access Point
    WiFi.softAP(ap_ssid, ap_pass);
    IPAddress IP = WiFi.softAPIP();
    Serial.print(F("[OTA] Access Point Started. IP: "));
    Serial.println(IP);
    
    // Web Server setup
    server.on("/", HTTP_GET, []() {
        server.sendHeader("Connection", "close");
        server.send(200, "text/html", serverIndex);
    });
    
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
            if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                Update.printError(Serial);
            }
            if (upload.totalSize > 0) {
                otaProgress = (upload.currentSize * 100) / upload.totalSize;
            }
            drawOTAScreen();
        } else if (upload.status == UPLOAD_FILE_END) {
            if (Update.end(true)) {
                Serial.printf("[OTA] Update Success: %u bytes\n", upload.totalSize);
                otaStatusText = "Success! Rebooting...";
                otaProgress = 100;
                drawOTAScreen();
            } else {
                Update.printError(Serial);
                otaStatusText = "Update Failed!";
                drawOTAScreen();
            }
        }
    });
    
    server.begin();
    
    // ArduinoOTA setup (for PlatformIO / espota direct CLI uploading)
    ArduinoOTA.setHostname("lifeline-tx-pro");
    ArduinoOTA.onStart([]() {
        Serial.println(F("[ArduinoOTA] Firmware Update Started"));
        otaStatusText = "PIO Upload Active...";
        otaProgress = 0;
        drawOTAScreen();
    });
    ArduinoOTA.onEnd([]() {
        Serial.println(F("\n[ArduinoOTA] Firmware Update Success"));
        otaStatusText = "Success! Rebooting...";
        otaProgress = 100;
        drawOTAScreen();
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        otaProgress = (progress * 100) / total;
        Serial.printf("[ArduinoOTA] Progress: %d%%\r", otaProgress);
        drawOTAScreen();
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("[ArduinoOTA] Error[%u]\n", error);
        otaStatusText = "OTA Error!";
        drawOTAScreen();
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
    WiFi.softAPdisconnect(true);
    otaActive = false;
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

String getOTAIPAddress() {
    return WiFi.softAPIP().toString();
}

int getOTAProgress() {
    return otaProgress;
}

String getOTAStatusText() {
    return otaStatusText;
}
