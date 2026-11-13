#include "OTAManager.h"
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <ArduinoOTA.h>
#include <Preferences.h>
#include "Config.h"
#include "DisplayUI.h"
#include "BuzzerLED.h"

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
        ".theme-btn{display:inline-flex;align-items:center;gap:6px;padding:5px 12px;border-radius:9999px;border:1px solid var(--badge-border);background:var(--badge-bg);color:var(--badge-text);font-size:11px;font-weight:600;cursor:pointer;}"
        ".theme-icon-svg{display:block;width:13px;height:13px;stroke:currentColor;}"
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
        ".buzzer-power-row{display:grid;grid-template-columns:1fr 1fr;gap:6px;margin-bottom:6px;}"
        ".btn-power-on{padding:7px 10px;border:1px solid var(--card-border);background:#10b981;color:#ffffff;font-family:monospace;font-size:10px;font-weight:700;letter-spacing:0.5px;text-transform:uppercase;cursor:pointer;display:flex;align-items:center;justify-content:center;gap:4px;}"
        ".btn-power-off{padding:7px 10px;border:1px solid var(--card-border);background:#ef4444;color:#ffffff;font-family:monospace;font-size:10px;font-weight:700;letter-spacing:0.5px;text-transform:uppercase;cursor:pointer;display:flex;align-items:center;justify-content:center;gap:4px;}"
        ".slider-container{margin-bottom:6px;background:var(--card-inner);border:1px solid var(--card-inner-border);padding:5px 8px;}"
        ".slider-header{display:flex;justify-content:space-between;align-items:center;font-family:monospace;font-size:9.5px;margin-bottom:3px;}"
        ".slider-header span{font-weight:700;color:var(--text-muted);text-transform:uppercase;}"
        ".slider-header b{color:var(--text-main);font-size:10.5px;}"
        ".range-slider{width:100%;height:5px;background:var(--progress-bg);outline:none;-webkit-appearance:none;cursor:pointer;margin-bottom:4px;}"
        ".range-slider::-webkit-slider-thumb{-webkit-appearance:none;width:12px;height:12px;border-radius:50%;background:var(--text-main);cursor:pointer;}"
        ".chip-row{display:flex;flex-wrap:wrap;gap:4px;}"
        ".chip-btn{padding:2px 6px;border:1px solid var(--tab-border);background:var(--tab-bg);color:var(--tab-text);font-family:monospace;font-size:9px;font-weight:600;cursor:pointer;border-radius:9999px;}"
        ".chip-btn.active{background:var(--tab-active-bg);color:var(--tab-active-text);border-color:var(--tab-active-border);}"
        ".melodies-row{display:grid;grid-template-columns:repeat(4,1fr);gap:4px;margin-bottom:6px;}"
        ".btn-melody{padding:4px 2px;border:1px solid var(--tab-border);background:var(--tab-bg);color:var(--tab-text);font-family:monospace;font-size:8.5px;font-weight:700;text-align:center;cursor:pointer;text-transform:uppercase;}"
        ".btn-melody:hover{background:var(--tab-active-bg);color:var(--tab-active-text);}"
        ".acoustic-state-bar{display:flex;justify-content:space-between;align-items:center;background:var(--card-inner);border:1px solid var(--card-inner-border);padding:4px 6px;margin-bottom:6px;font-family:monospace;font-size:9.5px;}"
        ".wave-anim{display:inline-flex;align-items:center;gap:2px;height:10px;}"
        ".wave-bar{width:2px;height:3px;background:var(--text-muted);}"
        ".wave-anim.active .wave-bar{background:#10b981;animation:wavePulse 0.4s infinite alternate ease-in-out;}"
        ".wave-anim.active .wave-bar:nth-child(2){animation-delay:0.1s;}"
        ".wave-anim.active .wave-bar:nth-child(3){animation-delay:0.2s;}"
        ".wave-anim.active .wave-bar:nth-child(4){animation-delay:0.3s;}"
        "@keyframes wavePulse{0%{height:3px;}100%{height:10px;}}"
        "@media(max-width:768px){body{padding:10px 12px;}.card{grid-template-columns:1fr;padding:14px;gap:12px;}h1{font-size:22px;}.check-grid{display:none;}.step:nth-child(n+4){display:none;}.btn-action{width:100%;justify-content:center;}}"
        "</style></head><body><div class='frame'>"

        "<div class='top-row'>"
        "<div class='badge-pill'>LIFELINE TX PRO // TACTICAL NODE</div>"
        "<button class='theme-btn' id='t_btn'><span id='t_icon'><svg class='theme-icon-svg' width='13' height='13' viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2.2' stroke-linecap='round' stroke-linejoin='round'><circle cx='12' cy='12' r='5'></circle><line x1='12' y1='1' x2='12' y2='3'></line><line x1='12' y1='21' x2='12' y2='23'></line><line x1='4.22' y1='4.22' x2='5.64' y2='5.64'></line><line x1='18.36' y1='18.36' x2='19.78' y2='19.78'></line><line x1='1' y1='12' x2='3' y2='12'></line><line x1='21' y1='12' x2='23' y2='12'></line><line x1='4.22' y1='19.78' x2='5.64' y2='18.36'></line><line x1='18.36' y1='5.64' x2='19.78' y2='4.22'></line></svg></span><span id='t_txt'>LIGHT</span></button>"
        "</div>"

        "<div><h1 id='h_title'>Three services. One transmitter.</h1><p class='sub'>Select a service below to configure Wi-Fi credentials, flash wireless firmware, or control the buzzer sounder.</p></div>"

        "<div class='tabs'>"
        "<button class='tab active' id='t_ota' onclick='swTab(\"ota\")'>FIRMWARE OTA</button>"
        "<button class='tab' id='t_wifi' onclick='swTab(\"wifi\")'>WI-FI UPLINK</button>"
        "<button class='tab' id='t_buzz' onclick='swTab(\"buzz\")'>BUZZER CONTROL</button>"
        "</div>"

        "<div class='card'>"
        "<div class='panel'>"
        "<div><h2 id='p_title'>Firmware Flash (OTA)</h2><p id='p_desc'>Wireless firmware flashing over softAP. Zero cable connection required.</p>"
        "<div class='check-grid' id='c_grid'>"
        "<div class='check-item'><svg width='12' height='12' viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='3'><polyline points='20 6 9 17 4 12'></polyline></svg> Dual Partition Safe</div>"
        "<div class='check-item'><svg width='12' height='12' viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='3'><polyline points='20 6 9 17 4 12'></polyline></svg> 433MHz LoRa Active</div>"
        "<div class='check-item'><svg width='12' height='12' viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='3'><polyline points='20 6 9 17 4 12'></polyline></svg> Checksum Verified</div>"
        "<div class='check-item'><svg width='12' height='12' viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='3'><polyline points='20 6 9 17 4 12'></polyline></svg> Auto Flash Reboot</div>"
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

        "<div id='box_buzz' style='display:none;'>"
        "<div class='buzzer-power-row'>"
        "<button type='button' class='btn-power-on' onclick='sendBuzzer(\"on\")'>TURN ON (CONT)</button>"
        "<button type='button' class='btn-power-off' onclick='sendBuzzer(\"off\")'>TURN OFF / MUTE</button>"
        "</div>"
        "<div class='acoustic-state-bar'>"
        "<span>STATUS: <b id='b_stat_txt'>SILENT</b></span>"
        "<div class='wave-anim' id='b_wave'><div class='wave-bar'></div><div class='wave-bar'></div><div class='wave-bar'></div><div class='wave-bar'></div></div>"
        "</div>"
        "<div class='slider-container'>"
        "<div class='slider-header'><span>Frequency (Tone Pitch)</span><b id='f_disp'>2000 Hz</b></div>"
        "<input type='range' class='range-slider' id='b_freq' min='200' max='5000' step='50' value='2000' oninput='upFreq(this.value)'>"
        "<div class='chip-row' id='f_chips'>"
        "<button type='button' class='chip-btn' onclick='sqFreq(800)'>800Hz</button>"
        "<button type='button' class='chip-btn' onclick='sqFreq(1500)'>1500Hz</button>"
        "<button type='button' class='chip-btn active' onclick='sqFreq(2000)'>2000Hz</button>"
        "<button type='button' class='chip-btn' onclick='sqFreq(2500)'>2500Hz</button>"
        "<button type='button' class='chip-btn' onclick='sqFreq(3200)'>3200Hz</button>"
        "</div>"
        "</div>"
        "<div class='slider-container'>"
        "<div class='slider-header'><span>Duration</span><b id='d_disp'>500 ms</b></div>"
        "<input type='range' class='range-slider' id='b_dur' min='20' max='5000' step='20' value='500' oninput='upDur(this.value)'>"
        "<div class='chip-row' id='d_chips'>"
        "<button type='button' class='chip-btn' onclick='sqDur(50)'>50ms</button>"
        "<button type='button' class='chip-btn' onclick='sqDur(100)'>100ms</button>"
        "<button type='button' class='chip-btn' onclick='sqDur(250)'>250ms</button>"
        "<button type='button' class='chip-btn active' onclick='sqDur(500)'>500ms</button>"
        "<button type='button' class='chip-btn' onclick='sqDur(1000)'>1.0s</button>"
        "<button type='button' class='chip-btn' onclick='sqDur(2500)'>2.5s</button>"
        "</div>"
        "</div>"
        "<label>Tactical Melodies</label>"
        "<div class='melodies-row'>"
        "<button type='button' class='btn-melody' onclick='sendBuzzer(\"success\")'>Success</button>"
        "<button type='button' class='btn-melody' onclick='sendBuzzer(\"error\")'>Warning</button>"
        "<button type='button' class='btn-melody' onclick='sendBuzzer(\"siren\")'>Siren</button>"
        "<button type='button' class='btn-melody' onclick='sendBuzzer(\"sos\")'>SOS Alarm</button>"
        "</div>"
        "<button type='button' class='btn-action' onclick='sendBuzzer(\"beep\")'>TEST BEEP (PULSE) &rarr;</button>"
        "<div class='msg' id='buzz_msg'></div>"
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
        "var sSun=\"<svg class='theme-icon-svg' width='13' height='13' viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2.2' stroke-linecap='round' stroke-linejoin='round'><circle cx='12' cy='12' r='5'></circle><line x1='12' y1='1' x2='12' y2='3'></line><line x1='12' y1='21' x2='12' y2='23'></line><line x1='4.22' y1='4.22' x2='5.64' y2='5.64'></line><line x1='18.36' y1='18.36' x2='19.78' y2='19.78'></line><line x1='1' y1='12' x2='3' y2='12'></line><line x1='21' y1='12' x2='23' y2='12'></line><line x1='4.22' y1='19.78' x2='5.64' y2='18.36'></line><line x1='18.36' y1='5.64' x2='19.78' y2='4.22'></line></svg>\";"
        "var sMoon=\"<svg class='theme-icon-svg' width='13' height='13' viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2.2' stroke-linecap='round' stroke-linejoin='round'><path d='M21 12.79A9 9 0 1 1 11.21 3 7 7 0 0 0 21 12.79z'></path></svg>\";"
        "document.getElementById('t_btn').onclick=function(){"
        "  curTheme=(curTheme==='light'?'dark':'light');"
        "  document.documentElement.setAttribute('data-theme',curTheme);"
        "  document.getElementById('t_icon').innerHTML=(curTheme==='light'?sSun:sMoon);"
        "  document.getElementById('t_txt').innerText=(curTheme==='light'?'LIGHT':'DARK');"
        "};"
        "function swTab(t){"
        "  var tOta=document.getElementById('t_ota'),tWifi=document.getElementById('t_wifi'),tBuzz=document.getElementById('t_buzz');"
        "  var bOta=document.getElementById('box_ota'),bWifi=document.getElementById('box_wifi'),bBuzz=document.getElementById('box_buzz');"
        "  var pT=document.getElementById('p_title'),pD=document.getElementById('p_desc');"
        "  var sB=document.getElementById('s_box'),hT=document.getElementById('h_title'),cG=document.getElementById('c_grid');"
        "  if(t==='ota'){"
        "    tOta.className='tab active';tWifi.className='tab';tBuzz.className='tab';"
        "    bOta.style.display='block';bWifi.style.display='none';bBuzz.style.display='none';"
        "    pT.innerText='Firmware Flash (OTA)';pD.innerText='Wireless firmware flashing over softAP. Zero cable connection required.';"
        "    hT.innerText='Three services. One transmitter.';"
        "    cG.innerHTML='<div class=\"check-item\"><svg width=\"12\" height=\"12\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"3\"><polyline points=\"20 6 9 17 4 12\"></polyline></svg> Dual Partition Safe</div><div class=\"check-item\"><svg width=\"12\" height=\"12\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"3\"><polyline points=\"20 6 9 17 4 12\"></polyline></svg> 433MHz LoRa Active</div><div class=\"check-item\"><svg width=\"12\" height=\"12\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"3\"><polyline points=\"20 6 9 17 4 12\"></polyline></svg> Checksum Verified</div><div class=\"check-item\"><svg width=\"12\" height=\"12\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"3\"><polyline points=\"20 6 9 17 4 12\"></polyline></svg> Auto Flash Reboot</div>';"
        "    sB.innerHTML='<div class=\"step\"><div class=\"step-num\">1</div><div class=\"step-body\"><strong>1. Select Binary</strong><p>Pick compiled firmware.bin from your build folder.</p></div></div><div class=\"step\"><div class=\"step-num\">2</div><div class=\"step-body\"><strong>2. Stream Over SoftAP</strong><p>Streams in 4KB chunks directly into ESP32 OTA flash.</p></div></div><div class=\"step\"><div class=\"step-num\">3</div><div class=\"step-body\"><strong>3. Checksum Validation</strong><p>MD5 hash is checked against total written payload.</p></div></div><div class=\"step\"><div class=\"step-num\">4</div><div class=\"step-body\"><strong>4. Automated Reboot</strong><p>TX board commits boot partition and restarts cleanly.</p></div></div>';"
        "  }else if(t==='wifi'){"
        "    tWifi.className='tab active';tOta.className='tab';tBuzz.className='tab';"
        "    bWifi.style.display='block';bOta.style.display='none';bBuzz.style.display='none';"
        "    pT.innerText='Wi-Fi Field Uplink';pD.innerText='Configure field Wi-Fi network credentials for Network OTA & diagnostics.';"
        "    hT.innerText='Wireless Uplink. Instant Pairing.';"
        "    cG.innerHTML='<div class=\"check-item\"><svg width=\"12\" height=\"12\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"3\"><polyline points=\"20 6 9 17 4 12\"></polyline></svg> WPA2-PSK Support</div><div class=\"check-item\"><svg width=\"12\" height=\"12\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"3\"><polyline points=\"20 6 9 17 4 12\"></polyline></svg> NVS Non-Volatile</div><div class=\"check-item\"><svg width=\"12\" height=\"12\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"3\"><polyline points=\"20 6 9 17 4 12\"></polyline></svg> Auto Network Join</div><div class=\"check-item\"><svg width=\"12\" height=\"12\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"3\"><polyline points=\"20 6 9 17 4 12\"></polyline></svg> Sub-GHz Socket Sync</div>';"
        "    sB.innerHTML='<div class=\"step\"><div class=\"step-num\">1</div><div class=\"step-body\"><strong>1. Enter SSID</strong><p>Provide network SSID for local field access point.</p></div></div><div class=\"step\"><div class=\"step-num\">2</div><div class=\"step-body\"><strong>2. Set WPA2 Passkey</strong><p>Stored safely in ESP32 non-volatile NVS flash.</p></div></div><div class=\"step\"><div class=\"step-num\">3</div><div class=\"step-body\"><strong>3. Network Join</strong><p>Unit auto-connects upon switching to NET mode.</p></div></div><div class=\"step\"><div class=\"step-num\">4</div><div class=\"step-body\"><strong>4. Field Telemetry</strong><p>LoRa emergency packets route across Wi-Fi gateway.</p></div></div>';"
        "  }else{"
        "    tBuzz.className='tab active';tOta.className='tab';tWifi.className='tab';"
        "    bBuzz.style.display='block';bOta.style.display='none';bWifi.style.display='none';"
        "    pT.innerText='Tactical Buzzer Sounder';pD.innerText='Test acoustic audio driver, modulate tone frequencies, and set pulse duration.';"
        "    hT.innerText='Tactical Sounder. Acoustic Telemetry.';"
        "    cG.innerHTML='<div class=\"check-item\"><svg width=\"12\" height=\"12\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"3\"><polyline points=\"20 6 9 17 4 12\"></polyline></svg> GPIO 22 Piezo Driver</div><div class=\"check-item\"><svg width=\"12\" height=\"12\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"3\"><polyline points=\"20 6 9 17 4 12\"></polyline></svg> 200Hz - 5000Hz PWM</div><div class=\"check-item\"><svg width=\"12\" height=\"12\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"3\"><polyline points=\"20 6 9 17 4 12\"></polyline></svg> Timed Pulse Duration</div><div class=\"check-item\"><svg width=\"12\" height=\"12\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"3\"><polyline points=\"20 6 9 17 4 12\"></polyline></svg> Web Audio Live Sync</div>';"
        "    sB.innerHTML='<div class=\"step\"><div class=\"step-num\">1</div><div class=\"step-body\"><strong>1. Choose Tone Frequency</strong><p>Tune frequency slider (200Hz-5000Hz) or pick presets like 800Hz Warn or 3200Hz Siren.</p></div></div><div class=\"step\"><div class=\"step-num\">2</div><div class=\"step-body\"><strong>2. Set Pulse Duration</strong><p>Adjust pulse duration from 50ms up to 5000ms for single acoustic alerts.</p></div></div><div class=\"step\"><div class=\"step-num\">3</div><div class=\"step-body\"><strong>3. Test Emission &amp; Melodies</strong><p>Click TEST BEEP or select tactical melodies like SOS Morse Code &amp; Siren Warble.</p></div></div><div class=\"step\"><div class=\"step-num\">4</div><div class=\"step-body\"><strong>4. Sustained Search Beacon</strong><p>Engage TURN ON (CONT) for rescue teams to track the TX unit sound in smoke/darkness.</p></div></div>';"
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
        "var aCtx=null,aOsc=null,aGain=null,aPlay=false,aTimer=null;"
        "function getACtx(){if(!aCtx){var AC=window.AudioContext||window.webkitAudioContext;if(AC)aCtx=new AC();}if(aCtx&&aCtx.state==='suspended')aCtx.resume();return aCtx;}"
        "function setVisBuzzer(on,f){"
        "  var txt=document.getElementById('b_stat_txt'),wav=document.getElementById('b_wave');"
        "  if(on){txt.innerText='ACTIVE ('+f+' Hz)';txt.style.color='#10b981';wav.className='wave-anim active';}"
        "  else{txt.innerText='SILENT';txt.style.color='var(--text-main)';wav.className='wave-anim';}"
        "}"
        "function playToneWeb(f,d){"
        "  var ctx=getACtx();if(!ctx)return;stopToneWeb();"
        "  try{"
        "    var osc=ctx.createOscillator(),gain=ctx.createGain();"
        "    osc.type='square';osc.frequency.setValueAtTime(f,ctx.currentTime);"
        "    gain.gain.setValueAtTime(0.001,ctx.currentTime);gain.gain.exponentialRampToValueAtTime(0.18,ctx.currentTime+0.01);"
        "    osc.connect(gain);gain.connect(ctx.destination);osc.start();"
        "    aOsc=osc;aGain=gain;aPlay=true;setVisBuzzer(true,f);"
        "    if(d&&d>0){"
        "      gain.gain.setValueAtTime(0.18,ctx.currentTime+(d/1000)-0.01);"
        "      gain.gain.exponentialRampToValueAtTime(0.001,ctx.currentTime+(d/1000));"
        "      aTimer=setTimeout(stopToneWeb,d);"
        "    }"
        "  }catch(e){}"
        "}"
        "function stopToneWeb(){"
        "  if(aTimer){clearTimeout(aTimer);aTimer=null;}"
        "  if(aGain&&aCtx){try{aGain.gain.setValueAtTime(0.001,aCtx.currentTime);aGain.disconnect();}catch(e){}aGain=null;}"
        "  if(aOsc){try{aOsc.stop();aOsc.disconnect();}catch(e){}aOsc=null;}"
        "  aPlay=false;setVisBuzzer(false);"
        "}"
        "function upFreq(v){document.getElementById('f_disp').innerText=v+' Hz';var c=document.querySelectorAll('#f_chips .chip-btn');c.forEach(function(el){el.className=(el.innerText.indexOf(v+'Hz')===0?'chip-btn active':'chip-btn');});if(aPlay&&aOsc)aOsc.frequency.setValueAtTime(v,aCtx.currentTime);}"
        "function sqFreq(v){document.getElementById('b_freq').value=v;upFreq(v);}"
        "function upDur(v){document.getElementById('d_disp').innerText=v+' ms';var c=document.querySelectorAll('#d_chips .chip-btn');c.forEach(function(el){var l=(v>=1000?(v/1000).toFixed(1)+'s':v+'ms');el.className=(el.innerText===l||el.innerText===v+'ms'?'chip-btn active':'chip-btn');});}"
        "function sqDur(v){document.getElementById('b_dur').value=v;upDur(v);}"
        "function sendBuzzer(action){"
        "  var f=parseInt(document.getElementById('b_freq').value,10)||2000;"
        "  var d=parseInt(document.getElementById('b_dur').value,10)||500;"
        "  if(action==='on')playToneWeb(f,0);"
        "  else if(action==='off')stopToneWeb();"
        "  else if(action==='beep')playToneWeb(f,d);"
        "  else if(action==='success'){playToneWeb(2200,80);setTimeout(function(){playToneWeb(2700,100);},110);}"
        "  else if(action==='error'){playToneWeb(1200,100);setTimeout(function(){playToneWeb(800,150);},130);}"
        "  else if(action==='siren'){var sf=1600,st=200,sc=0;playToneWeb(sf,40);var stm=setInterval(function(){sf+=st;if(sf>=3200)st=-200;if(sf<=1600)st=200;playToneWeb(sf,35);sc++;if(sc>20){clearInterval(stm);setTimeout(stopToneWeb,100);}},40);}"
        "  else if(action==='sos'){var seq=[{f:2400,d:90,p:60},{f:2400,d:90,p:60},{f:2400,d:90,p:140},{f:2400,d:250,p:60},{f:2400,d:250,p:60},{f:2400,d:250,p:140},{f:2400,d:90,p:60},{f:2400,d:90,p:60},{f:2400,d:90,p:60}];var off=0;seq.forEach(function(it){setTimeout(function(){playToneWeb(it.f,it.d);},off);off+=it.d+it.p;});}"
        "  var msg=document.getElementById('buzz_msg');msg.style.display='block';msg.style.color='#10b981';msg.innerHTML='<strong>TRANSMITTING:</strong> ['+action.toUpperCase()+']...';"
        "  var xhr=new XMLHttpRequest();xhr.open('POST','/buzzer');xhr.setRequestHeader('Content-Type','application/x-www-form-urlencoded');"
        "  xhr.onload=function(){if(xhr.status==200){msg.innerHTML='<strong>OK:</strong> Buzzer command executed.';}else{msg.innerHTML='<strong>LOCAL AUDIO:</strong> Synthesized ('+xhr.status+').';}};"
        "  xhr.onerror=function(){msg.innerHTML='<strong>LOCAL AUDIO:</strong> Synthesized.';};"
        "  xhr.send('action='+encodeURIComponent(action)+'&freq='+encodeURIComponent(f)+'&duration='+encodeURIComponent(d));"
        "}"
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

    server.on("/buzzer", HTTP_POST, []() {
        String action = server.hasArg("action") ? server.arg("action") : "beep";
        uint16_t freq = server.hasArg("freq") ? server.arg("freq").toInt() : 2000;
        uint32_t duration = server.hasArg("duration") ? server.arg("duration").toInt() : 500;

        if (freq < 100) freq = 2000;
        if (freq > 5000) freq = 5000;

        if (action.equalsIgnoreCase("on")) {
            setBuzzerState(true, freq);
        } else if (action.equalsIgnoreCase("off")) {
            stopBuzzer();
        } else if (action.equalsIgnoreCase("beep")) {
            triggerBuzzer(freq, duration);
        } else if (action.equalsIgnoreCase("siren")) {
            playSirenTone();
        } else if (action.equalsIgnoreCase("sos")) {
            playSosTone();
        } else if (action.equalsIgnoreCase("success")) {
            playSuccessTone();
        } else if (action.equalsIgnoreCase("error")) {
            playErrorTone();
        } else if (action.equalsIgnoreCase("confirm")) {
            playConfirmTone();
        } else {
            triggerBuzzer(freq, duration);
        }

        String json = "{\"status\":\"ok\",\"action\":\"" + action + "\",\"buzzerActive\":" + 
                      String(isBuzzerActive() ? "true" : "false") + 
                      ",\"freq\":" + String(getCurrentBuzzerFreq()) + 
                      ",\"duration\":" + String(duration) + "}";
        server.sendHeader("Access-Control-Allow-Origin", "*");
        server.send(200, "application/json", json);
    });

    server.on("/buzzer", HTTP_GET, []() {
        String json = "{\"status\":\"ok\",\"buzzerActive\":" + String(isBuzzerActive() ? "true" : "false") + 
                      ",\"freq\":" + String(getCurrentBuzzerFreq()) + "}";
        server.sendHeader("Access-Control-Allow-Origin", "*");
        server.send(200, "application/json", json);
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
    updateBuzzer();
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
