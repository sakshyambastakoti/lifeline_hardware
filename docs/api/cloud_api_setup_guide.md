# 🌐 LifeLine — Master Cloud API & VPS Deployment Guide

> **The Complete, Production-Ready Server & API Deployment Specification for the LifeLine Off-Grid Emergency Response System**  
> **Firmware Version Compatibility:** v4.2 PRO (LifeLine RX Pro Gateway, TX Pro Handheld & SPU)  
> **Server Stack:** Ubuntu / Debian Linux, Nginx, PHP 8.1+ (FPM), MariaDB / MySQL 8.0+  
> **Default Gateway Ingestion Target:** `https://<YOUR_DOMAIN>/lifeline/API/Create/message.php`  
> **Default Gateway Downlink Poller Target:** `https://<YOUR_DOMAIN>/lifeline/API/Read/pending_commands.php`  

---

## 📑 Table of Contents
1. [Overview & Master Architecture](#1-overview--master-architecture)
2. [What the LifeLine RX Pro Base Station Sends to the Cloud](#2-what-the-lifeline-rx-pro-base-station-sends-to-the-cloud)
3. [VPS Server Prerequisites & Environment Installation](#3-vps-server-prerequisites--environment-installation)
4. [Complete Server Directory Layout](#4-complete-server-directory-layout)
5. [Complete Master Database Setup (`lifeline_complete.sql`)](#5-complete-master-database-setup-lifeline_completesql)
6. [Core Database Connection & Settings (`database.php`)](#6-core-database-connection--settings-databasephp)
7. [Telemetry & Emergency Ingestion API (`API/Create/message.php`)](#7-telemetry--emergency-ingestion-api-apicreatemessagephp)
8. [Two-Way Closed-Loop Downlink Messaging (`Website ➔ RX ➔ TX`)](#8-two-way-closed-loop-downlink-messaging-website--rx--tx)
   - [8.1 Queue Outgoing Command (`API/Create/command.php`)](#81-queue-outgoing-command-apicreatecommandphp)
   - [8.2 Gateway Poll Pending Commands (`API/Read/pending_commands.php`)](#82-gateway-poll-pending-commands-apireadpending_commandsphp)
   - [8.3 Gateway Downlink Status Ack (`API/Update/command_status.php`)](#83-gateway-downlink-status-ack-apiupdatecommand_statusphp)
9. [Fleet & Device Management APIs (`/API/devices/`)](#9-fleet--device-management-apis-apidevices)
   - [9.1 List Fleet & Heartbeats (`API/Read/device.php`)](#91-list-fleet--heartbeats-apireaddevicephp)
   - [9.2 Register New Field Node (`API/Create/device.php`)](#92-register-new-field-node-apicreatedevicephp)
   - [9.3 Update Node Attributes (`API/Update/device.php`)](#93-update-node-attributes-apiupdatedevicephp)
   - [9.4 Delete Decommissioned Node (`API/Delete/device.php`)](#94-delete-decommissioned-node-apideletedevicephp)
10. [Dashboard Incident Query & Resolution APIs (`/API/messages/`)](#10-dashboard-incident-query--resolution-apis-apimessages)
    - [10.1 Live Incident Query with Filters & Pagination (`API/Read/message.php`)](#101-live-incident-query-with-filters--pagination-apireadmessagephp)
    - [10.2 Mark Incident as Resolved (`API/Update/message.php`)](#102-mark-incident-as-resolved-apiupdatemessagephp)
    - [10.3 Delete Message Record (`API/Delete/message.php`)](#103-delete-message-record-apideletemessagephp)
11. [Rescue Teams & Taxonomy Index APIs](#11-rescue-teams--taxonomy-index-apis)
    - [11.1 Responders Query & Management (`API/Read/helps.php`, `API/Create/helps.php`)](#111-responders-query--management-apireadhelpsphp-apicreatehelpsphp)
    - [11.2 Dynamic JSON Taxonomy Dictionaries (`API/Read/index.php`, `API/Create/index.php`)](#112-dynamic-json-taxonomy-dictionaries-apireadindexphp-apicreateindexphp)
    - [11.3 Alert Email Subscribers (`API/Read/emails.php`, `API/Create/emails.php`)](#113-alert-email-subscribers-apireademailsphp-apicreateemailsphp)
12. [Authentication & Operator Access APIs (`/API/auth/`)](#12-authentication--operator-access-apis-apiauth)
    - [12.1 Operator Login (`API/auth/login.php`)](#121-operator-login-apiauthloginphp)
    - [12.2 Session Verification Heartbeat (`API/auth/check.php`)](#122-session-verification-heartbeat-apiauthcheckphp)
    - [12.3 Logout & Session Cleanup (`API/auth/logout.php`)](#123-logout--session-cleanup-apiauthlogoutphp)
13. [Automated Dispatch Helpers (Email & Web Push)](#13-automated-dispatch-helpers-email--web-push)
    - [13.1 Emergency HTML Email Dispatcher (`API/email_helper.php`)](#131-emergency-html-email-dispatcher-apiemail_helperphp)
    - [13.2 Google Firebase HTTP v1 Web Push (`API/fcm_helper.php`)](#132-google-firebase-http-v1-web-push-apifcm_helperphp)
14. [Web Dashboard Two-Way Dispatch Console Component](#14-web-dashboard-two-way-dispatch-console-component)
15. [Configuring the LifeLine RX Pro Gateway Hardware](#15-configuring-the-lifeline-rx-pro-gateway-hardware)
16. [End-to-End Verification Playbook with `curl`](#16-end-to-end-verification-playbook-with-curl)

---

## 📌 1. Overview & Master Architecture

The **LifeLine Emergency Ecosystem** bridges off-grid Himalayan disaster zones with centralized search-and-rescue headquarters. In areas where cellular towers and fiber backbones have collapsed due to earthquakes, landslides, or floods, LifeLine establishes a multi-tiered communication mesh:

```text
┌─────────────────────────────────────────────────────────────────────────────────────────┐
│                                 FIELD TIER (OFF-GRID)                                   │
│                                                                                         │
│   ┌─────────────────────────┐                                                           │
│   │     LifeLine TX Pro     │◄─── Bluetooth 4.2/5.0 ───► ┌───────────────────────────┐  │
│   │ (Field Handheld Device) │   (Nordic UART NUS)        │  Field Smartphone / App   │  │
│   └────────────┬────────────┘                            │ (React Native / PWA Chat) │  │
│                │                                         └───────────────────────────┘  │
│                │ 433 MHz SX1278 LoRa RF (Long-Range Uplink & Downlink)                  │
│                ▼                                                                        │
│   ┌─────────────────────────┐                                                           │
│   │     LifeLine RX Pro     │◄─── Bluetooth 4.2/5.0 ───► ┌───────────────────────────┐  │
│   │  (Base Station Gateway) │   (Nordic UART NUS)        │ Base Station Commander    │  │
│   │   [Full 16x2 LCD Display│                            │ Smartphone / Terminal     │  │
│   └────────────┬────────────┘                            └───────────────────────────┘  │
└────────────────┼────────────────────────────────────────────────────────────────────────┘
                 │
                 │ HTTPS REST API over Wi-Fi / Cellular Hotspot / Satellite
                 ▼
┌─────────────────────────────────────────────────────────────────────────────────────────┐
│                             CENTRAL CLOUD PLATFORM (VPS)                                │
│                                                                                         │
│   ┌─────────────────────────────────────────────────────────────────────────────────┐   │
│   │                             Nginx Web Server / HTTPS                            │   │
│   └────────────────────────────────────────┬────────────────────────────────────────┘   │
│                                            │                                            │
│   ┌────────────────────────────────────────┴────────────────────────────────────────┐   │
│   │                               PHP 8.x REST API Engine                           │   │
│   │                                                                                 │   │
│   │  • /API/Create/message.php     ◄── Ingests SOS, Chat SITREPs, SNR, Distance     │   │
│   │  • /API/Read/pending_commands  ◄── Polled by RX Gateway for Web Dispatches      │   │
│   │  • /API/Update/command_status  ◄── Confirms LoRa RF broadcast back to Web       │   │
│   │  • /API/Read/message.php       ◄── Serves Web Dashboard & Mapping UI           │   │
│   │  • /API/devices/*              ◄── Fleet management & automated ping sync       │   │
│   │  • email_helper & fcm_helper   ───► Sends Instant Alerts (SMS, Email, WebPush)  │   │
│   └────────────────────────────────────────┬────────────────────────────────────────┘   │
│                                            │                                            │
│   ┌────────────────────────────────────────┴────────────────────────────────────────┐   │
│   │                       MariaDB / MySQL Relational Database                       │   │
│   │  [messages, devices, downlink_commands, helps, indexes, emails, user]           │   │
│   └─────────────────────────────────────────────────────────────────────────────────┘   │
│                                            ▲                                            │
│                                            │ AJAX / Fetch Polling                       │
│   ┌────────────────────────────────────────┴────────────────────────────────────────┐   │
│   │                      Web Portal & Command Center Dashboard                      │   │
│   │  • Live Incident Map (Google Maps / Leaflet)                                    │   │
│   │  • Two-Way Web-to-LoRa Dispatch Console                                         │   │
│   │  • Fleet Status & Battery / Sensor Telemetry HUD                                │   │
│   └─────────────────────────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────────────────────┘
```

---

## 📡 2. What the LifeLine RX Pro Base Station Sends to the Cloud

The ESP32 Base Station (`lifeline_rx_pro`) acts as an intelligent edge computer. When it receives radio packets or commands, it formats structured JSON payloads and performs HTTPS requests to your VPS.

### 2.1 Complete Request Key Dictionary

| Key | Type | Example | Purpose / Meaning |
| :--- | :--- | :--- | :--- |
| `DID` | Integer | `1` | **Device ID**. `1-999` for field handheld units; `0` indicates a local message initiated at the Base Station gateway itself. |
| `message_code` | Integer | `0` to `14` | **Emergency Alert Code index**: `0` = Critical SOS, `1` = Delivery/Labor, `2` = Heli Rescue, `3` = Medicine, `4` = Oxygen, `5` = Injury, `6` = Blood, `7` = Altitude, `8` = Food, `9` = Water, `10` = Outbreak, `11` = Shelter, `12` = Landslide, `13` = Doctor, `14` = Status OK. For freeform chat, `0` is passed for priority attention. |
| `code` | String | `"M"` | **Single-letter protocol identifier**: `'A'-'O'` (Standard Alerts), `'M'` (Custom Chat/SITREP), `'N'` (Full Multi-Sensor Telemetry). |
| `RSSI` | Integer | `-68` | **Received Signal Strength Indicator** in dBm. Ranges from `-40` dBm (very close) to `-125` dBm (extreme fringe limit). |
| `snr` | Float | `8.5` | **Signal-to-Noise Ratio** in dB provided directly by the SX1278 hardware register. Allows the dashboard to distinguish true signal strength from RF noise. |
| `distance_km` | Float | `1.45` | **Estimated distance in kilometers** calculated on the ESP32 in real time using the calibrated log-distance path loss model and GPS reference coordinates. |
| `is_chat` | Boolean | `true` | `true` if this packet is a freeform custom text report (SITREP); `false` if it is a pre-defined hardware alert code. |
| `custom_msg` | String | `"Trapped on north trail"` | **Verbatim text of the message**. Sanitized by the ESP32 to escape quotes and remove carriage returns. Maximum 48 to 96 characters for RF efficiency. |
| `source` | String | `"LORA"` / `"BLE"` | Identifies the physical transmission interface: `"LORA"` for off-grid radio transmissions from field handhelds; `"BLE"` for local mobile commander entries. |
| `temp` | Float | `22.4` | Ambient temperature in °C (from SPU sensor node). |
| `humidity` | Float | `68.0` | Relative humidity in % (from SPU sensor node). |
| `gas_ppm` | Integer | `280` | Hazardous gas / smoke / air pollution reading in PPM (MQ135 sensor). |
| `lat` | Double | `27.717245` | GPS Latitude coordinate in decimal degrees. |
| `lon` | Double | `85.324000` | GPS Longitude coordinate in decimal degrees. |
| `alt` | Integer | `1350` | GPS Altitude in meters above sea level. |
| `health` | Integer | `98` | Edge node hardware health score (0–100%). |
| `risk` | Integer | `15` | Composite environmental hazard score (0–100%). |
| `api_key` | String | `"LF_PRO_KEY_2026"` | Pre-shared authentication key configured in the ESP32 NVS settings. |

---

### 2.2 The Three Payload Modes Sent by the Gateway

#### Mode 1: Custom Freeform SITREP Chat Message (Uplinked from Field or Base BLE)
Sent via `pushCustomChatMessageToAPI(...)` when a responder types a custom report on their mobile companion app (`CHAT:<devId>:<text>`) or when the base station commander types locally:
```json
{
  "api_key": "LF_PRO_KEY_2026",
  "DID": 3,
  "message_code": 0,
  "code": "M",
  "RSSI": -74,
  "snr": 7.2,
  "distance_km": 2.15,
  "is_chat": true,
  "custom_msg": "Landslide on north trail. 2 hikers stranded with hypothermia.",
  "source": "LORA"
}
```

#### Mode 2: Standard Categorized Emergency SOS Alert
Sent via `pushAlertToAPI(...)` when a field responder presses an emergency key on the 4×4 keypad or selects an alert category in the app:
```json
{
  "api_key": "LF_PRO_KEY_2026",
  "DID": 2,
  "message_code": 1,
  "RSSI": -62,
  "snr": 9.8,
  "distance_km": 0.85,
  "is_chat": false,
  "custom_msg": "",
  "source": "LORA"
}
```

#### Mode 3: Periodic Full Multi-Sensor Telemetry Log
Sent via `pushFullTelemetryToAPI(...)` containing comprehensive environmental, seismic, and health data:
```json
{
  "api_key": "LF_PRO_KEY_2026",
  "DID": 1,
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
  "source": "LORA"
}
```

---

## 🛠️ 3. VPS Server Prerequisites & Environment Installation

To host the LifeLine cloud backend on a clean Linux VPS (Ubuntu 22.04 or 24.04 LTS recommended):

### Step 3.1: Install Nginx, PHP, and MariaDB
Connect to your VPS via SSH and execute:

```bash
sudo apt update && sudo apt upgrade -y

# Install Nginx Web Server, MariaDB Database, and Git
sudo apt install -y nginx mariadb-server curl git ufw

# Install PHP 8.2 with all mandatory extensions
sudo apt install -y php8.2 php8.2-fpm php8.2-mysql php8.2-curl \
                    php8.2-mbstring php8.2-xml php8.2-zip php8.2-bcmath
```

### Step 3.2: Secure Database & Create User
```bash
sudo mysql_secure_installation
```
Log into MariaDB as root:
```bash
sudo mysql -u root -p
```
Run the following SQL commands to create the database and user:
```sql
CREATE DATABASE IF NOT EXISTS `lifeline` CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
CREATE USER IF NOT EXISTS 'lifeline_user'@'localhost' IDENTIFIED BY 'LifeLineSecurePass2026!';
GRANT ALL PRIVILEGES ON `lifeline`.* TO 'lifeline_user'@'localhost';
FLUSH PRIVILEGES;
EXIT;
```

### Step 3.3: Configure Nginx VirtualHost
Create a site configuration file:
```bash
sudo nano /etc/nginx/sites-available/lifeline
```
Paste the following configuration (replace `zenithkandel.com.np` with your domain or server IP):

```nginx
server {
    listen 80;
    listen [::]:80;
    server_name zenithkandel.com.np www.zenithkandel.com.np;

    root /var/www/html;
    index index.php index.html;

    # Maximum payload size for OTA firmware uploads
    client_max_body_size 16M;

    # Security Headers
    add_header X-Frame-Options "SAMEORIGIN";
    add_header X-Content-Type-Options "nosniff";
    add_header X-XSS-Protection "1; mode=block";

    location / {
        try_files $uri $uri/ /index.php?$query_string;
    }

    # Pass PHP scripts to FastCGI server
    location ~ \.php$ {
        include snippets/fastcgi-php.conf;
        fastcgi_pass unix:/run/php/php8.2-fpm.sock;
        fastcgi_param SCRIPT_FILENAME $document_root$fastcgi_script_name;
        include fastcgi_params;
        fastcgi_read_timeout 60;
    }

    # Block access to hidden files (.git, .env)
    location ~ /\. {
        deny all;
    }
}
```

Enable the configuration and reload Nginx:
```bash
sudo ln -s /etc/nginx/sites-available/lifeline /etc/nginx/sites-enabled/
sudo nginx -t
sudo systemctl reload nginx
```

### Step 3.4: Install Free SSL Certificate (HTTPS)
ESP32 HTTPS requires a valid SSL certificate. Use Let's Encrypt Certbot:
```bash
sudo apt install -y certbot python3-certbot-nginx
sudo certbot --nginx -d zenithkandel.com.np -d www.zenithkandel.com.np
```

---

## 📂 4. Complete Server Directory Layout

Create the directory structure under `/var/www/html/lifeline/`:

```bash
sudo mkdir -p /var/www/html/lifeline/API/{auth,Create,Read,Update,Delete,Config}
sudo mkdir -p /var/www/html/lifeline/portal/{css,js}
sudo mkdir -p /var/www/html/lifeline/ota
```

The resulting directory tree on your VPS:

```text
/var/www/html/lifeline/
├── database.php                        <-- Reusable PDO Database Connector (Singleton)
├── API/
│   ├── Config/
│   │   ├── config.php                  <-- System constants, API keys, SMTP credentials
│   │   └── database.php                <-- Optional alias pointing to ../../database.php
│   ├── auth/
│   │   ├── login.php                   <-- Operator authentication & session generation
│   │   ├── check.php                   <-- Active session validation heartbeat
│   │   └── logout.php                  <-- Session termination
│   ├── Create/
│   │   ├── message.php                 <-- Core Gateway Ingestion (SOS, Chat SITREPs, SNR, Distance)
│   │   ├── command.php                 <-- Queues Web-to-LoRa downlink commands
│   │   ├── device.php                  <-- Registers new LoRa field transmitter nodes
│   │   ├── helps.php                   <-- Registers rescue squads & capability tags
│   │   ├── emails.php                  <-- Subscribes alert email recipients
│   │   └── index.php                   <-- Creates dynamic taxonomy mappings
│   ├── Read/
│   │   ├── message.php                 <-- Incident log retrieval with filtering & pagination
│   │   ├── pending_commands.php        <-- Polled by RX Base Station every 3.5s
│   │   ├── device.php                  <-- Fleet inventory, status, and last ping monitor
│   │   ├── helps.php                   <-- Available rescue squads & emergency matching
│   │   ├── emails.php                  <-- Query subscribed emergency contacts
│   │   └── index.php                   <-- Taxonomy translation dictionaries (Locations, SOS codes)
│   ├── Update/
│   │   ├── command_status.php          <-- Confirms LoRa RF dispatch status from RX Base Station
│   │   ├── message.php                 <-- Mark emergency incident as resolved
│   │   ├── device.php                  <-- Edit device name or assigned village
│   │   └── helps.php                   <-- Update rescue team readiness / availability
│   ├── Delete/
│   │   ├── message.php                 <-- Remove incident record
│   │   ├── device.php                  <-- Decommission hardware device
│   │   ├── helps.php                   <-- Remove rescue responder
│   │   └── emails.php                  <-- Unsubscribe email contact
│   ├── email_helper.php                <-- Formatted HTML emergency email dispatch engine
│   └── fcm_helper.php                  <-- Google Firebase HTTP v1 push notification engine
├── ota/
│   ├── version.json                    <-- Firmware version manifest for remote ESP32 OTA
│   └── firmware_v3.2.0.bin             <-- Compiled binary firmware image
└── portal/
    ├── index.php                       <-- Main web dashboard application shell
    ├── dashboard.php                   <-- Live incident overview, KPIs, Google Maps
    ├── messages.php                    <-- Historical message audit trail & filters
    ├── devices.php                     <-- Fleet hardware management
    └── helps.php                       <-- Rescue resource matching & dispatch
```

Set file permissions so Nginx can access all files:
```bash
sudo chown -R www-data:www-data /var/www/html/lifeline
sudo chmod -R 755 /var/www/html/lifeline
```

---

## 🗄️ 5. Complete Master Database Setup (`lifeline_complete.sql`)

Save this file as `lifeline_complete.sql` and import it into MariaDB:

```bash
mysql -u lifeline_user -p lifeline < lifeline_complete.sql
```

```sql
-- ═══════════════════════════════════════════════════════════════════════════════════
--                 LIFELINE EMERGENCY SYSTEM — MASTER DATABASE SCHEMA
-- ═══════════════════════════════════════════════════════════════════════════════════

SET FOREIGN_KEY_CHECKS = 0;
DROP TABLE IF EXISTS `downlink_commands`;
DROP TABLE IF EXISTS `messages`;
DROP TABLE IF EXISTS `devices`;
DROP TABLE IF EXISTS `helps`;
DROP TABLE IF EXISTS `indexes`;
DROP TABLE IF EXISTS `emails`;
DROP TABLE IF EXISTS `user`;
SET FOREIGN_KEY_CHECKS = 1;

-- 1. USER TABLE: Administrative and Operator authentication
CREATE TABLE `user` (
    `UID` INT AUTO_INCREMENT PRIMARY KEY,
    `name` VARCHAR(100) NOT NULL,
    `email` VARCHAR(150) NOT NULL UNIQUE,
    `password` VARCHAR(255) NOT NULL,
    `role` ENUM('admin', 'operator') NOT NULL DEFAULT 'operator',
    `last_login` DATETIME DEFAULT NULL,
    `created_at` DATETIME DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 2. DEVICES TABLE: Registered Field Transmitters & Gateways
CREATE TABLE `devices` (
    `DID` INT PRIMARY KEY,
    `device_name` VARCHAR(100) NOT NULL,
    `LID` INT NOT NULL DEFAULT 1 COMMENT 'Location ID referencing indexes.mapping',
    `status` ENUM('active', 'inactive', 'maintenance') NOT NULL DEFAULT 'active',
    `last_ping` DATETIME DEFAULT NULL COMMENT 'Updated on every received RF packet',
    `battery_pct` INT DEFAULT 100,
    `created_at` DATETIME DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 3. MESSAGES TABLE: Master Emergency Incidents & SITREP Communications Log
CREATE TABLE `messages` (
    `MID` INT AUTO_INCREMENT PRIMARY KEY,
    `DID` INT NOT NULL,
    `message_code` INT NOT NULL DEFAULT 0,
    `code` VARCHAR(2) DEFAULT 'A',
    `RSSI` INT NOT NULL DEFAULT -80 COMMENT 'Signal strength in dBm',
    `distance_km` DECIMAL(6,2) DEFAULT NULL COMMENT 'Estimated real-world distance in km',
    `snr` DECIMAL(4,1) DEFAULT NULL COMMENT 'Signal-to-Noise Ratio in dB',
    `is_chat` TINYINT(1) NOT NULL DEFAULT 0 COMMENT '1 = Freeform custom text report; 0 = Predefined SOS alert',
    `custom_msg` TEXT DEFAULT NULL COMMENT 'Verbatim SITREP or chat text',
    `source` ENUM('LORA', 'BLE', 'WEB') NOT NULL DEFAULT 'LORA',
    `status` ENUM('active', 'resolved') NOT NULL DEFAULT 'active',
    `timestamp` DATETIME DEFAULT CURRENT_TIMESTAMP,
    INDEX `idx_msg_did` (`DID`),
    INDEX `idx_msg_status` (`status`),
    INDEX `idx_msg_is_chat` (`is_chat`),
    INDEX `idx_msg_time` (`timestamp`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 4. DOWNLINK_COMMANDS TABLE: Two-Way Closed-Loop Web-to-LoRa Dispatch Queue
CREATE TABLE `downlink_commands` (
    `id` INT AUTO_INCREMENT PRIMARY KEY,
    `rx_id` INT NOT NULL DEFAULT 1 COMMENT 'Target Base Station Gateway ID',
    `target_did` INT NOT NULL DEFAULT 0 COMMENT '0 = Broadcast to All TX units; 1-999 = Specific TX unit',
    `action` VARCHAR(32) NOT NULL DEFAULT 'MSG' COMMENT 'MSG, DISPATCH, EVAC, MEDIC, PING',
    `message` VARCHAR(96) NOT NULL COMMENT 'Command text (max 48-96 characters for LoRa)',
    `status` ENUM('PENDING', 'DISPATCHED_LORA', 'TX_FAILED', 'ACK_CONFIRMED') NOT NULL DEFAULT 'PENDING',
    `lora_tx_ok` TINYINT(1) DEFAULT 0,
    `created_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    `dispatched_at` TIMESTAMP NULL DEFAULT NULL,
    `acknowledged_at` TIMESTAMP NULL DEFAULT NULL,
    INDEX `idx_rx_pending` (`rx_id`, `status`),
    INDEX `idx_target_did` (`target_did`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 5. HELPS TABLE: Search and Rescue Responders Registry
CREATE TABLE `helps` (
    `HID` INT AUTO_INCREMENT PRIMARY KEY,
    `name` VARCHAR(150) NOT NULL,
    `contact` VARCHAR(50) NOT NULL,
    `location` VARCHAR(200) NOT NULL,
    `eta` VARCHAR(50) NOT NULL DEFAULT '30-60 mins',
    `status` ENUM('available', 'dispatched', 'busy') NOT NULL DEFAULT 'available',
    `for_messages` JSON DEFAULT NULL COMMENT 'Array of message_codes this unit can service, e.g. [0, 1, 2, 5]',
    `created_at` DATETIME DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 6. INDEXES TABLE: Dynamic JSON Taxonomy & Translation Dictionary
CREATE TABLE `indexes` (
    `IID` INT AUTO_INCREMENT PRIMARY KEY,
    `type` ENUM('location', 'message', 'help') NOT NULL UNIQUE,
    `mapping` JSON NOT NULL COMMENT 'JSON key-value mapping of numeric IDs to human descriptions',
    `description` VARCHAR(255) DEFAULT NULL,
    `updated_at` DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 7. EMAILS TABLE: Emergency Alert Email Subscribers
CREATE TABLE `emails` (
    `SN` INT AUTO_INCREMENT PRIMARY KEY,
    `email` VARCHAR(255) NOT NULL UNIQUE,
    `name` VARCHAR(100) DEFAULT 'Emergency Coordinator',
    `created_at` DATETIME DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ═══════════════════════════════════════════════════════════════════════════════════
--                             DEFAULT SEED DATA
-- ═══════════════════════════════════════════════════════════════════════════════════

-- Default Administrator (Email: admin@lifeline.org | Password: adminpassword123)
-- Hash generated using password_hash('adminpassword123', PASSWORD_BCRYPT)
INSERT INTO `user` (`UID`, `name`, `email`, `password`, `role`) VALUES 
(1, 'Central Command Admin', 'admin@lifeline.org', '$2y$10$e7QJ64u3hVvJms0uWzGf2e3aH9o/1.z0X3Z5.wYh5aB8eG1jG5n6q', 'admin');

-- Seed Field Hardware Units
INSERT INTO `devices` (`DID`, `device_name`, `LID`, `status`) VALUES
(0, 'LifeLine RX Pro Gateway #01', 1, 'active'),
(1, 'Rescue Team Alpha (TX #001)', 1, 'active'),
(2, 'Medical Team Bravo (TX #002)', 2, 'active'),
(3, 'Evac Squad Charlie (TX #003)', 3, 'active');

-- Dynamic Mapping 1: Locations in Nepal (Khumbu / Himalayan Sector)
INSERT INTO `indexes` (`type`, `mapping`, `description`) VALUES
('location', JSON_OBJECT(
    '1', 'Namche Bazaar (3,440m)',
    '2', 'Lukla Airport Sector (2,860m)',
    '3', 'Tengboche Monastery (3,867m)',
    '4', 'Dingboche Valley (4,410m)',
    '5', 'Lobuche High Camp (4,940m)',
    '6', 'Gorakshep / EBC Route (5,164m)'
), 'Regional geographic coordinates & village settlements');

-- Dynamic Mapping 2: Emergency Alert Categories (0 to 14 matching firmware)
INSERT INTO `indexes` (`type`, `mapping`, `description`) VALUES
('message', JSON_OBJECT(
    '0', 'CRITICAL SOS (Immediate Evacuation)',
    '1', 'DELIVERY / LABOR (Maternal Childbirth Emergency)',
    '2', 'HELI RESCUE NEEDED (Medical Air Evacuation)',
    '3', 'MEDICINE SHORTAGE (Essential Drugs Needed)',
    '4', 'OXYGEN SHORTAGE (Cylinders Depleted)',
    '5', 'SEVERE INJURY (Trauma / Fracture / Bleeding)',
    '6', 'BLOOD NEEDED (Urgent Transfusion)',
    '7', 'ALTITUDE SICKNESS (Severe AMS / HAPE / HACE)',
    '8', 'FOOD SHORTAGE (Rations Depleted)',
    '9', 'WATER SHORTAGE (Drinking Water Crisis)',
    '10', 'DISEASE OUTBREAK (Epidemic Cluster)',
    '11', 'FREEZING / SHELTER (Extreme Cold Blankets)',
    '12', 'LANDSLIDE / HAZARD (Trail Collapsed)',
    '13', 'DOCTOR / NURSE NEED (Personnel Required)',
    '14', 'STATUS OK / ALL SAFE (Routine Check-in)'
), 'Official 15-category LifeLine emergency taxonomy');

-- Dynamic Mapping 3: Emergency Responder Types
INSERT INTO `indexes` (`type`, `mapping`, `description`) VALUES
('help', JSON_OBJECT(
    '1', 'High-Altitude Ground SAR Team',
    '2', 'Nepal Army Air Evacuation Helicopter',
    '3', 'Maternal Childbirth Mobile Clinic',
    '4', 'Himalayan Rescue Association (HRA) Paramedics',
    '5', 'Disaster Relief Rations & Shelter Squad'
), 'Emergency capability classifications');

-- Seed Rescue Squads
INSERT INTO `helps` (`name`, `contact`, `location`, `eta`, `status`, `for_messages`) VALUES
('Nepal Army Heli Rescue Unit', '+977-1-4267055', 'Kathmandu / Lukla Heliport', '30-45 mins', 'available', '[0, 2, 5, 7]'),
('HRA Medical Clinic Everest', '+977-38-540022', 'Pheriche Aid Post', '45 mins', 'available', '[0, 1, 3, 4, 5, 6, 7, 13]'),
('Namche Community Mountain SAR', '+977-9841000000', 'Namche Bazaar Ward 3', '15-20 mins', 'available', '[0, 5, 8, 9, 11, 12, 14]');

-- Seed Initial Email Subscriber
INSERT INTO `emails` (`email`, `name`) VALUES 
('rescue-desk@lifeline.org', 'Central SAR Dispatch Desk');
```

---

## 🔌 6. Core Database Connection & Settings (`database.php`)

Save this file as `/var/www/html/lifeline/database.php`. It provides a high-performance **PDO Singleton Connection Pool**:

```php
<?php
/**
 * LifeLine Emergency Response System — Database Singleton Connection
 * File: /var/www/html/lifeline/database.php
 */

class Database {
    private static ?Database $instance = null;
    private ?PDO $conn = null;

    // Database Credentials
    private string $host = '127.0.0.1';
    private string $db_name = 'lifeline';
    private string $username = 'lifeline_user';
    private string $password = 'LifeLineSecurePass2026!';
    private int $port = 3306;

    private function __construct() {
        $dsn = "mysql:host={$this->host};port={$this->port};dbname={$this->db_name};charset=utf8mb4";
        $options = [
            PDO::ATTR_ERRMODE            => PDO::ERRMODE_EXCEPTION,
            PDO::ATTR_DEFAULT_FETCH_MODE => PDO::FETCH_ASSOC,
            PDO::ATTR_EMULATE_PREPARES   => false,
            PDO::ATTR_PERSISTENT         => true // Connection pooling
        ];

        try {
            $this->conn = new PDO($dsn, $this->username, $this->password, $options);
        } catch (PDOException $e) {
            http_response_code(500);
            header('Content-Type: application/json');
            echo json_encode([
                'status'  => 'error',
                'message' => 'Database connection failed: ' . $e->getMessage()
            ]);
            exit();
        }
    }

    public static function getInstance(): Database {
        if (self::$instance === null) {
            self::$instance = new self();
        }
        return self::$instance;
    }

    public function getConnection(): PDO {
        return $this->conn;
    }

    // Prevent cloning and un-serialization
    private function __clone() {}
    public function __wakeup() {
        throw new Exception("Cannot unserialize singleton");
    }
}
```

Also create a simple alias `/var/www/html/lifeline/API/Config/database.php`:
```php
<?php
// /var/www/html/lifeline/API/Config/database.php
require_once __DIR__ . '/../../database.php';
```

---

## 📥 7. Telemetry & Emergency Ingestion API (`API/Create/message.php`)

Save this file as `/var/www/html/lifeline/API/Create/message.php`. This is the **primary ingestion engine** hit by the ESP32 Base Station:

```php
<?php
/**
 * LifeLine Emergency Telemetry & SITREP Ingestion Endpoint
 * File: /var/www/html/lifeline/API/Create/message.php
 * Handles: SOS alerts, Freeform SITREP Chats, Signal Metrics (RSSI, SNR, Distance), and Sensor Data.
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

require_once __DIR__ . '/../../database.php';
$db = Database::getInstance()->getConnection();

// Read incoming JSON body
$rawBody = file_get_contents('php://input');
$payload = json_decode($rawBody, true);

if (!$payload) {
    http_response_code(400);
    echo json_encode(['status' => 'error', 'message' => 'Invalid JSON body provided.']);
    exit;
}

// Extract parameters
$did          = isset($payload['DID']) ? intval($payload['DID']) : null;
$messageCode  = isset($payload['message_code']) ? intval($payload['message_code']) : 0;
$code         = isset($payload['code']) ? substr(trim($payload['code']), 0, 2) : 'A';
$rssi         = isset($payload['RSSI']) ? intval($payload['RSSI']) : -80;
$snr          = isset($payload['snr']) ? floatval($payload['snr']) : 0.0;
$distanceKm   = isset($payload['distance_km']) ? floatval($payload['distance_km']) : null;
$isChat       = !empty($payload['is_chat']) ? 1 : 0;
$customMsg    = isset($payload['custom_msg']) && trim($payload['custom_msg']) !== '' ? trim($payload['custom_msg']) : null;
$source       = isset($payload['source']) && in_array(strtoupper($payload['source']), ['LORA', 'BLE', 'WEB']) ? strtoupper($payload['source']) : 'LORA';

if ($did === null) {
    http_response_code(422);
    echo json_encode(['status' => 'error', 'message' => 'DID (Device ID) is mandatory.']);
    exit;
}

try {
    // 1. Ensure Device exists in `devices` table or auto-register it
    $devStmt = $db->prepare("SELECT DID FROM devices WHERE DID = :did");
    $devStmt->execute([':did' => $did]);
    if ($devStmt->rowCount() === 0) {
        $devName = ($did === 0) ? "LifeLine RX Pro Gateway" : "Disaster Field Node #{$did}";
        $insDev = $db->prepare("INSERT INTO devices (DID, device_name, LID, status, last_ping) VALUES (:did, :name, 1, 'active', NOW())");
        $insDev->execute([':did' => $did, ':name' => $devName]);
    } else {
        // Update device heartbeat
        $updDev = $db->prepare("UPDATE devices SET last_ping = NOW(), status = 'active' WHERE DID = :did");
        $updDev->execute([':did' => $did]);
    }

    // 2. Insert into `messages` table
    $stmt = $db->prepare("
        INSERT INTO `messages` 
            (`DID`, `message_code`, `code`, `RSSI`, `distance_km`, `snr`, `is_chat`, `custom_msg`, `source`, `status`, `timestamp`)
        VALUES 
            (:did, :mcode, :code, :rssi, :dist, :snr, :is_chat, :custom_msg, :source, 'active', NOW())
    ");

    $stmt->execute([
        ':did'        => $did,
        ':mcode'      => $messageCode,
        ':code'       => $code,
        ':rssi'       => $rssi,
        ':dist'       => $distanceKm,
        ':snr'        => $snr,
        ':is_chat'    => $isChat,
        ':custom_msg' => $customMsg,
        ':source'     => $source
    ]);

    $messageId = $db->lastInsertId();

    // 3. Resolve human labels from taxonomy index
    $idxStmt = $db->prepare("SELECT mapping FROM indexes WHERE type = 'message' LIMIT 1");
    $idxStmt->execute();
    $messageMapping = json_decode($idxStmt->fetchColumn() ?: '{}', true);
    $resolvedCategory = $messageMapping[(string)$messageCode] ?? "Emergency Alert #{$messageCode}";

    // 4. Trigger Notifications if Critical (Optional Helper Hooks)
    $emailSuccess = false;
    $pushSuccess = false;
    if (file_exists(__DIR__ . '/../email_helper.php')) {
        require_once __DIR__ . '/../email_helper.php';
        $emailSuccess = sendEmergencyBroadcastEmail($messageId, $did, $resolvedCategory, $customMsg, $distanceKm, $rssi);
    }

    http_response_code(201);
    echo json_encode([
        'status'        => 'success',
        'message_id'    => (int)$messageId,
        'DID'           => $did,
        'category'      => $resolvedCategory,
        'is_chat'       => (bool)$isChat,
        'custom_msg'    => $customMsg,
        'distance_km'   => $distanceKm,
        'snr'           => $snr,
        'source'        => $source,
        'notifications' => [
            'email_sent' => $emailSuccess,
            'push_sent'  => $pushSuccess
        ]
    ]);

} catch (Exception $e) {
    http_response_code(500);
    echo json_encode(['status' => 'error', 'message' => 'Ingestion failed: ' . $e->getMessage()]);
}
```

---

## 🔁 8. Two-Way Closed-Loop Downlink Messaging (`Website ➔ RX ➔ TX`)

Enables web operators to type orders and SITREPs that travel across the internet to the Base Station Gateway, down to the off-grid handheld over 433 MHz LoRa, and display as a popup dialog on the field unit's screen.

### 8.1 Queue Outgoing Command (`API/Create/command.php`)
Save as `/var/www/html/lifeline/API/Create/command.php`:

```php
<?php
/**
 * LifeLine Web-to-LoRa Command Queueing Endpoint
 * File: /var/www/html/lifeline/API/Create/command.php
 */

header("Access-Control-Allow-Origin: *");
header("Access-Control-Allow-Methods: POST, OPTIONS");
header("Access-Control-Allow-Headers: Content-Type, Authorization, X-API-Key");
header("Content-Type: application/json; charset=UTF-8");

if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') { http_response_code(200); exit(); }
if ($_SERVER['REQUEST_METHOD'] !== 'POST') { http_response_code(405); exit(); }

require_once __DIR__ . '/../../database.php';
$db = Database::getInstance()->getConnection();

$data = json_decode(file_get_contents("php://input"));

if (!$data || empty($data->message)) {
    http_response_code(400);
    echo json_encode(["status" => "error", "message" => "Message content is required"]);
    exit();
}

$rx_id      = isset($data->rx_id) ? (int)$data->rx_id : 1;
$target_did = isset($data->target_did) ? (int)$data->target_did : 0;
$action     = isset($data->action) ? strtoupper(trim($data->action)) : 'MSG';
$message    = substr(trim($data->message), 0, 96);

// Clean string: replace commas and newlines to preserve LoRa CSV framing
$message = str_replace([",", "\n", "\r"], [" ", " ", ""], $message);

$query = "INSERT INTO downlink_commands (rx_id, target_did, action, message, status) 
          VALUES (:rx_id, :target_did, :action, :message, 'PENDING')";
$stmt = $db->prepare($query);
$stmt->execute([
    ':rx_id'      => $rx_id,
    ':target_did' => $target_did,
    ':action'     => $action,
    ':message'    => $message
]);

$cmd_id = $db->lastInsertId();
http_response_code(201);
echo json_encode([
    "status"      => "success",
    "message"     => "Command queued for gateway LoRa dispatch",
    "command_id"  => (int)$cmd_id,
    "target_did"  => $target_did,
    "action"      => $action,
    "text"        => $message
]);
```

### 8.2 Gateway Poll Pending Commands (`API/Read/pending_commands.php`)
Save as `/var/www/html/lifeline/API/Read/pending_commands.php`. Polled by the ESP32 Base Station every 3.5s:

```php
<?php
/**
 * LifeLine Gateway Downlink Command Poller
 * File: /var/www/html/lifeline/API/Read/pending_commands.php
 */

header("Access-Control-Allow-Origin: *");
header("Content-Type: application/json; charset=UTF-8");

require_once __DIR__ . '/../../database.php';
$db = Database::getInstance()->getConnection();

$rx_id = isset($_GET['rx_id']) ? (int)$_GET['rx_id'] : 1;

// Fetch oldest pending command for this gateway
$stmt = $db->prepare("
    SELECT id, rx_id, target_did, action, message, created_at 
    FROM downlink_commands 
    WHERE rx_id = :rx_id AND status = 'PENDING' 
    ORDER BY id ASC LIMIT 1
");
$stmt->execute([':rx_id' => $rx_id]);

if ($stmt->rowCount() > 0) {
    $row = $stmt->fetch();
    echo json_encode([
        "status"      => "success",
        "has_command" => true,
        "command_id"  => (int)$row['id'],
        "target_did"  => (int)$row['target_did'],
        "action"      => $row['action'],
        "message"     => $row['message'],
        "created_at"  => $row['created_at']
    ]);
} else {
    echo json_encode([
        "status"      => "success",
        "has_command" => false
    ]);
}
```

### 8.3 Gateway Downlink Status Ack (`API/Update/command_status.php`)
Save as `/var/www/html/lifeline/API/Update/command_status.php`:

```php
<?php
/**
 * LifeLine Gateway Downlink Acknowledgment Receiver
 * File: /var/www/html/lifeline/API/Update/command_status.php
 */

header("Access-Control-Allow-Origin: *");
header("Access-Control-Allow-Methods: POST, OPTIONS");
header("Access-Control-Allow-Headers: Content-Type, Authorization, X-API-Key");
header("Content-Type: application/json; charset=UTF-8");

if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') { http_response_code(200); exit(); }

require_once __DIR__ . '/../../database.php';
$db = Database::getInstance()->getConnection();

$data = json_decode(file_get_contents("php://input"));

if (!$data || !isset($data->command_id)) {
    http_response_code(400);
    echo json_encode(["status" => "error", "message" => "command_id is mandatory"]);
    exit();
}

$command_id = (int)$data->command_id;
$status     = isset($data->status) ? trim($data->status) : 'DISPATCHED_LORA';
$lora_tx_ok = (!empty($data->lora_tx_ok)) ? 1 : 0;

$stmt = $db->prepare("
    UPDATE downlink_commands 
    SET status = :status, 
        lora_tx_ok = :lora_tx_ok, 
        dispatched_at = CURRENT_TIMESTAMP 
    WHERE id = :command_id
");
$stmt->execute([
    ':status'      => $status,
    ':lora_tx_ok'  => $lora_tx_ok,
    ':command_id'  => $command_id
]);

echo json_encode([
    "status"     => "success",
    "command_id" => $command_id,
    "new_status" => $status
]);
```

---

## 📟 9. Fleet & Device Management APIs (`/API/devices/`)

### 9.1 List Fleet & Heartbeats (`API/Read/device.php`)
Save as `/var/www/html/lifeline/API/Read/device.php`:

```php
<?php
header('Content-Type: application/json; charset=utf-8');
header('Access-Control-Allow-Origin: *');

require_once __DIR__ . '/../../database.php';
$db = Database::getInstance()->getConnection();

// Fetch devices joined with dynamic location mapping
$stmt = $db->query("SELECT * FROM devices ORDER BY DID ASC");
$devices = $stmt->fetchAll();

// Resolve locations
$idxStmt = $db->query("SELECT mapping FROM indexes WHERE type = 'location' LIMIT 1");
$locMap = json_decode($idxStmt->fetchColumn() ?: '{}', true);

foreach ($devices as &$dev) {
    $dev['location_name'] = $locMap[(string)$dev['LID']] ?? 'Unknown Sector';
    // Mark as inactive if last ping was more than 10 minutes ago
    if (!empty($dev['last_ping']) && strtotime($dev['last_ping']) < (time() - 600)) {
        if ($dev['status'] === 'active') $dev['status'] = 'inactive';
    }
}

echo json_encode(['status' => 'success', 'devices' => $devices]);
```

### 9.2 Register New Field Node (`API/Create/device.php`)
Save as `/var/www/html/lifeline/API/Create/device.php`:

```php
<?php
header('Content-Type: application/json; charset=utf-8');
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: POST');

require_once __DIR__ . '/../../database.php';
$db = Database::getInstance()->getConnection();

$data = json_decode(file_get_contents("php://input"), true);
if (empty($data['DID']) || empty($data['device_name'])) {
    http_response_code(400);
    echo json_encode(['status' => 'error', 'message' => 'DID and device_name are required']);
    exit;
}

$stmt = $db->prepare("INSERT INTO devices (DID, device_name, LID, status) VALUES (:did, :name, :lid, 'active')");
$stmt->execute([
    ':did'  => (int)$data['DID'],
    ':name' => trim($data['device_name']),
    ':lid'  => (int)($data['LID'] ?? 1)
]);

echo json_encode(['status' => 'success', 'message' => 'Device registered successfully']);
```

### 9.3 Update Node Attributes (`API/Update/device.php`)
Save as `/var/www/html/lifeline/API/Update/device.php`:

```php
<?php
header('Content-Type: application/json; charset=utf-8');
header('Access-Control-Allow-Origin: *');

require_once __DIR__ . '/../../database.php';
$db = Database::getInstance()->getConnection();

$data = json_decode(file_get_contents("php://input"), true);
if (empty($data['DID'])) {
    http_response_code(400);
    echo json_encode(['status' => 'error', 'message' => 'DID is required']);
    exit;
}

$stmt = $db->prepare("UPDATE devices SET device_name = :name, LID = :lid, status = :status WHERE DID = :did");
$stmt->execute([
    ':did'    => (int)$data['DID'],
    ':name'   => trim($data['device_name']),
    ':lid'    => (int)$data['LID'],
    ':status' => trim($data['status'])
]);

echo json_encode(['status' => 'success', 'message' => 'Device updated']);
```

### 9.4 Delete Decommissioned Node (`API/Delete/device.php`)
Save as `/var/www/html/lifeline/API/Delete/device.php`:

```php
<?php
header('Content-Type: application/json; charset=utf-8');
header('Access-Control-Allow-Origin: *');

require_once __DIR__ . '/../../database.php';
$db = Database::getInstance()->getConnection();

$did = $_GET['DID'] ?? json_decode(file_get_contents("php://input"), true)['DID'] ?? null;
if (!$did) {
    http_response_code(400);
    echo json_encode(['status' => 'error', 'message' => 'DID is required']);
    exit;
}

$stmt = $db->prepare("DELETE FROM devices WHERE DID = :did");
$stmt->execute([':did' => (int)$did]);

echo json_encode(['status' => 'success', 'message' => 'Device deleted']);
```

---

## 📊 10. Dashboard Incident Query & Resolution APIs (`/API/messages/`)

### 10.1 Live Incident Query with Filters & Pagination (`API/Read/message.php`)
Save as `/var/www/html/lifeline/API/Read/message.php`:

```php
<?php
header('Content-Type: application/json; charset=utf-8');
header('Access-Control-Allow-Origin: *');

require_once __DIR__ . '/../../database.php';
$db = Database::getInstance()->getConnection();

$page   = max(1, intval($_GET['page'] ?? 1));
$limit  = min(100, max(1, intval($_GET['limit'] ?? 20)));
$offset = ($page - 1) * $limit;

$did    = isset($_GET['did']) ? intval($_GET['did']) : null;
$status = isset($_GET['status']) ? trim($_GET['status']) : null;

$where = [];
$params = [];

if ($did !== null) { $where[] = "m.DID = :did"; $params[':did'] = $did; }
if ($status !== null && in_array($status, ['active', 'resolved'])) {
    $where[] = "m.status = :status"; 
    $params[':status'] = $status; 
}

$whereSql = count($where) > 0 ? "WHERE " . implode(" AND ", $where) : "";

// Count total
$countStmt = $db->prepare("SELECT COUNT(*) FROM messages m {$whereSql}");
$countStmt->execute($params);
$total = (int)$countStmt->fetchColumn();

// Fetch records joined with device info
$stmt = $db->prepare("
    SELECT m.*, d.device_name, d.LID 
    FROM messages m
    LEFT JOIN devices d ON m.DID = d.DID
    {$whereSql}
    ORDER BY m.MID DESC
    LIMIT :limit OFFSET :offset
");
foreach ($params as $k => $v) $stmt->bindValue($k, $v);
$stmt->bindValue(':limit', $limit, PDO::PARAM_INT);
$stmt->bindValue(':offset', $offset, PDO::PARAM_INT);
$stmt->execute();
$messages = $stmt->fetchAll();

// Resolve mappings
$idxStmt = $db->query("SELECT type, mapping FROM indexes");
$maps = [];
while ($row = $idxStmt->fetch()) {
    $maps[$row['type']] = json_decode($row['mapping'], true);
}

foreach ($messages as &$msg) {
    $msg['category_name'] = $maps['message'][(string)$msg['message_code']] ?? "Emergency #{$msg['message_code']}";
    $msg['location_name'] = $maps['location'][(string)($msg['LID'] ?? 1)] ?? "Himalayan Sector";
}

echo json_encode([
    'status'       => 'success',
    'total'        => $total,
    'current_page' => $page,
    'total_pages'  => ceil($total / $limit),
    'messages'     => $messages
]);
```

### 10.2 Mark Incident as Resolved (`API/Update/message.php`)
Save as `/var/www/html/lifeline/API/Update/message.php`:

```php
<?php
header('Content-Type: application/json; charset=utf-8');
header('Access-Control-Allow-Origin: *');

require_once __DIR__ . '/../../database.php';
$db = Database::getInstance()->getConnection();

$data = json_decode(file_get_contents("php://input"), true);
$mid = $data['MID'] ?? $_GET['MID'] ?? null;
$status = $data['status'] ?? 'resolved';

if (!$mid) {
    http_response_code(400);
    echo json_encode(['status' => 'error', 'message' => 'MID is required']);
    exit;
}

$stmt = $db->prepare("UPDATE messages SET status = :status WHERE MID = :mid");
$stmt->execute([':status' => $status, ':mid' => (int)$mid]);

echo json_encode(['status' => 'success', 'message' => "Incident #{$mid} marked as {$status}"]);
```

### 10.3 Delete Message Record (`API/Delete/message.php`)
Save as `/var/www/html/lifeline/API/Delete/message.php`:

```php
<?php
header('Content-Type: application/json; charset=utf-8');
header('Access-Control-Allow-Origin: *');

require_once __DIR__ . '/../../database.php';
$db = Database::getInstance()->getConnection();

$mid = $_GET['MID'] ?? json_decode(file_get_contents("php://input"), true)['MID'] ?? null;
if (!$mid) {
    http_response_code(400);
    echo json_encode(['status' => 'error', 'message' => 'MID is required']);
    exit;
}

$stmt = $db->prepare("DELETE FROM messages WHERE MID = :mid");
$stmt->execute([':mid' => (int)$mid]);

echo json_encode(['status' => 'success', 'message' => 'Message deleted']);
```

---

## 🚑 11. Rescue Teams & Taxonomy Index APIs

### 11.1 Responders Query & Management (`API/Read/helps.php`, `API/Create/helps.php`)
Save as `/var/www/html/lifeline/API/Read/helps.php`:

```php
<?php
header('Content-Type: application/json; charset=utf-8');
header('Access-Control-Allow-Origin: *');

require_once __DIR__ . '/../../database.php';
$db = Database::getInstance()->getConnection();

$msgCode = isset($_GET['message_code']) ? intval($_GET['message_code']) : null;

$stmt = $db->query("SELECT * FROM helps ORDER BY status ASC, HID ASC");
$all = $stmt->fetchAll();

if ($msgCode !== null) {
    $filtered = [];
    foreach ($all as $h) {
        $allowed = json_decode($h['for_messages'] ?: '[]', true);
        if (in_array($msgCode, $allowed)) {
            $filtered[] = $h;
        }
    }
    echo json_encode(['status' => 'success', 'responders' => $filtered]);
} else {
    echo json_encode(['status' => 'success', 'responders' => $all]);
}
```

Save as `/var/www/html/lifeline/API/Create/helps.php`:

```php
<?php
header('Content-Type: application/json; charset=utf-8');
header('Access-Control-Allow-Origin: *');

require_once __DIR__ . '/../../database.php';
$db = Database::getInstance()->getConnection();

$data = json_decode(file_get_contents("php://input"), true);
if (empty($data['name']) || empty($data['contact'])) {
    http_response_code(400);
    echo json_encode(['status' => 'error', 'message' => 'Name and contact are required']);
    exit;
}

$stmt = $db->prepare("
    INSERT INTO helps (name, contact, location, eta, status, for_messages)
    VALUES (:name, :contact, :location, :eta, 'available', :for_messages)
");
$stmt->execute([
    ':name'         => trim($data['name']),
    ':contact'      => trim($data['contact']),
    ':location'     => trim($data['location'] ?? 'Namche Sector'),
    ':eta'          => trim($data['eta'] ?? '30 mins'),
    ':for_messages' => json_encode($data['for_messages'] ?? [0, 1, 2, 5])
]);

echo json_encode(['status' => 'success', 'message' => 'Responder registered']);
```

### 11.2 Dynamic JSON Taxonomy Dictionaries (`API/Read/index.php`, `API/Create/index.php`)
Save as `/var/www/html/lifeline/API/Read/index.php`:

```php
<?php
header('Content-Type: application/json; charset=utf-8');
header('Access-Control-Allow-Origin: *');

require_once __DIR__ . '/../../database.php';
$db = Database::getInstance()->getConnection();

$type = $_GET['type'] ?? null;
if ($type) {
    $stmt = $db->prepare("SELECT * FROM indexes WHERE type = :type LIMIT 1");
    $stmt->execute([':type' => $type]);
} else {
    $stmt = $db->query("SELECT * FROM indexes");
}

$results = $stmt->fetchAll();
foreach ($results as &$r) {
    $r['mapping'] = json_decode($r['mapping'], true);
}

echo json_encode(['status' => 'success', 'indexes' => $results]);
```

### 11.3 Alert Email Subscribers (`API/Read/emails.php`, `API/Create/emails.php`)
Save as `/var/www/html/lifeline/API/Read/emails.php`:

```php
<?php
header('Content-Type: application/json; charset=utf-8');
header('Access-Control-Allow-Origin: *');

require_once __DIR__ . '/../../database.php';
$db = Database::getInstance()->getConnection();

$stmt = $db->query("SELECT * FROM emails ORDER BY SN ASC");
echo json_encode(['status' => 'success', 'subscribers' => $stmt->fetchAll()]);
```

Save as `/var/www/html/lifeline/API/Create/emails.php`:

```php
<?php
header('Content-Type: application/json; charset=utf-8');
header('Access-Control-Allow-Origin: *');

require_once __DIR__ . '/../../database.php';
$db = Database::getInstance()->getConnection();

$data = json_decode(file_get_contents("php://input"), true);
if (empty($data['email']) || !filter_var($data['email'], FILTER_VALIDATE_EMAIL)) {
    http_response_code(400);
    echo json_encode(['status' => 'error', 'message' => 'Valid email is required']);
    exit;
}

$stmt = $db->prepare("INSERT INTO emails (email, name) VALUES (:email, :name)");
$stmt->execute([
    ':email' => trim($data['email']),
    ':name'  => trim($data['name'] ?? 'Coordinator')
]);

echo json_encode(['status' => 'success', 'message' => 'Email subscribed for alerts']);
```

---

## 🔒 12. Authentication & Operator Access APIs (`/API/auth/`)

### 12.1 Operator Login (`API/auth/login.php`)
Save as `/var/www/html/lifeline/API/auth/login.php`:

```php
<?php
header('Content-Type: application/json; charset=utf-8');
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: POST');

require_once __DIR__ . '/../../database.php';
$db = Database::getInstance()->getConnection();

$data = json_decode(file_get_contents("php://input"), true);
$email = trim($data['email'] ?? '');
$pass  = $data['password'] ?? '';

if (!$email || !$pass) {
    http_response_code(400);
    echo json_encode(['status' => 'error', 'message' => 'Email and password required']);
    exit;
}

$stmt = $db->prepare("SELECT * FROM user WHERE email = :email LIMIT 1");
$stmt->execute([':email' => $email]);
$user = $stmt->fetch();

if ($user && password_verify($pass, $user['password'])) {
    session_start();
    $_SESSION['user_id'] = $user['UID'];
    $_SESSION['user_name'] = $user['name'];
    $_SESSION['user_role'] = $user['role'];

    // Update last login
    $upd = $db->prepare("UPDATE user SET last_login = NOW() WHERE UID = :uid");
    $upd->execute([':uid' => $user['UID']]);

    echo json_encode([
        'status' => 'success',
        'user'   => [
            'id'    => (int)$user['UID'],
            'name'  => $user['name'],
            'email' => $user['email'],
            'role'  => $user['role']
        ]
    ]);
} else {
    http_response_code(401);
    echo json_encode(['status' => 'error', 'message' => 'Invalid email or password']);
}
```

### 12.2 Session Verification Heartbeat (`API/auth/check.php`)
Save as `/var/www/html/lifeline/API/auth/check.php`:

```php
<?php
header('Content-Type: application/json; charset=utf-8');
header('Access-Control-Allow-Origin: *');

session_start();
if (!empty($_SESSION['user_id'])) {
    echo json_encode([
        'authenticated' => true,
        'user' => [
            'id'   => $_SESSION['user_id'],
            'name' => $_SESSION['user_name'],
            'role' => $_SESSION['user_role']
        ]
    ]);
} else {
    echo json_encode(['authenticated' => false]);
}
```

### 12.3 Logout & Session Cleanup (`API/auth/logout.php`)
Save as `/var/www/html/lifeline/API/auth/logout.php`:

```php
<?php
header('Content-Type: application/json; charset=utf-8');
session_start();
$_SESSION = [];
session_destroy();
echo json_encode(['status' => 'success', 'message' => 'Logged out successfully']);
```

---

## 📢 13. Automated Dispatch Helpers (Email & Web Push)

### 13.1 Emergency HTML Email Dispatcher (`API/email_helper.php`)
Save as `/var/www/html/lifeline/API/email_helper.php`:

```php
<?php
/**
 * LifeLine Emergency Automated Email Broadcaster
 * File: /var/www/html/lifeline/API/email_helper.php
 */

function sendEmergencyBroadcastEmail($mid, $did, $category, $customMsg, $distKm, $rssi) {
    try {
        $db = Database::getInstance()->getConnection();
        $subscribers = $db->query("SELECT email FROM emails")->fetchAll(PDO::FETCH_COLUMN);
        if (empty($subscribers)) return false;

        $subject = "🚨 [LIFELINE ALERT] Device #{$did}: {$category}";
        $distText = $distKm !== null ? "{$distKm} km from Base Station" : "Calculated from RF Link";
        $customSection = $customMsg ? "<div style='background:#fef2f2; border-left:4px solid #ef4444; padding:12px; margin:15px 0;'><strong>Field SITREP:</strong> {$customMsg}</div>" : "";

        $htmlBody = "
        <div style='font-family:-apple-system,BlinkMacSystemFont,Segoe UI,Roboto,sans-serif; max-width:600px; margin:0 auto; border:1px solid #e5e7eb; border-radius:8px; overflow:hidden;'>
            <div style='background:#dc2626; color:#ffffff; padding:18px 24px;'>
                <h2 style='margin:0;'>⚠️ LifeLine Emergency SOS Broadcast</h2>
            </div>
            <div style='padding:24px; color:#1f2937;'>
                <p style='font-size:1.1rem; margin-top:0;'>An emergency transmission was ingested by the Base Station Gateway:</p>
                <table style='width:100%; border-collapse:collapse; margin-bottom:20px;'>
                    <tr><td style='padding:8px 0; color:#6b7280;'>Incident ID:</td><td><strong>#{$mid}</strong></td></tr>
                    <tr><td style='padding:8px 0; color:#6b7280;'>Origin Device:</td><td><strong>Node #{$did}</strong></td></tr>
                    <tr><td style='padding:8px 0; color:#6b7280;'>Emergency Category:</td><td><strong style='color:#dc2626;'>{$category}</strong></td></tr>
                    <tr><td style='padding:8px 0; color:#6b7280;'>Distance Estimate:</td><td><strong>{$distText}</strong></td></tr>
                    <tr><td style='padding:8px 0; color:#6b7280;'>Signal Strength:</td><td><strong>{$rssi} dBm</strong></td></tr>
                </table>
                {$customSection}
                <div style='margin-top:24px;'>
                    <a href='https://zenithkandel.com.np/lifeline/portal/dashboard.php' style='background:#2563eb; color:#ffffff; text-decoration:none; padding:12px 20px; border-radius:6px; font-weight:600; display:inline-block;'>Open Central Incident Dashboard</a>
                </div>
            </div>
        </div>";

        $headers = "MIME-Version: 1.0\r\n";
        $headers .= "Content-type:text/html;charset=UTF-8\r\n";
        $headers .= "From: LifeLine Emergency Center <alert@zenithkandel.com.np>\r\n";

        foreach ($subscribers as $to) {
            @mail($to, $subject, $htmlBody, $headers);
        }
        return true;
    } catch (Exception $e) {
        return false;
    }
}
```

---

## 💻 14. Web Dashboard Two-Way Dispatch Console Component

Integrate this HTML, CSS, and JavaScript widget into your web portal (`dashboard.php` or `index.html`). It connects directly to `/API/Create/command.php` and listens for field replies:

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
    <div class="stream-logs" id="streamLogs"></div>
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
.form-row { display: flex; gap: 16px; margin-bottom: 12px; }
.form-group { flex: 1; display: flex; flex-direction: column; }
.form-group label { font-size: 0.82rem; color: #9ca3af; margin-bottom: 6px; }
.label-row { display: flex; justify-content: space-between; }
.char-counter { font-size: 0.8rem; color: #9ca3af; }
.console-input {
  background: rgba(10, 15, 26, 0.8);
  border: 1px solid rgba(255, 255, 255, 0.15);
  border-radius: 8px;
  padding: 10px 14px;
  color: #fff;
  font-size: 0.95rem;
}
.console-input:focus { border-color: #3b82f6; outline: none; }
.dispatch-actions { display: flex; align-items: center; gap: 16px; margin-top: 14px; }
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
.btn-dispatch:hover { background: linear-gradient(135deg, #3b82f6, #2563eb); }
.dispatch-status { font-size: 0.85rem; color: #9ca3af; }
.conversation-stream { margin-top: 20px; border-top: 1px solid rgba(255, 255, 255, 0.08); padding-top: 14px; }
.stream-header { font-size: 0.85rem; text-transform: uppercase; letter-spacing: 0.05em; color: #9ca3af; margin-bottom: 10px; }
.stream-logs { max-height: 220px; overflow-y: auto; display: flex; flex-direction: column; gap: 8px; }
.msg-bubble { padding: 8px 14px; border-radius: 8px; font-size: 0.88rem; max-width: 80%; }
.msg-downlink { background: rgba(37, 99, 235, 0.2); border-left: 4px solid #3b82f6; align-self: flex-start; }
.msg-uplink { background: rgba(16, 185, 129, 0.2); border-left: 4px solid #10b981; align-self: flex-end; }
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
    const res = await fetch("/lifeline/API/Create/command.php", {
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
      statusElem.textContent = `✓ Queued (#${result.command_id}) ➔ Dispatched via LoRa in ~3s`;
      statusElem.style.color = "#10b981";
      document.getElementById('customMessage').value = "";
      updateCharCount(document.getElementById('customMessage'));
      
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

## ⚙️ 15. Configuring the LifeLine RX Pro Gateway Hardware

Your friend can point their ESP32 Base Station (`lifeline_rx_pro`) to their new VPS in two easy ways:

### Method A: Via Hardware Wi-Fi Setup Portal (No Recompilation Required)
1. Hold down the **Wi-Fi Push Button** (GPIO 14) on the Base Station for 3 seconds.
2. The 16×2 LCD screen displays `AP: LifeLine-RX-Setup | IP: 192.168.4.1`.
3. Connect a laptop or smartphone to the Wi-Fi network `LifeLine-RX-Setup`.
4. Open your browser to `http://192.168.4.1`.
5. Under **Cloud API Settings**, enter:
   - **API Endpoint URL**: `https://<YOUR_VPS_DOMAIN>/lifeline/API/Create/message.php`
   - **API Key**: `LF_PRO_KEY_2026`
6. Click **Save & Restart**. The Base Station stores these in Non-Volatile Storage (NVS) and automatically polls `API/Read/pending_commands.php` and posts to `API/Create/message.php`.

### Method B: In Source Code (`lifeline_rx_pro/Config.h`)
Edit lines 24–26 in `lifeline_rx_pro/Config.h`:
```cpp
#define API_ENDPOINT               "https://<YOUR_VPS_DOMAIN>/lifeline/API/Create/message.php"
#define API_DOWNLINK_POLL_ENDPOINT "https://<YOUR_VPS_DOMAIN>/lifeline/API/Read/pending_commands.php"
#define API_DOWNLINK_ACK_ENDPOINT  "https://<YOUR_VPS_DOMAIN>/lifeline/API/Update/command_status.php"
```
Recompile and flash via PlatformIO:
```bash
pio run -d lifeline_rx_pro -t upload
```

---

## 🧪 16. End-to-End Verification Playbook with `curl`

Once your friend has deployed the files and imported the database, they can test the entire pipeline directly from their terminal:

### 1. Test Ingestion of a Custom SITREP Chat (Uplink)
```bash
curl -X POST https://<YOUR_DOMAIN>/lifeline/API/Create/message.php \
  -H "Content-Type: application/json" \
  -d '{
    "DID": 1,
    "message_code": 0,
    "code": "M",
    "RSSI": -68,
    "snr": 9.2,
    "distance_km": 1.25,
    "is_chat": true,
    "custom_msg": "Avalanche debris cleared. Team proceeding to camp 2.",
    "source": "LORA"
  }'
```
Expected Response: `HTTP 201 Created` with `status: "success"` and `message_id`.

### 2. Test Ingestion of Standard SOS Alert
```bash
curl -X POST https://<YOUR_DOMAIN>/lifeline/API/Create/message.php \
  -H "Content-Type: application/json" \
  -d '{
    "DID": 2,
    "message_code": 1,
    "RSSI": -75,
    "snr": 8.0,
    "distance_km": 2.10,
    "is_chat": false,
    "custom_msg": "",
    "source": "LORA"
  }'
```

### 3. Test Dashboard Query to Verify Saved Fields
```bash
curl -X GET "https://<YOUR_DOMAIN>/lifeline/API/Read/message.php?limit=2"
```
Verify that the output contains `distance_km`, `snr`, `is_chat`, `custom_msg`, and `source`.

### 4. Test Web-to-LoRa Downlink Queueing
```bash
curl -X POST https://<YOUR_DOMAIN>/lifeline/API/Create/command.php \
  -H "Content-Type: application/json" \
  -d '{
    "rx_id": 1,
    "target_did": 1,
    "action": "DISPATCH",
    "message": "Medic helicopter en route to your LZ"
  }'
```
Expected Response: `command_id: 1` with status `success`.

### 5. Test Gateway Polling for Pending Downlink
```bash
curl -X GET "https://<YOUR_DOMAIN>/lifeline/API/Read/pending_commands.php?rx_id=1"
```
Expected Response: `has_command: true` with `target_did: 1` and the message text.

### 6. Test Gateway Status Acknowledgment
```bash
curl -X POST https://<YOUR_DOMAIN>/lifeline/API/Update/command_status.php \
  -H "Content-Type: application/json" \
  -d '{
    "command_id": 1,
    "rx_id": 1,
    "status": "DISPATCHED_LORA",
    "lora_tx_ok": true
  }'
```
Expected Response: `status: "success"` and `new_status: "DISPATCHED_LORA"`.
