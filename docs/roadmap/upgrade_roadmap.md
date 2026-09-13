# LifeLine Hardware & Cloud Upgrade Master Plan

This document outlines the complete upgrade roadmap to enable **Dual-Mode Transmission**:
1. **Manual Send**: Immediate SOS dispatch via matrix keypad or physical button.
2. **Automatic Emergency Detection**: Instant dispatch when sensors detect abnormal conditions (Fire, Landslide, Earthquake, Gas Leak, Heat/Cold wave).
3. **Periodic 1-Hour Full Sensor Logging**: Complete telemetry heartbeat sent every 1 hour during normal operation.

---

## 📋 Comprehensive Upgrade Matrix

| Component | Target File | Description of Upgrade | Status |
| :--- | :--- | :--- | :--- |
| **TX Pro** | [DisplayUI.cpp](file:///d:/lifeline_hardware/lifeline_tx_pro/DisplayUI.cpp) | "MESSAGE SENDING" Pop Screen with live animated progress bar, word-wrapped sitrep body, and closed-loop ACK badge | **Completed** |
| **TX Pro** | [BLEManager.cpp](file:///d:/lifeline_hardware/lifeline_tx_pro/BLEManager.cpp) | BLE Nordic UART peripheral, companion app pairing, and 5,000ms ACK listen window | **Completed** |
| **RX Pro** | [DisplayUI.cpp](file:///d:/lifeline_hardware/lifeline_rx_pro/DisplayUI.cpp) | Full 16×2 LCD custom message screen dedicating all 32 chars to message with smart word-wrapping (`formatLCDTwoRows`) | **Completed** |
| **RX Pro** | [lifeline_rx_pro.ino](file:///d:/lifeline_hardware/lifeline_rx_pro/lifeline_rx_pro.ino) | Hands-free 4s auto-paging for long messages and direct BLE chat ingestion (`MSG:<text>`) | **Completed** |
| **Mobile App** | [lifeline_companion/](file:///d:/lifeline_hardware/lifeline_companion/) | React Native / Expo tactical mobile app with Radar HUD, SITREP composer, and offline mock BLE engine | **Completed** |
| **Web Client** | [bluefy_companion/](file:///d:/lifeline_hardware/bluefy_companion/) | Zero-install offline Web Bluetooth PWA compatible with iPhone Bluefy and Android Chrome with camera QR pairing | **Completed** |
| **SPU** | [ConfigSPU.h](file:///d:/lifeline_hardware/lifeline_tx_spu/include/ConfigSPU.h) | Set `TELEMETRY_SEND_INTERVAL` to 1 Hour (`3600000` ms) & keep `EMERGENCY_SEND_INTERVAL` at `500` ms. | Planned |
| **SPU** | [sensor_manager.cpp](file:///d:/lifeline_hardware/lifeline_tx_spu/src/sensor_manager.cpp) | Reset timer immediately on emergency trigger so alerts skip the 1-hour wait. | Planned |
| **CCU** | [lora_manager.cpp](file:///d:/lifeline_hardware/lifeline_tx_ccu/src/lora_manager.cpp) | Expand `buildBaseStationPayload()` from basic `TX003,F` to full CSV carrying GPS, Temp, Gas, Health & Risk. | Planned |
| **RX Pro** | [LoRaComm.cpp](file:///d:/lifeline_hardware/lifeline_rx_pro/LoRaComm.cpp) | Upgrade `parseLoRaPacket()` to parse CSV payload tokens into a `ParsedTelemetry` struct. | Planned |
| **RX Pro** | [APIClient.cpp](file:///d:/lifeline_hardware/lifeline_rx_pro/APIClient.cpp) | Update `pushAlertToAPI()` to post rich JSON body (GPS, Temp, Gas, Health, RSSI) to cloud. | Planned |
| **Cloud** | Web Server Backend & DB | Update HTTP POST API endpoint to accept rich JSON payload, save to DB, and push to Web Dashboard map & graphs. | Planned |

---

## 1. SPU (Sensor Processing Unit) Upgrades

### File: [ConfigSPU.h](file:///d:/lifeline_hardware/lifeline_tx_spu/include/ConfigSPU.h#L72-L78)
Update timing definitions:

```cpp
// Timing & Sampling Intervals
#define SENSOR_SAMPLE_INTERVAL  200       // ms - Sensor sampling loop (5 Hz)
#define EMERGENCY_SEND_INTERVAL 500       // ms - Rapid dispatch during active emergency (0.5 sec)
#define TELEMETRY_SEND_INTERVAL 3600000   // ms - Periodic full sensor telemetry log (1 Hour)
```

### File: [sensor_manager.cpp](file:///d:/lifeline_hardware/lifeline_tx_spu/src/sensor_manager.cpp#L45-L70)
Ensure instant timer reset on emergency state change:

```cpp
void SensorManager::loop() {
    webServerManager.update();
    unsigned long now = millis();

    // 1. High frequency sensor sampling
    if (now - _last_sample_time >= SENSOR_SAMPLE_INTERVAL) {
        _last_sample_time = now;
        gpsManager.update();
        envManager.update();
        mpuManager.update();
        gasManager.update();

        emergencyDetector.update(envManager.getData(), mpuManager.getData(), gasManager.getData());
    }

    const EmergencyState& emergency = emergencyDetector.getState();
    
    // Instant override: If an emergency just became active, reset last send time to force immediate send!
    static bool previous_emergency_state = false;
    if (emergency.is_active && !previous_emergency_state) {
        _last_send_time = 0; // Force immediate dispatch on next line
    }
    previous_emergency_state = emergency.is_active;

    unsigned long dispatch_interval = emergency.is_active ? EMERGENCY_SEND_INTERVAL : TELEMETRY_SEND_INTERVAL;

    if (now - _last_send_time >= dispatch_interval) {
        _last_send_time = now;

        SystemHealthMetrics health = HealthCalculator::calculate(envManager.getData(), mpuManager.getData(), gasManager.getData(), gpsManager.getData());
        TelemetryPacket pkt = buildTelemetryPacket(envManager.getData(), mpuManager.getData(), gasManager.getData(), gpsManager.getData(), emergency, health);

        digitalWrite(STATUS_LED_PIN, HIGH);
        uartManager.sendTelemetry(pkt);
        digitalWrite(STATUS_LED_PIN, LOW);
    }
}
```

---

## 2. CCU (Communication Controller Unit) Upgrades

### File: [lora_manager.cpp](file:///d:/lifeline_hardware/lifeline_tx_ccu/src/lora_manager.cpp#L33-L38)
Expand LoRa payload to transmit full sensor metrics in a compact CSV string:

```cpp
String LoRaManager::buildBaseStationPayload(const TelemetryPacket& packet) {
    // Format: TX[ID],[CODE],[TEMP_X10],[HUM_X10],[GAS],[LAT_E7],[LON_E7],[ALT],[HEALTH],[RISK]
    // Example: TX003,F,285,650,350,27717245,85323960,1350,98,80
    char payloadStr[80];
    snprintf(payloadStr, sizeof(payloadStr), 
             "TX%03d,%c,%d,%u,%u,%ld,%ld,%d,%u,%u",
             packet.node_id,
             packet.emergency_code,
             packet.temp_c_x10,
             packet.humidity_x10,
             packet.gas_ppm,
             (long)packet.lat_deg_e7,
             (long)packet.lon_deg_e7,
             packet.alt_meters,
             packet.health_score,
             packet.risk_score);
             
    return String(payloadStr);
}
```

---

## 3. RX Pro (Base Station Receiver) Upgrades

### File: [LoRaComm.h](file:///d:/lifeline_hardware/lifeline_rx_pro/LoRaComm.h)
Define rich telemetry structure for incoming packets:

```cpp
struct FullTelemetryData {
    int deviceId;
    char emergencyCode; // 'N', 'F', 'L', 'Q', 'S', etc.
    float temperature;
    float humidity;
    int gasPpm;
    double latitude;
    double longitude;
    int altitude;
    int healthScore;
    int riskScore;
    int rssi;
    bool isFullTelemetry;
};

bool parseLoRaPacketExtended(FullTelemetryData& telemetry);
```

### File: [LoRaComm.cpp](file:///d:/lifeline_hardware/lifeline_rx_pro/LoRaComm.cpp)
Parse both legacy string alerts (`TX003,F`) and extended CSV packets (`TX003,N,285,650,...`):

```cpp
bool parseLoRaPacketExtended(FullTelemetryData& telemetry) {
    int packetSize = LoRa.parsePacket();
    if (packetSize == 0) return false;

    String incoming = "";
    while (LoRa.available()) {
        incoming += (char)LoRa.read();
    }
    incoming.trim();

    telemetry.rssi = LoRa.packetRssi();

    // Check header 'TX'
    if (!incoming.startsWith("TX")) return false;

    // Split CSV by commas
    // Extract: deviceId, code, temp, humidity, gas, lat, lon, alt, health, risk
    // Convert e7 coordinates back to floating point degrees (lat = lat_e7 / 1e7)
    // ... CSV parsing logic ...
    
    return true;
}
```

### File: [APIClient.cpp](file:///d:/lifeline_hardware/lifeline_rx_pro/APIClient.cpp#L7-L34)
Update cloud POST body format:

```cpp
void pushFullTelemetryToAPI(const FullTelemetryData& data) {
    if (!wifiConnected || WiFi.status() != WL_CONNECTED) return;
    
    HTTPClient http;
    http.setTimeout(3000);
    http.begin(API_ENDPOINT);
    http.addHeader("Content-Type", "application/json");
    
    // Construct rich JSON string
    String jsonPayload = "{";
    jsonPayload += "\"DID\":" + String(data.deviceId) + ",";
    jsonPayload += "\"code\":\"" + String(data.emergencyCode) + "\",";
    jsonPayload += "\"temp\":" + String(data.temperature, 1) + ",";
    jsonPayload += "\"humidity\":" + String(data.humidity, 1) + ",";
    jsonPayload += "\"gas_ppm\":" + String(data.gasPpm) + ",";
    jsonPayload += "\"lat\":" + String(data.latitude, 6) + ",";
    jsonPayload += "\"lon\":" + String(data.longitude, 6) + ",";
    jsonPayload += "\"alt\":" + String(data.altitude) + ",";
    jsonPayload += "\"health\":" + String(data.healthScore) + ",";
    jsonPayload += "\"risk\":" + String(data.riskScore) + ",";
    jsonPayload += "\"rssi\":" + String(data.rssi);
    jsonPayload += "}";
    
    int httpCode = http.POST(jsonPayload);
    http.end();
}
```

### File: [lifeline_rx_pro.ino](file:///d:/lifeline_hardware/lifeline_rx_pro/lifeline_rx_pro.ino#L128-L137)
Selective Audio Siren Logic:

```cpp
if (telemetry.emergencyCode == 'N') {
    // Normal 1-Hour Periodic Log
    // 1. Update LCD line showing telemetry updated
    // 2. Silent alert (NO SIREN BUZZER)
    // 3. Push full telemetry JSON to Cloud API
    pushFullTelemetryToAPI(telemetry);
} else {
    // Active Emergency (Fire, Landslide, Manual SOS, etc.)
    // 1. Trigger Loud Alarm Siren
    // 2. Display red alert screen on LCD
    // 3. Push emergency JSON to Cloud API immediately
    triggerRxBlink();
    drawAlertScreen(telemetry.deviceId, telemetry.emergencyCode, telemetry.rssi);
    pushFullTelemetryToAPI(telemetry);
}
```

---

## 4. Cloud Server Backend & Web Dashboard Requirements

### A. HTTP API Gateway Endpoint (`POST /api/telemetry`)
* **Input Body**: Accepts incoming JSON payloads from Base Station.
* **Logic**:
  1. Store log entry in `telemetry_logs` database table.
  2. If `code != 'N'`, create active incident in `active_emergencies` table and trigger WebSocket broadcast / SMS / Email alert.

### B. Database Schema (`telemetry_logs`)
```sql
CREATE TABLE telemetry_logs (
    id BIGINT AUTO_INCREMENT PRIMARY KEY,
    device_id INT NOT NULL,
    emergency_code CHAR(1) NOT NULL,
    temperature DECIMAL(5,2),
    humidity DECIMAL(5,2),
    gas_ppm INT,
    latitude DECIMAL(10,7),
    longitude DECIMAL(10,7),
    altitude INT,
    health_score INT,
    risk_score INT,
    rssi INT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```

### C. Web Dashboard UI Features
1. **Live GPS Map**: Render dynamic pins for all active Sensor Nodes using `latitude` and `longitude`. Color-code green for normal, red for emergency.
2. **Periodic Sensor Charts**: Display 24-hour time series graphs (Temperature, Humidity, Gas PPM) updated by the 1-hour automatic telemetry logs.
3. **Emergency Alert Banner**: Red flashing header banner with siren sound when emergency code `F`, `L`, `Q`, or `S` is received.

---

## 5. Completed Upgrades: Two-Way LoRa/BLE Communication & Tactical Display Suite

The following mission-critical tactical upgrades have been implemented, integrated into the firmware and client repositories, and verified:

### A. LifeLine TX Pro — Tactical "MESSAGE SENDING" Pop Screen
* **Interactive Modal Pop Screen**: When a custom BLE situation report is sent from a paired smartphone (`MSG:<text>`), the ST7789 display interrupts its current state and opens a dedicated modal pop screen.
* **Animated Progress & Closed-Loop ACK**:
  * Sweeping segmented progress bar animating at ~20 FPS across radio airtime and the 5,000ms ACK listen window.
  * Word-wrapped SITREP card displaying up to full message payload.
  * Real-time closed-loop confirmation banner (`[ACK CONFIRMED!]` / `[SENT (NO ACK)]`).
* **Non-Blocking Dismissal**: 4-second auto-dismiss or instant manual keypress dismissal smoothly restoring the previous screen (`SCREEN_BLE_PORTAL`, `SCREEN_MENU`, etc.).

### B. LifeLine RX Pro — Full 16×2 LCD Custom Message Display
* **Full 32-Character Utilization**: Dedicates both Row 0 and Row 1 (all 32 LCD characters) exclusively to message text rather than truncating messages with static headers.
* **Smart Word-Wrapping Engine (`formatLCDTwoRows`)**: Automatically calculates line breaks, scanning backwards up to 5 characters for word spaces to avoid splitting words across rows.
* **Dual Progression**:
  * **Hands-Free Auto-Page**: Messages > 28 characters auto-advance every 4 seconds without resetting the 15-second screen return timer.
  * **Manual Button Advance**: Short press on GPIO 14 (Wi-Fi button) advances the page instantly with a confirmation chirp and resets the 15-second timer.
* **Direct BLE Command Ingestion**: Ingests `MSG:<text>` and `CHAT:<text>` directly from incident commander phones via Nordic UART.

### C. Companion Ecosystem Tier
* **Tactical Mobile Companion (`lifeline_companion/`)**: React Native & Expo app featuring a 60 FPS sweeping Radar HUD, 48-character LoRa SITREP composer with emergency macros, and an offline mock BLE simulation engine.
* **Offline Web Bluetooth Suite (`bluefy_companion/` & `portal_preview/`)**: Zero-install PWAs compatible with iPhone Bluefy and Android Chrome with camera QR pairing.

---

## 6. Verification & Testing Checklist

- [x] **TX Pro BLE Pop Screen**: PlatformIO build clean, modal triggers on BLE chat, animates progress bar, updates ACK outcome, auto-dismisses after 4s.
- [x] **RX Pro Full 16×2 LCD**: PlatformIO build clean, dedicates Rows 0 & 1 to message, smart word-wraps, auto-pages every 4s, button advances page with chirp.
- [x] **Companion Apps**: Expo app boots cleanly; Web Bluetooth PWA connects and parses packet stream.
- [ ] **SPU Hardware**: Verify DHT, MQ135, MPU6050, and NEO-6M GPS read successfully.
- [ ] **SPU 1-Hour Timer**: Test setting `TELEMETRY_SEND_INTERVAL` to 10 seconds for debugging before setting to 1 Hour.
- [ ] **Automatic Anomaly Trigger**: Blow gas onto MQ135 or tilt MPU6050 past 60° to verify immediate emergency override.
- [ ] **Manual SOS Trigger**: Push physical SOS button or send code via TX Pro keypad.
- [ ] **CCU LoRa Transmission**: Verify CCU outputs full CSV string over Serial monitor.
- [ ] **RX Base Station Reception**: Verify RX parses CSV tokens correctly and displays normal telemetry vs alert screen.
- [ ] **Cloud Push Verification**: Confirm JSON POST arrives at web server with full sensor and GPS metrics.
