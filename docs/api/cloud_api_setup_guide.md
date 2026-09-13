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

---

## 📡 8. Bidirectional Two-Way Communication (Website ➔ RX Gateway ➔ TX Handheld)

LifeLine supports full **two-way closed-loop messaging**. Incident commanders on the central website or local base station can compose custom SITREPs, operational orders, and evacuation advisories that are delivered directly onto the screens of field handhelds in off-grid disaster zones.

```text
┌────────────────────────────────┐
│   LifeLine Central Website     │  (Incident Commander Dashboard)
│      (Web Dispatch Console)    │
└────────────────────────────────┘
               │  1. HTTP POST (Queue message)
               ▼
┌────────────────────────────────┐
│       Cloud REST API           │  (MySQL Database: downlink_commands table)
│   (zenithkandel.com.np)        │
└────────────────────────────────┘
               │  2. HTTP GET Poll (every 3.5s over Wi-Fi)
               ▼
┌────────────────────────────────┐
│      LifeLine RX Pro           │  • Sounds alert tone & displays on Full 16×2 LCD
│    (Base Station Gateway)      │  • Sends HTTP POST Ack (DISPATCHED_LORA) back to Cloud
└────────────────────────────────┘
               │  3. 433 MHz RF LoRa (CMD<did>,<action>,<msg>)
               ▼
┌────────────────────────────────┐
│      LifeLine TX Pro           │  • Displays popup on Handheld Screen
│    (Field Handheld Unit)       │  • Sounds audio buzzer & status LED
└────────────────────────────────┘  • Pushes BLE notification to Responder's Phone
               │
               ▼  4. Field Responder replies via BLE App (CHAT:<devId>:<reply>)
┌────────────────────────────────┐
│    Full Loop Uplink Return     │  LoRa CHAT ➔ RX Base Gateway ➔ Cloud API ➔ Website Feed
└────────────────────────────────┘
```

---

### 8.1 Database Migration: `downlink_commands` Table

Run the following SQL migration on your MySQL server:

```sql
-- Migration: Add downlink_commands table for bidirectional Web-to-LoRa dispatching
CREATE TABLE IF NOT EXISTS `downlink_commands` (
    `id` INT AUTO_INCREMENT PRIMARY KEY,
    `rx_id` INT NOT NULL DEFAULT 1 COMMENT 'Target Base Station Gateway ID',
    `target_did` INT NOT NULL DEFAULT 0 COMMENT '0 = Broadcast to All TX units; 1-999 = Specific TX device',
    `action` VARCHAR(32) NOT NULL DEFAULT 'MSG' COMMENT 'MSG, DISPATCH, EVAC, MEDIC, PING, ALL_CLEAR',
    `message` VARCHAR(96) NOT NULL COMMENT 'Command text / SITREP (max 48-96 characters)',
    `status` ENUM('PENDING', 'DISPATCHED_LORA', 'TX_FAILED', 'ACK_CONFIRMED') NOT NULL DEFAULT 'PENDING',
    `lora_tx_ok` TINYINT(1) DEFAULT 0 COMMENT '1 if RF packet transmission succeeded',
    `created_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    `dispatched_at` TIMESTAMP NULL DEFAULT NULL,
    `acknowledged_at` TIMESTAMP NULL DEFAULT NULL,
    INDEX `idx_rx_status` (`rx_id`, `status`),
    INDEX `idx_target_did` (`target_did`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

---

### 8.2 Server-Side PHP Endpoints

#### 1. Queue Outgoing Web Command: `API/Create/command.php`

Used by the web dashboard when an incident commander clicks **"Transmit via LoRa"**:

```php
<?php
// API/Create/command.php
header("Access-Control-Allow-Origin: *");
header("Access-Control-Allow-Methods: POST, OPTIONS");
header("Access-Control-Allow-Headers: Content-Type, Authorization, X-API-Key");
header("Content-Type: application/json; charset=UTF-8");

if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(200);
    exit();
}

require_once '../Config/database.php';
$database = new Database();
$db = $database->getConnection();

$raw = file_get_contents("php://input");
$data = json_decode($raw);

if (!$data || !isset($data->message) || trim($data->message) === '') {
    http_response_code(400);
    echo json_encode(["status" => "error", "message" => "Message content is required"]);
    exit();
}

$rx_id      = isset($data->rx_id) ? (int)$data->rx_id : 1;
$target_did = isset($data->target_did) ? (int)$data->target_did : 0;
$action     = isset($data->action) ? strtoupper(trim($data->action)) : 'MSG';
$message    = substr(trim($data->message), 0, 96);

// Sanitize message - replace commas and newlines for LoRa CSV safety
$message = str_replace([",", "\n", "\r"], [" ", " ", ""], $message);

$query = "INSERT INTO downlink_commands (rx_id, target_did, action, message, status) 
          VALUES (:rx_id, :target_did, :action, :message, 'PENDING')";
$stmt = $db->prepare($query);
$stmt->bindParam(":rx_id", $rx_id);
$stmt->bindParam(":target_did", $target_did);
$stmt->bindParam(":action", $action);
$stmt->bindParam(":message", $message);

if ($stmt->execute()) {
    $cmd_id = $db->lastInsertId();
    http_response_code(201);
    echo json_encode([
        "status" => "success",
        "message" => "Command queued for LoRa dispatch",
        "command_id" => (int)$cmd_id,
        "target_did" => $target_did,
        "action" => $action,
        "text" => $message
    ]);
} else {
    http_response_code(500);
    echo json_encode(["status" => "error", "message" => "Failed to queue command"]);
}
?>
```

---

#### 2. Base Station Polling Endpoint: `API/Read/pending_commands.php`

Called by `lifeline_rx_pro` every 3.5 seconds over Wi-Fi:

```php
<?php
// API/Read/pending_commands.php
header("Access-Control-Allow-Origin: *");
header("Content-Type: application/json; charset=UTF-8");

require_once '../Config/database.php';
$database = new Database();
$db = $database->getConnection();

$rx_id = isset($_GET['rx_id']) ? (int)$_GET['rx_id'] : 1;

// Retrieve the oldest pending command for this gateway
$query = "SELECT id, rx_id, target_did, action, message, created_at 
          FROM downlink_commands 
          WHERE rx_id = :rx_id AND status = 'PENDING' 
          ORDER BY id ASC LIMIT 1";
$stmt = $db->prepare($query);
$stmt->bindParam(":rx_id", $rx_id);
$stmt->execute();

if ($stmt->rowCount() > 0) {
    $row = $stmt->fetch(PDO::FETCH_ASSOC);
    http_response_code(200);
    echo json_encode([
        "status" => "success",
        "has_command" => true,
        "command_id" => (int)$row['id'],
        "target_did" => (int)$row['target_did'],
        "action" => $row['action'],
        "message" => $row['message'],
        "created_at" => $row['created_at']
    ]);
} else {
    http_response_code(200);
    echo json_encode([
        "status" => "success",
        "has_command" => false
    ]);
}
?>
```

---

#### 3. Gateway Status Confirmation: `API/Update/command_status.php`

Called by `lifeline_rx_pro` immediately after transmitting over LoRa:

```php
<?php
// API/Update/command_status.php
header("Access-Control-Allow-Origin: *");
header("Access-Control-Allow-Methods: POST, OPTIONS");
header("Access-Control-Allow-Headers: Content-Type, Authorization, X-API-Key");
header("Content-Type: application/json; charset=UTF-8");

if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(200);
    exit();
}

require_once '../Config/database.php';
$database = new Database();
$db = $database->getConnection();

$raw = file_get_contents("php://input");
$data = json_decode($raw);

if (!$data || !isset($data->command_id)) {
    http_response_code(400);
    echo json_encode(["status" => "error", "message" => "command_id is required"]);
    exit();
}

$command_id = (int)$data->command_id;
$status     = isset($data->status) ? trim($data->status) : 'DISPATCHED_LORA';
$lora_tx_ok = (isset($data->lora_tx_ok) && $data->lora_tx_ok) ? 1 : 0;

$query = "UPDATE downlink_commands 
          SET status = :status, 
              lora_tx_ok = :lora_tx_ok, 
              dispatched_at = CURRENT_TIMESTAMP 
          WHERE id = :command_id";
$stmt = $db->prepare($query);
$stmt->bindParam(":status", $status);
$stmt->bindParam(":lora_tx_ok", $lora_tx_ok);
$stmt->bindParam(":command_id", $command_id);

if ($stmt->execute()) {
    http_response_code(200);
    echo json_encode([
        "status" => "success",
        "message" => "Command status updated successfully",
        "command_id" => $command_id,
        "new_status" => $status
    ]);
} else {
    http_response_code(500);
    echo json_encode(["status" => "error", "message" => "Failed to update command status"]);
}
?>
```

---

### 8.3 Web Dashboard Two-Way Dispatch Console Component

Integrate this interactive component into your web portal (`dashboard.php` or `index.html`). It connects to `API/Create/command.php` and polls `API/Read/message.php` for incoming replies:

```html
<!-- LifeLine Two-Way Downlink Dispatcher Widget -->
<div class="dispatch-console-card">
  <div class="dispatch-header">
    <h3><span class="pulse-dot"></span> Two-Way LoRa Dispatch Console</h3>
    <span class="badge-gateway">RX Gateway #01 (Active)</span>
  </div>

  <div class="dispatch-form">
    <div class="form-row">
      <div class="form-group">
        <label for="targetDid">Target Field Unit:</label>
        <select id="targetDid" class="console-input">
          <option value="0">📢 Broadcast to All Units (TX #000)</option>
          <option value="1" selected>👤 Node #001 (Rescue Team Alpha)</option>
          <option value="2">👤 Node #002 (Medical Team Bravo)</option>
          <option value="3">👤 Node #003 (Evacuation Team Charlie)</option>
        </select>
      </div>

      <div class="form-group">
        <label for="cmdAction">Action Preset:</label>
        <select id="cmdAction" class="console-input" onchange="applyPresetAction(this.value)">
          <option value="MSG">💬 Custom SITREP / Message</option>
          <option value="DISPATCH">🚁 Team Dispatched</option>
          <option value="MEDIC">🚑 Medic En Route</option>
          <option value="EVAC">⚠️ Evacuate Immediately</option>
          <option value="PING">📡 Ping / Status Check</option>
        </select>
      </div>
    </div>

    <div class="form-group">
      <div class="label-row">
        <label for="customMessage">Message Text (Max 48 chars recommended for LoRa):</label>
        <span id="charCount" class="char-counter">0 / 48</span>
      </div>
      <input type="text" id="customMessage" class="console-input message-box" 
             placeholder="Type message for field unit..." maxlength="60" 
             oninput="updateCharCount(this)">
    </div>

    <div class="dispatch-actions">
      <button id="sendLoRaBtn" class="btn-dispatch" onclick="submitLoRaDownlink()">
        <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
          <line x1="22" y1="2" x2="11" y2="13"></line>
          <polygon points="22 2 15 22 11 13 2 9 22 2"></polygon>
        </svg>
        Transmit via LoRa Downlink
      </button>
      <span id="dispatchStatus" class="dispatch-status">Ready</span>
    </div>
  </div>

  <!-- Bidirectional Chat & SITREP Stream -->
  <div class="conversation-stream" id="conversationStream">
    <div class="stream-header">Live Field Communications & SITREPs</div>
    <div class="stream-logs" id="streamLogs">
      <!-- Dynamically populated -->
    </div>
  </div>
</div>

<style>
.dispatch-console-card {
  background: rgba(16, 24, 39, 0.85);
  backdrop-filter: blur(12px);
  border: 1px solid rgba(255, 255, 255, 0.1);
  border-radius: 12px;
  padding: 20px;
  color: #f3f4f6;
  font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
  margin-bottom: 24px;
}
.dispatch-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 16px;
  border-bottom: 1px solid rgba(255,255,255,0.08);
  padding-bottom: 12px;
}
.pulse-dot {
  display: inline-block;
  width: 10px;
  height: 10px;
  background: #10b981;
  border-radius: 50%;
  box-shadow: 0 0 10px #10b981;
  margin-right: 8px;
}
.badge-gateway {
  font-size: 0.8rem;
  background: rgba(59, 130, 246, 0.2);
  color: #60a5fa;
  border: 1px solid rgba(59, 130, 246, 0.4);
  padding: 4px 10px;
  border-radius: 20px;
}
.form-row {
  display: flex;
  gap: 16px;
  margin-bottom: 12px;
}
.form-group {
  flex: 1;
  display: flex;
  flex-direction: column;
}
.form-group label {
  font-size: 0.82rem;
  color: #9ca3af;
  margin-bottom: 6px;
}
.label-row {
  display: flex;
  justify-content: space-between;
}
.char-counter {
  font-size: 0.8rem;
  color: #9ca3af;
}
.console-input {
  background: rgba(10, 15, 26, 0.8);
  border: 1px solid rgba(255, 255, 255, 0.15);
  border-radius: 8px;
  padding: 10px 14px;
  color: #fff;
  font-size: 0.95rem;
}
.console-input:focus {
  border-color: #3b82f6;
  outline: none;
}
.dispatch-actions {
  display: flex;
  align-items: center;
  gap: 16px;
  margin-top: 14px;
}
.btn-dispatch {
  background: linear-gradient(135deg, #2563eb, #1d4ed8);
  color: #fff;
  border: none;
  border-radius: 8px;
  padding: 10px 20px;
  font-size: 0.95rem;
  font-weight: 600;
  display: flex;
  align-items: center;
  gap: 8px;
  cursor: pointer;
  transition: 0.2s;
}
.btn-dispatch:hover {
  background: linear-gradient(135deg, #3b82f6, #2563eb);
}
.dispatch-status {
  font-size: 0.85rem;
  color: #9ca3af;
}
.conversation-stream {
  margin-top: 20px;
  border-top: 1px solid rgba(255, 255, 255, 0.08);
  padding-top: 14px;
}
.stream-header {
  font-size: 0.85rem;
  text-transform: uppercase;
  letter-spacing: 0.05em;
  color: #9ca3af;
  margin-bottom: 10px;
}
.stream-logs {
  max-height: 220px;
  overflow-y: auto;
  display: flex;
  flex-direction: column;
  gap: 8px;
}
.msg-bubble {
  padding: 8px 14px;
  border-radius: 8px;
  font-size: 0.88rem;
  max-width: 80%;
}
.msg-downlink {
  background: rgba(37, 99, 235, 0.2);
  border-left: 4px solid #3b82f6;
  align-self: flex-start;
}
.msg-uplink {
  background: rgba(16, 185, 129, 0.2);
  border-left: 4px solid #10b981;
  align-self: flex-end;
}
</style>

<script>
function updateCharCount(input) {
  document.getElementById('charCount').textContent = input.value.length + " / 48";
}

function applyPresetAction(val) {
  const msgInput = document.getElementById('customMessage');
  if (val === 'MEDIC') msgInput.value = "Medic en route to your position";
  else if (val === 'EVAC') msgInput.value = "EVACUATE immediately to Sector 4";
  else if (val === 'DISPATCH') msgInput.value = "Search team dispatched to GPS loc";
  else if (val === 'PING') msgInput.value = "Confirm operational safety status";
  updateCharCount(msgInput);
}

async function submitLoRaDownlink() {
  const did = document.getElementById('targetDid').value;
  const action = document.getElementById('cmdAction').value;
  const msg = document.getElementById('customMessage').value.trim();
  const statusElem = document.getElementById('dispatchStatus');
  const btn = document.getElementById('sendLoRaBtn');

  if (!msg) {
    alert("Please enter a message to send.");
    return;
  }

  btn.disabled = true;
  statusElem.textContent = "Queuing command...";
  statusElem.style.color = "#f59e0b";

  try {
    const res = await fetch("https://zenithkandel.com.np/lifeline/API/Create/command.php", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({
        rx_id: 1,
        target_did: parseInt(did),
        action: action,
        message: msg
      })
    });
    const result = await res.json();

    if (result.status === "success") {
      statusElem.textContent = `✓ Queued (ID #${result.command_id}) ➔ Gateway will dispatch in ~3s`;
      statusElem.style.color = "#10b981";
      document.getElementById('customMessage').value = "";
      updateCharCount(document.getElementById('customMessage'));
      
      // Append to live conversation preview
      const logs = document.getElementById('streamLogs');
      const item = document.createElement('div');
      item.className = "msg-bubble msg-downlink";
      item.innerHTML = `<strong>WEB ➔ TX #${did == 0 ? 'ALL' : did} [${action}]:</strong> ${msg} <span style="font-size:0.75rem; color:#9ca3af; margin-left:8px;">Just now</span>`;
      logs.prepend(item);
    } else {
      statusElem.textContent = "Failed to queue command.";
      statusElem.style.color = "#ef4444";
    }
  } catch (err) {
    statusElem.textContent = "Network error: " + err.message;
    statusElem.style.color = "#ef4444";
  } finally {
    btn.disabled = false;
  }
}
</script>
```

---

### 8.4 Local Gateway Direct LoRa Dispatch (`/send-downlink`)

If operating in field headquarters where the Base Station gateway is operating locally on a LAN without internet connectivity, commanders can connect directly to the ESP32 Base Station web portal (`http://192.168.4.1` or the assigned LAN IP) and trigger LoRa downlinks directly:

```bash
# Direct HTTP POST to Base Station ESP32 Gateway
curl -X POST http://192.168.4.1/send-downlink \
  -d "did=1" \
  -d "action=MSG" \
  -d "msg=Stay in camp, supplies inbound"
```

Response:
```json
{
  "status": "success",
  "did": 1,
  "lora_tx_ok": true
}
```

---

### 8.5 Testing Bidirectional LoRa Downlink with `curl`

#### Step 1: Queue a Command from Central Website
```bash
curl -X POST https://zenithkandel.com.np/lifeline/API/Create/command.php \
  -H "Content-Type: application/json" \
  -d '{
    "rx_id": 1,
    "target_did": 1,
    "action": "DISPATCH",
    "message": "Medic helicopter en route to your LZ"
  }'
```

Output:
```json
{
  "status": "success",
  "message": "Command queued for LoRa dispatch",
  "command_id": 105,
  "target_did": 1,
  "action": "DISPATCH",
  "text": "Medic helicopter en route to your LZ"
}
```

#### Step 2: Simulate Gateway Poll
```bash
curl -X GET "https://zenithkandel.com.np/lifeline/API/Read/pending_commands.php?rx_id=1"
```

Output:
```json
{
  "status": "success",
  "has_command": true,
  "command_id": 105,
  "target_did": 1,
  "action": "DISPATCH",
  "message": "Medic helicopter en route to your LZ",
  "created_at": "2026-09-13 10:15:00"
}
```

#### Step 3: Gateway Acknowledges RF LoRa Transmission
```bash
curl -X POST https://zenithkandel.com.np/lifeline/API/Update/command_status.php \
  -H "Content-Type: application/json" \
  -d '{
    "command_id": 105,
    "rx_id": 1,
    "status": "DISPATCHED_LORA",
    "lora_tx_ok": true
  }'
```

Output:
```json
{
  "status": "success",
  "message": "Command status updated successfully",
  "command_id": 105,
  "new_status": "DISPATCHED_LORA"
}
```

