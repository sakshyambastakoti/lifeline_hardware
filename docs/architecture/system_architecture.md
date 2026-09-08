# LifeLine System Architecture & Technical Overview

## 1. Executive Summary

**LifeLine** is an off-grid disaster monitoring, emergency notification, and life-safety communication ecosystem engineered for remote regions, rugged mountainous environments, disaster-prone valleys, and areas lacking cellular or grid infrastructure.

The system combines long-range **LoRa (SX1278 433 MHz)** spread-spectrum radio communication, multi-sensor IoT edge computing on **ESP32** microcontrollers, local display and acoustic notification networks, and automated **Cloud Web Dashboard** integration.

---

## 2. End-to-End System Topology

```
┌──────────────────────────────────────────────┐
│             FIELD SENSING LAYER              │
│                                              │
│  ┌────────────────────┐   ┌───────────────┐  │
│  │  LifeLine TX Pro   │   │ LifeLine SPU  │  │
│  │ (Field Transmitter)│   │ (Sensor Node) │  │
│  │   [Keypad + TFT]   │   │  [GPS + IMU   │  │
│  │                    │   │  + Temp + Gas]│  │
│  └─────────┬──────────┘   └───────┬───────┘  │
└────────────┼──────────────────────┼──────────┘
             │                      │ Direct Wi-Fi
             │ LoRa 433 MHz         │ or CCU LoRa
             ▼                      ▼
┌──────────────────────────────────────────────┐
│           COMMAND & RECEIVER LAYER           │
│                                              │
│  ┌────────────────────────────────────────┐  │
│  │          LifeLine RX Pro               │  │
│  │       (Base Station Receiver)          │  │
│  │    [SX1278 + 16x2 LCD + Siren]         │  │
│  └───────────────────┬────────────────────┘  │
└──────────────────────┼───────────────────────┘
                       │
                       │ HTTPS REST API Gateway
                       ▼
┌──────────────────────────────────────────────┐
│               CLOUD & WEB LAYER              │
│                                              │
│  ┌────────────────────────────────────────┐  │
│  │         Central Web Server             │  │
│  │       (Emergency Dashboard)            │  │
│  │  [Live Map, Real-time SMS, Email Push] │  │
│  └────────────────────────────────────────┘  │
└──────────────────────────────────────────────┘
```

---

## 3. Subsystem Breakdown

### 3.1 Field Transmitter (`lifeline_tx_pro`)
* **Purpose**: Portable field communicator used by rangers, mountaineers, or village wardens to broadcast instant manual SOS alerts.
* **Key Components**: ESP32, SX1278 LoRa Transceiver, 2.8" SPI TFT (ST7789), 4x4 Matrix Keypad, Status LEDs, Audio Piezo Buzzer.
* **Transmission Protocol**: Lightweight ASCII syntax: `TX[ID],[ALERT_CODE]` (e.g. `TX003,E`).
* **RF Link**: 433 MHz, Spreading Factor SF12, Bandwidth 125 kHz, Coding Rate 4/8, Output Power +18 dBm.

### 3.2 Base Station Receiver (`lifeline_rx_pro`)
* **Purpose**: Stationary receiving hub installed in emergency operation centers, local hospitals, or police posts.
* **Key Components**: ESP32, SX1278 LoRa Receiver, 16x2 I2C Character LCD, High-Decibel Siren, Wi-Fi Push Button.
* **Functions**: Continuous RF listening, RSSI calculation, LCD alert rendering, loud siren actuation, Wi-Fi captive portal, and automated REST API alert forwarding.

### 3.3 Sensor Processing Unit (`lifeline_tx_spu`)
* **Purpose**: Autonomous environmental watchdog deployed in hazard zones (landslide tracks, riverbanks, forest perimeters).
* **Key Sensors**: NEO-6M GPS, DHT11/DHT22 (Temperature & Humidity), MPU6050 (6-DOF IMU), MQ135 (Air Quality & Toxic Gas).
* **Edge Intelligence**: Real-time DSP filtering (10-sample moving average), tilt angle computation, sudden impact and seismic vibration detection, quantitative Node Health & Environmental Risk scoring.
* **Cloud Uplink**: Built-in Wi-Fi Captive Portal and HTTP REST Client for periodic sensor telemetry upload.

### 3.4 Communication Controller Unit (`lifeline_tx_ccu`)
* **Purpose**: Dedicated LoRa communications engine paired with the SPU for environments completely lacking Wi-Fi coverage.
* **Functions**: Reads structured telemetry packets from SPU, performs CRC-16 validation, queues messages, and transmits them over LoRa to the LifeLine RX Pro base station.

---

## 4. Hardware Interconnect & Protocol Reference

Detailed documentation for each layer is available in the `docs/` tree:
- [Hardware Wiring & Pinout Tables](file:///d:/lifeline_hardware/docs/hardware/wiring_and_pinouts.md)
- [Dual-ESP32 Sensor Node Architecture & Theory](file:///d:/lifeline_hardware/docs/architecture/sensornode_dual_esp32.md)
- [Base Station UI Flow & Menu States](file:///d:/lifeline_hardware/docs/architecture/rx_pro_ui_flow.md)
- [Cloud Telemetry REST API Specification](file:///d:/lifeline_hardware/docs/api/spu_telemetry_api.md)
- [Local & Remote Wireless OTA Flashing Guide](file:///d:/lifeline_hardware/docs/guides/local_ota_guide.md)
- [Master Feature Matrix](file:///d:/lifeline_hardware/docs/roadmap/features_matrix.md)
- [Future Upgrade Roadmap](file:///d:/lifeline_hardware/docs/roadmap/upgrade_roadmap.md)
