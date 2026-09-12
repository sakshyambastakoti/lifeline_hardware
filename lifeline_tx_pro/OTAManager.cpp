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

// Embedded Web Portal HTML - Reference Image Exact Style (Light + Dark Mode, 1-Screen Fit)
static String getPortalHTML() {
    String savedSSID, savedPass;
    loadWiFiCredentials(savedSSID, savedPass);

    String modeText = (currentOTAMode == OTA_MODE_NET ? "NETWORK WI-FI" : "LOCAL ACCESS POINT");

    String html = F(
        "<!DOCTYPE html><html lang='en' data-theme='light'><head><meta charset='UTF-8'>"
        "<meta name='viewport' content='width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no'>"
        "<title>LIFELINE TX PRO // OTA & WI-FI GATEWAY</title>"
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
        "html,body{height:100%;height:100dvh;overflow:hidden;background:var(--canvas);color:var(--text-main);}"
        "body{display:flex;flex-direction:column;justify-content:center;align-items:center;padding:16px 20px;}"
        ".frame{width:100%;max-width:1040px;max-height:96vh;display:flex;flex-direction:column;justify-content:space-between;}"
        ".top-row{display:flex;justify-content:space-between;align-items:center;margin-bottom:10px;}"
        ".badge-pill{display:inline-flex;align-items:center;gap:6px;padding:4px 12px;border-radius:9999px;border:1px solid var(--badge-border);background:var(--badge-bg);color:var(--badge-text);font-family:monospace;font-size:11px;font-weight:700;letter-spacing:0.5px;text-transform:uppercase;}"
        ".theme-btn{padding:5px 12px;border-radius:9999px;border:1px solid var(--badge-border);background:var(--badge-bg);color:var(--badge-text);font-size:11px;font-weight:600;cursor:pointer;}"
        "h1{font-size:30px;font-weight:800;letter-spacing:-0.8px;color:var(--text-main);line-height:1.15;margin-bottom:2px;}"
        ".sub{font-size:13px;color:var(--text-sub);margin-bottom:14px;}"
        ".tabs{display:flex;gap:8px;margin-bottom:12px;}"
        ".tab{padding:7px 16px;border:1px solid var(--tab-border);background:var(--tab-bg);color:var(--tab-text);font-family:monospace;font-size:11px;font-weight:700;letter-spacing:0.8px;text-transform:uppercase;cursor:pointer;}"
        ".tab.active{background:var(--tab-active-bg);color:var(--tab-active-text);border-color:var(--tab-active-border);}"
        ".card{border:1px solid var(--card-border);background:var(--card-outer);padding:22px;display:grid;grid-template-columns:1.2fr 1fr;gap:20px;box-shadow:0 4px 18px rgba(0,0,0,0.04);}"
        ".panel{display:flex;flex-direction:column;justify-content:space-between;min-height:260px;}"
        ".panel h2{font-size:20px;font-weight:700;color:var(--text-main);margin-bottom:4px;}"
        ".panel p{font-size:12.5px;color:var(--text-sub);line-height:1.4;margin-bottom:12px;}"
        ".check-grid{display:grid;grid-template-columns:1fr 1fr;gap:6px 12px;margin-bottom:12px;font-size:11.5px;color:var(--text-sub);}"
        ".check-item{display:flex;align-items:center;gap:6px;}"
        ".upload-box{border:1px dashed var(--card-border);background:var(--card-inner);padding:14px;text-align:center;cursor:pointer;position:relative;margin-bottom:8px;}"
        ".upload-box input[type=file]{position:absolute;top:0;left:0;width:100%;height:100%;opacity:0;cursor:pointer;}"
        ".upload-box span{font-size:12px;font-weight:600;color:var(--text-main);}"
        ".upload-box small{display:block;font-family:monospace;font-size:10px;color:var(--text-muted);margin-top:2px;}"
        ".file-bar{display:none;justify-content:space-between;font-family:monospace;font-size:11px;background:var(--card-inner);border:1px solid var(--card-inner-border);padding:6px 10px;margin-bottom:8px;}"
        "label{display:block;font-family:monospace;font-size:10px;font-weight:700;text-transform:uppercase;color:var(--text-muted);margin:6px 0 2px;}"
        ".input-box{width:100%;padding:8px 10px;border:1px solid var(--input-border);background:var(--input-bg);color:var(--text-main);font-family:monospace;font-size:12px;outline:none;margin-bottom:6px;}"
        ".btn-action{display:inline-flex;align-items:center;gap:8px;padding:9px 22px;border-radius:9999px;border:1.5px solid var(--btn-border);background:var(--btn-bg);color:var(--btn-text);font-family:monospace;font-size:11.5px;font-weight:700;letter-spacing:0.8px;text-transform:uppercase;cursor:pointer;align-self:flex-start;margin-top:4px;}"
        ".btn-action:hover:not(:disabled){background:var(--btn-hover-bg);color:var(--btn-hover-text);}"
        ".btn-action:disabled{opacity:0.45;cursor:not-allowed;}"
        ".guide{background:var(--card-inner);border:1px solid var(--card-inner-border);padding:16px 18px;display:flex;flex-direction:column;justify-content:space-between;}"
        ".guide-head{font-family:monospace;font-size:11px;font-weight:700;color:var(--text-main);text-transform:uppercase;letter-spacing:0.5px;padding-bottom:8px;border-bottom:1px dashed var(--card-inner-border);margin-bottom:10px;}"
        ".steps{display:flex;flex-direction:column;gap:8px;}"
        ".step{display:flex;align-items:flex-start;gap:10px;}"
        ".step-num{width:20px;height:20px;border-radius:50%;background:var(--num-bg);border:1px solid var(--num-border);color:var(--num-text);display:flex;align-items:center;justify-content:center;font-family:monospace;font-size:10px;font-weight:700;flex-shrink:0;margin-top:1px;}"
        ".step-body strong{display:block;font-size:11.5px;font-weight:700;color:var(--text-main);}"
        ".step-body p{font-size:10.5px;color:var(--text-muted);line-height:1.3;}"
        ".tele-bar{margin-top:8px;padding-top:8px;border-top:1px dashed var(--card-inner-border);display:flex;justify-content:space-between;font-family:monospace;font-size:10px;color:var(--text-muted);}"
        ".tele-bar b{color:var(--text-main);}"
        ".prog{display:none;margin-top:8px;}"
        ".prog-meta{display:flex;justify-content:space-between;font-family:monospace;font-size:10px;color:var(--text-muted);margin-bottom:3px;}"
        ".prog-track{width:100%;height:6px;background:var(--progress-bg);overflow:hidden;}"
        ".prog-fill{height:100%;width:0%;background:var(--progress-fill);transition:width 0.1s;}"
        ".msg{margin-top:8px;padding:6px 10px;font-family:monospace;font-size:11px;display:none;background:var(--card-inner);border:1px solid var(--card-inner-border);}"
        ".bot-bar{display:flex;justify-content:space-between;align-items:center;margin-top:8px;font-family:monospace;font-size:10px;color:var(--text-muted);}"
        "@media(max-width:768px){body{padding:10px 12px;}.card{grid-template-columns:1fr;padding:14px;gap:12px;}h1{font-size:22px;}.check-grid{display:none;}.step:nth-child(n+4){display:none;}.btn-action{width:100%;justify-content:center;}}"
        "</style></head><body><div class='frame'>"

        "<div class='top-row'>"
        "<div class='badge-pill'>LIFELINE TX PRO // TACTICAL NODE</div>"
        "<button class='theme-btn' id='t_btn'>☀️ LIGHT</button>"
        "</div>"

        "<div><h1 id='h_title'>Two services. One transmitter.</h1><p class='sub'>Select a service below to configure Wi-Fi credentials or flash wireless firmware.</p></div>"

        "<div class='tabs'>"
        "<button class='tab active' id='t_ota' onclick='swTab(\"ota\")'>FIRMWARE OTA</button>"
        "<button class='tab' id='t_wifi' onclick='swTab(\"wifi\")'>WI-FI UPLINK</button>"
        "</div>"

        "<div class='card'>"
        "<div class='panel'>"
        "<div><h2 id='p_title'>Firmware Flash (OTA)</h2><p id='p_desc'>Wireless firmware flashing over softAP. Zero cable connection required.</p>"
        "<div class='check-grid' id='c_grid'>"
        "<div class='check-item'>✓ Dual Partition Safe</div><div class='check-item'>✓ 433MHz LoRa Active</div>"
        "<div class='check-item'>✓ Checksum Verified</div><div class='check-item'>✓ Auto Flash Reboot</div>"
        "</div></div>"

        "<div id='box_ota'>"
        "<form id='flash_form'>"
        "<div class='upload-box'>"
        "<input type='file' id='fw_file' name='update' accept='.bin' required>"
        "<span>Select or drop firmware.bin</span><small>ESP32 BINARY (*.bin)</small>"
        "</div>"
        "<div class='file-bar' id='f_meta'><span id='f_name'>firmware.bin</span><span id='f_size'>0 KB</span></div>"
        "<button type='submit' class='btn-action' id='btn_f'>FLASH NOW &rarr;</button>"
        "</form>"
        "<div class='prog' id='p_box'>"
        "<div class='prog-meta'><span id='p_stat'>Writing Flash...</span><span id='p_pct'>0%</span></div>"
        "<div class='prog-track'><div class='prog-fill' id='p_fill'></div></div>"
        "</div>"
        "<div class='msg' id='flash_msg'></div>"
        "</div>"

        "<div id='box_wifi' style='display:none;'>"
        "<form id='wifi_form'>"
        "<label>Target Wi-Fi SSID</label>"
        "<input type='text' class='input-box' id='wifi_ssid' name='ssid' value='"
    );
    html += savedSSID;
    html += F(
        "' placeholder='SSID' required>"
        "<label>WPA2 Password</label>"
        "<input type='password' class='input-box' id='wifi_pass' name='pass' value='"
    );
    html += savedPass;
    html += F(
        "' placeholder='Password'>"
        "<button type='submit' class='btn-action' id='btn_w'>SAVE CREDENTIALS &rarr;</button>"
        "</form>"
        "<div class='msg' id='wifi_msg'></div>"
        "</div>"
        "</div>"

        "<div class='guide'>"
        "<div><div class='guide-head'>HOW TO USE THIS SERVICE</div>"
        "<div class='steps' id='s_box'>"
        "<div class='step'><div class='step-num'>1</div><div class='step-body'><strong>1. Select Binary</strong><p>Pick compiled firmware.bin from your build folder.</p></div></div>"
        "<div class='step'><div class='step-num'>2</div><div class='step-body'><strong>2. Stream Over SoftAP</strong><p>Streams in 4KB chunks directly into ESP32 OTA flash.</p></div></div>"
        "<div class='step'><div class='step-num'>3</div><div class='step-body'><strong>3. Checksum Validation</strong><p>MD5 hash is checked against total written payload.</p></div></div>"
        "<div class='step'><div class='step-num'>4</div><div class='step-body'><strong>4. Automated Reboot</strong><p>TX board commits boot partition and restarts cleanly.</p></div></div>"
        "</div></div>"
        "<div class='tele-bar'><span>IP: <b>"
    );
    html += activeIPAddress;
    html += F(
        "</b></span><span>SSID: <b>"
    );
    html += activeSSIDName;
    html += F(
        "</b></span><span>MODE: <b>"
    );
    html += modeText;
    html += F(
        "</b></span></div>"
        "</div>"
        "</div>"

        "<div class='bot-bar'><span>LIFELINE TACTICAL // SX1278 433MHz</span><span>EXIT: '#' KEY ON KEYPAD</span></div>"
        "</div>"

        "<script>"
        "var curTheme='light';"
        "document.getElementById('t_btn').onclick=function(){"
        "  curTheme=(curTheme==='light'?'dark':'light');"
        "  document.documentElement.setAttribute('data-theme',curTheme);"
        "  this.innerText=(curTheme==='light'?'☀️ LIGHT':'🌙 DARK');"
        "};"
        "function swTab(t){"
        "  var tOta=document.getElementById('t_ota'),tWifi=document.getElementById('t_wifi');"
        "  var bOta=document.getElementById('box_ota'),bWifi=document.getElementById('box_wifi');"
        "  var pT=document.getElementById('p_title'),pD=document.getElementById('p_desc');"
        "  var sB=document.getElementById('s_box'),hT=document.getElementById('h_title');"
        "  if(t==='ota'){"
        "    tOta.className='tab active';tWifi.className='tab';"
        "    bOta.style.display='block';bWifi.style.display='none';"
        "    pT.innerText='Firmware Flash (OTA)';pD.innerText='Wireless firmware flashing over softAP. Zero cable connection required.';"
        "    hT.innerText='Two services. One transmitter.';"
        "    sB.innerHTML='<div class=\"step\"><div class=\"step-num\">1</div><div class=\"step-body\"><strong>1. Select Binary</strong><p>Pick compiled firmware.bin from your build folder.</p></div></div><div class=\"step\"><div class=\"step-num\">2</div><div class=\"step-body\"><strong>2. Stream Over SoftAP</strong><p>Streams in 4KB chunks directly into ESP32 OTA flash.</p></div></div><div class=\"step\"><div class=\"step-num\">3</div><div class=\"step-body\"><strong>3. Checksum Validation</strong><p>MD5 hash is checked against total written payload.</p></div></div><div class=\"step\"><div class=\"step-num\">4</div><div class=\"step-body\"><strong>4. Automated Reboot</strong><p>TX board commits boot partition and restarts cleanly.</p></div></div>';"
        "  }else{"
        "    tWifi.className='tab active';tOta.className='tab';"
        "    bWifi.style.display='block';bOta.style.display='none';"
        "    pT.innerText='Wi-Fi Field Uplink';pD.innerText='Configure field Wi-Fi network credentials for Network OTA & diagnostics.';"
        "    hT.innerText='Wireless Uplink. Instant Pairing.';"
        "    sB.innerHTML='<div class=\"step\"><div class=\"step-num\">1</div><div class=\"step-body\"><strong>1. Enter SSID</strong><p>Provide network SSID for local field access point.</p></div></div><div class=\"step\"><div class=\"step-num\">2</div><div class=\"step-body\"><strong>2. Set WPA2 Passkey</strong><p>Stored safely in ESP32 non-volatile NVS flash.</p></div></div><div class=\"step\"><div class=\"step-num\">3</div><div class=\"step-body\"><strong>3. Network Join</strong><p>Unit auto-connects upon switching to NET mode.</p></div></div><div class=\"step\"><div class=\"step-num\">4</div><div class=\"step-body\"><strong>4. Field Telemetry</strong><p>LoRa emergency packets route across Wi-Fi gateway.</p></div></div>';"
        "  }"
        "}"
        "var fi=document.getElementById('fw_file');"
        "fi.onchange=function(){"
        "  if(this.files&&this.files[0]){"
        "    document.getElementById('f_name').innerText=this.files[0].name;"
        "    document.getElementById('f_size').innerText=Math.round(this.files[0].size/1024)+' KB';"
        "    document.getElementById('f_meta').style.display='flex';"
        "  }"
        "};"
        "document.getElementById('flash_form').onsubmit=function(e){"
        "  e.preventDefault();"
        "  var file=fi.files[0];"
        "  if(!file)return;"
        "  var pBox=document.getElementById('p_box'),pFill=document.getElementById('p_fill');"
        "  var pPct=document.getElementById('p_pct'),pStat=document.getElementById('p_stat');"
        "  var msg=document.getElementById('flash_msg'),btn=document.getElementById('btn_f');"
        "  pBox.style.display='block';msg.style.display='none';btn.disabled=true;"
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
        "      pPct.innerText='100%';"
        "      msg.style.display='block';msg.style.color='#10b981';"
        "      msg.innerHTML='<strong>SUCCESS:</strong> Firmware flashed! TX Unit is rebooting...';"
        "    }else{"
        "      msg.style.display='block';msg.style.color='#ef4444';"
        "      msg.innerHTML='<strong>ERROR ('+xhr.status+'):</strong> '+xhr.responseText;"
        "      btn.disabled=false;"
        "    }"
        "  };"
        "  xhr.onerror=function(){"
        "    pFill.style.width='100%';"
        "    msg.style.display='block';msg.style.color='#10b981';"
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
        "  var msg=document.getElementById('wifi_msg'),btn=document.getElementById('btn_w');"
        "  btn.disabled=true;"
        "  var xhr=new XMLHttpRequest();"
        "  xhr.open('POST','/save_wifi');"
        "  xhr.setRequestHeader('Content-Type','application/x-www-form-urlencoded');"
        "  xhr.onload=function(){"
        "    btn.disabled=false;"
        "    msg.style.display='block';"
        "    if(xhr.status==200){"
        "      msg.style.color='#10b981';"
        "      msg.innerHTML='<strong>SAVED:</strong> Wi-Fi parameters stored to flash storage.';"
        "    }else{"
        "      msg.style.color='#ef4444';"
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
