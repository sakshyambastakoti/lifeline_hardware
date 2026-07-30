# LifeLine SPU Cloud Telemetry API & Notification Specification

## 1. Overview & Connection Architecture

The **Sensor Processing Unit (SPU)** operates as an autonomous edge-sensing node within the LifeLine ecosystem. Every **3 seconds** (`3000 ms`), the SPU packages all real-time sensor measurements, calculated health metrics, motion vectors, GPS coordinates, and emergency condition flags into a single structured JSON payload and transmits it to the Cloud API endpoint.

* **API Endpoint URL**: `https://zenithkandel.com.np/lifeline/api/recieve.php`
* **HTTP Method**: `POST`
* **Content-Type**: `application/json`
* **Transmission Interval**: Every 3 seconds (periodic heartbeat & real-time telemetry)
* **Payload Structure**: Dynamic JSON containing complete multi-sensor telemetry packet

---

## 2. Telemetry JSON Payload Specification

Below is the complete dictionary of every field transmitted by the SPU in each 3-second cycle.

### JSON Payload Schema Table

| JSON Key | Data Type | Unit | Source Sensor / Module | Description & Valid Ranges |
| :--- | :--- | :--- | :--- | :--- |
| `DID` | Integer | - | Config (`SPU_DEVICE_ID`) | Device Unique ID (Default: `3` for SPU node) |
| `device_name` | String | - | Config | Device hardware identity string (`"LifeLine SPU"`) |
| `firmware` | String | - | Firmware Build | Firmware version identifier (`"v3.1.0 SPU"`) |
| `uptime_sec` | Unsigned Long | Seconds | ESP32 System (`millis()`) | Total system uptime in seconds |
| `message_code` | Integer | - | State Mapper | Mapped message classification code (`0`: Critical Alert, `1`: Medical, `11`: Landslide, `13`: Battery Alert, `4`: Normal Nominal) |
| `code` | Char / String | - | Emergency Fusion Engine | Single-character ASCII hazard code (`'N'`, `'L'`, `'Q'`, `'F'`, `'G'`, `'H'`, `'C'`, `'W'`, `'S'`, `'B'`) |
| `emergency_desc` | String | - | Emergency Fusion Engine | Human-readable emergency description string |
| `priority` | Integer (1-5) | - | Fusion Matrix | Urgency level: `1` (Normal), `2` (Low), `3` (Medium), `4` (High), `5` (Critical SOS) |
| `temp` | Float | °C | DHT11 / DHT22 | Ambient Temperature in Celsius |
| `humidity` | Float | % | DHT11 / DHT22 | Ambient Relative Humidity percentage |
| `pressure` | Float | hPa | Barometer / Simulated | Atmospheric Pressure in Hectopascals |
| `gas_ppm` | Unsigned Int | PPM | MQ135 Air Quality Sensor | Gas & Air Pollution concentration estimate (PPM) |
| `accel_x` | Float | G | MPU6050 (3-Axis IMU) | Accelerometer X-axis force in Gs |
| `accel_y` | Float | G | MPU6050 (3-Axis IMU) | Accelerometer Y-axis force in Gs |
| `accel_z` | Float | G | MPU6050 (3-Axis IMU) | Accelerometer Z-axis force in Gs |
| `gyro_x` | Float | °/s | MPU6050 (3-Axis IMU) | Gyroscope X-axis rotation rate |
| `gyro_y` | Float | °/s | MPU6050 (3-Axis IMU) | Gyroscope Y-axis rotation rate |
| `gyro_z` | Float | °/s | MPU6050 (3-Axis IMU) | Gyroscope Z-axis rotation rate |
| `total_g` | Float | G | Computed (`sqrt(X²+Y²+Z²)`) | Total vector acceleration magnitude |
| `tilt_deg` | Float | Degrees | Computed (`MPU6050`) | Angular inclination tilt relative to horizontal |
| `is_moving` | Boolean | - | IMU Motion Engine | `true` if motion/vibration is above baseline |
| `sudden_impact` | Boolean | - | IMU Spike Engine | `true` if acceleration spike exceeds threshold (> 3.0G) |
| `free_fall` | Boolean | - | IMU Free-Fall Engine | `true` if weightlessness detected (< 0.3G total acceleration) |
| `seismic_vibe` | Boolean | - | IMU Earthquake Engine | `true` if continuous high-frequency vibration detected (> 1.8G for 15+ samples) |
| `gps_fix` | Boolean | - | NEO-6M GPS Module | `true` if valid satellite lock acquired |
| `satellites` | Unsigned Int | Count | NEO-6M GPS Module | Number of visible locked GPS satellites |
| `lat` | Float | Degrees | NEO-6M GPS Module | Geographic Latitude (Decimal degrees, e.g. 27.717245) |
| `lon` | Float | Degrees | NEO-6M GPS Module | Geographic Longitude (Decimal degrees, e.g. 85.324000) |
| `alt` | Float | Meters | NEO-6M GPS Module | Altitude above sea level in meters |
| `health` | Unsigned Int | % (0-100) | Health Calculator | Node System Health Score |
| `risk` | Unsigned Int | % (0-100) | Risk Calculator | Combined Environmental & Disaster Risk Index |
| `battery` | Unsigned Int | % (0-100) | Power Management | Battery charge state percentage |
| `RSSI` | Integer | dBm | ESP32 Wi-Fi | Wi-Fi Signal Strength in dBm |

---

## 3. Sensor Change & Hazard Condition Notification Matrix

When the Cloud API at `zenithkandel.com.np/lifeline/api/recieve.php` receives a telemetry packet every 3 seconds, it inspects the sensor values and hazard flags. 

If any critical sensor threshold is exceeded or state transition occurs, **Push Notifications** and **SMS Alerts** must be triggered immediately according to the routing target rules below:

### Hazard Notification Trigger Rules

```
┌────────────────────────────────────────────────────────────────────────────────────────┐
│                              SENSOR HAZARD ROUTING MATRIX                              │
├───────────────────┬──────────────────────────────────┬──────────┬──────────────────────┤
│ Hazard Type       │ Trigger Condition                │ Priority │ Notification Targets │
├───────────────────┼──────────────────────────────────┼──────────┼──────────────────────┤
│ Critical Landslide│ code == 'L' OR                   │ HIGH (4) │ • Mobile App FCM Push│
│                   │ (tilt_deg > 60° AND              │          │ • Emergency Command  │
│                   │  (sudden_impact OR is_moving))   │          │ • SMS to Rescue Team │
├───────────────────┼──────────────────────────────────┼──────────┼──────────────────────┤
│ Free Fall / Severe│ free_fall == true OR             │ HIGH (4) │ • Mobile App Push    │
│ Fall Impact       │ sudden_impact == true            │ CRITICAL │ • First Responder SMS│
│                   │ (total_g > 3.0G or < 0.3G)       │          │ • Web Dashboard Alert│
├───────────────────┼──────────────────────────────────┼──────────┼──────────────────────┤
│ Earthquake /      │ code == 'Q' OR                   │ CRITICAL │ • Mass Push Broadcast│
│ Seismic Anomaly   │ seismic_vibe == true             │ (5)      │ • Disaster Mgmt SMS  │
│                   │ (Sustained > 1.8G vibration)     │          │ • Web Live Red Banner│
├───────────────────┼──────────────────────────────────┼──────────┼──────────────────────┤
│ Forest Fire / Gas │ code == 'F' OR                   │ CRITICAL │ • Fire Dept & Push   │
│ Explosion         │ (gas_ppm > 600 AND temp > 42°C)  │ (5)      │ • Local SMS Alerts   │
├───────────────────┼──────────────────────────────────┼──────────┼──────────────────────┤
│ Hazardous Gas Leak│ code == 'G' OR gas_ppm > 600     │ HIGH (4) │ • Hazmat Push Alert  │
│ / Air Pollution   │                                  │          │ • Admin Console      │
├───────────────────┼──────────────────────────────────┼──────────┼──────────────────────┤
│ Extreme Heat /    │ code == 'H' (temp >= 45°C) OR    │ MEDIUM   │ • Mobile Push Alert  │
│ Cold Wave         │ code == 'C' (temp <= 0°C)        │ (3)      │ • Daily Weather Log  │
├───────────────────┼──────────────────────────────────┼──────────┼──────────────────────┤
│ Low Battery /     │ battery < 20% OR health < 50%    │ LOW (2)  │ • Maintenance Push   │
│ Health Degraded   │                                  │          │ • Admin Console Log  │
└───────────────────┴──────────────────────────────────┴──────────┴──────────────────────┘
```

---

## 4. Where & How to Push Notifications (Backend Processing Guide)

### Notification Channels & Target Routing

1. **Mobile Push Notifications (Firebase Cloud Messaging - FCM)**:
   * **Target Topic**: `/topics/lifeline_emergencies` (All user devices), `/topics/spu_node_3` (Geofenced users).
   * **Payload Data**: Includes `DID`, `lat`, `lon`, `emergency_desc`, `priority`, and timestamp.
   * **Action**: Displays high-priority pop-up notification with siren sound on mobile devices.

2. **SMS Emergency Alerts (Twilio / GSM Gateway API)**:
   * **Target Recipients**: Emergency Command Officers, Disaster Response Leads, Registered Field Responders.
   * **SMS Message Format**:
     ```
     🚨 LIFELINE CRITICAL ALERT [SPU Node #3]
     Type: POSSIBLE LANDSLIDE / COLLAPSE
     Location: https://maps.google.com/?q=27.717245,85.324000
     Tilt: 64.5°, Impact: YES, Health: 85%
     Time: 2026-07-30 21:37:51
     ```

3. **Web Dashboard & Live Map (WebSockets / Server-Sent Events)**:
   * **Target Endpoint**: LifeLine Real-Time Incident Control Board.
   * **Visual Action**: SPU node marker turns Red, triggers auditory alarm, centers map on GPS coordinates (`lat`, `lon`).

---

## 5. Complete JSON Payload Examples

### A. Normal Telemetry JSON (Transmitted every 3 seconds under normal conditions)

```json
{
  "DID": 3,
  "device_name": "LifeLine SPU",
  "firmware": "v3.1.0 SPU",
  "uptime_sec": 1420,
  "message_code": 4,
  "code": "N",
  "emergency_desc": "NORMAL TELEMETRY",
  "priority": 1,
  "temp": 24.50,
  "humidity": 55.20,
  "pressure": 1013.25,
  "gas_ppm": 120,
  "accel_x": 0.012,
  "accel_y": 0.045,
  "accel_z": 0.988,
  "gyro_x": 0.10,
  "gyro_y": 0.05,
  "gyro_z": -0.02,
  "total_g": 0.989,
  "tilt_deg": 2.60,
  "is_moving": false,
  "sudden_impact": false,
  "free_fall": false,
  "seismic_vibe": false,
  "gps_fix": true,
  "satellites": 8,
  "lat": 27.717245,
  "lon": 85.324000,
  "alt": 1350.5,
  "health": 98,
  "risk": 5,
  "battery": 100,
  "RSSI": -62
}
```

### B. Critical Landslide Alert JSON Payload

```json
{
  "DID": 3,
  "device_name": "LifeLine SPU",
  "firmware": "v3.1.0 SPU",
  "uptime_sec": 1580,
  "message_code": 11,
  "code": "L",
  "emergency_desc": "POSSIBLE LANDSLIDE / COLLAPSE",
  "priority": 4,
  "temp": 22.10,
  "humidity": 88.50,
  "pressure": 998.10,
  "gas_ppm": 145,
  "accel_x": 0.850,
  "accel_y": 0.620,
  "accel_z": 0.310,
  "gyro_x": 45.20,
  "gyro_y": 32.10,
  "gyro_z": 12.80,
  "total_g": 1.096,
  "tilt_deg": 68.40,
  "is_moving": true,
  "sudden_impact": true,
  "free_fall": false,
  "seismic_vibe": false,
  "gps_fix": true,
  "satellites": 9,
  "lat": 27.717245,
  "lon": 85.324000,
  "alt": 1342.0,
  "health": 85,
  "risk": 92,
  "battery": 100,
  "RSSI": -65
}
```

### C. Free Fall / Sudden Impact Detection JSON Payload

```json
{
  "DID": 3,
  "device_name": "LifeLine SPU",
  "firmware": "v3.1.0 SPU",
  "uptime_sec": 1640,
  "message_code": 0,
  "code": "E",
  "emergency_desc": "FREE FALL / SEVERE IMPACT DETECTED",
  "priority": 5,
  "temp": 23.00,
  "humidity": 60.00,
  "pressure": 1012.00,
  "gas_ppm": 130,
  "accel_x": 2.450,
  "accel_y": 1.820,
  "accel_z": 3.100,
  "gyro_x": 120.50,
  "gyro_y": 88.30,
  "gyro_z": 44.10,
  "total_g": 4.350,
  "tilt_deg": 42.10,
  "is_moving": true,
  "sudden_impact": true,
  "free_fall": true,
  "seismic_vibe": false,
  "gps_fix": true,
  "satellites": 7,
  "lat": 27.717245,
  "lon": 85.324000,
  "alt": 1348.0,
  "health": 75,
  "risk": 88,
  "battery": 100,
  "RSSI": -68
}
```

### D. Forest Fire / Gas Explosion Hazard JSON Payload

```json
{
  "DID": 3,
  "device_name": "LifeLine SPU",
  "firmware": "v3.1.0 SPU",
  "uptime_sec": 1720,
  "message_code": 0,
  "code": "F",
  "emergency_desc": "POSSIBLE FIRE / GAS EXPLOSION",
  "priority": 5,
  "temp": 52.80,
  "humidity": 18.50,
  "pressure": 1008.40,
  "gas_ppm": 780,
  "accel_x": 0.020,
  "accel_y": 0.030,
  "accel_z": 0.980,
  "gyro_x": 0.10,
  "gyro_y": 0.00,
  "gyro_z": 0.00,
  "total_g": 0.981,
  "tilt_deg": 1.80,
  "is_moving": false,
  "sudden_impact": false,
  "free_fall": false,
  "seismic_vibe": false,
  "gps_fix": true,
  "satellites": 8,
  "lat": 27.717245,
  "lon": 85.324000,
  "alt": 1350.0,
  "health": 60,
  "risk": 98,
  "battery": 100,
  "RSSI": -60
}
```

---

## 6. PHP Backend Implementation (`recieve.php`) Reference

Below is a reference PHP script for `https://zenithkandel.com.np/lifeline/api/recieve.php` that receives the JSON payload sent by the SPU every 3 seconds, logs the telemetry into the database, evaluates emergency changes, and triggers FCM Push / SMS alerts:

```php
<?php
// zenithkandel.com.np/lifeline/api/recieve.php
header("Content-Type: application/json; charset=UTF-8");
header("Access-Control-Allow-Origin: *");
header("Access-Control-Allow-Methods: POST");

// Read raw JSON input stream
$rawInput = file_get_contents('php://input');
$data = json_decode($rawInput, true);

if (!$data || !isset($data['DID'])) {
    http_response_code(400);
    echo json_encode(["status" => "error", "message" => "Invalid JSON payload"]);
    exit();
}

// Extract SPU telemetry fields
$deviceID     = intval($data['DID']);
$code         = $data['code'] ?? 'N';
$priority     = intval($data['priority'] ?? 1);
$emergencyDesc= $data['emergency_desc'] ?? 'NORMAL';
$temp         = floatval($data['temp'] ?? 0);
$humidity     = floatval($data['humidity'] ?? 0);
$gasPpm       = intval($data['gas_ppm'] ?? 0);
$tiltDeg      = floatval($data['tilt_deg'] ?? 0);
$totalG       = floatval($data['total_g'] ?? 0);
$isFreeFall   = !empty($data['free_fall']);
$isImpact     = !empty($data['sudden_impact']);
$isSeismic    = !empty($data['seismic_vibe']);
$lat          = floatval($data['lat'] ?? 0);
$lon          = floatval($data['lon'] ?? 0);

// 1. Log telemetry to MySQL Database (Omitted DB connection details)
// $db->query("INSERT INTO telemetry_logs (...) VALUES (...)");

// 2. Emergency Trigger Evaluation & Push Notification Logic
$shouldPushNotification = false;
$notificationTitle = "";
$notificationBody = "";

if ($code === 'L' || ($tiltDeg > 60.0 && ($isImpact || !empty($data['is_moving'])))) {
    $shouldPushNotification = true;
    $notificationTitle = "🚨 CRITICAL LANDSLIDE WARNING!";
    $notificationBody = "Node #{$deviceID} detected ground failure & tilt ({$tiltDeg}°). Location: {$lat}, {$lon}";
} elseif ($isFreeFall || $isImpact) {
    $shouldPushNotification = true;
    $notificationTitle = "⚠️ SEVERE FALL / IMPACT DETECTED!";
    $notificationBody = "Node #{$deviceID} experienced free-fall / impact ({$totalG}G). Location: {$lat}, {$lon}";
} elseif ($code === 'Q' || $isSeismic) {
    $shouldPushNotification = true;
    $notificationTitle = "⚡ EARTHQUAKE / SEISMIC ALERT!";
    $notificationBody = "Node #{$deviceID} logged continuous seismic vibration. Location: {$lat}, {$lon}";
} elseif ($code === 'F' || ($gasPpm > 600 && $temp > 42.0)) {
    $shouldPushNotification = true;
    $notificationTitle = "🔥 FOREST FIRE / EXPLOSION HAZARD!";
    $notificationBody = "High heat ({$temp}°C) & Gas ({$gasPpm} PPM) detected at Node #{$deviceID}.";
} elseif ($code === 'G' || $gasPpm > 600) {
    $shouldPushNotification = true;
    $notificationTitle = "☣️ HAZARDOUS GAS LEAK ALERT!";
    $notificationBody = "Gas concentration level {$gasPpm} PPM exceeds safety limits.";
}

// 3. Dispatch Push Notification / SMS if triggered
if ($shouldPushNotification) {
    // sendFCMPushNotification($notificationTitle, $notificationBody, $data);
    // sendTwilioSMSAlert($notificationTitle . " " . $notificationBody);
}

// Response back to SPU node
http_response_code(200);
echo json_encode([
    "status" => "success",
    "received_time" => date("Y-m-d H:i:s"),
    "device_id" => $deviceID,
    "hazard_active" => $shouldPushNotification
]);
?>
```
