# LifeLine — Off-Grid Emergency Communication System

> A ruggedized, autonomous emergency communication ecosystem built for remote mountain villages, trekking routes, and disaster-affected areas where cellular and internet connectivity are unavailable or destroyed.

---

## Table of Contents

- [System Overview](#system-overview)
- [Architecture](#architecture)
- [Repository Structure](#repository-structure)
- [Hardware Units](#hardware-units)
  - [RX Pro — Base Station Receiver](#rx-pro--base-station-receiver)
  - [TX Pro — Field Handheld Transmitter](#tx-pro--field-handheld-transmitter)
- [Software & Web](#software--web)
  - [LifeLine Companion — React Native Mobile App](#lifeline-companion--react-native-mobile-app)
  - [Bluefy Companion — Web Bluetooth PWA](#bluefy-companion--web-bluetooth-pwa)
  - [Portal Preview — Hardware Web Interfaces](#portal-preview--hardware-web-interfaces)
  - [Cloud API & Live Website](#cloud-api--live-website)
- [Alert Codes](#alert-codes)
- [Communication Protocols](#communication-protocols)
- [OTA Firmware Updates](#ota-firmware-updates)
- [Build & Flash Instructions](#build--flash-instructions)
- [Running the Companion Apps](#running-the-companion-apps)
- [Documentation Index](#documentation-index)

---

## System Overview

LifeLine bridges the critical communication gap during natural disasters (earthquakes, landslides, flash floods) by combining:

- **Long-range LoRa radio** (SX1278 433 MHz) for multi-kilometre off-grid transmission between field units and a base station
- **ESP32 microcontrollers** running modular firmware with real-time display, BLE, Wi-Fi, and cloud integration
- **Two-way packet protocol** with downlink ACK confirmation from the base station back to the field transmitter
- **Cloud web API** at `zenithkandel.com.np/lifeline` for live alert logging, map display, and remote command dispatch
- **React Native mobile app** (LifeLine Companion) for BLE-tethered smartphone command and control
- **Web Bluetooth PWA** (Bluefy Companion) for iOS/browser BLE access without a native app
- **Over-the-air firmware updates** via local SoftAP and remote HTTPS VPS

---

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                      FIELD LAYER                                │
│                                                                 │
│   LifeLine TX Pro (Field Handheld)                              │
│   ESP32 · SX1278 LoRa · ST7789 TFT · 4×4 Keypad               │
│   Green + Red LED · Passive Buzzer · BLE Nordic UART           │
└─────────────────────┬───────────────────────────────────────────┘
                      │  LoRa 433 MHz (SF12, BW 125 kHz, CR 4/8)
                      │  Up to ~5 km line-of-sight
                      ▼
┌─────────────────────────────────────────────────────────────────┐
│                   BASE STATION LAYER                            │
│                                                                 │
│   LifeLine RX Pro (Base Station)                                │
│   ESP32 · SX1278 LoRa · 16×2 LCD (I2C) · Passive Buzzer       │
│   Green + Red LED · BLE Nordic UART · Wi-Fi (Multi-Network)    │
└──────────────┬──────────────────────┬───────────────────────────┘
               │                      │
               │ BLE Nordic UART      │ HTTPS REST API
               ▼                      ▼
   ┌──────────────────┐    ┌────────────────────────────────────┐
   │  Smartphone      │    │  Cloud Web Server                  │
   │  LifeLine        │    │  zenithkandel.com.np/lifeline      │
   │  Companion App   │    │                                    │
   │  (React Native)  │    │  · Alert log & live map            │
   │                  │    │  · Downlink command dispatch       │
   │  or              │    │  · OTA firmware manifest           │
   │  Bluefy PWA      │    │  · Custom message relay (SITREP)   │
   └──────────────────┘    └────────────────────────────────────┘
```

**Data flow for an emergency alert:**

1. Field operator selects alert on TX Pro keypad → TX transmits LoRa packet
2. RX Pro receives packet → immediately triggers LCD alert screen + buzzer/LED
3. RX sends downlink ACK back over LoRa to TX → TX confirms delivery on screen
4. RX pushes full telemetry to cloud API over HTTPS
5. Cloud website updates live alert log, map, and notifies operators
6. Website operator can dispatch a downlink command → RX polls API → relays over LoRa to TX

---

## Repository Structure

```
lifeline_hardware/
├── lifeline_rx_pro/          # Base Station receiver firmware (ESP32 + PlatformIO)
│   ├── lifeline_rx_pro.ino   # Main application entry point
│   ├── Config.h              # Pin definitions, LoRa settings, alert table, timing
│   ├── LoRaComm.cpp/h        # LoRa packet parsing, downlink ACK & command TX
│   ├── BLEManager.cpp/h      # Nordic UART BLE GATT server (notify + command receive)
│   ├── WiFiPortal.cpp/h      # Captive portal, multi-network credential store, web server
│   ├── APIClient.cpp/h       # HTTPS cloud push (alerts, telemetry, chat) & downlink poll
│   ├── OTAManager.cpp/h      # Remote VPS OTA + local SoftAP OTA
│   ├── DisplayUI.cpp/h       # 16×2 LCD screen state machine & custom CGRAM glyphs
│   └── BuzzerLED.cpp/h       # Passive buzzer tone sequences & LED blink patterns
│
├── lifeline_tx_pro/          # Field transmitter firmware (ESP32 + PlatformIO)
│   ├── lifeline_tx_pro.ino   # Main application entry point
│   ├── Config.h              # Pin definitions, LoRa settings, ST7789 display config
│   ├── LoRaComm.cpp/h        # LoRa alert transmission, ACK wait, retry logic
│   ├── BLEManager.cpp/h      # Nordic UART BLE GATT server for mobile companion
│   ├── KeypadInput.cpp/h     # 4×4 matrix keypad scanning with debounce & repeat
│   ├── OTAManager.cpp/h      # Local SoftAP OTA portal
│   ├── DisplayUI.cpp/h       # ST7789 TFT UI: menu, confirm, sending, result screens
│   ├── BuzzerLED.cpp/h       # Buzzer feedback and LED status
│   ├── SharedProtocol.h      # Shared packet format constants (TX ↔ RX)
│   └── Config.cpp            # Alert name/priority tables, alert code mapping
│
├── lifeline_companion/       # React Native mobile companion app (Expo)
│   ├── App.tsx               # Root component and navigation
│   ├── src/
│   │   ├── screens/          # DashboardScreen, SosScreen, ChatScreen, RadarScreen, SettingsScreen
│   │   ├── services/         # BleService.ts (react-native-ble-plx), MockBleService.ts
│   │   ├── components/       # TacticalHeader.tsx and shared UI components
│   │   ├── context/          # LifeLineContext.tsx — global BLE state provider
│   │   ├── constants/        # ble.ts (Nordic UART UUIDs), theme.ts
│   │   └── utils/            # base64.ts (BLE payload encode/decode)
│   └── scan_qr.html          # QR code pairing page for local dev BLE testing
│
├── bluefy_companion/         # Standalone Web Bluetooth PWA (iOS Bluefy / Chrome)
│   ├── index.html            # Single-file BLE companion (no install required)
│   └── server.js             # Local Node.js dev server for testing
│
├── portal_preview/           # Static HTML previews of hardware web portals
│   ├── index.html            # Design showroom — tabbed preview of all portals
│   ├── rx_portal.html        # RX Base Station web interface (served from ESP32)
│   ├── tx_portal.html        # TX Field Unit web interface
│   └── companion_app.html    # Companion app portal preview
│
├── docs/                     # Architecture, hardware, API, and guide documentation
├── assests/                  # Project photos and showcase images
├── build.ps1                 # Unified PowerShell build & flash script
├── serve_demo.js             # Node.js server to preview portal_preview locally
├── presentation.html         # Project presentation slides
└── lifeline.code-workspace   # VS Code multi-root workspace
```

---

## Hardware Units

### RX Pro — Base Station Receiver

The base station is deployed at a command post, rescue coordination centre, or village health post. It runs continuously on mains or battery power and acts as the bridge between the off-grid LoRa radio network and the internet.

**Microcontroller:** ESP32 (Espressif ESP32-WROOM-32)

**Pin Configuration:**

| Component | GPIO | Interface |
| :--- | :---: | :--- |
| SX1278 LoRa CS | 18 | SPI Chip Select |
| SX1278 LoRa RST | 23 | Digital Output |
| SX1278 LoRa DIO0 | 19 | Interrupt Input |
| SPI SCK | 5 | SPI Clock |
| SPI MISO | 17 | SPI Data In |
| SPI MOSI | 27 | SPI Data Out |
| 16×2 LCD SDA | 4 | I2C Data |
| 16×2 LCD SCL | 16 | I2C Clock |
| LCD I2C Address | 0x27 | PCF8574 backpack |
| Wi-Fi Portal Button | 14 | Active LOW (INPUT_PULLUP) |
| Wi-Fi / Green LED | 21 | Digital Output (Active HIGH) |
| Data / Red LED | 13 | Digital Output (Active HIGH) |
| Passive Buzzer | 12 | PWM Tone Output |

**LoRa RF Configuration:**

| Parameter | Value |
| :--- | :--- |
| Frequency | 433 MHz |
| Spreading Factor | SF12 (maximum range) |
| Bandwidth | 125 kHz |
| Coding Rate | 4/8 (maximum error correction) |
| Preamble | 16 symbols |
| TX Power | 18 dBm |
| Sync Word | 0x12 |

**Key firmware features:**

- Receives full telemetry packets from TX Pro: alert code, device ID, temperature, humidity, gas PPM, GPS coordinates, altitude, health/risk scores, RSSI, and SNR
- Two-way LoRa: sends downlink ACK back to TX immediately after receiving each packet
- 16×2 LCD state machine: Boot → Idle → Alert → Custom Message → No-Wi-Fi, with custom CGRAM glyphs (radar, bell, check, signal, Wi-Fi icons)
- 15-second alert timeout with auto-return to idle; 60-second No-Wi-Fi screen timeout
- Multi-network Wi-Fi credential store (up to 3 networks) in NVMe; captive portal for setup (press and hold GPIO 14 for 3 seconds)
- HTTPS push to cloud API (`zenithkandel.com.np/lifeline/API/Create/message.php`) on every packet received
- Cloud downlink polling every 3.5 seconds — relays web operator commands over LoRa to field units
- BLE Nordic UART GATT server: notifies paired smartphone on every alert, chat, and telemetry packet; accepts commander downlink commands and evacuation broadcasts from phone
- Remote VPS OTA: checks `version.json` manifest on boot, streams `.bin` over HTTPS if newer firmware available
- Local SoftAP OTA: activated by triple-press of Wi-Fi button, serves firmware upload page at `LifeLine-RX-Setup` AP
- NTP time sync (Nepal Standard Time, GMT+5:45)

---

### TX Pro — Field Handheld Transmitter

The field unit is carried by community health workers, trekking guides, village volunteers, or disaster response teams. It operates fully offline and transmits emergency alerts over LoRa to the base station.

**Microcontroller:** ESP32 (Espressif ESP32-WROOM-32)

**Pin Configuration:**

| Component | GPIO | Interface |
| :--- | :---: | :--- |
| SX1278 LoRa CS | 18 | SPI Chip Select |
| SX1278 LoRa RST | 23 | Digital Output |
| SX1278 LoRa DIO0 | 19 | Interrupt Input |
| SPI SCK | 5 | SPI Clock |
| SPI MISO | 17 | SPI Data In |
| SPI MOSI | 27 | SPI Data Out |
| ST7789 TFT CS | 16 | SPI Chip Select |
| ST7789 TFT DC | 4 | Data/Command |
| ST7789 TFT RST | 2 | Reset |
| Green LED | 21 | Digital Output (Active HIGH) |
| Red LED | 13 | Digital Output (Active HIGH) |
| Passive Buzzer | 22 | PWM Tone Output |
| SPU UART RX | 34 | Hardware Serial2 RX |

**LoRa RF Configuration:** Identical to RX Pro — same frequency, SF, BW, CR, and sync word (required for interoperability).

**Display:** ST7789 TFT (240×320 or 320×480, auto-adaptive layout), landscape orientation. Dark-themed tactical UI with RGB565 color palette — critical alerts in red, success in green, information in cyan/amber.

**UI Screen States:**

| Screen | Description |
| :--- | :--- |
| `SCREEN_BOOT` | Splash logo (~1.8 s) |
| `SCREEN_MENU` | Scrollable alert selection list (5 visible items) |
| `SCREEN_CONFIRM` | Alert confirmation dialog before transmit |
| `SCREEN_SENDING` | Live transmission animation with retry counter |
| `SCREEN_RESULT` | Success (green, auto-returns in 1.8 s) or Failure (red, awaits key) |
| `SCREEN_SYSTEM_INFO` | Firmware version, device ID, LoRa stats |
| `SCREEN_USER_MANUAL` | 4-page scrollable on-device user manual |
| `SCREEN_SENSOR_LOG` | Live SPU sensor data dashboard (refreshes every 1 s) |
| `SCREEN_OTA_SELECT` | Interactive menu: Local AP OTA or Network OTA |
| `SCREEN_BLE_PORTAL` | BLE pairing status and incoming message history |
| `SCREEN_MESSAGE_POPUP` | Auto-dismissing modal for downlink messages from base |

**Key firmware features:**

- 4×4 matrix keypad with debounce (30 ms) and auto-repeat (80 ms) for fast navigation
- Up to 3 transmission retry attempts with ACK wait window (5 000 ms, accommodating SF12 ~2 s airtime)
- Continuous background LoRa listener (between transmissions) for downlink commands and evacuation broadcasts from base
- BLE Nordic UART: accepts alert triggers and custom chat messages from LifeLine Companion app; displays received base messages in BLE portal screen
- SPU telemetry input on UART2 (GPIO 34) — displays live sensor readings when sensor node is wired
- Local SoftAP OTA portal

---

## Software & Web

### LifeLine Companion — React Native Mobile App

A cross-platform mobile application (iOS and Android) built with **Expo** and **React Native**, connecting to either the RX Base Station or TX Field Unit over BLE.

**Tech stack:** React Native · TypeScript · Expo · `react-native-ble-plx` · `expo-linear-gradient`

**Screens:**

| Screen | Function |
| :--- | :--- |
| Dashboard | Real-time incoming alert feed from paired device; device status bar |
| SOS | One-tap BLE alert trigger — sends selected alert code to TX over BLE |
| Chat | Free-text message input; message history with RSSI and sender info |
| Radar | Visual representation of active devices and signal health |
| Settings | BLE device scan, connect/disconnect, mock mode toggle |

**BLE protocol:** Nordic UART Service (NUS)
- Service UUID: `6E400001-B5A3-F393-E0A9-E50E24DCCA9E`
- Write characteristic (mobile → device): `6E400002-...`
- Notify characteristic (device → mobile): `6E400003-...`

**Run:**
```powershell
cd lifeline_companion
npm install
npx expo start
```

---

### Bluefy Companion — Web Bluetooth PWA

A single-file Web Bluetooth application (`bluefy_companion/index.html`) that connects directly to the RX or TX ESP32 over BLE without requiring any app installation. Compatible with:

- **iOS:** [Bluefy Browser](https://apps.apple.com/app/bluefy-web-ble-browser/id1492822055) (supports Web Bluetooth API)
- **Android / Desktop Chrome:** Native Web Bluetooth API

Provides QR code pairing, BLE connect/disconnect, and a command interface for base station control.

**Run locally:**
```powershell
cd bluefy_companion
node server.js
# Open http://localhost:3000 in Bluefy or Chrome
```

---

### Portal Preview — Hardware Web Interfaces

The ESP32 RX Pro serves a local web interface when connected to Wi-Fi (accessible at the device's IP address). The `portal_preview/` directory contains static HTML previews of these pages for design review and development.

**Pages:**

| File | Description |
| :--- | :--- |
| `index.html` | Tabbed design showroom — previews all hardware portals in one page (light/dark theme toggle) |
| `rx_portal.html` | RX Base Station web UI (alert history, device status, downlink commands) |
| `tx_portal.html` | TX Field Unit web UI (alert log, transmission stats, sensor readings) |
| `companion_app.html` | Companion app portal preview |

**Preview locally:**
```powershell
node serve_demo.js
# Open: http://localhost:8080/portal_preview/index.html
```

---

### Cloud API & Live Website

The RX Base Station pushes data to a live cloud backend at **`zenithkandel.com.np/lifeline`**.

**API Endpoints (used by RX firmware):**

| Method | Endpoint | Purpose |
| :--- | :--- | :--- |
| `POST` | `/API/Create/message.php` | Push emergency alert or telemetry packet |
| `GET` | `/API/Read/pending_commands.php` | Poll for pending downlink commands from web operators |
| `POST` | `/API/Update/command_status.php` | Acknowledge execution of a downlink command |
| `GET` | `/ota/version.json` | OTA firmware version manifest |

**Payload fields sent per packet:** `device_id`, `alert_code`, `alert_name`, `priority`, `temperature`, `humidity`, `gas_ppm`, `latitude`, `longitude`, `altitude`, `rssi`, `snr`, `distance_km`, `source` (`LORA` / `BLE`), `custom_message`, timestamp

**Downlink flow (Website → Field):**
1. Web operator dispatches a command on the website (message, evacuation broadcast, or custom SITREP)
2. RX polls API every 3.5 seconds and retrieves the pending command
3. RX displays it on the LCD, sounds an alert tone, and relays it over LoRa to the target TX unit
4. RX acknowledges execution status back to the API (`DISPATCHED_LORA` or `TX_FAILED`)
5. If a BLE commander phone is paired, the confirmation string is also forwarded over BLE

---

## Alert Codes

15 emergency categories are supported, selectable from the TX keypad or triggered remotely via BLE or cloud downlink.

| Code | Index | Alert Name | Priority |
| :---: | :---: | :--- | :---: |
| A | 0 | CRITICAL SOS | 🔴 CRITICAL |
| B | 1 | DELIVERY / LABOR | 🔴 CRITICAL |
| C | 2 | HELI RESCUE NEEDED | 🔴 CRITICAL |
| D | 3 | MEDICINE SHORTAGE | 🟡 MEDIUM |
| E | 4 | OXYGEN SHORTAGE | 🔴 CRITICAL |
| F | 5 | SEVERE INJURY | 🟠 HIGH |
| G | 6 | BLOOD NEEDED | 🔴 CRITICAL |
| H | 7 | ALTITUDE SICKNESS | 🟠 HIGH |
| I | 8 | FOOD SHORTAGE | 🟡 MEDIUM |
| J | 9 | WATER SHORTAGE | 🟡 MEDIUM |
| K | 10 | DISEASE OUTBREAK | 🟠 HIGH |
| L | 11 | FREEZING / SHELTER | 🟡 MEDIUM |
| M | 12 | LANDSLIDE / HAZARD | 🟠 HIGH |
| N | 13 | DOCTOR / NURSE NEEDED | 🟡 MEDIUM |
| O | 14 | STATUS OK / ALL SAFE | ✅ OK |

---

## Communication Protocols

### LoRa Packet Format (TX → RX)

Packets carry a compact payload including: device ID, emergency code, full telemetry (temperature, humidity, gas PPM, GPS lat/lon, altitude, health score, risk score), and an optional free-text chat message field. RSSI and SNR are measured at the receiver.

### LoRa Downlink Format (RX → TX)

The base station sends downlink packets back to field units with:
- **ACK:** Confirms receipt of a specific alert (`DEVICE_ID, CODE, STATUS, MESSAGE`)
- **Command:** Targeted action for a specific TX device (e.g., `EVACUATE`, custom SITREP text)
- **Broadcast Evacuation:** Sent to all devices simultaneously

**ACK wait window:** 5 000 ms (required to accommodate SF12 ≈ 2 s airtime). Up to 3 retries on the TX side.

### BLE (Nordic UART Service)

Both RX and TX expose a Nordic UART Service GATT server. The smartphone companion app (or Bluefy PWA) connects and:
- **Receives** structured JSON-like strings for alerts, telemetry, chat messages, and status updates
- **Sends** alert trigger codes (`SOS:A`), chat messages (`MSG:text`), commander replies (`REPLY:DEV,ACTION,MSG`), and evacuation broadcasts (`EVAC:message`)

---

## OTA Firmware Updates

### Remote VPS OTA (RX Pro only)

On every boot, the RX firmware fetches `https://zenithkandel.com.np/lifeline/ota/version.json`. If the manifest reports a newer version than the running firmware, it streams the `.bin` file over HTTPS directly to the ESP32 OTA partition, shows real-time progress on the 16×2 LCD, and reboots automatically.

### Local SoftAP OTA (RX Pro and TX Pro)

Triggered by holding the Wi-Fi Portal button (GPIO 14) for 3 seconds on RX, or via the OTA menu on TX. The ESP32 starts a SoftAP (`LifeLine-RX-Setup` / `LifeLine-TX-Setup`) and serves a firmware upload page. Connect to the AP and browse to the device IP to upload a new `.bin`.

---

## Build & Flash Instructions

### Prerequisites

- [PlatformIO Core (CLI)](https://platformio.org/install/cli) or the [PlatformIO IDE extension](https://marketplace.visualstudio.com/items?itemName=platformio.platformio-ide) for VS Code
- Espressif 32 platform package (installed automatically by PlatformIO on first build)

### Option A — Unified PowerShell Script (`build.ps1`)

```powershell
# Compile both firmware projects
.\build.ps1

# Compile a specific unit
.\build.ps1 rx        # RX Base Station
.\build.ps1 tx        # TX Field Transmitter

# Compile and flash over USB
.\build.ps1 rx -Upload
.\build.ps1 tx -Upload

# Clean build caches
.\build.ps1 clean
```

### Option B — PlatformIO CLI

```powershell
pio run -d lifeline_rx_pro
pio run -d lifeline_tx_pro

# Flash directly
pio run -d lifeline_rx_pro --target upload
pio run -d lifeline_tx_pro --target upload
```

### Option C — Visual Studio Code

1. Open [`lifeline.code-workspace`](file:///d:/lifeline_hardware/lifeline.code-workspace) in VS Code.
2. Press `Ctrl+Shift+B` → select your build target from the task menu.

---

## Running the Companion Apps

```powershell
# React Native mobile app (iOS & Android via Expo Go)
cd lifeline_companion
npm install
npx expo start

# Web preview of hardware portals (portal_preview/)
node serve_demo.js
# Access: http://localhost:8080/portal_preview/index.html

# Bluefy Web BLE companion
cd bluefy_companion
node server.js
# Access: http://localhost:3000  (open in Bluefy on iOS or Chrome on Android)
```

---

## Documentation Index

All in-depth documentation is in the [`docs/`](file:///d:/lifeline_hardware/docs) directory.

| Domain | Document | Description |
| :--- | :--- | :--- |
| **Architecture** | [System Architecture](file:///d:/lifeline_hardware/docs/architecture/system_architecture.md) | End-to-end data flow and system design |
| **Architecture** | [Base Station UI Flow](file:///d:/lifeline_hardware/docs/architecture/rx_pro_ui_flow.md) | 16×2 LCD screen states, buttons, and alert popups |
| **Architecture** | [Two-Way LoRa & BLE Architecture](file:///d:/lifeline_hardware/docs/architecture/two_way_ble_nepal_terrain_upgrade.md) | v4.0 two-way LoRa ACK, BLE smartphone integration, Nepal mountain resilience design |
| **Hardware** | [Wiring & Pinouts Master](file:///d:/lifeline_hardware/docs/hardware/wiring_and_pinouts.md) | Complete pin connection tables for all boards |
| **Hardware** | [Schematics & Assets](file:///d:/lifeline_hardware/docs/hardware/schematics/) | Circuit schematics and breadboard diagrams |
| **API** | [Cloud API Setup & Web Integration](file:///d:/lifeline_hardware/docs/api/cloud_api_setup_guide.md) | Ingestion guide, JSON schemas, and database setup |
| **Guides** | [Local Wireless OTA Flashing](file:///d:/lifeline_hardware/docs/guides/local_ota_guide.md) | Flash TX and RX wirelessly over SoftAP |
| **Guides** | [VPS Remote HTTPS OTA](file:///d:/lifeline_hardware/docs/guides/vps_ota_guide.md) | Configure VPS cloud auto-updates for the base station |
| **Guides** | [Bluetooth BLE & Mobile Companion](file:///d:/lifeline_hardware/docs/guides/bluetooth_ble_integration_guide.md) | BLE GATT Nordic UART, mobile companion, and Web Bluetooth PWA guide |
| **Roadmap** | [Master Feature Matrix](file:///d:/lifeline_hardware/docs/roadmap/features_matrix.md) | Feature comparison across all units and versions |
| **Roadmap** | [Upgrade Roadmap](file:///d:/lifeline_hardware/docs/roadmap/upgrade_roadmap.md) | Planned features and version roadmap |

---

## License

Open-source hardware and firmware developed for humanitarian disaster relief and community safety in Nepal.
