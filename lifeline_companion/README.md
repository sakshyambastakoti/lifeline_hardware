# 📱 LifeLine Companion — Tactical Mobile Application

> **Ruggedized Cross-Platform Mobile Companion for Off-Grid Disaster Monitoring & Search and Rescue**  
> Built with **React Native**, **Expo**, and **TypeScript** for iOS and Android.

---

## 📌 Overview

The **LifeLine Companion** mobile application acts as an off-grid tactical field terminal, connecting via **Bluetooth Low Energy (BLE 4.2 / 5.0 Nordic UART)** directly to **LifeLine TX Pro** (field handheld communicators) and **LifeLine RX Pro** (incident command base stations).

It operates **100% autonomously without cellular service, Wi-Fi routers, or internet access**, allowing first responders and disaster coordinators to:
1. Compose and transmit freeform situation reports (SITREPs) across mountain valleys over 433 MHz LoRa.
2. Track active field responders and distress beacons on a dynamic 60 FPS sweeping Radar HUD.
3. Receive closed-loop Base Station ACKs and dispatcher orders with acoustic chimes and haptic vibrations.
4. Simulate complete emergency drills using the built-in **Offline Mock BLE Simulation Engine**.

---

## 🎨 Tactical Cybernetic Design System

Engineered specifically for low-light rescue operations and high-glare alpine sunlight:
* **Obsidian Canvas**: `#0A0F1D` (Deep tactical dark background)
* **Card & Panel Layer**: `#111927` with `#1E293B` micro-borders
* **Telemetry Cyan**: `#00F3FF` (Primary telemetry beacons, active scans, and radio indicators)
* **Distress Electric Red**: `#FF003C` (Critical alerts, evacuations, and SOS triggers)
* **Nominal Emerald Green**: `#00FF66` (Closed-loop ACK confirmations and healthy links)
* **Warning Amber**: `#FFB800` (Degraded links and pending ACKs)

---

## 🏗️ Architecture & Project Structure

```text
lifeline_companion/
├── App.tsx                             <-- Main navigation container & tab navigator
├── app.json                            <-- Expo configuration (Bluetooth & background permissions)
├── package.json                        <-- Dependencies (Expo, React Native, Vector Icons)
│
└── src/
    ├── components/
    │   ├── TacticalHeader.tsx          <-- Real-time hardware status, battery, and link quality bar
    │   ├── StatusBadge.tsx             <-- Severity indicators (NOMINAL, DISTRESS, OFFLINE)
    │   └── LinkQualityIndicator.tsx    <-- Visual RSSI meter with dBm readout
    │
    ├── context/
    │   └── LifeLineContext.tsx         <-- Central BLE state machine, message history, and nodes
    │
    ├── screens/
    │   ├── RadarScreen.tsx             <-- 60 FPS sweeping radar HUD & node discovery list
    │   ├── SitrepScreen.tsx            <-- Freeform LoRa message composer with emergency macros
    │   └── SettingsScreen.tsx          <-- Device scanner, telemetry counters, and mock simulator
    │
    ├── services/
    │   ├── BleService.ts               <-- Core BLE GATT Nordic UART driver (rx/tx characteristics)
    │   ├── MockBleService.ts           <-- Synthetic disaster scenario generator for training
    │   └── NotificationService.ts      <-- Acoustic sound chimes and haptic feedback
    │
    └── theme/
        └── colors.ts                   <-- Military-grade cybernetic color tokens
```

---

## 🚀 Getting Started

### Prerequisites
* [Node.js](https://nodejs.org/) (v18 or v20 LTS recommended)
* [Expo Go](https://expo.dev/go) app installed on your physical iOS or Android smartphone (optional, or run on Web)

### Installation & Launch

```powershell
# 1. Navigate to the companion app directory
cd lifeline_companion

# 2. Install dependencies
npm install

# 3. Start the Expo development server
npx expo start
```

### Running on Physical Smartphone
1. In the terminal, a QR code will be generated.
2. **iOS**: Open the native Camera app and scan the QR code to launch in **Expo Go**.
3. **Android**: Open the **Expo Go** app and scan the QR code.
4. Ensure Bluetooth is enabled on your phone.

### Running on Web Browser
Press **`w`** in the terminal to launch the web preview in your default browser.

---

## 🧪 Testing Without Hardware (Mock Simulation Mode)

If physical LifeLine hardware is not currently connected:
1. Open the app and navigate to the **Settings** tab.
2. Toggle **"Enable Offline Simulation Mode"** to `ON`.
3. The app will immediately initialize `MockBleService`, streaming realistic synthetic emergency beacons, varying RSSI values, and simulated Base Station ACKs for training exercises and UI demonstration.

---

## 🔗 Hardware BLE Specification Parity

The mobile app interfaces with firmware using the standard **Nordic UART Service (NUS)**:
* **Service UUID**: `6E400001-B5A3-F393-E0A9-E50E24DCCA9E`
* **RX Characteristic (Write)**: `6E400002-B5A3-F393-E0A9-E50E24DCCA9E`
  * Uplink payloads: `MSG:<text>`, `ALERT:<code>`, `STATUS`, `PING`
* **TX Characteristic (Notify)**: `6E400003-B5A3-F393-E0A9-E50E24DCCA9E`
  * Downlink streams: `ACK_RECV:...`, `ALERT:...`, `CHAT:...`, `STATUS:...`

---

## 📄 License
Developed as part of the **LifeLine Off-Grid Emergency Response System** for humanitarian relief and public safety.
