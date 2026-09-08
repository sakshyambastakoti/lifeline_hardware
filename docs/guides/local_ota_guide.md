# 📡 LifeLine Ecosystem - Wireless OTA Firmware Upload Guide

This document provides step-by-step instructions for uploading firmware wirelessly (Over-The-Air) to all boards in the LifeLine Emergency System, including the **Field Transmitter (`lifeline_tx_pro`)** and the **Base Station Receiver (`lifeline_rx_pro`)**.

---

## 🛠️ Summary of Board OTA Features

| Board Unit | Trigger to Enter OTA Mode | How to Exit OTA Mode | Wireless AP SSID | Default IP | Supported Upload Methods |
|---|---|---|---|---|---|
| **Transmitter (`lifeline_tx_pro`)** | **Hold Key `0` for 3 Seconds** on Keypad | Press **`#`** Key | `LifeLine-TX-OTA` | `192.168.4.1` | Browser, PIO CLI, PIO IDE, cURL, `espota.py` |
| **Receiver (`lifeline_rx_pro`)** | **Press Wi-Fi Button 3 Times** (within 1.5s) | Press **Wi-Fi Button 1 Time** | `LifeLine-RX-OTA` | `192.168.4.1` | Browser, PIO CLI, PIO IDE, cURL, VPS HTTPS |

---

## 1. 📻 Field Transmitter Unit (`lifeline_tx_pro`)

### **Step 1: Enter OTA Mode**
1. Power on the `lifeline_tx_pro` transmitter unit.
2. On the 4x4 Keypad, **press and hold Key `0` for 3 seconds**.
3. The ST7789 TFT display will switch to the **WIRELESS OTA UPDATE** screen:
   - **SSID (AP)**: `LifeLine-TX-OTA`
   - **Password**: `12345678`
   - **Web URL**: `http://192.168.4.1`

### **Step 2: Connect Computer to Wi-Fi**
Connect your PC or Laptop Wi-Fi to network **`LifeLine-TX-OTA`** using password **`12345678`**.

### **Step 3: Upload Firmware (Choose any method)**

#### **Method A: Web Browser Upload (Easiest)**
1. Open your browser and go to: `http://192.168.4.1`
2. Click **Choose File** → Select `.pio/build/esp32dev/firmware.bin` from the `lifeline_tx_pro` directory.
3. Click **Upload & Flash Firmware**.
4. The TFT display will show live upload progress `0%` to `100%`, then automatically reboot with new code!

#### **Method B: PlatformIO Terminal / CLI Command**
Open Terminal in VS Code / PowerShell and run:
```powershell
cd d:\lifeline_hardware\lifeline_tx_pro
C:\Users\ACER\.platformio\penv\Scripts\platformio.exe run -e esp32dev_ota -t upload
```

#### **Method C: cURL Command Line**
```powershell
curl -F "firmware=@.pio/build/esp32dev/firmware.bin" http://192.168.4.1/update
```

#### **Method D: PlatformIO IDE GUI (VS Code)**
1. Open `platformio.ini` in `lifeline_tx_pro`.
2. Under `[env:esp32dev_ota]`, click **Upload** in the PlatformIO Sidebar.

### **Step 4: Exit OTA Mode**
If you want to cancel or leave OTA mode without flashing:
- Press key **`#`** on the Keypad to return to the main menu.

---

## 2. 📺 Base Station Receiver (`lifeline_rx_pro`)

The Receiver unit supports **both Local Wireless OTA** and **Automatic VPS HTTPS Cloud OTA**.

---

### 🌐 **Mode A: Local Wireless OTA (Button Triple Press)**

#### **Step 1: Enter Local OTA Mode**
1. On the Receiver unit, **press the physical Wi-Fi button (GPIO 14) 3 times quickly** (within 1.5 seconds).
2. The 16x2 LCD display will show:
   ```text
   OTA LOCAL PORTAL
   192.168.4.1     
   ```

#### **Step 2: Connect Computer to Wi-Fi**
Connect your PC or Laptop Wi-Fi to network **`LifeLine-RX-OTA`** using password **`12345678`**.

#### **Step 3: Upload Firmware (Choose any method)**

##### **Method 1: Web Browser Portal**
1. Open `http://192.168.4.1` in your browser.
2. Select `.pio/build/esp32dev/firmware.bin` from `lifeline_rx_pro`.
3. Click **Flash Firmware**.
4. The 16x2 LCD will show progress `Updating FW  45%` and reboot upon completion.

##### **Method 2: PlatformIO Terminal / CLI Command**
```powershell
cd d:\lifeline_hardware\lifeline_rx_pro
C:\Users\ACER\.platformio\penv\Scripts\platformio.exe run -e esp32dev_ota -t upload
```

##### **Method 3: cURL Command Line**
```powershell
curl -F "firmware=@.pio/build/esp32dev/firmware.bin" http://192.168.4.1/update
```

#### **Step 4: Exit Local OTA Mode**
- Press the **Wi-Fi Button 1 time** (single short click).
- The RX unit closes the AP and returns to the normal `IDLE` screen.

---

### ☁️ **Mode B: VPS HTTPS Cloud Remote OTA**

1. Ensure the RX Base Station is connected to Wi-Fi.
2. Every time the device boots, it automatically checks the remote VPS manifest (`version.json`).
3. If a newer version is found on the VPS server, the device streams the `.bin` update over HTTPS, updates the 16x2 LCD with download progress, and reboots automatically.

---

## ⚡ Quick Flashing Cheatsheet

### Build Firmware `.bin` Files
```powershell
# Build Transmitter Firmware
cd d:\lifeline_hardware\lifeline_tx_pro
C:\Users\ACER\.platformio\penv\Scripts\platformio.exe run

# Build Receiver Firmware
cd d:\lifeline_hardware\lifeline_rx_pro
C:\Users\ACER\.platformio\penv\Scripts\platformio.exe run
```

### USB Cable Flashing (Default Fallback)
```powershell
# Flash Transmitter via USB Cable
cd d:\lifeline_hardware\lifeline_tx_pro
C:\Users\ACER\.platformio\penv\Scripts\platformio.exe run --target upload

# Flash Receiver via USB Cable
cd d:\lifeline_hardware\lifeline_rx_pro
C:\Users\ACER\.platformio\penv\Scripts\platformio.exe run --target upload
```

> 💡 **Tip for USB Flashing**: If flashing stalls at `Connecting........___`, press and hold the **BOOT** button on the ESP32 board for 1-2 seconds until progress starts.
