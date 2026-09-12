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

// Embedded Web Portal HTML - Bugatti-Inspired Austere Luxury Design (DESIGN_PALETTE.md)
static String getPortalHTML() {
    String savedSSID, savedPass;
    loadWiFiCredentials(savedSSID, savedPass);

    String modeText = (currentOTAMode == OTA_MODE_NET ? "NETWORK WI-FI" : "LOCAL ACCESS POINT");

    String html = F(
        "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'>"
        "<meta name='viewport' content='width=device-width,initial-scale=1'>"
        "<title>LIFELINE TX PRO // OTA & WI-FI GATEWAY</title>"
        "<style>"
        ":root{--bg-0:#000000;--bg-1:#0d0d0d;--bg-2:#141414;--bg-3:#1f1f1f;--border:#262626;--border-strong:#3a3a3a;--text-0:#ffffff;--text-1:#cccccc;--text-2:#999999;--text-3:#666666;--danger:#ff1e42;--success:#5fa657;--radius:0px;--radius-btn:9999px;}"
        "*{box-sizing:border-box;margin:0;padding:0;border-radius:var(--radius);}"
        "body{background:var(--bg-0);color:var(--text-1);font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;min-height:100vh;display:flex;flex-direction:column;align-items:center;padding:24px 14px 48px;position:relative;-webkit-font-smoothing:antialiased;}"
        ".top-rail{position:fixed;top:0;left:0;width:100%;height:3px;background:linear-gradient(90deg,#6366f1,#06b6d4,#8b5cf6,#ff1e42);z-index:999;box-shadow:0 0 12px rgba(99,102,241,0.6);}"
        ".container{width:100%;max-width:460px;margin:0 auto;position:relative;z-index:1;}"
        ".card{background:rgba(18,18,24,0.92);border:1px solid var(--border-strong);padding:28px 24px;box-shadow:0 20px 40px -15px rgba(0,0,0,0.8);position:relative;}"
        ".card::before{content:'';position:absolute;top:0;left:0;width:4px;height:100%;background:linear-gradient(180deg,var(--danger) 0%,#6366f1 100%);}"
        ".header{border-bottom:1px solid var(--border);padding-bottom:16px;margin-bottom:20px;}"
        ".meta-row{display:flex;align-items:center;justify-content:space-between;margin-bottom:6px;}"
        ".brand-tag{font-family:Consolas,Monaco,monospace;font-size:11px;font-weight:700;letter-spacing:1.5px;color:var(--danger);text-transform:uppercase;}"
        ".badge-mode{font-family:Consolas,Monaco,monospace;font-size:10px;text-transform:uppercase;letter-spacing:1px;padding:3px 9px;background:var(--bg-1);border:1px solid var(--border-strong);color:var(--text-2);border-radius:var(--radius-btn);}"
        "h1{font-family:'Space Grotesk',sans-serif;font-size:22px;font-weight:700;letter-spacing:-0.5px;color:var(--text-0);margin-bottom:4px;}"
        ".sub{font-size:12px;color:var(--text-2);}"
        ".tele-strip{background:var(--bg-1);border:1px solid var(--border);padding:10px 12px;margin-bottom:22px;display:grid;grid-template-columns:1fr 1fr;gap:8px;font-family:Consolas,Monaco,monospace;font-size:11px;}"
        ".tele-lbl{font-size:9px;color:var(--text-3);text-transform:uppercase;letter-spacing:0.8px;display:block;}"
        ".tele-val{font-weight:600;color:var(--text-0);}"
        ".tele-val.green{color:#6ee7b7;}"
        ".sec-head{display:flex;align-items:center;gap:8px;margin:24px 0 10px;}"
        ".sec-num{font-family:monospace;font-size:10px;font-weight:700;background:var(--text-0);color:var(--bg-0);padding:2px 5px;}"
        ".sec-title{font-size:13px;font-weight:700;text-transform:uppercase;letter-spacing:1.2px;color:var(--text-0);}"
        ".sec-desc{font-size:11.5px;color:var(--text-2);line-height:1.4;margin-bottom:12px;}"
        ".upload-box{border:1px dashed var(--border-strong);background:var(--bg-1);padding:22px 14px;text-align:center;cursor:pointer;position:relative;margin-bottom:10px;transition:border-color 0.2s;}"
        ".upload-box:hover{border-color:var(--danger);background:rgba(255,30,66,0.04);}"
        ".upload-box input[type=file]{position:absolute;top:0;left:0;width:100%;height:100%;opacity:0;cursor:pointer;}"
        ".upload-prompt{font-size:12px;color:var(--text-1);font-weight:500;margin-bottom:2px;}"
        ".upload-prompt span{color:var(--danger);text-decoration:underline;}"
        ".upload-hint{font-family:monospace;font-size:10px;color:var(--text-3);}"
        ".file-sel{display:none;background:var(--bg-2);border:1px solid var(--border-strong);padding:8px 12px;margin-bottom:10px;justify-content:space-between;font-family:monospace;font-size:11px;color:var(--text-0);}"
        "label{display:block;font-family:Consolas,Monaco,monospace;font-size:10px;font-weight:700;color:var(--text-2);text-transform:uppercase;letter-spacing:0.8px;margin:10px 0 4px;}"
        "input[type=text],input[type=password]{width:100%;padding:11px 12px;background:var(--bg-1);border:1px solid var(--border);color:var(--text-0);font-family:monospace;font-size:12px;outline:none;transition:border-color 0.2s;}"
        "input[type=text]:focus,input[type=password]:focus{border-color:var(--text-0);}"
        ".btn{display:inline-flex;align-items:center;justify-content:center;width:100%;padding:13px 18px;font-family:Consolas,Monaco,monospace;font-size:12px;font-weight:700;letter-spacing:1.2px;text-transform:uppercase;border-radius:var(--radius-btn);cursor:pointer;position:relative;overflow:hidden;transition:all 0.2s ease;border:none;margin-top:10px;}"
        ".btn-danger{background:var(--danger);color:#fff;box-shadow:0 4px 16px rgba(255,30,66,0.35);}"
        ".btn-danger:hover:not(:disabled){background:#ff3355;box-shadow:0 6px 24px rgba(255,30,66,0.55);}"
        ".btn-primary{background:var(--text-0);color:var(--bg-0);box-shadow:0 4px 14px rgba(255,255,255,0.15);}"
        ".btn-primary:hover:not(:disabled){background:#fff;box-shadow:0 6px 20px rgba(255,255,255,0.25);}"
        ".btn:disabled{opacity:0.45;cursor:not-allowed;}"
        ".progress-wrap{display:none;margin-top:14px;}"
        ".progress-meta{display:flex;justify-content:space-between;font-family:monospace;font-size:11px;margin-bottom:5px;color:var(--text-2);}"
        ".progress-bar-bg{width:100%;height:8px;background:var(--bg-1);border:1px solid var(--border-strong);overflow:hidden;}"
        ".progress-bar-fill{width:0%;height:100%;background:linear-gradient(90deg,#ff1e42,#ff6b81);transition:width 0.1s;}"
        ".msg{margin-top:12px;padding:10px 12px;font-family:monospace;font-size:11px;line-height:1.4;display:none;border-left:3px solid;}"
        ".msg.err{background:rgba(255,30,66,0.12);border-color:var(--danger);color:#fca5a5;}"
        ".msg.ok{background:rgba(95,166,87,0.12);border-color:var(--success);color:#86efac;}"
        ".footer{margin-top:22px;padding-top:14px;border-top:1px solid var(--border);display:flex;justify-content:space-between;font-family:monospace;font-size:10px;color:var(--text-3);}"
        "</style></head><body><div class='top-rail'></div><div class='container'><div class='card'>"
        "<div class='header'><div class='meta-row'><span class='brand-tag'>LIFELINE TX PRO</span>"
        "<span class='badge-mode'>"
    );
    html += modeText;
    html += F(
        "</span></div><h1>TACTICAL FIELD UNIT</h1><div class='sub'>OTA Firmware Gateway &amp; Telemetry Uplink</div></div>"
        "<div class='tele-strip'><div><span class='tele-lbl'>Active IP</span><span class='tele-val green'>"
    );
    html += activeIPAddress;
    html += F(
        "</span></div><div><span class='tele-lbl'>Gateway SSID</span><span class='tele-val'>"
    );
    html += activeSSIDName;
    html += F(
        "</span></div><div><span class='tele-lbl'>Radio Telemetry</span><span class='tele-val'>SX1278 // 433MHz</span></div>"
        "<div><span class='tele-lbl'>Exit Portal</span><span class='tele-val'>Press '#' Keypad</span></div></div>"

        "<div class='sec-head'><span class='sec-num'>01</span><span class='sec-title'>Firmware Flash (OTA)</span></div>"
        "<div class='sec-desc'>Select compiled firmware .bin file. Flash commits directly to active partition.</div>"
        "<form id='flash_form'>"
        "<div class='upload-box' id='drop_box'>"
        "<input type='file' id='fw_file' name='update' accept='.bin' required>"
        "<div class='upload-prompt'><span>Browse file</span> or tap here</div>"
        "<div class='upload-hint'>ESP32 BINARY (*.bin)</div>"
        "</div>"
        "<div class='file-sel' id='file_meta'><span id='file_name'>firmware.bin</span><span id='file_size'>0 KB</span></div>"
        "<button type='submit' class='btn btn-danger' id='btn_flash'>UPLOAD &amp; FLASH FIRMWARE</button>"
        "</form>"
        "<div class='progress-wrap' id='p_wrap'>"
        "<div class='progress-meta'><span id='p_status'>Writing Flash...</span><span id='p_pct'>0%</span></div>"
        "<div class='progress-bar-bg'><div class='progress-bar-fill' id='p_fill'></div></div>"
        "</div>"
        "<div class='msg' id='flash_msg'></div>"

        "<div class='sec-head'><span class='sec-num'>02</span><span class='sec-title'>Wi-Fi Credentials Config</span></div>"
        "<div class='sec-desc'>Configure Wi-Fi client network for Network OTA mode.</div>"
        "<form id='wifi_form'>"
        "<label>Target Wi-Fi SSID:</label>"
        "<input type='text' id='wifi_ssid' name='ssid' value='"
    );
    html += savedSSID;
    html += F(
        "' placeholder='SSID' required>"
        "<label>WPA2 Password:</label>"
        "<input type='password' id='wifi_pass' name='pass' value='"
    );
    html += savedPass;
    html += F(
        "' placeholder='Password'>"
        "<button type='submit' class='btn btn-primary' id='btn_save'>SAVE WI-FI CREDENTIALS</button>"
        "</form>"
        "<div class='msg' id='wifi_msg'></div>"

        "<div class='footer'><span>MIL-SPEC TELEMETRY // SYSTEM 01-TX</span><span>SEC: WPA2-PSK</span></div>"
        "</div></div>"

        "<script>"
        "var fi=document.getElementById('fw_file');"
        "fi.onchange=function(){"
        "  if(this.files&&this.files[0]){"
        "    document.getElementById('file_name').innerText=this.files[0].name;"
        "    document.getElementById('file_size').innerText=Math.round(this.files[0].size/1024)+' KB';"
        "    document.getElementById('file_meta').style.display='flex';"
        "  }"
        "};"
        "document.getElementById('flash_form').onsubmit=function(e){"
        "  e.preventDefault();"
        "  var file=fi.files[0];"
        "  if(!file)return;"
        "  var pWrap=document.getElementById('p_wrap');"
        "  var pFill=document.getElementById('p_fill');"
        "  var pPct=document.getElementById('p_pct');"
        "  var pStat=document.getElementById('p_status');"
        "  var msg=document.getElementById('flash_msg');"
        "  var btn=document.getElementById('btn_flash');"
        "  pWrap.style.display='block';msg.style.display='none';btn.disabled=true;"
        "  var xhr=new XMLHttpRequest();"
        "  xhr.upload.onprogress=function(evt){"
        "    if(evt.lengthComputable){"
        "      var pct=Math.round((evt.loaded/evt.total)*100);"
        "      pFill.style.width=pct+'%';"
        "      pPct.innerText=pct+'%';"
        "      pStat.innerText='Writing Flash: '+pct+'% ('+Math.round(evt.loaded/1024)+' KB)';"
        "    }"
        "  };"
        "  xhr.onload=function(){"
        "    if(xhr.status==200){"
        "      pFill.style.width='100%';"
        "      pFill.style.background='#10b981';"
        "      pPct.innerText='100%';"
        "      msg.className='msg ok';"
        "      msg.style.display='block';"
        "      msg.innerHTML='<strong>SUCCESS:</strong> Firmware flashed! TX Unit is rebooting...';"
        "    }else{"
        "      msg.className='msg err';"
        "      msg.style.display='block';"
        "      msg.innerHTML='<strong>ERROR ('+xhr.status+'):</strong> '+xhr.responseText;"
        "      btn.disabled=false;"
        "    }"
        "  };"
        "  xhr.onerror=function(){"
        "    pFill.style.width='100%';"
        "    pFill.style.background='#10b981';"
        "    msg.className='msg ok';"
        "    msg.style.display='block';"
        "    msg.innerHTML='<strong>COMPLETE:</strong> Upload finished. Device rebooting...';"
        "  };"
        "  var data=new FormData();"
        "  data.append('update',file);"
        "  xhr.open('POST','/update?size='+file.size);"
        "  xhr.send(data);"
        "};"
        "document.getElementById('wifi_form').onsubmit=function(e){"
        "  e.preventDefault();"
        "  var ssid=document.getElementById('wifi_ssid').value;"
        "  var pass=document.getElementById('wifi_pass').value;"
        "  var msg=document.getElementById('wifi_msg');"
        "  var btn=document.getElementById('btn_save');"
        "  btn.disabled=true;"
        "  var xhr=new XMLHttpRequest();"
        "  xhr.open('POST','/save_wifi');"
        "  xhr.setRequestHeader('Content-Type','application/x-www-form-urlencoded');"
        "  xhr.onload=function(){"
        "    btn.disabled=false;"
        "    if(xhr.status==200){"
        "      msg.className='msg ok';"
        "      msg.style.display='block';"
        "      msg.innerHTML='<strong>SAVED:</strong> Wi-Fi parameters stored to flash storage.';"
        "    }else{"
        "      msg.className='msg err';"
        "      msg.style.display='block';"
        "      msg.innerText='Failed to save Wi-Fi settings.';"
        "    }"
        "  };"
        "  xhr.send('ssid='+encodeURIComponent(ssid)+'&pass='+encodeURIComponent(pass));"
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
    static size_t txOtaAccumulated = 0;
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
            txOtaAccumulated = 0;
            
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
            txOtaAccumulated += upload.currentSize;
            if (txOtaExpected > 0) {
                int pct = (txOtaAccumulated * 100) / txOtaExpected;
                pct = constrain(pct, 0, 99);
                if (pct != lastReportedProgress) {
                    lastReportedProgress = pct;
                    otaProgress = pct;
                    updateOTAProgressBar(otaProgress, "Flashing Firmware...");
                }
            }
        } else if (upload.status == UPLOAD_FILE_END) {
            if (Update.end(true)) {
                Serial.printf("[OTA] Update Success: %u bytes\n", (unsigned int)txOtaAccumulated);
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
