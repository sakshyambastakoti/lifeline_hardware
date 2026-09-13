# 🌐 Bluefy & Web Bluetooth Offline Companion

> **Zero-Install Offline Progressive Web Application (PWA) for iOS (Bluefy Browser) and Android (Google Chrome)**

---

## 📌 Overview

The **Bluefy Companion** is a standalone single-file HTML/JavaScript Web Bluetooth terminal and situational dashboard. It allows any smartphone or laptop to pair with **LifeLine TX Pro** and **LifeLine RX Pro** without installing an app from an app store.

* **iOS Support**: Runs in the free [Bluefy Web BLE Browser](https://apps.apple.com/app/bluefy-web-ble-browser/id1492822055) or WebBLE (Apple Safari blocks Web Bluetooth).
* **Android / Desktop Support**: Runs natively in Google Chrome, Microsoft Edge, Opera, and Brave.
* **100% Offline**: Works completely without internet once loaded.

---

## 🚀 How to Run

### Method 1: Local HTTP Server
```powershell
cd bluefy_companion
node server.js
```
Open your browser at `http://localhost:8080` (or `http://<YOUR_LAN_IP>:8080` from your phone).

### Method 2: Direct File Open
Simply double-click `index.html` on your desktop or open from file storage.

---

## 📡 Hardware Connection
1. Power on your LifeLine TX Pro (`LifeLine-TX-XXX`) or RX Pro (`LifeLine-RX-Base`).
2. Tap **"CONNECT BLE"** in the companion web app.
3. Select the LifeLine device from the browser Bluetooth pairing dialog.
4. The terminal will establish bidirectional communication via Nordic UART Service (NUS).
