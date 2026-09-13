# 🚨 LifeLine — Off-Grid Disaster Monitoring & Emergency Response System

> **A ruggedized, autonomous emergency hardware and IoT ecosystem designed for remote mountain villages, trekking routes, disaster hazard zones, and off-grid environments where cellular and internet connectivity are unavailable.**

---

## 📌 Table of Contents
- [1. System Overview & Architecture](#1-system-overview--architecture)
- [2. Repository Structure](#2-repository-structure)
- [3. Firmware Subprojects & Companion Applications](#3-firmware-subprojects--companion-applications)
  - [LifeLine RX Pro (Base Station Receiver)](#-lifeline-rx-pro-base-station-receiver)
  - [LifeLine TX Pro (Field Transmitter)](#-lifeline-tx-pro-field-transmitter)
  - [LifeLine Companion Ecosystem (Mobile & Web)](#-lifeline-companion-ecosystem-mobile--web)
  - [LifeLine SPU (Sensor Processing Unit)](#-lifeline-spu-sensor-processing-unit)
  - [LifeLine CCU (Communication Controller Unit)](#-lifeline-ccu-communication-controller-unit)
- [4. Hardware Pinouts Quick Reference](#4-hardware-pinouts-quick-reference)
- [5. Build & Deployment Instructions](#5-build--deployment-instructions)
- [6. Documentation Index](#6-documentation-index)

---

## 1. System Overview & Architecture

LifeLine bridges the critical communication gap during natural disasters (earthquakes, landslides, flash floods, forest fires) by combining long-range **LoRa (SX1278 433 MHz)** radio links, multi-sensor IoT edge computing on **ESP32**, local acoustic and visual alerting, and cloud integration.

```
┌─────────────────────────────────────────────────────────────┐
│                    FIELD SENSING LAYER                      │
│                                                             │
│   ┌────────────────────────┐      ┌──────────────────────┐  │
│   │   LifeLine TX Pro      │      │     LifeLine SPU     │  │
│   │  (Field Handheld Unit) │      │  (Autonomous Node)   │  │
│   │   [Keypad + ST7789]    │      │ [GPS+IMU+Temp+Gas]   │  │
│   └───────────┬────────────┘      └──────────┬───────────┘  │
└───────────────┼──────────────────────────────┼──────────────┘
                │                              │ Direct Wi-Fi
                │ LoRa 433 MHz RF              │ or CCU LoRa
                ▼                              ▼
┌─────────────────────────────────────────────────────────────┐
│                  COMMAND & BASE STATION                     │
│                                                             │
│   ┌──────────────────────────────────────────────────────┐  │
│   │                 LifeLine RX Pro                      │  │
│   │            (Emergency Base Station)                  │  │
│   │          [SX1278 + 16x2 LCD + Loud Siren]            │  │
│   └──────────────────────────┬───────────────────────────┘  │
└──────────────────────────────┼──────────────────────────────┘
                               │
                               │ HTTPS REST API Gateway
                               ▼
┌─────────────────────────────────────────────────────────────┐
│                    CENTRAL CLOUD PLATFORM                   │
│                                                             │
│   ┌──────────────────────────────────────────────────────┐  │
│   │                 Web Server & DB                      │  │
│   │    [Live Map, Real-Time SMS, Email & Push Alert]     │  │
│   └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

---

## 2. Repository Structure

```text
lifeline_hardware/
├── README.md                               <-- Master system documentation
├── build.ps1                               <-- Unified PowerShell build & flash tool
├── lifeline.code-workspace                 <-- Multi-project VS Code workspace file
├── .gitignore                              <-- Production gitignore (blocks .pio, binaries)
│
├── docs/                                   <-- Domain-organized technical documentation
│   ├── architecture/
│   │   ├── system_architecture.md          <-- Full end-to-end system architecture
│   │   ├── sensornode_dual_esp32.md        <-- Dual-ESP32 (SPU+CCU) working principle & DSP
│   │   ├── sensornode_specification.md     <-- Original hardware specification & design goals
│   │   └── rx_pro_ui_flow.md               <-- Base station LCD UI state machine & screens
│   ├── hardware/
│   │   ├── wiring_and_pinouts.md           <-- Master hardware pinout & connection tables
│   │   └── schematics/                     <-- Cleanly named circuit diagrams & breadboards
│   │       ├── receiver_breadboard.png
│   │       ├── transmitter_breadboard.png
│   │       ├── lifeline_circuit_schematic.pdf
│   │       ├── lifeline_components_list.pdf
│   │       ├── lifeline_intro_presentation.pdf
│   │       └── lifeline_system_manual.pdf
│   ├── api/
│   │   └── spu_telemetry_api.md            <-- Direct Cloud REST API specifications
│   ├── guides/
│   │   ├── local_ota_guide.md              <-- Local SoftAP & IDE wireless flashing guide
│   │   └── vps_ota_guide.md                <-- VPS Cloud remote HTTPS OTA firmware guide
│   └── roadmap/
│       ├── features_matrix.md              <-- Master feature matrix across all 4 units
│       └── upgrade_roadmap.md              <-- Dual-mode transmission & cloud upgrade plan
│
├── lifeline_rx_pro/                        <-- Base Station Receiver Firmware
│   ├── platformio.ini                      <-- PlatformIO configuration (USB & OTA envs)
│   ├── lifeline_rx_pro.ino                 <-- Main Arduino sketch entrypoint
│   ├── APIClient.cpp / .h                  <-- HTTPS Cloud REST API forwarder
│   ├── BLEManager.cpp / .h                 <-- Bluetooth Low Energy Nordic UART Hub & commander dispatch
│   ├── BuzzerLED.cpp / .h                  <-- Multi-tone siren & status LED driver
│   ├── Config.h                            <-- Pinouts, timings, and network defaults
│   ├── DisplayUI.cpp / .h                  <-- Full 16x2 LCD custom message formatter & custom glyphs
│   ├── LoRaComm.cpp / .h                   <-- SX1278 RF receiver, CHAT parser & 2-way ACK
│   ├── OTAManager.cpp / .h                 <-- Remote HTTPS OTA client
│   └── WiFiPortal.cpp / .h                 <-- Captive web portal & Multi-Wi-Fi manager
│
├── lifeline_tx_pro/                        <-- Field Handheld Transmitter Firmware
│   ├── platformio.ini                      <-- PlatformIO configuration (USB & OTA envs)
│   ├── lifeline_tx_pro.ino                 <-- Main Arduino sketch entrypoint
│   ├── BLEManager.cpp / .h                 <-- Bluetooth Low Energy Nordic UART companion interface
│   ├── BuzzerLED.cpp / .h                  <-- Keypad acoustic feedback & LED driver
│   ├── Config.cpp / .h                     <-- Alert dictionary, RF frequencies, pinouts & popup states
│   ├── DisplayUI.cpp / .h                  <-- 2.8" SPI TFT (ST7789) dark theme & "MESSAGE SENDING" pop screen
│   ├── KeypadInput.cpp / .h                <-- 4x4 matrix keypad scanner & debouncer
│   ├── LoRaComm.cpp / .h                   <-- SX1278 RF transmitter & 5,000ms ACK listen window
│   ├── OTAManager.cpp / .h                 <-- Local SoftAP wireless firmware update
│   ├── SharedProtocol.h                    <-- Common binary packet structures & CRC16
│   └── SPUReceiver.h                       <-- Telemetry interface for SPU display
│
├── lifeline_companion/                     <-- Tactical Mobile Companion App (React Native / Expo)
│   ├── src/
│   │   ├── components/                     <-- TacticalHeader, StatusBadge, LinkQualityIndicator
│   │   ├── context/                        <-- LifeLineContext provider & BLE state engine
│   │   ├── screens/                        <-- RadarScreen, SitrepScreen, SettingsScreen, AlertModal
│   │   ├── services/                       <-- BleService, MockBleService, NotificationService
│   │   └── theme/                          <-- Military-grade cybernetic dark design tokens
│   └── App.tsx                             <-- Main navigation & application entrypoint
│
├── bluefy_companion/                       <-- Zero-Install Web Bluetooth PWA (iOS Bluefy & Android Chrome)
│   └── index.html                          <-- Standalone offline Web Bluetooth situational terminal
│
├── portal_preview/                         <-- Interactive Web Companion & QR Scanner Demo
│   ├── companion_app.html                  <-- Standalone Web Bluetooth companion simulator
│   └── scan_qr.html                        <-- Fast QR-based hardware pairing scanner
│
├── lifeline_tx_spu/                        <-- Autonomous Sensor Processing Unit Firmware
│   ├── platformio.ini                      <-- PlatformIO configuration
│   ├── include/
│   │   ├── ConfigSPU.h                     <-- Sensor pins, sampling rates & thresholds
│   │   └── SharedProtocol.h                <-- Common binary packet structures & CRC16
│   └── src/
│       ├── main.cpp                        <-- SPU task loop entrypoint
│       ├── api_client_spu.cpp / .h         <-- Direct HTTP POST cloud telemetry client
│       ├── emergency_detector.cpp / .h     <-- Sensor fusion & emergency rule engine
│       ├── environment_manager.cpp / .h    <-- DHT11/22/BME280 temperature & humidity
│       ├── gas_manager.cpp / .h            <-- MQ135 air quality & smoke detection
│       ├── gps_manager.cpp / .h            <-- NEO-6M GPS NMEA sentence parser
│       ├── health_calculator.cpp / .h      <-- Quantitative Node Health & Risk scoring
│       ├── mpu_manager.cpp / .h            <-- 6-DOF IMU accelerometer & seismic filter
│       ├── sensor_manager.cpp / .h         <-- Master FreeRTOS non-blocking scheduler
│       ├── web_server_manager.cpp / .h     <-- Local web server & diagnostics portal
│       └── wifi_portal_spu.cpp / .h        <-- SPU Wi-Fi captive setup portal
│
└── lifeline_tx_ccu/                        <-- Communication Controller Unit Firmware
    ├── platformio.ini                      <-- PlatformIO configuration
    ├── include/
    │   ├── ConfigCCU.h                     <-- CCU pinout, LoRa settings & retries
    │   └── SharedProtocol.h                <-- Common binary packet structures & CRC16
    └── src/
        ├── main.cpp                        <-- CCU forwarding loop
        ├── diagnostics.cpp / .h            <-- Packet counters & RF health telemetry
        ├── lora_manager.cpp / .h           <-- LoRa SX1278 transmitter & queue engine
        ├── power_manager.cpp / .h          <-- Low power & light sleep manager
        └── uart_receiver.cpp / .h          <-- High-speed UART ring buffer parser
```

---

## 3. Firmware Subprojects & Companion Applications

### 📡 LifeLine RX Pro (Base Station Receiver)
* **Target Board**: ESP32 DevKit V1
* **Display**: 16×2 Character I2C LCD (PCF8574 @ `0x27`)
* **RF Transceiver**: SX1278 LoRa @ 433 MHz (SF12, BW 125 kHz, CR 4/8, Sync Word `0x12`)
* **Bluetooth**: BLE 4.2/5.0 Nordic UART Service (NUS) Advertising (`LifeLine-RX-Base`)
* **Key Features**:
  - Continuous low-power RF reception with live RSSI and SNR telemetry.
  - **Full 16×2 LCD Custom Message Display**: Uses both Row 0 and Row 1 (all 32 characters) for freeform chat/SITREP text with smart word-wrapping, auto-scroll paging, and manual button advance.
  - **Bluetooth Commander Hub**: Ingests direct mobile commands (`MSG:`, `REPLY:`, `EVAC:`) and broadcasts push notifications (`CHAT:`, `ALERT:`, `TELEMETRY:`) to paired smartphones.
  - Multi-Wi-Fi memory (stores up to 3 network credentials with auto-fallback).
  - Captive Web Setup Portal (`192.168.4.1`) launched via dedicated hardware push-button.
  - HTTPS Cloud REST API gateway automatically posting emergency alerts to central dashboard.
  - Remote VPS Over-The-Air (OTA) firmware upgrade checking on boot with dual-partition safety.

### 📟 LifeLine TX Pro (Field Transmitter)
* **Target Board**: ESP32 DevKit V1
* **Display**: 2.8" SPI Color TFT (ST7789, 240×320 / 320×240 landscape)
* **Input**: 4×4 Tactile Matrix Keypad
* **RF Transceiver**: SX1278 LoRa @ 433 MHz (+18 dBm boosted output)
* **Bluetooth**: BLE 4.2/5.0 Nordic UART Service (NUS) Advertising (`LifeLine-TX-XXX`)
* **Key Features**:
  - High-contrast graphical cybernetic dark theme designed for sunlight anti-glare readability.
  - **Tactical "MESSAGE SENDING" Pop Screen**: When a custom BLE message is sent from the companion phone (`MSG:<text>`), an interactive pop screen appears displaying:
    - Glowing header transmission beacon and `[BLE -> LoRa]` mode badge.
    - Word-wrapped SITREP payload card showing the complete custom message.
    - Live ~20 FPS sweeping segmented progress bar active during airtime and 5,000ms ACK listen window.
    - Closed-loop Gateway ACK confirmation badge (`[ACK CONFIRMED!]` / `[SENT (NO ACK)]`) with Gateway ID, RSSI, and note.
    - Auto-dismiss (4s) or manual keypress dismissal, smoothly restoring the previous screen (`SCREEN_BLE_PORTAL`, `SCREEN_MENU`, `SCREEN_SYSTEM_INFO`, etc.).
  - 15 pre-configured emergency categories tailored for Himalayan health, maternal delivery, and relief supplies.
  - Hotkey mapping on 4x4 keypad (Keys 1–9 and 0 for top 10 time-critical alerts) with double confirmation screen.
  - Wireless SoftAP OTA update mode triggered by holding Key `0` for 3 seconds.

### 📱 LifeLine Companion Ecosystem (Mobile & Web)
* **LifeLine Companion Mobile App (`lifeline_companion/`)**:
  - Built with React Native & Expo for iOS and Android.
  - Tactical military-grade dark theme with real-time BLE connection management.
  - Radar Screen with dynamic signal sweeps, distance estimation, and RSSI tracking.
  - Freeform SITREP composer with one-touch emergency macros.
  - Acoustic and haptic feedback on incoming distress alerts and ACK receipts.
* **Zero-Install Web Bluetooth Companion (`bluefy_companion/` & `portal_preview/`)**:
  - Offline Progressive Web Application (PWA) compatible with iOS (Bluefy Browser) and Android (Google Chrome).
  - Instant pairing via QR Code camera scanning.
  - Full packet stream terminal and hardware status monitoring.

### 🔬 LifeLine SPU (Sensor Processing Unit)
* **Target Board**: ESP32 DevKit V1
* **Sensors**: NEO-6M GPS, DHT11/22, MPU6050 (6-DOF IMU), MQ135 (Air Quality/Gas)
* **Key Features**:
  - Real-time DSP filtering (10-sample circular moving average, tilt angle calculation).
  - Multi-sensor fusion engine: classifies Forest Fire, Earthquake, Landslide, Toxic Gas, and Extreme Weather.
  - Computes dynamic **Node Health Score (0-100%)** and **Environmental Risk Score (0-100%)**.
  - Built-in Wi-Fi Captive Portal and direct HTTP REST client uploading telemetry logs to Cloud API.

### 🛰️ LifeLine CCU (Communication Controller Unit)
* **Target Board**: ESP32 DevKit V1
* **RF Transceiver**: SX1278 LoRa @ 433 MHz
* **Key Features**:
  - Isolated microcontroller dedicated strictly to RF packet dispatching.
  - Parses binary telemetry packets over high-speed UART from SPU with CRC-16 integrity verification.
  - Formats data into Base Station compatible payloads (`TX[ID],[ALERT_CODE]`) with exponential backoff retries.

---

## 4. Hardware Pinouts Quick Reference

### LifeLine RX Pro Pinout
| Function | ESP32 Pin | Connected Component |
| :--- | :--- | :--- |
| **LoRa SCK** | GPIO 18 | SX1278 SCK |
| **LoRa MISO** | GPIO 19 | SX1278 MISO |
| **LoRa MOSI** | GPIO 23 | SX1278 MOSI |
| **LoRa NSS / CS** | GPIO 5 | SX1278 NSS |
| **LoRa RESET** | GPIO 15 | SX1278 RESET |
| **LoRa DIO0** | GPIO 2 | SX1278 DIO0 |
| **I2C SDA** | GPIO 21 | 16×2 LCD SDA |
| **I2C SCL** | GPIO 22 | 16×2 LCD SCL |
| **Wi-Fi Setup Button** | GPIO 14 | Push Button (Active LOW) |
| **Emergency Siren** | GPIO 25 | Active Buzzer / Transistor Driver |
| **Status LEDs** | GPIO 26 (Red Data), GPIO 27 (Green Wi-Fi) | 330Ω Current Limiting Resistors |

### LifeLine TX Pro Pinout
| Function | ESP32 Pin | Connected Component |
| :--- | :--- | :--- |
| **TFT MOSI / SDA** | GPIO 23 | ST7789 MOSI |
| **TFT SCLK / SCL** | GPIO 18 | ST7789 SCLK |
| **TFT CS** | GPIO 15 | ST7789 CS |
| **TFT DC** | GPIO 4 | ST7789 DC |
| **TFT RESET** | GPIO 2 | ST7789 RES |
| **Keypad Rows (R1-R4)** | GPIO 13, 12, 14, 27 | 4×4 Matrix Keypad Rows |
| **Keypad Cols (C1-C4)** | GPIO 26, 25, 33, 32 | 4×4 Matrix Keypad Columns |
| **LoRa NSS / CS** | GPIO 5 | SX1278 NSS |
| **LoRa RESET** | GPIO 0 | SX1278 RESET |
| **LoRa DIO0** | GPIO 34 | SX1278 DIO0 (Input only) |

### LifeLine SPU Pinout
| Sensor / Peripheral | ESP32 Pin | Protocol / Mode |
| :--- | :--- | :--- |
| **DHT11 / DHT22 Data** | GPIO 23 | 1-Wire Digital (10kΩ Pull-up) |
| **MQ135 Gas Analog** | GPIO 34 | ADC1 Channel (Analog IN) |
| **MPU6050 SDA** | GPIO 21 | I2C Data |
| **MPU6050 SCL** | GPIO 22 | I2C Clock |
| **NEO-6M GPS TX / RX** | GPIO 16 (RX2), GPIO 17 (TX2) | UART2 (9600 Baud) |
| **Manual SOS Push Button** | GPIO 26 | Hardware Interrupt (Active LOW) |
| **Status / Wi-Fi LED** | GPIO 2 | Digital Output |

---

## 5. Build & Deployment Instructions

### Prerequisites
- Install [PlatformIO Core (CLI)](https://platformio.org/install/cli) or the [PlatformIO IDE extension](https://marketplace.visualstudio.com/items?itemName=platformio.platformio-ide) in VS Code.
- Ensure the Espressif 32 platform package is installed.

### Option A: Using the Unified PowerShell Tool (`build.ps1`)
From the repository root, run:
```powershell
# Compile all 4 firmware projects in sequence
.\build.ps1

# Compile specific units
.\build.ps1 rx       # Base Station Receiver
.\build.ps1 tx       # Handheld Field Transmitter
.\build.ps1 spu      # Sensor Node SPU
.\build.ps1 ccu      # Communication Bridge CCU

# Compile and flash directly to connected hardware over USB
.\build.ps1 rx -Upload
.\build.ps1 tx -Upload

# Clean all temporary .pio build caches
.\build.ps1 clean
```

### Option B: Using PlatformIO CLI Directly
```powershell
pio run -d lifeline_rx_pro
pio run -d lifeline_tx_pro
pio run -d lifeline_tx_spu
pio run -d lifeline_tx_ccu
```

### Option C: In Visual Studio Code
1. Open the multi-project workspace by clicking on [lifeline.code-workspace](file:///d:/lifeline_hardware/lifeline.code-workspace).
2. Press `Ctrl+Shift+B` to launch the build task menu and choose your target.

### Option D: Running the Mobile & Web Companion Applications
```powershell
# 1. Launch the React Native Expo Mobile App
cd lifeline_companion
npm install
npx expo start

# 2. Launch the Web Companion & Offline QR Pairing Demo Server
cd ..
node serve_demo.js
# Access: http://localhost:8080/portal_preview/companion_app.html
# Or open bluefy_companion/index.html on iOS (Bluefy) or Android (Chrome)
```

---

## 6. Documentation Index

All in-depth documentation is organized inside the [`docs/`](file:///d:/lifeline_hardware/docs) directory:

| Domain | Document | Description |
| :--- | :--- | :--- |
| **Architecture** | [System Architecture](file:///d:/lifeline_hardware/docs/architecture/system_architecture.md) | High-level system design and end-to-end data flow |
| **Architecture** | [Dual-ESP32 Sensor Node](file:///d:/lifeline_hardware/docs/architecture/sensornode_dual_esp32.md) | Detailed DSP filtering equations, fusion logic & theory |
| **Architecture** | [Sensor Node Specification](file:///d:/lifeline_hardware/docs/architecture/sensornode_specification.md) | Original architectural requirements and design baseline |
| **Architecture** | [Base Station UI Flow](file:///d:/lifeline_hardware/docs/architecture/rx_pro_ui_flow.md) | 16×2 LCD screen states, buttons & alert popups |
| **Architecture** | [Two-Way LoRa & BLE Architecture](file:///d:/lifeline_hardware/docs/architecture/two_way_ble_nepal_terrain_upgrade.md) | v4.0 Two-Way LoRa ACK, BLE smartphone integration & Nepal mountain resilience |
| **Hardware** | [Wiring & Pinouts Master](file:///d:/lifeline_hardware/docs/hardware/wiring_and_pinouts.md) | Complete pin connection tables for all boards |
| **Hardware** | [Schematics & Assets](file:///d:/lifeline_hardware/docs/hardware/schematics/) | Circuit schematics, breadboard diagrams, and PDFs |
| **API** | [SPU Telemetry REST API](file:///d:/lifeline_hardware/docs/api/spu_telemetry_api.md) | Direct Cloud upload JSON schemas and HTTP endpoints |
| **API** | [Cloud API Setup & Web Integration](file:///d:/lifeline_hardware/docs/api/cloud_api_setup_guide.md) | Ingestion guide for distance, custom SITREPs, SNR, and database migrations |
| **Guides** | [Local Wireless OTA Flashing](file:///d:/lifeline_hardware/docs/guides/local_ota_guide.md) | How to flash TX and RX wirelessly over SoftAP |
| **Guides** | [VPS Remote HTTPS OTA](file:///d:/lifeline_hardware/docs/guides/vps_ota_guide.md) | How to configure VPS cloud auto-updates for Base Stations |
| **Guides** | [Bluetooth BLE & Mobile Companion](file:///d:/lifeline_hardware/docs/guides/bluetooth_ble_integration_guide.md) | Dual-unit BLE GATT Nordic UART, mobile chat & offline Web Bluetooth PWA guide |
| **Roadmap** | [Master Feature Matrix](file:///d:/lifeline_hardware/docs/roadmap/features_matrix.md) | Comprehensive feature comparison across all units |
| **Roadmap** | [Upgrade Roadmap](file:///d:/lifeline_hardware/docs/roadmap/upgrade_roadmap.md) | Roadmap for 1-hour sensor logging & dual-mode transmission |

---

## 📄 License
This project is open-source hardware & firmware developed for humanitarian disaster relief and community safety.
