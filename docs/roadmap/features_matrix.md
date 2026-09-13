# LifeLine Emergency Ecosystem — Master Feature Matrix

This document provides a comprehensive technical, hardware, software, user interface, RF communication, and cloud feature catalog for the complete **LifeLine Emergency Platform**, comprising four specialized hardware units:
1. **LifeLine TX Pro** (Handheld Emergency Field Transmitter)
2. **LifeLine RX Pro** (Base Station Receiver & Cloud Gateway)
3. **LifeLine SPU** (Sensor Processing Unit — Autonomous Environmental Node)
4. **LifeLine CCU** (Communication Controller Unit — Sensor LoRa Dispatcher)

---

## 1. LifeLine TX Pro — Emergency Field Transmitter

The **LifeLine TX Pro** is a ruggedized, handheld emergency communication transmitter engineered for off-grid operations in extreme terrain (e.g., Himalayan valleys, deep forests, search & rescue routes).

### 📡 Wireless & Communication Core
* **LoRa SX1278 Transceiver (433 MHz)**: High-penetration long-range spread-spectrum RF transceiver.
* **RF Parameter Configuration**:
  * **Spreading Factor**: SF12 (Maximum range & obstacle penetration)
  * **Bandwidth**: 125 kHz
  * **Coding Rate**: 4/8 (Maximum forward error correction)
  * **Preamble Length**: 16 symbols
  * **Transmit Power**: +18 dBm (~63 mW boosted RF output)
  * **Sync Word**: `0x12` (Isolated emergency channel network)
* **Compact Packet Protocol**: Standard payload syntax: `TX[ID],[ALERT_CODE]` (e.g., `TX003,A`) and freeform chat uplink `TX[ID],CHAT,[MESSAGE]`.
* **Bluetooth Low Energy (BLE) Companion Core**:
  * Nordic UART Service (NUS) GATT advertising (`LifeLine-TX-XXX`).
  * Ingests mobile app distress commands (`ALERT:<code-or-text>`) and freeform situation reports (`MSG:<text>`).
  * 5,000ms ACK listen window listening for downlink Base Station confirmation packets.

### 📟 Display & Visual Interface
* **2.8" SPI Color TFT Display (ST7789)**:
  * **Adaptive Resolution Engine**: Supports both 240×320 and 320×480 screen configurations in landscape orientation.
  * **High-Contrast Dark Theme**: Ultra-modern dark palette (RGB565 `#0D0D0F` background, neon green `#00FF87`, electric red `#FF3B3B`, cyan `#00D4FF`) designed for anti-glare sunlight readability.
  * **Animated Boot Sequence**: Displays radar sweeping graphic, peripheral self-test, and firmware version.
  * **Tactical "MESSAGE SENDING" Pop Screen**:
    * Interactive modal popup triggered whenever a paired mobile app dispatches a custom BLE message.
    * Glowing cyan transmission beacon and `[BLE -> LoRa Uplink]` status badge.
    * Word-wrapped SITREP card displaying full text payload.
    * Live ~20 FPS sweeping progress bar active during radio airtime and 5,000ms ACK wait window.
    * Closed-loop confirmation outcome badge: `[ACK CONFIRMED!]` (Green) or `[SENT (NO ACK)]` (Amber).
    * Auto-dismisses after 4 seconds or on any keypress, cleanly restoring the previous screen.
  * **15 Pre-configured Categorized Alerts**:
    1. `EMERGENCY` (Critical)
    2. `MEDICAL EMERGENCY` (Critical)
    3. `MEDICINE SHORTAGE` (Medium)
    4. `EVACUATION NEEDED` (Critical)
    5. `STATUS OK` (Info)
    6. `INJURY REPORTED` (High)
    7. `FOOD SHORTAGE` (Medium)
    8. `WATER SHORTAGE` (Medium)
    9. `WEATHER ALERT` (Medium)
    10. `LOST PERSON` (High)
    11. `ANIMAL ATTACK` (High)
    12. `LANDSLIDE` (High)
    13. `SNOW STORM` (High)
    14. `EQUIPMENT FAILURE` (Medium)
    15. `OTHER EMERGENCY` (Neutral)
  * **Confirmation & Safety Dialogs**: Hold-to-confirm screens preventing accidental false alarms.
  * **Live Transmission Feedback**: Displays RF frequency, TX power, payload code, and dynamic signal animation.
  * **System Telemetry Screen**: Shows Device ID, Firmware Version, RF status, and battery state.

### ⌨️ Controls & Feedback
* **4×4 Matrix Keypad**: Tactile alphanumeric membrane keypad operational with heavy winter gloves.
* **Dual Status LEDs**:
  * **Green LED**: Successful packet transmission & system ready.
  * **Red LED**: Error / transmission failure alert.
* **Piezo Audio Buzzer**: Key click acoustic feedback, confirmation chime, and failure warning tones.

---

## 2. LifeLine RX Pro — Base Station Receiver & Gateway

The **LifeLine RX Pro** is a high-availability base station installed in emergency command centers, medical posts, or village administrative hubs.

### 📡 Wireless Reception & Bluetooth Hub
* **LoRa SX1278 Receiver (433 MHz)**: Continuous low-power RF listening matching TX Pro and CCU transmission parameters.
* **Real-time Signal Quality Assessment**: Computes Received Signal Strength Indicator (RSSI in dBm) and SNR for each incoming packet to estimate sender proximity.
* **Bluetooth Low Energy (BLE) Commander Hub**:
  * Advertises Nordic UART Service (`LifeLine-RX-Base`).
  * Direct mobile chat ingestion (`MSG:<text>` / `CHAT:<text>`) from incident commander phones.
  * Streams incoming field distress alerts and situation reports to paired phones in real time.

### 📟 Display & Audio Interface
* **16×2 Character I2C LCD Display (PCF8574 @ 0x27)**:
  * Low-power industrial character display with dynamic backlight management.
  * **Custom CGRAM Character Glyphs**: Custom-designed 5x8 pixel icons for Radar (`CGRAM 0`), Bell (`CGRAM 1`), Checkmark (`CGRAM 2`), Signal Bars (`CGRAM 3`), and Warning (`CGRAM 4`).
* **Multi-Screen State Machine**:
  * `SCREEN_BOOT`: Initialization sequence and peripheral validation.
  * `SCREEN_IDLE`: Listening screen with radar sweep, active Wi-Fi SSID, NTP clock, and alert counter.
  * `SCREEN_ALERT`: Instant high-priority popup showing Device ID, Alert Name, Priority, RSSI, and timestamp.
  * **`SCREEN_CUSTOM_MSG` (Full 16×2 LCD Custom Message Mode)**:
    * Deducates both Row 0 and Row 1 (all 32 characters) entirely to message text.
    * Smart word-wrapping (`formatLCDTwoRows`) avoiding mid-word line splits.
    * Hands-free auto-paging every 4 seconds for messages > 28 chars without resetting the 15-second return timer.
    * Manual page advancement via short press on the physical Wi-Fi button (GPIO 14) with confirmation chirp.
  * `SCREEN_NO_WIFI`: Offline fallback screen with 60-second auto-timeout back to local RF monitoring.
  * `SCREEN_COUNTDOWN`: Visual progress bar when holding the Wi-Fi setup button.
  * `SCREEN_PORTAL`: Setup Access Point name (`LifeLine-RX-Setup`) and IP address (`192.168.4.1`).
  * `SCREEN_OTA`: Dedicated remote update screen displaying live download percentage and progress bar.
* **Audible Siren & Indicators**:
  * **Red Data LED**: Blinks instantly upon valid RF packet arrival.
  * **Green Wi-Fi LED**: Continuous status indication for network link.
  * **High-Decibel Siren Buzzer**: Acoustic alarm triggered upon receiving critical emergencies.

### 🌐 Connectivity, Web Portal & Cloud Gateway
* **Multi-Wi-Fi Auto-Connect**:
  * Stores up to 3 Wi-Fi network credentials in non-volatile flash memory (`Preferences`).
  * Scans and attaches to the strongest available network on boot.
* **Captive Portal Configuration (`WiFiPortal`)**:
  * Launched via physical Wi-Fi button (GPIO 14) held for 3 seconds.
  * Local Access Point (`192.168.4.1`) with web portal to scan networks and update credentials.
* **Cloud REST API Gateway (`APIClient`)**:
  * Dispatches emergency alerts directly to a central cloud server via HTTPS POST.
  * Transmits `deviceId`, `alertIndex`, `rssi`, `receiverId`, and UTC timestamp.
* **NTP Time Synchronization**: Automatically synchronizes system clock via global NTP servers with configurable timezone offset.
* **Remote HTTPS OTA System (`OTAManager`)**:
  * Queries VPS server (`version.json`) on boot.
  * Automatically downloads and burns newer firmware binaries to secondary `ota_1` partition with rollback safety.

---

## 3. LifeLine SPU — Sensor Processing Unit

The **LifeLine SPU** is an autonomous environmental monitoring node running on ESP32 #1, dedicated to continuous sensor telemetry acquisition, edge DSP filtering, and automated disaster classification.

### 🔬 Sensor Suite & Acquisition Engine
* **NEO-6M GPS Navigation Core**:
  * High-precision NMEA sentence parser extracting Latitude, Longitude, Altitude, Ground Speed, Satellite count, and Fix Status.
* **Environmental Sensing (DHT11 / DHT22 / BME280)**:
  * Measures ambient Temperature ($\pm 0.5^\circ\text{C}$) and Relative Humidity ($\pm 2\%$).
  * Automated threshold flags for Extreme Heat Wave ($>45^\circ\text{C}$), Extreme Cold Wave ($<0^\circ\text{C}$), High Humidity ($>85\%$), and Dry Air ($<20\%$).
* **6-DOF Inertial Motion Unit (MPU6050 Accelerometer & Gyroscope)**:
  * 10-sample circular moving average filter to suppress transient mechanical vibration.
  * Real-time vector magnitude calculation ($A = \sqrt{a_x^2 + a_y^2 + a_z^2}$).
  * Dynamic tilt angle tracking ($\theta_{\text{tilt}}$ relative to gravity).
  * Automated Free-Fall detection ($<0.3g$), Sudden Impact detection ($>3.0g$), Excessive Tilt warning ($>60^\circ$), and Seismic Vibration detection.
* **MQ135 Hazardous Gas & Air Quality Sensor**:
  * Baseline resistance ($R_0$) auto-calibration.
  * PPM estimation for smoke, hazardous gas leaks, and air pollution.

### 🧠 Sensor Fusion & Anomaly Classification
* **Multi-Sensor Cross-Correlation Engine**:
  * **Forest Fire Alert**: Fuses Gas PPM ($>600$) with Temperature ($\ge 42^\circ\text{C}$).
  * **Earthquake / Seismic Alert**: Fuses continuous multi-axis vibration ($>1.8g$) over sustained sample windows.
  * **Landslide / Collapse Warning**: Fuses tilt shift ($>60^\circ$) with sudden impact acceleration spikes.
* **Quantitative Health & Risk Scoring**:
  * **Node Health Score ($0 - 100\%$)**: Assesses sensor availability, GPS fix status, and operating parameters.
  * **Environmental Risk Score ($0 - 100\%$)**: Composite risk index aggregating gas levels, extreme temperatures, and seismic activity.

### 🌐 Direct Cloud Telemetry Portal
* **Integrated Wi-Fi Portal & REST API Client**:
  * Configurable Wi-Fi portal (`wifi_portal_spu.cpp`) for network connection.
  * Directly uploads rich JSON sensor telemetry logs to the central Cloud Server endpoint at high frequency or 1-hour intervals.

---

## 4. LifeLine CCU — Communication Controller Unit

The **LifeLine CCU** is a dedicated RF transmission node running on ESP32 #2, designed to bridge sensor telemetry onto the long-range LoRa network.

### 📡 LoRa RF Engine & Protocol Bridge
* **SX1278 433 MHz LoRa Transceiver**: Identical RF physical layer parameters (SF12, BW 125 kHz, CR 4/8, Sync Word `0x12`).
* **Base Station Protocol Compatibility**: Converts binary sensor packets into standard Base Station alerts (`TX003,F`) for seamless compatibility with LifeLine RX Pro.
* **Transmission Queue & Retries**: Packet deduplication, ACK verification, and exponential backoff retry logic.
* **Optional AES-128 Encryption**: Hardware-accelerated payload obfuscation.

---

---

## 5. LifeLine Companion Ecosystem (Mobile & Web)

The **LifeLine Companion Ecosystem** provides off-grid situational awareness and tactical control without requiring cellular towers or internet connections.

### 📱 LifeLine Companion Mobile App (`lifeline_companion/`)
* **Framework**: React Native & Expo TypeScript for iOS and Android.
* **Tactical Cybernetic Design**: Deep obsidian background (`#0A0F1D`) with high-contrast electric accents (`#00F3FF`, `#FF003C`).
* **Tactical Header**: Real-time BLE link status, battery telemetry, and ping indicator.
* **Radar Screen**: 60 FPS sweeping radar HUD with 1/3/5 km concentric distance rings, dynamic RSSI tracking, and field unit status cards.
* **SITREP Composer**: 48-character LoRa text report composer with field emergency macros and closed-loop base station ACK tracker.
* **Mock BLE Engine**: Standalone offline simulation mode for training and exercises without physical hardware.

### 🌐 Zero-Install Web Bluetooth Companion (`bluefy_companion/` & `portal_preview/`)
* **Zero App Installation**: Runs inside Google Chrome (Android/Windows) and Bluefy Browser (iOS) via Web Bluetooth API.
* **Fast Chassis QR Pairing**: Scans QR code on device hardware for one-touch pairing.
* **Real-time Terminal & Diagnostics**: Bidirectional live packet stream and system log monitor.

---

## 6. System Comparison Matrix

| Feature | LifeLine TX Pro | LifeLine RX Pro | LifeLine SPU | LifeLine CCU |
| :--- | :---: | :---: | :---: | :---: |
| **Primary Role** | Handheld SOS Field Unit | Command Base Station | Autonomous Sensor Node | Sensor RF Transmitter |
| **Microcontroller** | ESP32 | ESP32 | ESP32 | ESP32 |
| **RF Transceiver** | SX1278 (433 MHz) | SX1278 (433 MHz) | Optional via CCU | SX1278 (433 MHz) |
| **Bluetooth BLE Link** | BLE 4.2/5.0 NUS (Peripheral) | BLE 4.2/5.0 NUS (Hub & Peripheral) | N/A | N/A |
| **Display** | 2.8" ST7789 Color TFT | 16×2 Character LCD | Status LED | Status LED |
| **Custom Chat / SITREP** | App Composer + Modal Pop Screen | Full 16×2 LCD + Auto-Page | N/A | Binary Relay Frame |
| **User Input** | 4×4 Matrix Keypad | Push Button (Wi-Fi) | Wi-Fi Web Portal | N/A |
| **GPS Tracking** | Optional / SPU Telemetry | N/A | NEO-6M Core | Relayed via SPU |
| **Sensors** | N/A | RSSI Signal Meter | DHT22, MPU6050, MQ135 | N/A |
| **Companion App Support** | Mobile App + Web PWA | Mobile App + Web PWA | Web Setup Portal | N/A |
| **Wi-Fi Portal** | Local AP OTA Mode | Captive Portal Setup | Captive Portal Setup | N/A |
| **Cloud REST API** | N/A | HTTPS Alert Forwarding | Direct Telemetry Upload | Relayed via RX Pro |
| **Audio Feedback** | Piezo Chimes | High-Decibel Siren | Piezo Status Tones | N/A |
| **Power Target** | Portable Li-Ion Battery | Base Station 5V/12V DC | Solar / Battery Bank | Solar / Battery Bank |
