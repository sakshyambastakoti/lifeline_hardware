#include "web_server_manager.h"

WebServerManager webServerManager;

static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>SPU Serial Monitor Diagnostic Report</title>
    <style>
        :root {
            --bg-color: #06090e;
            --term-green: #00ff66;
            --term-cyan: #00e5ff;
            --term-amber: #ffb700;
            --term-red: #ff3333;
            --term-dim: #7a8b9e;
            --term-text: #e1f0ff;
            --term-border: #1a293d;
            --card-bg: #090e17;
        }

        * { margin: 0; padding: 0; box-sizing: border-box; font-family: 'Consolas', 'Fira Code', 'Courier New', monospace; }

        body {
            background-color: var(--bg-color);
            color: var(--term-text);
            padding: 1.5rem;
            min-height: 100vh;
            display: flex;
            flex-direction: column;
            align-items: center;
            line-height: 1.45;
        }

        /* CRT Scanline Overlay */
        .scanlines {
            position: fixed; top: 0; left: 0; width: 100vw; height: 100vh;
            pointer-events: none;
            background: linear-gradient(rgba(18, 16, 16, 0) 50%, rgba(0, 0, 0, 0.25) 50%);
            background-size: 100% 4px;
            z-index: 99;
            opacity: 0.6;
        }

        .terminal-container {
            width: 100%;
            max-width: 820px;
            background: var(--card-bg);
            border: 1px solid #1e3048;
            box-shadow: 0 0 25px rgba(0, 229, 255, 0.08), inset 0 0 15px rgba(0,0,0,0.8);
            border-radius: 8px;
            padding: 1.5rem;
            position: relative;
        }

        /* Retro Title Box */
        .title-box {
            border: 1px solid var(--term-cyan);
            padding: 0.6rem 1rem;
            text-align: center;
            font-weight: bold;
            font-size: 1.05rem;
            letter-spacing: 1px;
            color: var(--term-cyan);
            margin-bottom: 1.5rem;
            background: rgba(0, 229, 255, 0.04);
            box-shadow: 0 0 10px rgba(0, 229, 255, 0.1);
        }

        .status-bar {
            display: flex;
            justify-content: space-between;
            align-items: center;
            font-size: 0.85rem;
            color: var(--term-dim);
            border-bottom: 1px dashed var(--term-border);
            padding-bottom: 0.75rem;
            margin-bottom: 1.25rem;
            flex-wrap: wrap;
            gap: 0.5rem;
        }

        .status-item { display: flex; align-items: center; gap: 0.4rem; }
        .dot-green { width: 8px; height: 8px; border-radius: 50%; background: var(--term-green); box-shadow: 0 0 8px var(--term-green); }
        .dot-amber { width: 8px; height: 8px; border-radius: 50%; background: var(--term-amber); box-shadow: 0 0 8px var(--term-amber); }
        .dot-red { width: 8px; height: 8px; border-radius: 50%; background: var(--term-red); box-shadow: 0 0 8px var(--term-red); }

        /* Step Section Styling */
        .step-block { margin-bottom: 1.5rem; }
        .step-header {
            font-weight: bold;
            color: var(--term-cyan);
            font-size: 0.95rem;
            letter-spacing: 0.5px;
        }
        .step-divider {
            color: var(--term-dim);
            margin: 0.2rem 0 0.5rem 0;
            overflow: hidden;
            white-space: nowrap;
            opacity: 0.6;
        }
        .data-row {
            display: flex;
            font-size: 0.92rem;
            padding: 0.15rem 0;
        }
        .data-label {
            color: var(--term-text);
            white-space: pre;
            opacity: 0.9;
        }
        .data-value {
            font-weight: 600;
        }

        /* State colors */
        .val-ok { color: var(--term-green); }
        .val-warn { color: var(--term-amber); font-weight: bold; }
        .val-alert { color: var(--term-red); font-weight: bold; animation: blink 1.2s infinite; }
        .val-cyan { color: var(--term-cyan); }
        .val-dim { color: var(--term-dim); }

        @keyframes blink { 0%, 100% { opacity: 1; } 50% { opacity: 0.3; } }

        .cursor {
            display: inline-block;
            width: 9px;
            height: 1.1em;
            background: var(--term-cyan);
            vertical-align: text-bottom;
            animation: cursorBlink 0.8s infinite;
        }
        @keyframes cursorBlink { 0%, 100% { opacity: 1; } 50% { opacity: 0; } }

        /* Controls */
        .controls-bar {
            display: flex;
            justify-content: center;
            gap: 0.75rem;
            margin-top: 1.5rem;
            flex-wrap: wrap;
        }
        .btn-term {
            background: #0d1624;
            border: 1px solid var(--term-cyan);
            color: var(--term-cyan);
            padding: 0.4rem 0.8rem;
            font-size: 0.8rem;
            cursor: pointer;
            border-radius: 4px;
            transition: all 0.2s;
        }
        .btn-term:hover {
            background: var(--term-cyan);
            color: #000;
            box-shadow: 0 0 10px rgba(0,229,255,0.4);
        }

        footer {
            margin-top: 1.5rem;
            font-size: 0.78rem;
            color: var(--term-dim);
            text-align: center;
        }
    </style>
</head>
<body>

    <div class="scanlines" id="crt-overlay"></div>

    <div class="terminal-container">
        
        <div class="title-box">
            ┌─────────────────────────────────────────────────────────────┐<br>
            │       SPU STEP-BY-STEP SENSOR DIAGNOSTIC REPORT             │<br>
            └─────────────────────────────────────────────────────────────┘
        </div>

        <div class="status-bar">
            <div class="status-item">
                <span class="dot-green" id="status-dot"></span>
                <span>NODE ID: <strong id="node-id" class="val-cyan">SPU-#3</strong></span>
            </div>
            <div class="status-item">
                <span>UPTIME: <span id="uptime-val">0s</span></span>
            </div>
            <div class="status-item">
                <span>WIFI STA: <span id="wifi-sta-status" class="val-warn">DISCONNECTED</span></span>
            </div>
            <div class="status-item">
                <span>SERVER: <span id="server-status-val" class="val-dim">IDLE</span></span>
            </div>
        </div>

        <!-- [STEP 1/5] ENVIRONMENTAL SENSOR -->
        <div class="step-block">
            <div class="step-header">[STEP 1/5] ENVIRONMENTAL SENSOR (DHT11 / DHT22)</div>
            <div class="step-divider">-------------------------------------------------------------</div>
            <div class="data-row"><span class="data-label">  • Temperature       : </span><span class="data-value" id="dht-temp">-- °C</span></div>
            <div class="data-row"><span class="data-label">  • Humidity          : </span><span class="data-value" id="dht-hum">-- %</span></div>
            <div class="data-row"><span class="data-label">  • Sensor Status     : </span><span class="data-value" id="dht-status">--</span></div>
        </div>

        <!-- [STEP 2/5] AIR QUALITY & GAS SENSOR -->
        <div class="step-block">
            <div class="step-header">[STEP 2/5] AIR QUALITY & GAS SENSOR (MQ135)</div>
            <div class="step-divider">-------------------------------------------------------------</div>
            <div class="data-row"><span class="data-label">  • Gas Concentration : </span><span class="data-value" id="gas-ppm">-- PPM</span></div>
            <div class="data-row"><span class="data-label">  • Pollution Warning : </span><span class="data-value" id="gas-warn">--</span></div>
            <div class="data-row"><span class="data-label">  • Fire / Smoke Risk : </span><span class="data-value" id="gas-danger">--</span></div>
        </div>

        <!-- [STEP 3/5] MOTION & SEISMIC SENSOR -->
        <div class="step-block">
            <div class="step-header">[STEP 3/5] MOTION & SEISMIC SENSOR (MPU6050 6-DOF IMU)</div>
            <div class="step-divider">-------------------------------------------------------------</div>
            <div class="data-row"><span class="data-label">  • Raw Accel (X,Y,Z) : </span><span class="data-value" id="mpu-xyz">X=0.00G, Y=0.00G, Z=0.00G</span></div>
            <div class="data-row"><span class="data-label">  • Gyro Rate (X,Y,Z) : </span><span class="data-value" id="mpu-gyro">X=0.0°/s, Y=0.0°/s, Z=0.0°/s</span></div>
            <div class="data-row"><span class="data-label">  • Vector Magnitude  : </span><span class="data-value" id="mpu-mag">0.00 G</span></div>
            <div class="data-row"><span class="data-label">  • Dynamic Tilt Angle: </span><span class="data-value" id="mpu-tilt">0.0°</span></div>
            <div class="data-row"><span class="data-label">  • Motion Detected   : </span><span class="data-value" id="mpu-motion">NO (Still)</span></div>
            <div class="data-row"><span class="data-label">  • Sudden Impact     : </span><span class="data-value" id="mpu-impact">NO</span></div>
            <div class="data-row"><span class="data-label">  • Free Fall         : </span><span class="data-value" id="mpu-fall">NO</span></div>
            <div class="data-row"><span class="data-label">  • Seismic Anomaly   : </span><span class="data-value" id="mpu-quake">NO</span></div>
        </div>

        <!-- [STEP 4/5] GPS NAVIGATION MODULE -->
        <div class="step-block">
            <div class="step-header">[STEP 4/5] GPS NAVIGATION MODULE (NEO-6M)</div>
            <div class="step-divider">-------------------------------------------------------------</div>
            <div class="data-row"><span class="data-label">  • GPS Fix Status    : </span><span class="data-value" id="gps-status">SEARCHING...</span></div>
            <div class="data-row"><span class="data-label">  • Satellites & Loc  : </span><span class="data-value" id="gps-loc">0 Satellites</span></div>
            <div class="data-row"><span class="data-label">  • Tip               : </span><span class="data-value val-dim" id="gps-tip">Move antenna outdoors for satellite lock</span></div>
        </div>

        <!-- [STEP 5/5] EMERGENCY FUSION ENGINE & SYSTEM HEALTH -->
        <div class="step-block">
            <div class="step-header">[STEP 5/5] EMERGENCY FUSION ENGINE & SYSTEM HEALTH</div>
            <div class="step-divider">-------------------------------------------------------------</div>
            <div class="data-row"><span class="data-label">  • Active Emergency  : </span><span class="data-value" id="sys-emergency">'N' (NOMINAL / NORMAL OPERATION)</span></div>
            <div class="data-row"><span class="data-label">  • Priority Level    : </span><span class="data-value" id="sys-prio">1 (1=Normal, 5=Critical)</span></div>
            <div class="data-row"><span class="data-label">  • Node Health Score : </span><span class="data-value" id="sys-health">100 %</span></div>
            <div class="data-row"><span class="data-label">  • Env Risk Score    : </span><span class="data-value" id="sys-risk">0 %</span></div>
        </div>

        <!-- [STEP 6/6] WI-FI & REMOTE SERVER TRANSMISSION MONITOR -->
        <div class="step-block">
            <div class="step-header">[STEP 6/6] WI-FI & REMOTE SERVER TRANSMISSION MONITOR</div>
            <div class="step-divider">-------------------------------------------------------------</div>
            <div class="data-row"><span class="data-label">  • Wi-Fi AP Address  : </span><span class="data-value val-cyan" id="net-ap-ip">192.168.4.1 (LifeLine-Sensor-Node)</span></div>
            <div class="data-row"><span class="data-label">  • Wi-Fi STA Address : </span><span class="data-value" id="net-sta-ip">Disconnected</span></div>
            <div class="data-row"><span class="data-label">  • Server Endpoint   : </span><span class="data-value val-dim" id="net-server-url">http://...</span></div>
            <div class="data-row"><span class="data-label">  • Telemetry Sent    : </span><span class="data-value" id="net-sent-count">0 Packets</span></div>
            <div class="data-row"><span class="data-label">  • Last HTTP Status  : </span><span class="data-value" id="net-http-code">--</span> <span class="cursor"></span></div>
        </div>

        <div class="controls-bar">
            <button class="btn-term" id="btn-pause" onclick="togglePause()">[ ⏸ PAUSE STREAM ]</button>
            <button class="btn-term" onclick="fetchData()">[ ⚡ REFRESH NOW ]</button>
            <button class="btn-term" onclick="toggleCRT()">[ 📺 CRT SCANLINES ]</button>
        </div>

    </div>

    <footer>LifeLine Emergency Hardware Platform — ESP32 SPU Serial Monitor Dashboard</footer>

    <script>
        let isPaused = false;

        function togglePause() {
            isPaused = !isPaused;
            const btn = document.getElementById('btn-pause');
            btn.innerText = isPaused ? '[ ▶ RESUME STREAM ]' : '[ ⏸ PAUSE STREAM ]';
            btn.style.borderColor = isPaused ? 'var(--term-amber)' : 'var(--term-cyan)';
            btn.style.color = isPaused ? 'var(--term-amber)' : 'var(--term-cyan)';
        }

        function toggleCRT() {
            const crt = document.getElementById('crt-overlay');
            crt.style.display = crt.style.display === 'none' ? 'block' : 'none';
        }

        async function fetchData() {
            if (isPaused) return;
            try {
                const res = await fetch('/api/data');
                const d = await res.json();

                // Status Bar & Uptime
                document.getElementById('node-id').innerText = `SPU-#${d.device_id}`;
                document.getElementById('uptime-val').innerText = `${d.uptime_sec}s`;
                
                const wifiStaEl = document.getElementById('wifi-sta-status');
                if (d.wifi_sta_connected) {
                    wifiStaEl.innerText = `CONNECTED (${d.wifi_sta_ip})`;
                    wifiStaEl.className = 'val-ok';
                } else {
                    wifiStaEl.innerText = 'DISCONNECTED / SEARCHING';
                    wifiStaEl.className = 'val-warn';
                }

                const srvStatusEl = document.getElementById('server-status-val');
                srvStatusEl.innerText = d.server_status || 'IDLE';
                srvStatusEl.className = d.server_last_code === 200 ? 'val-ok' : (d.server_last_code > 0 ? 'val-warn' : 'val-dim');

                // Step 1: Environment
                document.getElementById('dht-temp').innerText = `${d.temp_c.toFixed(1)} °C`;
                document.getElementById('dht-hum').innerText = `${d.humidity_pct.toFixed(1)} %`;
                const dhtStat = document.getElementById('dht-status');
                if (d.temp_c > -50 && d.temp_c < 100) {
                    dhtStat.innerText = 'OK (Connected & Reading)';
                    dhtStat.className = 'val-ok';
                } else {
                    dhtStat.innerText = 'FAILED / NOT CONNECTED';
                    dhtStat.className = 'val-alert';
                }

                // Step 2: Gas
                document.getElementById('gas-ppm').innerText = `${d.gas_ppm} PPM`;
                const gasWarn = document.getElementById('gas-warn');
                if (d.gas_ppm > 300) {
                    gasWarn.innerText = 'ALERT! HIGH POLLUTION';
                    gasWarn.className = 'val-alert';
                } else {
                    gasWarn.innerText = 'NO (Normal)';
                    gasWarn.className = 'val-ok';
                }

                const gasDanger = document.getElementById('gas-danger');
                if (d.gas_ppm > 600) {
                    gasDanger.innerText = 'CRITICAL! DANGER LEVEL';
                    gasDanger.className = 'val-alert';
                } else {
                    gasDanger.innerText = 'NO (Normal)';
                    gasDanger.className = 'val-ok';
                }

                // Step 3: Motion
                document.getElementById('mpu-xyz').innerText = `X=${d.accel_x.toFixed(2)}G, Y=${d.accel_y.toFixed(2)}G, Z=${d.accel_z.toFixed(2)}G`;
                document.getElementById('mpu-gyro').innerText = `X=${d.gyro_x.toFixed(1)}°/s, Y=${d.gyro_y.toFixed(1)}°/s, Z=${d.gyro_z.toFixed(1)}°/s`;
                document.getElementById('mpu-mag').innerText = `${d.total_g.toFixed(2)} G`;
                document.getElementById('mpu-tilt').innerText = `${d.tilt_deg.toFixed(1)}°`;

                const isMoving = Math.abs(d.total_g - 1.0) > 0.15;
                document.getElementById('mpu-motion').innerText = isMoving ? 'YES (Moving)' : 'NO (Still)';
                document.getElementById('mpu-motion').className = isMoving ? 'val-cyan' : 'val-dim';

                const isImpact = d.emergency_code === 'I' || d.emergency_code === 'F';
                document.getElementById('mpu-impact').innerText = isImpact ? 'YES (Spike Detected!)' : 'NO';
                document.getElementById('mpu-impact').className = isImpact ? 'val-alert' : 'val-dim';

                const isFall = d.emergency_code === 'F';
                document.getElementById('mpu-fall').innerText = isFall ? 'YES (Free Fall Detected!)' : 'NO';
                document.getElementById('mpu-fall').className = isFall ? 'val-alert' : 'val-dim';

                const isQuake = d.emergency_code === 'Q';
                document.getElementById('mpu-quake').innerText = isQuake ? 'ALERT! SEISMIC VIBRATION' : 'NO';
                document.getElementById('mpu-quake').className = isQuake ? 'val-alert' : 'val-dim';

                // Step 4: GPS
                const gpsStat = document.getElementById('gps-status');
                const gpsLoc = document.getElementById('gps-loc');
                const gpsTip = document.getElementById('gps-tip');
                if (d.gps_fix) {
                    gpsStat.innerText = 'VALID FIX (Lock Acquired)';
                    gpsStat.className = 'val-ok';
                    gpsLoc.innerText = `${d.satellites} Satellites | Lat: ${d.latitude.toFixed(5)}°, Lon: ${d.longitude.toFixed(5)}°, Alt: ${d.altitude_m.toFixed(1)}m`;
                    gpsTip.innerText = 'Satellite lock healthy';
                } else {
                    gpsStat.innerText = `SEARCHING FOR SATELLITES... (${d.satellites} Seen)`;
                    gpsStat.className = 'val-warn';
                    gpsLoc.innerText = `${d.satellites} Satellites in view`;
                    gpsTip.innerText = 'Move antenna outdoors for satellite lock';
                }

                // Step 5: System & Emergency
                const sysEmerg = document.getElementById('sys-emergency');
                sysEmerg.innerText = `'${d.emergency_code}' (${d.emergency_desc.toUpperCase()})`;
                sysEmerg.className = d.emergency_code !== 'N' ? 'val-alert' : 'val-ok';

                document.getElementById('sys-prio').innerText = `${d.priority} (${d.priority === 1 ? '1=Normal' : d.priority === 5 ? '5=Critical' : 'Priority ' + d.priority})`;
                document.getElementById('sys-health').innerText = `${d.health_score} %`;
                document.getElementById('sys-risk').innerText = `${d.risk_score} %`;

                // Step 6: Network & Server Upload
                document.getElementById('net-sta-ip').innerText = d.wifi_sta_connected ? `${d.wifi_sta_ip}` : 'Disconnected';
                document.getElementById('net-sta-ip').className = d.wifi_sta_connected ? 'val-ok' : 'val-warn';
                document.getElementById('net-server-url').innerText = d.server_url || 'http://...';
                document.getElementById('net-sent-count').innerText = `${d.server_upload_count || 0} Packets Sent (${d.server_upload_fails || 0} Failed)`;
                
                const httpCodeEl = document.getElementById('net-http-code');
                httpCodeEl.innerText = d.server_status || '--';
                httpCodeEl.className = d.server_last_code === 200 ? 'val-ok' : (d.server_last_code > 0 ? 'val-warn' : 'val-dim');

                document.getElementById('status-dot').className = d.emergency_code !== 'N' ? 'dot-red' : 'dot-green';

            } catch (e) {
                console.error('Terminal polling error:', e);
                document.getElementById('status-dot').className = 'dot-amber';
            }
        }

        setInterval(fetchData, 1000);
        fetchData();
    </script>
</body>
</html>
)rawliteral";

WebServerManager::WebServerManager() 
    : _server(WEB_SERVER_PORT),
      _wifi_sta_connected(false),
      _wifi_sta_ip("0.0.0.0"),
      _last_server_upload_time(0),
      _server_upload_count(0),
      _server_upload_fail_count(0),
      _last_http_code(0),
      _last_server_status("IDLE / READY") {}

void WebServerManager::begin() {
    #if ENABLE_WEB_SERVER
    // Configure ESP32 Wi-Fi Mode (AP + STA if STA enabled, or AP only)
    #if ENABLE_WIFI_STA
    WiFi.mode(WIFI_AP_STA);
    WiFi.begin(WIFI_STA_SSID, WIFI_STA_PASS);
    #else
    WiFi.mode(WIFI_AP);
    #endif

    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS);

    IPAddress apIP = WiFi.softAPIP();

    #if SPU_DEBUG_ENABLE
    Serial.println(F("\n=================================================="));
    Serial.println(F("     LIFELINE LOCAL WEB SERVER INITIALIZED        "));
    Serial.printf( "     Wi-Fi AP SSID   : %s                         \n", WIFI_AP_SSID);
    Serial.printf( "     Wi-Fi AP Pass   : %s                         \n", WIFI_AP_PASS);
    Serial.printf( "     Local Dashboard : http://%s                 \n", apIP.toString().c_str());
    #if ENABLE_WIFI_STA
    Serial.printf( "     Target Router   : %s                         \n", WIFI_STA_SSID);
    Serial.printf( "     Remote Server   : %s                         \n", SERVER_TELEMETRY_URL);
    #endif
    Serial.println(F("==================================================\n"));
    #endif

    _server.on("/", std::bind(&WebServerManager::handleRoot, this));
    _server.on("/api/data", std::bind(&WebServerManager::handleApiData, this));
    _server.onNotFound(std::bind(&WebServerManager::handleNotFound, this));

    _server.begin();
    #endif
}

void WebServerManager::update() {
    #if ENABLE_WEB_SERVER
    _server.handleClient();

    #if ENABLE_WIFI_STA
    checkWiFiSTAConnection();

    #if ENABLE_SERVER_UPLOAD
    unsigned long now = millis();
    if (now - _last_server_upload_time >= SERVER_UPLOAD_INTERVAL) {
        _last_server_upload_time = now;
        uploadTelemetryToServer();
    }
    #endif
    #endif
    #endif
}

void WebServerManager::checkWiFiSTAConnection() {
    if (WiFi.status() == WL_CONNECTED) {
        if (!_wifi_sta_connected) {
            _wifi_sta_connected = true;
            _wifi_sta_ip = WiFi.localIP().toString();
            #if SPU_DEBUG_ENABLE
            Serial.printf("[WIFI STA] Connected to '%s'! Assigned IP: %s\n", WIFI_STA_SSID, _wifi_sta_ip.c_str());
            #endif
        }
    } else {
        if (_wifi_sta_connected) {
            _wifi_sta_connected = false;
            _wifi_sta_ip = "0.0.0.0";
            #if SPU_DEBUG_ENABLE
            Serial.println(F("[WIFI STA] Disconnected from Wi-Fi router. Background reconnect active..."));
            #endif
        }
    }
}

void WebServerManager::uploadTelemetryToServer() {
    if (!_wifi_sta_connected) {
        _last_server_status = "Wi-Fi Disconnected";
        return;
    }

    const EnvironmentData& env = envManager.getData();
    const MotionData& motion = mpuManager.getData();
    const GasData& gas = gasManager.getData();
    const GPSData& gps = gpsManager.getData();
    const EmergencyState& emergency = emergencyDetector.getState();
    SystemHealthMetrics health = HealthCalculator::calculate(env, motion, gas, gps);

    char jsonBuffer[600];
    snprintf(jsonBuffer, sizeof(jsonBuffer),
        "{"
        "\"device_id\":%d,"
        "\"firmware\":\"%s\","
        "\"uptime_sec\":%lu,"
        "\"temp_c\":%.2f,"
        "\"humidity_pct\":%.2f,"
        "\"pressure_hpa\":%.2f,"
        "\"gas_ppm\":%u,"
        "\"accel_x\":%.3f,\"accel_y\":%.3f,\"accel_z\":%.3f,"
        "\"gyro_x\":%.2f,\"gyro_y\":%.2f,\"gyro_z\":%.2f,"
        "\"total_g\":%.3f,\"tilt_deg\":%.2f,"
        "\"gps_fix\":%s,\"satellites\":%u,"
        "\"latitude\":%.6f,\"longitude\":%.6f,\"altitude_m\":%.2f,"
        "\"emergency_code\":\"%c\",\"emergency_desc\":\"%s\",\"priority\":%u,"
        "\"health_score\":%u,\"risk_score\":%u"
        "}",
        SPU_DEVICE_ID, SPU_FIRMWARE_VERSION, millis() / 1000,
        env.temperature_c, env.humidity_pct, env.pressure_hpa, gas.ppm_estimate,
        motion.accel_x, motion.accel_y, motion.accel_z,
        motion.gyro_x, motion.gyro_y, motion.gyro_z,
        motion.total_accel_g, motion.tilt_deg,
        gps.fix_valid ? "true" : "false", gps.satellites,
        gps.latitude, gps.longitude, gps.altitude_m,
        emergency.code, emergency.description, emergency.priority,
        health.node_health_score, health.environmental_risk_score
    );

    HTTPClient http;
    http.begin(SERVER_TELEMETRY_URL);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(1500); // 1.5 sec max timeout

    int httpResponseCode = http.POST(jsonBuffer);
    _last_http_code = httpResponseCode;

    if (httpResponseCode > 0) {
        _server_upload_count++;
        _last_server_status = "OK (" + String(httpResponseCode) + ")";
        #if SPU_DEBUG_ENABLE
        Serial.printf("[SERVER UPLOAD] Telemetry posted! HTTP %d | Count: %u\n", httpResponseCode, _server_upload_count);
        #endif
    } else {
        _server_upload_fail_count++;
        _last_server_status = "Error: " + http.errorToString(httpResponseCode);
        #if SPU_DEBUG_ENABLE
        Serial.printf("[SERVER UPLOAD FAILED] Error: %s | Fail Count: %u\n", _last_server_status.c_str(), _server_upload_fail_count);
        #endif
    }

    http.end();
}

void WebServerManager::handleRoot() {
    _server.send(200, "text/html", INDEX_HTML);
}

void WebServerManager::handleApiData() {
    const EnvironmentData& env = envManager.getData();
    const MotionData& motion = mpuManager.getData();
    const GasData& gas = gasManager.getData();
    const GPSData& gps = gpsManager.getData();
    const EmergencyState& emergency = emergencyDetector.getState();
    SystemHealthMetrics health = HealthCalculator::calculate(env, motion, gas, gps);

    char jsonBuffer[800];
    snprintf(jsonBuffer, sizeof(jsonBuffer),
        "{"
        "\"device_id\":%d,"
        "\"firmware\":\"%s\","
        "\"uptime_sec\":%lu,"
        "\"temp_c\":%.2f,"
        "\"humidity_pct\":%.2f,"
        "\"pressure_hpa\":%.2f,"
        "\"gas_ppm\":%u,"
        "\"accel_x\":%.3f,\"accel_y\":%.3f,\"accel_z\":%.3f,"
        "\"gyro_x\":%.2f,\"gyro_y\":%.2f,\"gyro_z\":%.2f,"
        "\"total_g\":%.3f,\"tilt_deg\":%.2f,"
        "\"gps_fix\":%s,\"satellites\":%u,"
        "\"latitude\":%.6f,\"longitude\":%.6f,\"altitude_m\":%.2f,"
        "\"emergency_code\":\"%c\",\"emergency_desc\":\"%s\",\"priority\":%u,"
        "\"health_score\":%u,\"risk_score\":%u,"
        "\"wifi_sta_connected\":%s,"
        "\"wifi_sta_ip\":\"%s\","
        "\"server_url\":\"%s\","
        "\"server_upload_count\":%u,"
        "\"server_upload_fails\":%u,"
        "\"server_last_code\":%d,"
        "\"server_status\":\"%s\""
        "}",
        SPU_DEVICE_ID,
        SPU_FIRMWARE_VERSION,
        millis() / 1000,
        env.temperature_c,
        env.humidity_pct,
        env.pressure_hpa,
        gas.ppm_estimate,
        motion.accel_x,
        motion.accel_y,
        motion.accel_z,
        motion.gyro_x,
        motion.gyro_y,
        motion.gyro_z,
        motion.total_accel_g,
        motion.tilt_deg,
        gps.fix_valid ? "true" : "false",
        gps.satellites,
        gps.latitude,
        gps.longitude,
        gps.altitude_m,
        emergency.code,
        emergency.description,
        emergency.priority,
        health.node_health_score,
        health.environmental_risk_score,
        _wifi_sta_connected ? "true" : "false",
        _wifi_sta_ip.c_str(),
        SERVER_TELEMETRY_URL,
        _server_upload_count,
        _server_upload_fail_count,
        _last_http_code,
        _last_server_status.c_str()
    );

    _server.send(200, "application/json", jsonBuffer);
}

void WebServerManager::handleNotFound() {
    _server.send(404, "text/plain", "404: Not Found");
}
