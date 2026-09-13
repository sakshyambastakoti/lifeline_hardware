# 🌐 LifeLine — Cloud REST API & Web Dashboard Integration Guide

> **Production Setup Specification for Ingestion of Distance, Custom BLE/LoRa SITREPs, Signal Quality (SNR), and Multi-Sensor Telemetry**  
> **Firmware Version:** v4.2 PRO (LifeLine RX Pro Gateway & TX Pro)  
> **Default Endpoint:** `https://zenithkandel.com.np/lifeline/API/Create/message.php` (Configurable via Base Station Web Portal)  

---

## 📌 1. Overview & Architecture

When emergency transmissions occur in off-grid terrain, the **LifeLine RX Pro Base Station** acts as an edge RF and BLE gateway. It ingests:
1. **Categorized Emergency SOS Alerts** (e.g. Delivery, Critical SOS, Landslide) from field handhelds over 433 MHz LoRa.
2. **Freeform Custom SITREP Chat Messages** typed on field smartphones and uplinked via LoRa (`CHAT:<devId>:<text>`).
3. **Local Commander Direct BLE Messages** (`MSG:<text>` / `CHAT:<text>`) entered via mobile companion apps at the base station.
4. **Periodic 1-Hour Full Multi-Sensor Telemetry Logs** (GPS, IMU tilt, Gas PPM, Temp, Humidity, Node Health & Risk).

The Base Station processes signal physics in real time (computing **RSSI in dBm**, **SNR in dB**, and **estimated distance in km**), renders them locally on the **Full 16×2 Character LCD**, and immediately posts a structured JSON payload via HTTPS to the central **LifeLine Cloud Web Platform**.

```text
┌─────────────────────────┐         433 MHz LoRa       ┌─────────────────────────┐
│     LifeLine TX Pro     │───────────────────────────►│     LifeLine RX Pro     │
│  (Field Handheld Unit)  │     (Alert / SITREP Chat)  │  (Base Station Gateway) │
└───────────┬─────────────┘                            └────────────┬────────────┘
            │ Local BLE                                             │ HTTPS POST JSON
            ▼                                                       ▼
┌─────────────────────────┐                            ┌─────────────────────────┐
│ Smartphone / Companion  │                            │ LifeLine Cloud Platform │
│  (Offline SITREP PWA)   │                            │  (Dashboard, DB & Maps) │
└─────────────────────────┘                            └─────────────────────────┘
```

---

## 📡 2. Cloud Ingestion API Specification

### Endpoint Details
* **URL**: `https://<YOUR_DOMAIN>/lifeline/API/Create/message.php`
* **HTTP Method**: `POST`
* **Headers**:
  ```http
  Content-Type: application/json
  X-API-Key: <OPTIONAL_AUTHENTICATION_KEY>
  Authorization: Bearer <OPTIONAL_AUTHENTICATION_KEY>
  ```

---

### 2.1 Complete JSON Request Schema

Below is the complete dictionary of fields transmitted by the LifeLine Base Station gateway:

| JSON Key | Data Type | Example | Required? | Source / Description |
| :--- | :--- | :--- | :---: | :--- |
| `DID` | Integer | `3` | **Yes** | Device ID (1..255 for remote field units, `0` for local Base Station commander). |
| `message_code` | Integer | `0` | **Yes** | Alert index (`0`..`14`). For freeform chat, set to `0` (High Priority Attention). |
| `code` | String (1 char) | `"A"`, `"M"`, `"N"` | Optional | Single-letter protocol code: `'A'-'O'` (SOS), `'M'` (Custom Chat/SITREP), `'N'` (Normal Telemetry). |
| `RSSI` | Integer | `-68` | **Yes** | Received Signal Strength Indicator in dBm. |
| `snr` | Float | `8.5` | **Yes** | Signal-to-Noise Ratio in dB (SX1278 physical layer packet quality). |
| `distance_km` | Float | `1.45` | **Yes** | **Estimated link distance in kilometers**, calculated using log-distance path loss and GPS. |
| `is_chat` | Boolean | `true` | **Yes** | `true` if this payload contains a freeform text SITREP / chat; `false` for standard alert codes. |
| `custom_msg` | String | `"3 trapped near bridge"` | Optional | **Full verbatim text of the custom message or situation report.** |
| `source` | String | `"LORA"` / `"BLE"` | **Yes** | Transmission medium: `"LORA"` (remote RF transmission) or `"BLE"` (local commander app). |
| `temp` | Float | `24.5` | Optional | Ambient temperature in °C (from SPU or sensor node). |
| `humidity` | Float | `65.2` | Optional | Relative humidity in % (from SPU). |
| `gas_ppm` | Integer | `320` | Optional | Air pollution / hazardous gas reading in PPM. |
| `lat` | Double | `27.717245` | Optional | GPS Latitude coordinate (decimal degrees). |
| `lon` | Double | `85.324000` | Optional | GPS Longitude coordinate (decimal degrees). |
| `alt` | Integer | `1350` | Optional | Altitude above sea level in meters. |
| `health` | Integer | `95` | Optional | Edge Node Health Score (0–100%). |
| `risk` | Integer | `80` | Optional | Environmental Composite Risk Score (0–100%). |
| `api_key` | String | `"secret_key"` | Optional | Embedded security key matching server configuration. |

---

### 2.2 Example Payloads

#### Example A: Custom SITREP Message Received via LoRa (Field Rescuer)
```json
{
  "DID": 3,
  "message_code": 0,
  "code": "M",
  "RSSI": -74,
  "snr": 7.2,
  "distance_km": 2.15,
  "is_chat": true,
  "custom_msg": "Landslide on north trail. 2 hikers stranded with hypothermia.",
  "source": "LORA",
  "api_key": "LF_PRO_KEY_2026"
}
```

#### Example B: Local Commander Message entered via Base Station BLE
```json
{
  "DID": 0,
  "message_code": 0,
  "code": "M",
  "RSSI": -50,
  "snr": 10.0,
  "distance_km": 0.01,
  "is_chat": true,
  "custom_msg": "Incident command established at Namche Bazaar. Heli dispatched.",
  "source": "BLE",
  "api_key": "LF_PRO_KEY_2026"
}
```

#### Example C: Standard Categorized Alert with Distance & Signal Telemetry
```json
{
  "DID": 2,
  "message_code": 1,
  "code": "B",
  "RSSI": -62,
  "snr": 9.8,
  "distance_km": 0.85,
  "is_chat": false,
  "custom_msg": "",
  "source": "LORA",
  "api_key": "LF_PRO_KEY_2026"
}
```

#### Example D: 1-Hour Full Multi-Sensor Telemetry Packet
```json
{
  "DID": 3,
  "message_code": 4,
  "code": "N",
  "temp": 18.2,
  "humidity": 78.4,
  "gas_ppm": 210,
  "lat": 27.717245,
  "lon": 85.324000,
  "alt": 1380,
  "health": 98,
  "risk": 15,
  "RSSI": -65,
  "snr": 9.0,
  "distance_km": 1.10,
  "is_chat": false,
  "custom_msg": "",
  "source": "LORA",
  "api_key": "LF_PRO_KEY_2026"
}
```

---

## 🗄️ 3. Database Schema & Migration SQL

To store distance, custom chat text, SNR, and transmission sources, update your central MySQL/MariaDB database with the following migration statements:

### Migration SQL (`database_migration_v42.sql`):
```sql
-- 1. Add new situational fields to the central emergency messages table
ALTER TABLE `messages` 
  ADD COLUMN `distance_km` DECIMAL(6,2) DEFAULT NULL AFTER `RSSI`,
  ADD COLUMN `snr` DECIMAL(4,1) DEFAULT NULL AFTER `distance_km`,
  ADD COLUMN `is_chat` TINYINT(1) DEFAULT 0 AFTER `snr`,
  ADD COLUMN `custom_msg` TEXT DEFAULT NULL AFTER `is_chat`,
  ADD COLUMN `source` ENUM('LORA', 'BLE', 'WEB') DEFAULT 'LORA' AFTER `custom_msg`;

-- 2. Add high-performance indexes for dashboard filtering and querying
CREATE INDEX idx_messages_chat ON `messages` (`is_chat`, `timestamp`);
CREATE INDEX idx_messages_source ON `messages` (`source`, `timestamp`);
CREATE INDEX idx_messages_did_time ON `messages` (`DID`, `timestamp`);

-- 3. Optional: Add distance and SNR to the historical telemetry logs table
ALTER TABLE `telemetry_logs`
  ADD COLUMN `distance_km` DECIMAL(6,2) DEFAULT NULL AFTER `rssi`,
  ADD COLUMN `snr` DECIMAL(4,1) DEFAULT NULL AFTER `distance_km`,
  ADD COLUMN `source` ENUM('LORA', 'BLE', 'WEB') DEFAULT 'LORA' AFTER `snr`;
```

---

## 💻 4. Production PHP Backend Implementation

Replace or update your server-side file at `API/Create/message.php`:

```php
<?php
/**
 * LifeLine Emergency Response System — Telemetry & Message Ingestion API
 * Handles: SOS alerts, Custom LoRa/BLE SITREP messages, and sensor data.
 */

header('Content-Type: application/json; charset=utf-8');
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: POST, OPTIONS');
header('Access-Control-Allow-Headers: Content-Type, Authorization, X-API-Key');

if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(200);
    exit;
}

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    http_response_code(405);
    echo json_encode(['status' => 'error', 'message' => 'Method not allowed. POST required.']);
    exit;
}

// 1. Database Connection (PDO Singleton)
require_once __DIR__ . '/../../database.php';
$db = Database::getInstance()->getConnection();

// 2. Read and parse incoming JSON payload
$rawBody = file_get_contents('php://input');
$payload = json_decode($rawBody, true);

if (!$payload) {
    http_response_code(400);
    echo json_encode(['status' => 'error', 'message' => 'Invalid JSON body provided.']);
    exit;
}

// 3. Extract and sanitize parameters
$did          = isset($payload['DID']) ? intval($payload['DID']) : null;
$messageCode  = isset($payload['message_code']) ? intval($payload['message_code']) : 0;
$rssi         = isset($payload['RSSI']) ? intval($payload['RSSI']) : null;
$snr          = isset($payload['snr']) ? floatval($payload['snr']) : 0.0;
$distanceKm   = isset($payload['distance_km']) ? floatval($payload['distance_km']) : null;
$isChat       = !empty($payload['is_chat']) ? 1 : 0;
$customMsg    = isset($payload['custom_msg']) ? trim($payload['custom_msg']) : null;
$source       = isset($payload['source']) && in_array(strtoupper($payload['source']), ['LORA', 'BLE', 'WEB']) ? strtoupper($payload['source']) : 'LORA';
$apiKey       = $payload['api_key'] ?? $_SERVER['HTTP_X_API_KEY'] ?? '';

if ($did === null || $rssi === null) {
    http_response_code(422);
    echo json_encode(['status' => 'error', 'message' => 'Missing required fields: DID and RSSI are mandatory.']);
    exit;
}

try {
    // 4. Insert into `messages` table
    $stmt = $db->prepare("
        INSERT INTO `messages` 
            (`DID`, `message_code`, `RSSI`, `distance_km`, `snr`, `is_chat`, `custom_msg`, `source`, `timestamp`)
        VALUES 
            (:did, :code, :rssi, :dist, :snr, :is_chat, :custom_msg, :source, NOW())
    ");

    $stmt->execute([
        ':did'        => $did,
        ':code'       => $messageCode,
        ':rssi'       => $rssi,
        ':dist'       => $distanceKm,
        ':snr'        => $snr,
        ':is_chat'    => $isChat,
        ':custom_msg' => $customMsg,
        ':source'     => $source
    ]);

    $mid = $db->lastInsertId();

    // 5. Update device last_ping timestamp
    $updateStmt = $db->prepare("UPDATE `devices` SET `last_ping` = NOW(), `status` = 'active' WHERE `DID` = :did");
    $updateStmt->execute([':did' => $did]);

    // 6. Optional: Log sensor metrics if telemetry fields are present
    if (isset($payload['temp']) || isset($payload['lat'])) {
        $telemStmt = $db->prepare("
            INSERT INTO `telemetry_logs`
                (`device_id`, `emergency_code`, `temperature`, `humidity`, `gas_ppm`, `latitude`, `longitude`, `altitude`, `health_score`, `risk_score`, `rssi`, `distance_km`, `snr`, `source`, `created_at`)
            VALUES
                (:did, :code, :temp, :hum, :gas, :lat, :lon, :alt, :health, :risk, :rssi, :dist, :snr, :source, NOW())
        ");
        $telemStmt->execute([
            ':did'     => $did,
            ':code'    => $payload['code'] ?? 'N',
            ':temp'    => $payload['temp'] ?? null,
            ':hum'     => $payload['humidity'] ?? null,
            ':gas'     => $payload['gas_ppm'] ?? null,
            ':lat'     => $payload['lat'] ?? null,
            ':lon'     => $payload['lon'] ?? null,
            ':alt'     => $payload['alt'] ?? null,
            ':health'  => $payload['health'] ?? 100,
            ':risk'    => $payload['risk'] ?? 0,
            ':rssi'    => $rssi,
            ':dist'    => $distanceKm,
            ':snr'     => $snr,
            ':source'  => $source
        ]);
    }

    // 7. Success response
    http_response_code(201);
    echo json_encode([
        'status'      => 'success',
        'message'     => 'Emergency message recorded successfully',
        'MID'         => $mid,
        'DID'         => $did,
        'is_chat'     => (bool)$isChat,
        'distance_km' => $distanceKm,
        'source'      => $source
    ]);

} catch (Exception $e) {
    http_response_code(500);
    echo json_encode(['status' => 'error', 'message' => 'Database error: ' . $e->getMessage()]);
}
```

---

## 🖥️ 5. Web Dashboard Display Implementation

To display **Distance**, **Custom Chat SITREPs**, and **SNR** on your website (`portal/dashboard.php` / `portal/js/dashboard.js`):

### 5.1 Distance Badge & Proximity Radar
```javascript
// Render distance in kilometers with dynamic precision and terrain indicator
function formatDistanceBadge(distanceKm) {
    if (distanceKm === null || distanceKm === undefined) {
        return `<span class="badge badge-muted">Distance Unknown</span>`;
    }
    const dist = parseFloat(distanceKm);
    if (dist < 0.1) {
        return `<span class="badge badge-local"><i class="icon-bluetooth"></i> Local Base Commander (< 100m)</span>`;
    } else if (dist < 2.0) {
        return `<span class="badge badge-success"><i class="icon-radar"></i> ${dist.toFixed(2)} km (Immediate Sector)</span>`;
    } else if (dist < 5.0) {
        return `<span class="badge badge-warning"><i class="icon-radar"></i> ${dist.toFixed(2)} km (Mountain Ridge Line-of-Sight)</span>`;
    } else {
        return `<span class="badge badge-danger"><i class="icon-radar"></i> ${dist.toFixed(2)} km (Deep Canyon / Extended Range)</span>`;
    }
}
```

### 5.2 Custom SITREP Chat Feed Card
```html
<!-- Real-time HTML component for live dashboard feed -->
<div class="incident-card ${msg.is_chat ? 'chat-incident' : 'standard-incident'}">
  <div class="incident-header">
    <span class="source-tag source-${msg.source.toLowerCase()}">${msg.source}</span>
    <span class="device-id">Node #${msg.DID}</span>
    <span class="distance-tag">${formatDistanceBadge(msg.distance_km)}</span>
    <span class="signal-tag">${msg.RSSI} dBm (SNR: ${msg.snr} dB)</span>
    <span class="timestamp">${msg.timestamp}</span>
  </div>

  <div class="incident-body">
    ${msg.is_chat ? `
      <div class="sitrep-bubble">
        <i class="icon-chat-quote"></i>
        <span class="sitrep-text">${escapeHtml(msg.custom_msg)}</span>
      </div>
    ` : `
      <div class="alert-banner">
        <strong>${msg.alert_name}</strong>
        <p class="alert-desc">${msg.alert_description}</p>
      </div>
    `}
  </div>
</div>
```

---

## 🧪 6. Testing & Validation (`curl` Examples)

Run these command-line tests to verify that your web API correctly receives and records all new fields:

### Test 1: Ingesting a Custom LoRa SITREP Message
```bash
curl -X POST https://zenithkandel.com.np/lifeline/API/Create/message.php \
  -H "Content-Type: application/json" \
  -d '{
    "DID": 3,
    "message_code": 0,
    "code": "M",
    "RSSI": -75,
    "snr": 8.2,
    "distance_km": 2.45,
    "is_chat": true,
    "custom_msg": "Trail impassable at Dudh Koshi bridge. Requesting medical team.",
    "source": "LORA"
  }'
```

**Expected Response**:
```json
{
  "status": "success",
  "message": "Emergency message recorded successfully",
  "MID": 1042,
  "DID": 3,
  "is_chat": true,
  "distance_km": 2.45,
  "source": "LORA"
}
```

---

### Test 2: Ingesting a Local Base Station Commander Message via BLE
```bash
curl -X POST https://zenithkandel.com.np/lifeline/API/Create/message.php \
  -H "Content-Type: application/json" \
  -d '{
    "DID": 0,
    "message_code": 0,
    "code": "M",
    "RSSI": -48,
    "snr": 11.0,
    "distance_km": 0.01,
    "is_chat": true,
    "custom_msg": "Incident commander on scene. Setting up satellite uplink.",
    "source": "BLE"
  }'
```

---

### Test 3: Querying the Message via API to Verify Saved Fields
```bash
curl -X GET "https://zenithkandel.com.np/lifeline/API/Read/message.php?limit=1"
```

Verify that the returned JSON object includes `distance_km`, `snr`, `is_chat`, `custom_msg`, and `source`.
