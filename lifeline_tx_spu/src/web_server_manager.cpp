#include "web_server_manager.h"

WebServerManager webServerManager;

static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>LifeLine Sensor Node — Local Dashboard</title>
    <style>
        :root {
            --bg-primary: #0a0b10;
            --bg-card: rgba(20, 24, 38, 0.75);
            --border-card: rgba(0, 240, 255, 0.15);
            --accent-cyan: #00f0ff;
            --accent-green: #00ff87;
            --accent-amber: #ffb800;
            --accent-red: #ff3b3b;
            --text-primary: #ffffff;
            --text-secondary: #94a3b8;
        }

        * { margin: 0; padding: 0; box-sizing: border-box; font-family: 'Segoe UI', system-ui, -apple-system, sans-serif; }

        body {
            background-color: var(--bg-primary);
            color: var(--text-primary);
            padding: 1.5rem;
            min-height: 100vh;
            background-image: 
                radial-gradient(circle at 10% 20%, rgba(0, 240, 255, 0.05) 0%, transparent 40%),
                radial-gradient(circle at 90% 80%, rgba(0, 255, 135, 0.05) 0%, transparent 40%);
        }

        .header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding-bottom: 1.5rem;
            border-bottom: 1px solid rgba(255, 255, 255, 0.1);
            margin-bottom: 1.5rem;
        }

        .header-title { font-size: 1.5rem; font-weight: 700; color: var(--text-primary); display: flex; align-items: center; gap: 0.75rem; }
        .header-title span { color: var(--accent-cyan); }
        .status-badge {
            display: inline-flex; align-items: center; gap: 0.5rem; padding: 0.4rem 0.8rem;
            border-radius: 9999px; font-size: 0.85rem; font-weight: 600;
            background: rgba(0, 255, 135, 0.1); border: 1px solid var(--accent-green); color: var(--accent-green);
        }
        .pulse-dot { width: 8px; height: 8px; border-radius: 50%; background: var(--accent-green); box-shadow: 0 0 10px var(--accent-green); animation: pulse 1.5s infinite; }
        @keyframes pulse { 0%, 100% { opacity: 1; transform: scale(1); } 50% { opacity: 0.4; transform: scale(1.2); } }

        .banner {
            padding: 1rem 1.5rem; border-radius: 12px; margin-bottom: 1.5rem;
            background: rgba(0, 255, 135, 0.08); border: 1px solid var(--accent-green);
            display: flex; justify-content: space-between; align-items: center;
            transition: all 0.3s ease;
        }
        .banner.emergency {
            background: rgba(255, 59, 59, 0.15); border-color: var(--accent-red); animation: blinkBanner 1s infinite alternate;
        }
        @keyframes blinkBanner { from { box-shadow: 0 0 10px rgba(255,59,59,0.2); } to { box-shadow: 0 0 25px rgba(255,59,59,0.6); } }

        .banner-text { font-size: 1.1rem; font-weight: 700; }
        .banner-sub { font-size: 0.85rem; color: var(--text-secondary); }

        .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(280px, 1fr)); gap: 1.25rem; }

        .card {
            background: var(--bg-card);
            border: 1px solid var(--border-card);
            border-radius: 16px;
            padding: 1.25rem;
            backdrop-filter: blur(12px);
            box-shadow: 0 8px 32px 0 rgba(0, 0, 0, 0.3);
            transition: transform 0.2s ease, border-color 0.2s ease;
        }
        .card:hover { transform: translateY(-3px); border-color: var(--accent-cyan); }

        .card-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 1rem; }
        .card-title { font-size: 0.95rem; text-transform: uppercase; letter-spacing: 0.05em; color: var(--text-secondary); font-weight: 600; }
        .card-icon { font-size: 1.3rem; }

        .value-large { font-size: 2.2rem; font-weight: 800; color: var(--text-primary); margin-bottom: 0.25rem; }
        .unit { font-size: 1rem; font-weight: 500; color: var(--text-secondary); }

        .stat-row { display: flex; justify-content: space-between; padding: 0.4rem 0; border-bottom: 1px solid rgba(255,255,255,0.05); font-size: 0.9rem; }
        .stat-row:last-child { border-bottom: none; }
        .stat-label { color: var(--text-secondary); }
        .stat-val { font-weight: 600; color: var(--text-primary); }

        .progress-bg { width: 100%; height: 8px; background: rgba(255,255,255,0.1); border-radius: 4px; overflow: hidden; margin-top: 0.5rem; }
        .progress-bar { height: 100%; background: linear-gradient(90deg, var(--accent-cyan), var(--accent-green)); transition: width 0.5s ease; }

        .btn-map {
            display: inline-block; width: 100%; text-align: center; padding: 0.6rem; margin-top: 1rem;
            border-radius: 8px; background: rgba(0, 240, 255, 0.1); border: 1px solid var(--accent-cyan);
            color: var(--accent-cyan); font-weight: 600; text-decoration: none; transition: all 0.2s;
        }
        .btn-map:hover { background: var(--accent-cyan); color: #000; }

        footer { text-align: center; margin-top: 2rem; color: var(--text-secondary); font-size: 0.85rem; }
    </style>
</head>
<body>

    <div class="header">
        <div class="header-title">⚡ LifeLine <span>Sensor Node</span></div>
        <div class="status-badge">
            <div class="pulse-dot"></div>
            <span id="node-id">Node TX #3</span>
        </div>
    </div>

    <!-- Active Emergency Banner -->
    <div class="banner" id="emergency-banner">
        <div>
            <div class="banner-text" id="emergency-title">NORMAL TELEMETRY</div>
            <div class="banner-sub" id="emergency-sub">All sensors operating within nominal safe parameters</div>
        </div>
        <div class="banner-text" id="emergency-priority">PRIORITY 1</div>
    </div>

    <div class="grid">
        <!-- 1. Environment Card -->
        <div class="card">
            <div class="card-header">
                <div class="card-title">Environment</div>
                <div class="card-icon">🌡️</div>
            </div>
            <div class="value-large"><span id="temp-val">--</span> <span class="unit">°C</span></div>
            <div class="progress-bg"><div class="progress-bar" id="temp-bar" style="width: 50%"></div></div>
            <div style="margin-top: 1rem;">
                <div class="stat-row">
                    <span class="stat-label">Humidity</span>
                    <span class="stat-val" id="humidity-val">-- %</span>
                </div>
                <div class="stat-row">
                    <span class="stat-label">Pressure</span>
                    <span class="stat-val" id="pressure-val">-- hPa</span>
                </div>
            </div>
        </div>

        <!-- 2. Air Quality Card -->
        <div class="card">
            <div class="card-header">
                <div class="card-title">Air Quality (MQ135)</div>
                <div class="card-icon">💨</div>
            </div>
            <div class="value-large"><span id="gas-val">--</span> <span class="unit">PPM</span></div>
            <div class="progress-bg"><div class="progress-bar" id="gas-bar" style="width: 20%; background: var(--accent-green);"></div></div>
            <div style="margin-top: 1rem;">
                <div class="stat-row">
                    <span class="stat-label">Pollution Level</span>
                    <span class="stat-val" id="gas-status">Normal</span>
                </div>
                <div class="stat-row">
                    <span class="stat-label">Smoke Risk</span>
                    <span class="stat-val" id="smoke-status">Clear</span>
                </div>
            </div>
        </div>

        <!-- 3. Motion & Seismic Card -->
        <div class="card">
            <div class="card-header">
                <div class="card-title">Motion & IMU (MPU6050)</div>
                <div class="card-icon">📐</div>
            </div>
            <div class="value-large"><span id="accel-val">--</span> <span class="unit">G</span></div>
            <div style="margin-top: 0.5rem;">
                <div class="stat-row">
                    <span class="stat-label">Tilt Angle</span>
                    <span class="stat-val" id="tilt-val">-- °</span>
                </div>
                <div class="stat-row">
                    <span class="stat-label">Raw Accel (X,Y,Z)</span>
                    <span class="stat-val" id="xyz-val">--</span>
                </div>
                <div class="stat-row">
                    <span class="stat-label">Motion Flag</span>
                    <span class="stat-val" id="motion-flag">Still</span>
                </div>
                <div class="stat-row">
                    <span class="stat-label">Earthquake Anomaly</span>
                    <span class="stat-val" id="earthquake-flag">None</span>
                </div>
            </div>
        </div>

        <!-- 4. GPS Navigation Card -->
        <div class="card">
            <div class="card-header">
                <div class="card-title">GPS Location (NEO-6M)</div>
                <div class="card-icon">🛰️</div>
            </div>
            <div class="stat-row">
                <span class="stat-label">Fix Status</span>
                <span class="stat-val" id="gps-fix">Searching...</span>
            </div>
            <div class="stat-row">
                <span class="stat-label">Satellites</span>
                <span class="stat-val" id="sat-count">0</span>
            </div>
            <div class="stat-row">
                <span class="stat-label">Latitude / Longitude</span>
                <span class="stat-val" id="latlon-val">--</span>
            </div>
            <div class="stat-row">
                <span class="stat-label">Altitude</span>
                <span class="stat-val" id="alt-val">-- m</span>
            </div>
            <a href="#" target="_blank" class="btn-map" id="map-link">View on Google Maps</a>
        </div>

        <!-- 5. System Health Card -->
        <div class="card">
            <div class="card-header">
                <div class="card-title">System Health & Risk</div>
                <div class="card-icon">🛡️</div>
            </div>
            <div class="stat-row">
                <span class="stat-label">Node Health Score</span>
                <span class="stat-val" id="health-val">100%</span>
            </div>
            <div class="progress-bg"><div class="progress-bar" id="health-bar" style="width: 100%"></div></div>
            
            <div class="stat-row" style="margin-top: 1rem;">
                <span class="stat-label">Environmental Risk</span>
                <span class="stat-val" id="risk-val">0%</span>
            </div>
            <div class="progress-bg"><div class="progress-bar" id="risk-bar" style="width: 0%; background: var(--accent-amber);"></div></div>
        </div>
    </div>

    <footer>LifeLine Emergency Hardware Platform — ESP32 SPU Local Web Dashboard</footer>

    <script>
        async function fetchSensorData() {
            try {
                const res = await fetch('/api/data');
                const data = await res.json();

                // Environment
                document.getElementById('temp-val').innerText = data.temp_c.toFixed(1);
                document.getElementById('humidity-val').innerText = data.humidity_pct.toFixed(1) + ' %';
                document.getElementById('pressure-val').innerText = data.pressure_hpa + ' hPa';
                document.getElementById('temp-bar').style.width = Math.min(100, Math.max(0, (data.temp_c / 50) * 100)) + '%';

                // Air Quality
                document.getElementById('gas-val').innerText = data.gas_ppm;
                document.getElementById('gas-status').innerText = data.gas_ppm > 300 ? 'High Warning' : 'Normal';
                document.getElementById('smoke-status').innerText = data.gas_ppm > 600 ? 'DANGER SMOKE' : 'Clear';
                document.getElementById('gas-bar').style.width = Math.min(100, (data.gas_ppm / 1000) * 100) + '%';
                document.getElementById('gas-bar').style.background = data.gas_ppm > 300 ? 'var(--accent-red)' : 'var(--accent-green)';

                // Motion
                document.getElementById('accel-val').innerText = data.total_g.toFixed(2);
                document.getElementById('tilt-val').innerText = data.tilt_deg.toFixed(1) + ' °';
                document.getElementById('xyz-val').innerText = `${data.accel_x.toFixed(2)}, ${data.accel_y.toFixed(2)}, ${data.accel_z.toFixed(2)}`;
                document.getElementById('motion-flag').innerText = Math.abs(data.total_g - 1.0) > 0.15 ? 'MOVING' : 'Still';
                document.getElementById('earthquake-flag').innerText = data.emergency_code === 'Q' ? 'SEISMIC VIBRATION' : 'None';

                // GPS
                document.getElementById('gps-fix').innerText = data.gps_fix ? 'VALID FIX' : 'Searching...';
                document.getElementById('sat-count').innerText = data.satellites;
                document.getElementById('latlon-val').innerText = data.gps_fix ? `${data.latitude.toFixed(5)}, ${data.longitude.toFixed(5)}` : 'No Lock';
                document.getElementById('alt-val').innerText = data.altitude_m.toFixed(1) + ' m';
                if (data.gps_fix) {
                    document.getElementById('map-link').href = `https://maps.google.com/?q=${data.latitude},${data.longitude}`;
                }

                // Health & Risk
                document.getElementById('health-val').innerText = data.health_score + '%';
                document.getElementById('health-bar').style.width = data.health_score + '%';
                document.getElementById('risk-val').innerText = data.risk_score + '%';
                document.getElementById('risk-bar').style.width = data.risk_score + '%';

                // Emergency Banner
                const banner = document.getElementById('emergency-banner');
                const title = document.getElementById('emergency-title');
                const sub = document.getElementById('emergency-sub');
                const prio = document.getElementById('emergency-priority');

                title.innerText = data.emergency_desc;
                prio.innerText = 'PRIORITY ' + data.priority;

                if (data.emergency_code !== 'N') {
                    banner.classList.add('emergency');
                    sub.innerText = 'ATTENTION: Emergency condition detected by Sensor Fusion engine';
                } else {
                    banner.classList.remove('emergency');
                    sub.innerText = 'All sensors operating within nominal safe parameters';
                }

            } catch (err) {
                console.error('Failed to fetch sensor data:', err);
            }
        }

        setInterval(fetchSensorData, 1500);
        fetchSensorData();
    </script>
</body>
</html>
)rawliteral";

WebServerManager::WebServerManager() : _server(WEB_SERVER_PORT) {}

void WebServerManager::begin() {
    #if ENABLE_WEB_SERVER
    // Configure ESP32 Wi-Fi SoftAP Access Point
    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS);

    IPAddress apIP = WiFi.softAPIP();

    #if SPU_DEBUG_ENABLE
    Serial.println(F("\n=================================================="));
    Serial.println(F("     LIFELINE LOCAL WEB SERVER INITIALIZED        "));
    Serial.printf( "     Wi-Fi AP SSID : %s                           \n", WIFI_AP_SSID);
    Serial.printf( "     Wi-Fi AP Pass : %s                           \n", WIFI_AP_PASS);
    Serial.printf( "     Local Dashboard: http://%s                   \n", apIP.toString().c_str());
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
    #endif
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

    char jsonBuffer[512];
    snprintf(jsonBuffer, sizeof(jsonBuffer),
        "{"
        "\"device_id\":%d,"
        "\"firmware\":\"%s\","
        "\"uptime_sec\":%lu,"
        "\"temp_c\":%.2f,"
        "\"humidity_pct\":%.2f,"
        "\"pressure_hpa\":%.2f,"
        "\"gas_ppm\":%u,"
        "\"accel_x\":%.3f,"
        "\"accel_y\":%.3f,"
        "\"accel_z\":%.3f,"
        "\"gyro_x\":%.2f,"
        "\"gyro_y\":%.2f,"
        "\"gyro_z\":%.2f,"
        "\"total_g\":%.3f,"
        "\"tilt_deg\":%.2f,"
        "\"gps_fix\":%s,"
        "\"satellites\":%u,"
        "\"latitude\":%.6f,"
        "\"longitude\":%.6f,"
        "\"altitude_m\":%.2f,"
        "\"emergency_code\":\"%c\","
        "\"emergency_desc\":\"%s\","
        "\"priority\":%u,"
        "\"health_score\":%u,"
        "\"risk_score\":%u"
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
        health.environmental_risk_score
    );

    _server.send(200, "application/json", jsonBuffer);
}

void WebServerManager::handleNotFound() {
    _server.send(404, "text/plain", "404: Not Found");
}
