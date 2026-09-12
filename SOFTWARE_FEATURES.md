# 🚨 LifeLine — Software Architecture & Features Specification

> **Comprehensive Technical Guide to the Software Stack, Portals, REST APIs, Notification Engines, and Data Layer**  
> *LifeLine: Off-Grid Emergency Response System for Remote Regions*

---

## 📑 Table of Contents

1. [Executive Summary & System Architecture](#1-executive-summary--system-architecture)
2. [Public Web Presentation & Educational Portal](#2-public-web-presentation--educational-portal)
3. [Authentication, Security & Access Control](#3-authentication-security--access-control)
4. [Central Operations Command Portal (`/portal/`)](#4-central-operations-command-portal-portal)
   - [4.1 Unified Portal Shell & Navigation](#41-unified-portal-shell--navigation)
   - [4.2 Live Operations Dashboard](#42-live-operations-dashboard)
   - [4.3 Emergency Messages Log & Incident Tracking](#43-emergency-messages-log--incident-tracking)
   - [4.4 Fleet & Device Management](#44-fleet--device-management)
   - [4.5 Emergency Responders & Resource Dispatch Management](#45-emergency-responders--resource-dispatch-management)
   - [4.6 Dynamic Taxonomy & Regional Index Mapping Engine](#46-dynamic-taxonomy--regional-index-mapping-engine)
   - [4.7 Emergency Email Subscribers Management](#47-emergency-email-subscribers-management)
5. [RESTful API Backend Architecture (`/API/`)](#5-restful-api-backend-architecture-api)
   - [5.1 API Design Standards & Response Envelope](#51-api-design-standards--response-envelope)
   - [5.2 Gateway Telemetry Ingestion Pipeline](#52-gateway-telemetry-ingestion-pipeline)
   - [5.3 Endpoints Reference Catalog](#53-endpoints-reference-catalog)
6. [Multi-Channel Notification & Alert Dispatch Engine](#6-multi-channel-notification--alert-dispatch-engine)
   - [6.1 PHPMailer SMTP Emergency Email Dispatcher](#61-phpmailer-smtp-emergency-email-dispatcher)
   - [6.2 Firebase Cloud Messaging (FCM) Push Engine](#62-firebase-cloud-messaging-fcm-push-engine)
7. [Database Architecture & Persistence Layer](#7-database-architecture--persistence-layer)
   - [7.1 Database Engine & Singleton Pattern](#71-database-engine--singleton-pattern)
   - [7.2 Relational Schema & MySQL JSON Querying](#72-relational-schema--mysql-json-querying)
8. [Comprehensive Software Feature Matrix](#8-comprehensive-software-feature-matrix)

---

## 1. Executive Summary & System Architecture

While the physical field layer of **LifeLine** relies on ESP32 microcontrollers, LoRa SX1278 transceivers, and edge sensors operating in off-grid mountain terrain, the **Software Layer** acts as the central intelligence, coordination, ingestion, visualization, and dispatch brain.

The software stack receives low-bandwidth integer packets forwarded by edge LoRa RX Gateways, decodes them into rich situational data, alerts rescue authorities across multiple channels (Email and Web Push Notifications), and provides emergency operators with a real-time command interface to deploy life-saving resources.

```
┌────────────────────────────────────────────────────────────────────────┐
│                        EDGE FIELD LAYER (LORA)                         │
│  [ESP32 TX Node] ──(LoRa 433MHz)──► [Mesh Relays] ──► [RX Gateway]     │
└───────────────────────────────────────────────────────────┬────────────┘
                                                            │ HTTP POST
                                                            ▼
┌────────────────────────────────────────────────────────────────────────┐
│                     LIFELINE SOFTWARE PLATFORM                         │
│                                                                        │
│  ┌──────────────────────────────────────────────────────────────────┐  │
│  │                    RESTful API Ingestion Layer                   │  │
│  │   • Token/Session Auth       • JSON Normalization                │  │
│  │   • Gateway Ingestion        • Dynamic Index Mapping             │  │
│  └──────────────┬─────────────────────────────┬─────────────────────┘  │
│                 │                             │                        │
│                 ▼                             ▼                        │
│  ┌──────────────────────────────┐ ┌─────────────────────────────────┐  │
│  │   Database & Storage Layer   │ │   Multi-Channel Dispatch Engine │  │
│  │   • MySQL PDO Singleton      │ │   • PHPMailer SMTP Email Alerts │  │
│  │   • JSON Mapping Schema      │ │   • Firebase Cloud Messaging v1 │  │
│  │   • Audit Trail Logging      │ │   • Push Notifications          │  │
│  └──────────────┬───────────────┘ └─────────────────┬───────────────┘  │
│                 │                                   │                  │
│                 └─────────────────┬─────────────────┘                  │
│                                   ▼                                    │
│  ┌──────────────────────────────────────────────────────────────────┐  │
│  │             Incident Command & Monitoring Portal                 │  │
│  │   • Live Operations Dashboard       • Fleet Device Management    │  │
│  │   • Emergency Incident Tracking     • Responder Dispatch Matrix  │  │
│  │   • Interactive Google Maps         • Taxonomy Index Editor      │  │
│  │   • Dual Dark/Light Theme           • Multi-Criteria Filtering   │  │
│  └──────────────────────────────────────────────────────────────────┘  │
│                                                                        │
│  ┌──────────────────────────────────────────────────────────────────┐  │
│  │                 Public Presentation Web Experience                │  │
│  │   • Educational Landing Page        • Visual Flow Simulator      │  │
│  │   • Offline-First Design Concept    • Responsive Touch Layout    │  │
│  └──────────────────────────────────────────────────────────────────┘  │
└────────────────────────────────────────────────────────────────────────┘
```

---

## 2. Public Web Presentation & Educational Portal

Located at the root (`index.html`, `CSS/style.css`, `JS/script.js`), the public portal provides a humanitarian, engineering-focused introduction to the LifeLine initiative.

### Key Features:
- **Clean, Rugged Humanitarian Design**: Adheres to strict aesthetic guidelines tailored for disaster management — avoiding consumer tech neon gradients in favor of clean slate tones, high readability, and sharp layout structures.
- **Mission Impact Counters**: Real-time display highlighting system parameters:
  - **10+ km** line-of-sight signal reach per hop.
  - **Zero Internet Requirement** on the field side.
  - **24/7 Always-On** network resilience.
- **Interactive System Flow Visualizer**:
  - Animated schematic illustrating packet travel: `Sensors / Emergency Button` ➔ `ESP32 TX Node` ➔ `LoRa Mesh Hop` ➔ `RX Gateway Base Station` ➔ `Cloud Ingestion & Dispatch`.
- **Himalayan & Rural Problem Statement Breakdown**:
  - Educational deep dive on terrain challenges in Nepal's Solukhumbu region (high altitude, delayed rescue helicopters, absence of GSM towers, high cost of satellite phones).
- **Interactive Technology Stack Matrix**:
  - Modular cards detailing Edge Processing (ESP32-S3), Radio (LoRa SX1278), Display/Input (ST7789 TFT & Matrix Keypad), Power (Li-ion 18650 & Solar), and Cloud Operations.
- **Responsive Mobile Navigation**:
  - Touch-friendly hamburger drawer navigation with smooth viewport scrolling and Intersection Observer scroll-triggered reveal animations.
- **Direct Portal Bridge**:
  - Direct secure entry link routing authorized personnel into the operational login gateway.

---

## 3. Authentication, Security & Access Control

Securing disaster operations from unauthorized tampering is handled by the authentication subsystem (`login.php`, `CSS/login.css`, `JS/login.js`, `API/auth/*`).

### Key Features:
- **Cryptographic Password Security**:
  - Passwords hashed using industry-standard PHP `password_hash()` and verified via `password_verify()` using strong bcrypt algorithms.
- **Role-Based Access Control (RBAC)**:
  - Distinguishes user privileges between `admin` (full system configuration, device registration, and mapping manipulation) and `operator` (incident monitoring and responder dispatching).
- **Dual Authentication Modes**:
  - **Web Session Authentication**: Secure PHP session cookies for interactive browser sessions.
  - **API Token Authentication**: Bearer tokens issued upon login for automated API interactions.
- **Persistent "Remember Me" State**:
  - Secure 30-day persistent authentication cookie handling for operator convenience during emergency field deployments.
- **Session Verification Heartbeat (`/API/auth/check.php`)**:
  - Dedicated endpoint verifying active user credentials, email, and roles; used by the single-page portal shell to prevent session drops.
- **Operator-Friendly Login Interface**:
  - Sleek glassmorphism dark interface with real-time field validation, password show/hide eye toggle, and automated alert toast feedback for invalid credentials.

---

## 4. Central Operations Command Portal (`/portal/`)

The primary software workspace is the **LifeLine Command Portal**, providing incident commanders and rescue personnel with real-time situational awareness.

---

### 4.1 Unified Portal Shell & Navigation

Files: `portal/index.php`, `portal/css/index.css`, `portal/js/index.js`

- **Single-Page Architecture (SPA-feel)**: Seamless iframe content switching without reloading the global navigation or disrupting active alert listening.
- **Persistent Sidebar Navigation**: Quick access to Dashboard, Messages, Devices, Responders, Mapping, and Email Receivers with active route indicators.
- **Live Connection Pulse Indicator**:
  - Dynamic status dot with audio-visual feedback indicating active communication with the backend API:
    - 🟢 Pulsing Green: "Listening..." (healthy background polling).
    - 🔴 Red: "Connection lost" (network or server downtime warning).
- **Dual Theme Engine (Dark & Light Mode)**:
  - Fully integrated dark/light theme toggle with instant cross-frame CSS variable synchronization.
  - User preference stored in `localStorage` (`lifeline-theme`) for persistent workstation settings.
- **Integrated Web Push Notification Listener**:
  - Pre-wired Firebase Cloud Messaging (FCM) modular SDK initialization in the portal shell to receive incoming push alerts even when the browser tab is idle.

---

### 4.2 Live Operations Dashboard

Files: `portal/dashboard.php`, `portal/css/dashboard.css`, `portal/js/dashboard.js`

- **Fleet Health & Incident KPI Cards**:
  - **Total Devices**: Total registered LoRa nodes across the territory.
  - **Active Devices**: Nodes online with calculated percentage of fleet operational (`% of fleet online`).
  - **Offline Devices**: Attention-needed counter alerting operators to silent or dead nodes.
  - **Active Alerts**: Unresolved emergency alerts requiring immediate tactical response.
- **5-Second Automated Telemetry Polling Engine**:
  - Asynchronous background polling (`setInterval` at 5000ms) with delta tracking (`lastMessageId`) detecting new transmissions without page refresh.
- **Dynamic Emergency Alert Stream**:
  - Urgent alerts displayed with real-time severity badges:
    - 🔴 **Emergency / Critical** (Codes 1–9: Altitude sickness, injury, search & rescue, avalanche, landslide, fire).
    - 🟡 **Warning / Medium** (Codes 10–14: Equipment failure, weather alert, infrastructure damage, food/water shortage, lost comms).
    - 🟢 **Info / Low** (Code 15: All Clear).
  - Shows originating Device ID, decoded Village/Location Name, relative timestamps ("Just now", "5m ago"), and responder count badge.
- **Interactive Emergency Detail Modal**:
  Clicking any alert card opens a comprehensive incident diagnostic modal:
  1. **Telemetry Readout**: Message ID, Device Name, Location, Full Date & Time, and raw Signal Strength (RSSI in dBm).
  2. **Interactive Google Maps Embed**: Generates an interactive map iframe focused directly on the village/location coordinates in Nepal.
  3. **Intelligent Responder Matching Engine**:
     - Automatically scans the `indexes` table (`type='help'`) to cross-reference the incoming emergency code with registered rescue resources.
     - Dynamically filters and displays only responders capable of addressing the specific emergency (e.g., matching Helicopters for Altitude Sickness or Avalanches; Clinics for Injuries; Ground Sherpa teams for Landslides).
     - Displays direct telephone contact links (`tel:`), base location, and live readiness status.
  4. **Direct External Map Action**: "Open in Google Maps" button to open detailed driving/hiking directions in a new tab.
  5. **One-Click Incident Resolution**: "Mark Resolved" button updates the message status in the database and refreshes the live board.
- **Live Device Status Feed**:
  - Secondary feed displaying the top 10 most recent node check-ins, reporting device status pills (`active`, `inactive`, `maintenance`) and last ping timestamps.

---

### 4.3 Emergency Messages Log & Incident Tracking

Files: `portal/messages.php`, `portal/css/messages.css`, `portal/js/messages.js`

- **Comprehensive Audit Trail**: Complete historical record of all emergency signals ever received by the system.
- **Multi-Parameter Search & Filtering**:
  - **Status Filter**: View `All Status`, `Active` incidents, or `Resolved` incidents.
  - **Device Filter**: Filter messages originating from specific LoRa nodes.
  - **Message Type Filter**: Filter by exact disaster category (e.g. Avalanche vs Fire).
  - **Date Range Filters**: Precise `From` and `To` date selectors.
- **Summary Metrics Bar**: Shows Total Messages, Active Emergency count (highlighted red), and Resolved incident count (highlighted green).
- **Paginated Grid**: Scalable table view with page navigation controls for historical auditing.
- **Manual Emergency Simulation & Testing Modal**:
  - Operators can manually trigger emergency events from within the portal by selecting a Device, Emergency Category, and entering an RSSI value.
  - Enables emergency simulation drills, field testing, and manual record creation.
- **In-Line Incident Resolution**: Direct action buttons to mark pending alerts as resolved.

---

### 4.4 Fleet & Device Management

Files: `portal/devices.php`, `portal/css/devices.css`, `portal/js/devices.js`

- **Full Lifecycle Device CRUD**:
  - **Add Device**: Register new LoRa nodes with custom labels (e.g., `Node-Namche-01`), assigned Location ID, and initial operational status.
  - **Edit Device**: Modify device naming, reassign to new geographic coordinates/villages, or switch state.
  - **Delete Device**: Remove decommissioned nodes with confirmation dialogs.
- **Operational Status Tracking**:
  - Categorizes nodes as `active` (broadcasting normally), `inactive` (offline/unreachable), or `maintenance` (bench testing or sensor servicing).
- **Automated Gateway Heartbeat**:
  - The software updates the `last_ping` timestamp in the database automatically every time a LoRa packet from that device reaches any gateway.
- **Faceted Fleet Filtering & Search**:
  - Real-time instant text search by device name or location name.
  - Status dropdown filter (`Active`, `Inactive`, `Maintenance`).
  - Geographic location dropdown filter populated dynamically from the location index.

---

### 4.5 Emergency Responders & Resource Dispatch Management

Files: `portal/helps.php`, `portal/css/helps.css`, `portal/js/helps.js`

- **Responder Resource Registry**:
  - Tracks rescue entities: Nepal Army Helicopter Units, Khumbu Ground Search Teams, Himalayan Rescue Association (HRA), Everest ER Clinics, Local Sherpa Rescue units, Red Cross Nepal, etc.
- **Responder Attributes**:
  - Unit/Organization Name.
  - Direct Phone Number (clickable `tel:` links).
  - Base Location (e.g. Kathmandu, Lukla, Namche Bazaar, Pheriche).
  - Estimated Time of Arrival (ETA) metric (e.g., "30-45 mins", "1-2 hours").
  - Current Availability Status (`available`, `dispatched`, `busy`).
- **Emergency Capability Matrix Assignment**:
  - Interactive multi-select dropdown enabling operators to define exactly which emergency categories each responder is equipped to service (e.g., High Altitude SAR, Trauma, Fire, Flood, Food Supply).
- **Live Resource Status Counters**:
  - Real-time counters showing Total Responders, Available Units (green), Dispatched Units (yellow), and Busy Units (red).
- **Real-Time Search & Filtering**:
  - Search responders by name, base location, or contact details; filter by live availability.

---

### 4.6 Dynamic Taxonomy & Regional Index Mapping Engine

Files: `portal/mapping.php`, `portal/css/mapping.css`, `portal/js/mapping.js`

One of the most powerful architectural features of LifeLine: **Low-Bandwidth Packet Compression and Dynamic Reconstruction**.

To preserve radio airtime and battery, LoRa packets transmit raw integer codes (e.g. `TX003,1` = Device 3, Code 1). The Mapping Engine translates these codes into human-readable data without requiring database schema changes.

- **Three Primary Mapping Types**:
  1. `location`: Maps integer Location IDs (1–15+) to real-world settlements and GPS reference points:
     - `1`: Namche Bazaar
     - `2`: Lukla Village
     - `3`: Tengboche
     - `4`: Dingboche
     - `5`: Gorak Shep
     - `6`: Phakding
     - `7`: Khumjung
     - `8`: Pangboche
     - `9`: Pheriche
     - `10`: Lobuche
     - `11`: Syangboche
     - `12`: Thame
     - `13`: Gokyo
     - `14`: Machermo
     - `15`: Chhukung
  2. `message`: Maps 15 standardized integer codes to crisis classifications:
     - `1`: Medical Emergency - Altitude Sickness
     - `2`: Medical Emergency - Injury
     - `3`: Medical Emergency - Illness
     - `4`: Search and Rescue Required
     - `5`: Avalanche Alert
     - `6`: Landslide Warning
     - `7`: Fire Emergency
     - `8`: Flood Warning
     - `9`: Lost/Missing Person
     - `10`: Equipment Failure
     - `11`: Weather Emergency
     - `12`: Infrastructure Damage
     - `13`: Food/Water Shortage
     - `14`: Communication Lost
     - `15`: All Clear - Situation Normal
  3. `help`: Maps Responder IDs (HID) to arrays of message codes they are equipped to handle (e.g. Helicopter `HID 1` ➔ `[1, 2, 3, 4, 5, 9]`).
- **Visual JSON Mapping Builder**:
  - Operators can view index cards with all key-value pairs formatted cleanly.
  - Interactive Modal to add new key-value entries, edit labels, or remove deprecated codes dynamically.
  - Real-time serialization to and from MySQL `JSON` column format.
  - Instant search across mapping types, keys, and descriptive labels.

---

### 4.7 Emergency Email Subscribers Management

Files: `portal/emails.php`, `portal/css/emails.css`, `portal/js/emails.js`

- **Subscriber Registry**: Centralized list of emergency stakeholders (district authorities, helicopter charter operators, hospital desks, rescue coordinators) who automatically receive critical alerts.
- **Subscriber CRUD**:
  - Add single or bulk email addresses.
  - Edit existing recipient contact records.
  - Safe removal with confirmation modal.
- **Instant Dispatch Integration**: Any email added here is automatically subscribed to the PHPMailer broadcast queue when an emergency message is created.

---

## 5. RESTful API Backend Architecture (`/API/`)

The backend is built as a lightweight, high-performance REST API written in clean PHP without heavy framework overhead, ensuring fast response times on low-cost server hardware.

---

### 5.1 API Design Standards & Response Envelope

- **Base URL**: `/API/`
- **CORS Handling**: Full cross-origin support (`Access-Control-Allow-Origin: *`) with automatic HTTP `OPTIONS` preflight handling.
- **Consistent Response Payload**:
```json
{
  "success": true,
  "data": { ... },
  "message": "Operation completed successfully",
  "timestamp": "2026-03-12 17:00:00"
}
```
- **Standard HTTP Status Codes**:
  - `200 OK`: Request succeeded.
  - `201 Created`: Resource created.
  - `400 Bad Request`: Validation or missing required parameter error.
  - `401 Unauthorized`: Authentication required or invalid session.
  - `404 Not Found`: Resource ID not found.
  - `405 Method Not Allowed`: Incorrect HTTP method used.
  - `500 Server Error`: Database or execution exception.

---

### 5.2 Gateway Telemetry Ingestion Pipeline

Endpoint: `POST /API/Create/message.php`

When an edge LoRa RX Gateway receives an off-grid transmission (e.g. `TX003,1`), its WiFi/cellular module sends an HTTP POST request to this endpoint.

#### Processing Steps:
1. **Input Validation**: Verifies presence of `DID` (Device ID) and `message_code`.
2. **Device Verification**: Confirms `DID` exists in the `devices` table; returns 404 if invalid.
3. **Database Insertion**: Inserts message record into `messages` table with current timestamp and signal strength (`RSSI`).
4. **Heartbeat Sync**: Executes `UPDATE devices SET last_ping = NOW() WHERE DID = :did`.
5. **Instant JSON Resolution**: Uses MySQL `JSON_EXTRACT` and `JSON_UNQUOTE` to join against the `indexes` table in a single query, resolving `location_name` and `message_text`.
6. **Multi-Channel Dispatch Execution**:
   - Spawns `FCMHelper` to dispatch real-time web push notifications.
   - Spawns `EmailHelper` to dispatch formatted HTML emergency emails to all registered receivers.
7. **Response Payload**: Returns full decoded message object including notification dispatch statistics (`emails.success`, `notifications.success`).

---

### 5.3 Endpoints Reference Catalog

#### Authentication Group (`/API/auth/`)
| Method | Endpoint | Description |
|---|---|---|
| `POST` | `/API/auth/login.php` | Authenticates email & password, starts session, returns user object & token |
| `GET` | `/API/auth/check.php` | Validates active session and returns authenticated user details |
| `POST` | `/API/auth/logout.php` | Terminates session and destroys cookies |

#### Resource Creation Group (`/API/Create/`)
| Method | Endpoint | Parameters / Body | Description |
|---|---|---|---|
| `POST` | `/API/Create/message.php` | `DID`, `message_code`, `RSSI` (optional) | Gateway ingestion & alert broadcast |
| `POST` | `/API/Create/device.php` | `device_name`, `LID`, `status` | Registers a new LoRa field device |
| `POST` | `/API/Create/helps.php` | `name`, `contact`, `location`, `eta`, `status`, `for_messages` | Registers a new rescue responder |
| `POST` | `/API/Create/emails.php` | `email` | Subscribes an email for emergency alerts |
| `POST` | `/API/Create/index.php` | `type`, `mapping` (JSON), `description` | Creates a new taxonomy index mapping |
| `POST` | `/API/Create/fcm_token.php`| `token`, `device_info` | Registers client browser for push notifications |

#### Resource Retrieval Group (`/API/Read/`)
| Method | Endpoint | Query Parameters | Description |
|---|---|---|---|
| `GET` | `/API/Read/message.php` | `id`, `did`, `lid`, `message_code`, `status`, `from`, `to`, `page`, `limit` | Retrieves decoded messages with filtering & pagination |
| `GET` | `/API/Read/device.php` | `id`, `status`, `lid`, `page`, `limit` | Retrieves device fleet status & location names |
| `GET` | `/API/Read/helps.php` | `id`, `status`, `message_code`, `limit` | Retrieves responders filtered by availability or emergency type |
| `GET` | `/API/Read/emails.php` | None | Retrieves all active email alert subscribers |
| `GET` | `/API/Read/index.php` | `id`, `type` (`location`, `message`, `help`) | Retrieves dictionary mappings |

#### Resource Update Group (`/API/Update/`)
| Method | Endpoint | Key Fields | Description |
|---|---|---|---|
| `PUT/POST` | `/API/Update/message.php` | `MID`, `status` (`active`/`resolved`) | Updates incident status |
| `PUT/POST` | `/API/Update/device.php` | `DID`, `device_name`, `LID`, `status` | Updates device attributes and location |
| `PUT/POST` | `/API/Update/helps.php` | `HID`, `name`, `contact`, `status`, `eta`, `for_messages` | Updates responder readiness & assignments |
| `PUT/POST` | `/API/Update/emails.php` | `SN`, `email` | Updates subscriber email address |
| `PUT/POST` | `/API/Update/index.php` | `IID`, `type`, `mapping`, `description` | Modifies taxonomy index dictionaries |

#### Resource Deletion Group (`/API/Delete/`)
| Method | Endpoint | Key Fields | Description |
|---|---|---|---|
| `DELETE/POST` | `/API/Delete/message.php` | `MID` | Removes an emergency message record |
| `DELETE/POST` | `/API/Delete/device.php` | `DID` | Deletes a device (cascades related records) |
| `DELETE/POST` | `/API/Delete/helps.php` | `HID` | Removes a responder resource |
| `DELETE/POST` | `/API/Delete/emails.php` | `SN` or `email` | Unsubscribes an email recipient |
| `DELETE/POST` | `/API/Delete/index.php` | `IID` | Deletes an index mapping |

---

## 6. Multi-Channel Notification & Alert Dispatch Engine

When a life-threatening disaster strikes in an off-grid village, mere storage in a database is insufficient. The software includes an automated dual-channel notification pipeline.

---

### 6.1 PHPMailer SMTP Emergency Email Dispatcher

File: `API/email_helper.php`

- **Automated Alert Generation**: Dispatches formatted, high-priority emergency notifications via SMTP.
- **Rich HTML Email Template**:
  - High-visibility red alert header with timestamp.
  - Incident Summary Table: Incident ID, Device Name, Village/Location Name, Emergency Classification, and Signal Strength.
  - Direct Action Buttons linking authorized coordinators straight to the live portal incident view.
- **Robust Enterprise Configuration via `.env`**:
  - Configurable SMTP Host (`LIFELINE_SMTP_HOST`).
  - Configurable SMTP Port (`LIFELINE_SMTP_PORT`, default: `465` SSL / `587` TLS).
  - Authenticated SMTP credentials (`LIFELINE_SMTP_USERNAME`, `LIFELINE_SMTP_PASSWORD`).
  - Sender Display Name (`LIFELINE_SMTP_FROM_NAME`).
- **Batch Resiliency**:
  - Automatically queries the `emails` table, cycling through all registered rescue contacts.
  - Logs per-recipient success/failure rates without breaking the main ingestion request.

---

### 6.2 Firebase Cloud Messaging (FCM) Push Engine

File: `API/fcm_helper.php`

- **Google Firebase HTTP v1 API**: Uses Google's latest secure OAuth2 v1 push notification standard.
- **Service Account Authentication**:
  - Automatically signs JWTs using local service account credentials (`lifeline-notification-firebase-adminsdk.json` or path in `LIFELINE_FIREBASE_SERVICE_ACCOUNT`).
  - Generates and caches Google OAuth2 Bearer Access Tokens with automatic expiration handling.
- **Instant Browser & Mobile Notifications**:
  - Sends high-priority push payloads containing the emergency type as the title, location as the body, and custom data payload with Incident ID.
  - Triggers push alerts on operator smartphones, tablets, and desktop workstations even when the dashboard tab is closed or in the background.

---

## 7. Database Architecture & Persistence Layer

Files: `database.php`, `lifeline_updated.sql`

---

### 7.1 Database Engine & Singleton Pattern

- **PDO Singleton Class**: Implements the GoF Singleton Pattern ensuring a single, persistent database connection pool per PHP request, minimizing TCP overhead and connection latency.
- **SQL Injection Immunization**: All queries use parameterized prepared statements (`$db->prepare()`) with typed bindings.
- **Character Encoding**: Native `utf8mb4` encoding supporting multilingual character sets.

---

### 7.2 Relational Schema & MySQL JSON Querying

#### 1. `user` Table
Stores administrative credentials and permissions.
- `UID`: INT (Primary Key, Auto-increment)
- `name`: VARCHAR(100)
- `email`: VARCHAR(150, UNIQUE)
- `password`: VARCHAR(255) (Bcrypt hash)
- `role`: ENUM('admin', 'operator')
- `last_login`: DATETIME
- `created_at`: DATETIME

#### 2. `devices` Table
Tracks registered field hardware nodes.
- `DID`: INT (Primary Key, Auto-increment)
- `device_name`: VARCHAR(100) (e.g. `Node-Namche-01`)
- `LID`: INT (Location ID mapping to `indexes.mapping['location']`)
- `status`: ENUM('active', 'inactive', 'maintenance')
- `last_ping`: DATETIME (Updated on every received transmission)

#### 3. `messages` Table
The central emergency message log.
- `MID`: INT (Primary Key, Auto-increment)
- `DID`: INT (Foreign Key referencing `devices.DID` with `ON DELETE CASCADE`)
- `RSSI`: INT (Signal Strength in dBm, e.g. -65 dBm)
- `message_code`: INT (Maps to `indexes.mapping['message']`)
- `timestamp`: DATETIME (Default `CURRENT_TIMESTAMP`)

#### 4. `helps` Table
Resource directory for rescue units.
- `HID`: INT (Primary Key, Auto-increment)
- `name`: VARCHAR(150)
- `contact`: VARCHAR(50)
- `eta`: VARCHAR(50) (e.g. "30-45 mins")
- `status`: ENUM('available', 'dispatched', 'busy')
- `location`: VARCHAR(200)

#### 5. `indexes` Table
Dynamic JSON mapping dictionary bridging LoRa packets to human context.
- `IID`: INT (Primary Key, Auto-increment)
- `type`: ENUM('location', 'message', 'help') (UNIQUE)
- `mapping`: JSON (Key-value map or array map)
- `description`: TEXT
- `updated_at`: DATETIME

#### 6. `emails` Table
Stores broadcast alert recipient addresses.
- `SN`: INT (Primary Key, Auto-increment)
- `email`: VARCHAR(255, UNIQUE)

---

## 8. Comprehensive Software Feature Matrix

| Functional Area | Feature | Description | Implementation File(s) |
|---|---|---|---|
| **Public Presentation** | Hero & Mission Counters | Showcases 10+ km range, 0 internet requirement, 24/7 reliability | `index.html`, `CSS/style.css` |
| **Public Presentation** | Animated Flow Simulator | Interactive visual schematic of sensor-to-cloud workflow | `index.html`, `JS/script.js` |
| **Public Presentation** | Responsive Mobile UX | Hamburger navigation drawer with smooth scroll reveals | `index.html`, `JS/script.js` |
| **Authentication** | Password Security | Secure bcrypt password hashing and verification | `login.php`, `API/auth/login.php` |
| **Authentication** | Role-Based Access (RBAC) | Distinguishes between `admin` and `operator` permissions | `login.php`, `database.php` |
| **Authentication** | Persistent Login | 30-day "Remember Me" persistent authentication cookies | `JS/login.js`, `API/auth/login.php` |
| **Authentication** | Session Verification | Automatic session validation preventing unauthorized access | `API/auth/check.php`, `portal/js/index.js` |
| **Portal Shell** | Unified Navigation Frame | SPA-like iframe navigation shell maintaining live state | `portal/index.php`, `portal/js/index.js` |
| **Portal Shell** | Live Connection Pulse | Visual indicator showing real-time API connectivity | `portal/js/index.js`, `portal/css/index.css` |
| **Portal Shell** | Dark/Light Theme Engine | Global theme switcher with `localStorage` persistence | `portal/js/index.js`, `portal/css/shared.css` |
| **Live Dashboard** | Fleet & Alert KPIs | Real-time counts of total/active/offline devices and alerts | `portal/dashboard.php`, `portal/js/dashboard.js` |
| **Live Dashboard** | 5s Background Polling | Automated asynchronous polling engine for real-time alerts | `portal/js/dashboard.js` |
| **Live Dashboard** | Severity Color Coding | Critical (Red), High (Orange), Medium (Yellow), Low (Green) | `portal/js/dashboard.js`, `portal/css/dashboard.css` |
| **Live Dashboard** | Google Maps Integration | Embedded map iframe centered on emergency coordinates | `portal/js/dashboard.js`, `portal/dashboard.php` |
| **Live Dashboard** | Responder Matching Engine | Automatically filters responders capable of handling specific crisis | `portal/js/dashboard.js` |
| **Live Dashboard** | One-Click Resolution | Mark emergency incidents as resolved with live UI refresh | `portal/js/dashboard.js` |
| **Live Dashboard** | Fleet Health Monitor | Real-time device ping timestamps and operational states | `portal/js/dashboard.js` |
| **Incident Messages** | Historical Message Audit | Complete historical record of all LoRa transmissions | `portal/messages.php`, `portal/js/messages.js` |
| **Incident Messages** | Multi-Criteria Filtering | Filter by status, device, emergency category, and date range | `portal/js/messages.js` |
| **Incident Messages** | Emergency Simulation | Modal allowing operators to manually inject test alerts | `portal/messages.php`, `portal/js/messages.js` |
| **Incident Messages** | Data Pagination | Scalable pagination controls for large message volumes | `portal/messages.php`, `portal/js/messages.js` |
| **Fleet Management** | Device CRUD | Create, view, edit, and delete LoRa nodes | `portal/devices.php`, `portal/js/devices.js` |
| **Fleet Management** | Geographic Assignment | Link nodes to regional villages from dynamic location index | `portal/js/devices.js` |
| **Fleet Management** | Automated Ping Sync | Automatically updates device `last_ping` on packet receipt | `API/Create/message.php` |
| **Fleet Management** | Instant Device Search | Live filtering by node name, location, or status | `portal/js/devices.js` |
| **Responder Registry** | Responder CRUD | Register rescue teams, medical units, helicopters | `portal/helps.php`, `portal/js/helps.js` |
| **Responder Registry** | Capability Tagging | Multi-select assignment of emergency categories handled | `portal/helps.php`, `portal/js/helps.js` |
| **Responder Registry** | Live Availability Tracker | Track `available`, `dispatched`, and `busy` resources | `portal/helps.php`, `portal/js/helps.js` |
| **Dynamic Taxonomy** | LoRa Integer Decoding | Decompresses 1-byte radio packets into rich situational data | `portal/mapping.php`, `API/Read/index.php` |
| **Dynamic Taxonomy** | Visual JSON Editor | In-portal editor to add/edit location & message mappings | `portal/mapping.php`, `portal/js/mapping.js` |
| **Alert Dispatch** | Automated Ingestion | Processes incoming gateway packets and triggers notifications | `API/Create/message.php` |
| **Alert Dispatch** | SMTP Email Dispatch | Sends rich HTML emergency alerts to registered subscribers | `API/email_helper.php` |
| **Alert Dispatch** | Firebase Web Push | Dispatches Google FCM v1 push notifications to browsers | `API/fcm_helper.php` |
| **Alert Dispatch** | Email Subscriber Manager | Manage email distribution list for emergency broadcasts | `portal/emails.php`, `portal/js/emails.js` |
| **API Architecture** | Uniform REST Envelope | Standardized JSON output format across all endpoints | `database.php` |
| **API Architecture** | Complete CRUD Suites | Full endpoints for auth, create, read, update, and delete | `API/Create/*`, `API/Read/*`, etc. |
| **Persistence Layer** | PDO Singleton Pool | Reusable, high-performance database connection instance | `database.php` |
| **Persistence Layer** | In-Query JSON Parsing | Fast MySQL `JSON_EXTRACT()` queries for instant decoding | `API/Read/message.php`, `API/Create/message.php` |
| **Persistence Layer** | Relational Integrity | Foreign keys with `CASCADE DELETE` preventing orphaned data | `lifeline_updated.sql` |

---

*LifeLine Emergency Response System — Resilient Off-Grid Communication for Rural & Remote Nepal.*
