# 🚨 LifeLine Command Center Portal — Complete Features Specification

> **Official Feature Documentation & Technical Reference for the LifeLine Emergency Operations Command Portal**  
> **Live Portal URL:** [https://zenithkandel.com.np/lifeline/portal/](https://zenithkandel.com.np/lifeline/portal/)  
> **Target System:** LifeLine Off-Grid Emergency Response & Search-and-Rescue (SAR) System  
> **Target Deployment Region:** Solukhumbu / Himalayan Remote Regions, Nepal  

---

## 📑 Table of Contents

1. [Executive Overview](#1-executive-overview)
2. [Portal Architecture & Shell Design](#2-portal-architecture--shell-design)
3. [Module 1: Live Operations Dashboard (`dashboard.php`)](#3-module-1-live-operations-dashboard-dashboardphp)
4. [Module 2: Emergency Alerts & Incident Log (`messages.php`)](#4-module-2-emergency-alerts--incident-log-messagesphp)
5. [Module 3: Two-Way Radio Dispatch Console (`dispatch.php`)](#5-module-3-two-way-radio-dispatch-console-dispatchphp)
6. [Module 4: Field Nodes & Gateway Fleet Management (`devices.php`)](#6-module-4-field-nodes--gateway-fleet-management-devicesphp)
7. [Module 5: Rescue Operations & Response Teams (`helps.php`)](#7-module-5-rescue-operations--response-teams-helpsphp)
8. [Module 6: Code Mappings & Geolocation Taxonomy (`mapping.php`)](#8-module-6-code-mappings--geolocation-taxonomy-mappingphp)
9. [Module 7: Emergency Email Alerts & System Thresholds (`emails.php`)](#9-module-7-emergency-email-alerts--system-thresholds-emailsphp)
10. [Multi-Channel Notification & Push Engine](#10-multi-channel-notification--push-engine)
11. [Visual Design, Living Motion & Responsive UX](#11-visual-design-living-motion--responsive-ux)
12. [Summary Feature Matrix](#12-summary-feature-matrix)

---

## 1. Executive Overview

The **LifeLine Portal** (`https://zenithkandel.com.np/lifeline/portal/`) is a centralized, real-time emergency response command center designed for search-and-rescue coordinators, disaster relief agencies, district administration offices, and emergency triage personnel.

While the field layer operates in rugged mountain terrain using ESP32 edge transceivers and LoRa (SX1278 433 MHz) radio packets without requiring GSM or internet connectivity, the **LifeLine Portal** bridges incoming edge radio telemetry into rich, actionable situational awareness and closed-loop two-way radio dispatch.

```
┌────────────────────────────────────────────────────────────────────────┐
│                        FIELD LAYER (OFF-GRID)                          │
│   [Handheld TX Pro] / [SPU Sensor Nodes] ──(LoRa RF 433MHz)──┐         │
│                                                              ▼         │
│                                                [RX Base Station Gateway]│
└──────────────────────────────────────────────────────────────┬─────────┘
                                                               │ HTTPS REST
                                                               ▼
┌────────────────────────────────────────────────────────────────────────┐
│               LIFELINE CLOUD COMMAND CENTER PORTAL                     │
│               (https://zenithkandel.com.np/lifeline/portal/)           │
│                                                                        │
│   ┌────────────────────┐  ┌────────────────────┐  ┌──────────────────┐ │
│   │   Live Dashboard   │  │  Emergency Alerts  │  │  Radio Dispatch  │ │
│   │   • Real-Time KPIs │  │  • Incident Audit  │  │  • Downlink TX   │ │
│   │   • Mesh Stations  │  │  • Multi-Filter    │  │  • Field SITREPs │ │
│   │   • Google Maps    │  │  • SOS Simulation  │  │  • LoRa Presets  │ │
│   └────────────────────┘  └────────────────────┘  └──────────────────┘ │
│   ┌────────────────────┐  ┌────────────────────┐  ┌──────────────────┐ │
│   │  Field Fleet Nodes │  │  Rescue Responders │  │  Taxonomy Engine │ │
│   │  • Node CRUD       │  │  • SAR Agencies    │  │  • JSON Location │ │
│   │  • Ping Heartbeat  │  │  • Readiness State │  │  • Disaster Code │ │
│   │  • RF Diagnostics  │  │  • Tel Click-Dial  │  │  • Help Matrices │ │
│   └────────────────────┘  └────────────────────┘  └──────────────────┘ │
│   ┌────────────────────┐  ┌────────────────────┐  ┌──────────────────┐ │
│   │ Email Alert Config │  │ Firebase Web Push  │  │ System Thresholds│ │
│   │  • PHPMailer SMTP  │  │  • FCM v1 Engine   │  │  • Sensor Alarms │ │
│   │  • Stakeholders    │  │  • Service Worker  │  │  • Polling Rate  │ │
│   └────────────────────┘  └────────────────────┘  └──────────────────┘ │
└────────────────────────────────────────────────────────────────────────┘
```

---

## 2. Portal Architecture & Shell Design

**Source Files:** `index.php`, `css/index.css`, `css/shared.css`, `js/index.js`, `js/motion.js`

### 2.1 Unified SPA-Style Shell & Navigation
- **Dynamic Content Canvas**: Operates as a seamless single-page application (SPA) shell utilizing an integrated dynamic iframe canvas (`#content-frame`). Navigation between modules occurs without page reloads, preserving background alert streams and connection states.
- **Deep Linking & URL History Sync**: Implements `history.pushState` and `window.onpopstate` synchronization so browser back/forward buttons work naturally while bookmarking specific views.
- **Session Persistence**: Remembers the user's last visited module via browser `localStorage` (`lifeline-page`) and automatically restores it upon return.

### 2.2 Obsidian Tactical Sidebar & Mobile Navigation
- **Desktop Sidebar**: High-contrast, obsidian floating navigation with branded icons, status markers, and route navigation (`Dashboard`, `Alerts`, `Radio Dispatch`, `Field Nodes`, `Responders`, `Code Mappings`, `Email Config`).
- **Mobile Responsive Drawer / Bottom Dock**: Automatically transforms into a bottom-anchored, touch-friendly navigation dock (`lifeline-mobile-nav`) on screens under 768px.
- **Full Viewport Shell Dimmers**: Triple-layer modal dimmers (`#shell-sidebar-dimmer`, `#shell-topbar-dimmer`, `#shell-bottom-dimmer`) guarantee that when modals open within the child canvas, the entire viewport (including top bar and sidebar) dims uniformly.

### 2.3 Live System Connection Heartbeat & Badges
- **Real-Time Pulsing Status Pill**: Visual network indicator displaying:
  - 🟢 **Pulsing Green ("Listening...")**: Active background polling and healthy connection to REST backend.
  - 🔴 **Red ("Disconnected")**: Network interrupt or API unreachable warning.
- **Active Incident Counter Badge**: Real-time counter badge on the Alerts navigation item reflecting unresolved emergency counts.

### 2.4 Commander Profile & Dual Theme Engine
- **Commander Popover**: Displays logged-in officer profile (e.g. `Admin`, `admin@lifeline.org.np`) with one-click theme switcher, quick email settings navigation, and session logout.
- **Dual Theme Support (Light / Dark)**:
  - Instantly toggles between high-readability daylight theme and low-light tactical dark theme.
  - Cross-frame theme sync: Custom event broadcast (`themeChanged`) and postMessage pipeline synchronizes CSS variables across parent shell and active iframe within 16ms.
  - User preference stored in `localStorage` (`lifeline-theme`).

---

## 3. Module 1: Live Operations Dashboard (`dashboard.php`)

**Source Files:** `dashboard.php`, `css/dashboard.css`, `js/dashboard.js`

The central nerve center providing real-time situational awareness for incident commanders.

### 3.1 Real-Time KPI Metric Cards
- **Active Emergencies Card**: Displays count of unresolved critical field alarms. Card dynamically switches color to emergency crimson when active incidents exist, with 60 FPS count-up animations.
- **Online Field Nodes Card**: Reports total count of active field transceivers and LoRa gateways transmitting pings within threshold intervals.
- **Ready Responders Card**: Real-time count of rescue teams, ground search units, and medical triage teams marked as `available`.

### 3.2 Automated 5-Second Telemetry Polling Engine
- Background asynchronous polling cycle (`setInterval` at 5000ms) that queries `/API/Read/message.php`, `/API/Read/device.php`, and `/API/Read/helps.php`.
- Evaluates `lastMessageId` deltas to detect new incoming emergency transmissions instantaneously without screen flicker or full page reloads.

### 3.3 Field Stations & Telemetry Nodes Grid
- Live status matrix displaying all deployed village nodes (e.g., Namche, Lukla, Gorak Shep, Dingboche).
- Displays node operational badge (`Online` / `Offline`), battery voltage status, relative ping time ("2m ago"), and direct jump link to full device management.

### 3.4 Live Radio Transmissions Feed Table
- **Station Identification**: Shows Node Name, Hardware Device ID (`DID`), and geographic village tag.
- **Incident / SITREP Badge**: Displays color-coded emergency severity:
  - 🔴 **Critical Emergency**: Codes 1–9 (Altitude Sickness, Severe Injury, Avalanche, Landslide, Fire, SAR).
  - 🟡 **Warning / Advisory**: Codes 10–14 (Equipment Failure, Weather Alert, Infrastructure, Comms Lost).
  - 🟢 **Normal / All Clear**: Code 15 (Situation Normal).
  - 💬 **Custom SITREP**: Decoded field text transmission.
- **Signal Telemetry Readout**: Displays Received Signal Strength Indicator (**RSSI** in dBm), Signal-to-Noise Ratio (**SNR** in dB), and estimated transmission distance in kilometers.
- **Timestamp**: Humanized relative time ("Just now", "4m ago") with hover tooltip showing exact date/time stamp.

### 3.5 Interactive Incident Diagnostic Modal
Clicking any alert opens an in-depth diagnostic panel:
1. **Full Telemetry Breakdown**: Incident ID, originating station, exact GPS/settlement name, timestamp, and RF reception metrics.
2. **Interactive Google Maps Embed**: Generates an interactive map iframe focused directly on the village coordinates in Nepal.
3. **Automated Intelligent Responder Matching**:
   - Cross-references incoming emergency codes with registered agency capabilities in the `indexes` taxonomy table.
   - Automatically surfaces only rescue teams trained and equipped for that specific disaster (e.g., matching Nepal Army Helicopter Units for Avalanches or Altitude Sickness, clinics for Trauma).
   - Displays unit base location, ETA metric, and clickable phone link (`tel:`).
4. **Action Handlers**:
   - "Open in Google Maps": Launches full route directions in an external window.
   - "Mark Resolved": Updates incident state in the database, sends resolution notification, and clears the active dashboard badge.

---

## 4. Module 2: Emergency Alerts & Incident Log (`messages.php`)

**Source Files:** `messages.php`, `css/messages.css`, `js/messages.js`

A complete tactical incident log and historical audit trail of all LoRa radio transmissions.

### 4.1 Filter Tabs & Incident Statistics
- **All Transmissions Tab**: Complete log of all RF signals received by any gateway.
- **Active SOS Incidents Tab**: Filtered view isolating unresolved critical emergencies requiring response.
- **Resolved History Tab**: Archive of cleared and completed SAR missions.

### 4.2 Multi-Parameter Search & Faceted Filtering
- **Full-Text Live Search**: Instant filtering across node names, settlement locations, and message contents.
- **Device Filter Dropdown**: Isolate messages originating from a specific handheld unit (e.g. `Node-Namche-01`).
- **Disaster Type Dropdown**: Filter by specific emergency classification (e.g. Avalanche vs. Landslide vs. High Altitude Pulmonary Edema).
- **Signal Quality Indicators**: Color-coded RF badges:
  - 🟢 Strong (`>= -70 dBm`)
  - 🟡 Medium (`-71 to -95 dBm`)
  - 🔴 Weak (`< -95 dBm`)

### 4.3 Manual Emergency Simulation & Field Drill Tool
- **"New Alert" Modal**: Built-in simulator for emergency drills, training, and manual incident logging:
  - **Device Selector**: Quick-select visual chips for registered field nodes.
  - **Disaster Category Selector**: Visual icon chips for all 15 standardized emergency codes.
  - **Signal Strength Presets**: One-click RSSI buttons (`Strong -55 dBm`, `Medium -78 dBm`, `Weak -108 dBm`) or custom input.
  - Submits payload to `/API/Create/message.php`, triggering multi-channel email alerts and Web Push notifications identically to a real LoRa transmission.

### 4.4 Incident Resolution & Management Actions
- **Single-Click Resolve**: Resolves alerts inline or via modal.
- **Single & Bulk Deletion**: Safely delete individual test messages or batch-delete records with confirmation safety modals.

---

## 5. Module 3: Two-Way Radio Dispatch Console (`dispatch.php`)

**Source Files:** `dispatch.php`, `css/dispatch.css`, `js/dispatch.js`

Enables closed-loop, two-way tactical communication from the web console down to off-grid field units.

### 5.1 Web-to-LoRa Downlink Transmission Pipeline
- Allows command center operators to type messages or instructions and inject them into the LoRa network.
- Commands are queued in the database table (`downlink_commands`) and polled every 3 seconds by the LifeLine RX Pro gateway via `/API/Read/pending_commands.php`.
- The gateway broadcasts the packet over 433 MHz LoRa; field handheld units (`lifeline_tx_pro`) receive and display it on their ST7789 TFT screens and companion mobile apps.

### 5.2 Target Field Unit Selector
- **Broadcast to All Units**: Transmit to `TX #000` for general area broadcast.
- **Targeted Addressing**: Interactive chips to direct instructions to specific field units (`TX #001`, `TX #002`, etc.) loaded dynamically from the device registry.

### 5.3 Quick Action Preset Chips
One-click pre-formatted downlink SITREPs:
- **`Custom SITREP` (`MSG`)**: Freeform operator instruction.
- **`Search Dispatched` (`DISPATCH`)**: Auto-fills `"SAR Team dispatched to your coordinates. Hold position."`
- **`Paramedic En Route` (`MEDIC`)**: Auto-fills `"Medical team en route with oxygen and trauma kit."`
- **`Evacuate Sector` (`EVAC`)**: Auto-fills `"URGENT: Evacuate sector immediately toward base camp."`
- **`Diagnostic Ping` (`PING`)**: Sends an RF telemetry ping to verify link connectivity.

### 5.4 Character Budget & Radio Feed Monitor
- **Payload Constraint Meter**: Real-time character counter badge (`0 / 48 chars` or max 96) to prevent packet fragmentation over low-bandwidth LoRa modulation.
- **Live Radio Transmission Stream**: Feed of recent downlinks and uplinks with search filter and refresh trigger.

---

## 6. Module 4: Field Nodes & Gateway Fleet Management (`devices.php`)

**Source Files:** `devices.php`, `css/devices.css`, `js/devices.js`

Full hardware lifecycle management for all deployed edge transceivers and gateway base stations.

### 6.1 Fleet Health Metrics & Telemetry
- Stat counters: **Total Registered Nodes**, **Active Nodes**, **Standby Nodes**, and **Maintenance Nodes**.
- Automated Ping Detection: Nodes automatically update their `last_ping` timestamp whenever their RF packet reaches any gateway.

### 6.2 Full CRUD Fleet Administration
- **Register New Node**: Modal form configuring Device Name, Hardware Device ID (`DID` 1–999), Assigned Settlement / Location, and operational state.
- **Edit Node**: Reassign node coordinates/villages, update naming, or alter mode.
- **Delete Node**: Remove decommissioned units with safety confirmation modal.

### 6.3 Fast Inline Status Toggles
- Quick-action buttons to toggle node modes:
  - `Set Active`: Put node into full emergency monitoring.
  - `Standby`: Power-save or off-season monitoring.
  - `Maintenance`: Flag node for battery replacement or sensor calibration.

### 6.4 Faceted Filtering & Search
- Live instant text search by node label, DID, or village name.
- Status filter dropdown (`All`, `Active`, `Inactive`, `Maintenance`).
- Location filter dropdown populated dynamically from the location index.

---

## 7. Module 5: Rescue Operations & Response Teams (`helps.php`)

**Source Files:** `helps.php`, `css/helps.css`, `js/helps.js`

The centralized directory of search-and-rescue agencies, medical evacuation units, and first responders.

### 7.1 Response Agency Registry
- Tracks organizations such as:
  - Nepal Army Helicopter SAR Wing
  - Himalayan Rescue Association (HRA)
  - Everest ER Clinic (Pheriche / Base Camp)
  - Khumbu Local Sherpa Rescue Group
  - Nepal Red Cross Disaster Response Team

### 7.2 Capability-Based Category Filtering
Filter response agencies by specialized disaster discipline:
- 🌊 **Swift Water & Flood**
- 🏔️ **Landslide & Slope SAR**
- 🔥 **Fire & Hazmat**
- 🏚️ **Earthquake & Structural Collapse**
- 🚁 **Medical Triage & Air Evac**

### 7.3 Live Readiness Status Toggles
- Responders categorized into:
  - 🟢 **Available**: Ready for immediate deployment.
  - 🟡 **Dispatched**: Currently en route to an incident.
  - 🔴 **Busy**: Committed to active rescue operations.
- Single-click interactive toggle directly from agency cards to update status in real time.

### 7.4 Capability Matrix & Click-to-Call
- **Emergency Capability Assignment**: Multi-select pills assigning exactly which disaster codes (1 to 15) each agency handles.
- **Direct Click-to-Call**: Interactive `tel:` telephone links allowing dispatchers on tablets or phones to place instant rescue calls.
- **ETA Metric Display**: Shows estimated arrival timeframe (e.g. "30–45 min Heli", "2–3 hrs Foot").

---

## 8. Module 6: Code Mappings & Geolocation Taxonomy (`mapping.php`)

**Source Files:** `mapping.php`, `css/mapping.css`, `js/mapping.js`

The intelligence engine responsible for **low-bandwidth integer packet decompression**.

### 8.1 Why Mapping Exists
To conserve battery and maximize radio range in extreme terrain, LifeLine edge transceivers do not send heavy strings. They send raw integer codes (e.g., `TX003,1` = Device 3, Emergency 1). The Mapping Engine translates these integers into rich descriptive data on the fly without schema migrations.

### 8.2 Three Standard Taxonomy Categories
1. **`location` (Himalayan Settlements & Coordinates)**:
   - Maps integer keys (1–15+) to geographical locations across the Khumbu / Solukhumbu region:
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
2. **`message` (Emergency Disaster Classifications)**:
   - Maps integer keys (1–15) to emergency descriptions:
     - `1`: Medical Emergency - Altitude Sickness (HAPE / HACE)
     - `2`: Medical Emergency - Severe Trauma / Injury
     - `3`: Medical Emergency - Acute Illness
     - `4`: Search and Rescue Required
     - `5`: Avalanche Alert
     - `6`: Landslide Warning
     - `7`: Fire Emergency
     - `8`: Flood / Glacial Lake Outburst (GLOF)
     - `9`: Lost / Missing Mountaineer
     - `10`: Critical Equipment Failure
     - `11`: Severe Mountain Weather Alert (Blizzard)
     - `12`: Infrastructure / Bridge Damage
     - `13`: Food / Clean Water Shortage
     - `14`: Base Comms Failure
     - `15`: All Clear - Situation Normal
3. **`help` (Agency Capability Matrix)**:
   - Maps Agency IDs to JSON arrays of emergency codes they are equipped to service (e.g., `HID 1` ➔ `[1, 2, 4, 5, 9]`).

### 8.3 Taxonomy Builder & JSON Editor
- **Card-Based Visual Presentation**: All key-value dictionaries rendered as visual badges.
- **Dynamic Entry Editor**: Add new keys, modify existing definitions, or delete deprecated codes.
- **Zero-Downtime JSON Serialization**: Stores data natively in MariaDB/MySQL `JSON` columns, enabling instant updates without database migrations.

---

## 9. Module 7: Emergency Email Alerts & System Thresholds (`emails.php`)

**Source Files:** `emails.php`, `css/emails.css`, `js/emails.js`

Manages automated stakeholder broadcasts and physical sensor alarm thresholds.

### 9.1 Emergency Email Subscriber Registry
- Centralized recipient list of emergency personnel, heli dispatch desks, hospital triage coordinators, and government authorities.
- Automated Broadcast Integration: Every email registered here automatically receives a high-priority HTML emergency alert email via PHPMailer whenever a critical incident is ingested.
- Subscriber CRUD: Add single or bulk email addresses, edit records, and remove subscribers.

### 9.2 Instant Live Test Alert Dispatcher
- **"Send Test Alert" Modal**: Sends a live test email through the server's SMTP transport pipeline to verify mail routing, spam score, and formatting.

### 9.3 System Sensor Threshold Configuration
- **"System Thresholds" Modal**: Configures environmental sensor parameters:
  - **Telemetry Polling Rate (seconds)**: Frequency of gateway check-ins.
  - **Max Temperature Threshold (°C)**: Overheat/fire trigger level.
  - **Max Humidity Threshold (%)**: Flash rain / storm detection level.
  - **Max Toxic Gas / Smoke Threshold (PPM)**: Gas leak / smoke detection level.
- Saved into the centralized `config_handler.php` backend for global synchronization.

---

## 10. Multi-Channel Notification & Push Engine

### 10.1 PHPMailer SMTP Emergency Dispatcher
- Triggered automatically by `/API/Create/message.php`.
- Assembles a high-contrast HTML email alert containing:
  - Emergency Classification & Severity Badge.
  - Originating Device Name & Village Location.
  - RSSI Signal Telemetry & Distance Estimate.
  - Direct deep link to open the incident on the LifeLine Portal.
  - Automatic dispatch to all active subscribers registered in `emails.php`.

### 10.2 Firebase Cloud Messaging (FCM) v1 Web Push
- **Client-Side Worker**: Service worker (`sw.js`) registered in the portal shell.
- **Token Management**: Generates browser push token and registers it via `/API/Create/fcm_token.php`.
- **Background Push Alerts**: Delivers instant native OS popups and audible chimes to desktop and mobile browsers even when the portal tab is minimized or in the background.

---

## 11. Visual Design, Living Motion & Responsive UX

### 11.1 Humanitarian Tactical Design Palette
- **High Contrast & Clarity**: Engineered specifically for high-stress disaster command centers and daylight field readability.
- **Slate & Obsidian Theme**: Avoids generic neon gradients, using disciplined slate greys, deep charcoal dark mode backgrounds, and emergency crimson/amber accents.
- **Typography**: Inter for crisp interface typography, paired with JetBrains Mono for telemetry, RSSI, and radio codes.

### 11.2 Living Motion Engine (`motion.js`)
- **60 FPS Number Interpolation**: Animated count-ups with cubic ease-out curves when metric values update.
- **Interactive Click Ripple**: Fluid expanding water ripples on button clicks.
- **Staggered Entrance Physics**: Cascading entry animations for tables and station grids using dynamic CSS `--i` variables.
- **Universal LifeLine Toast System**: Floating status toasts with integrated countdown progress bars.

---

## 12. Summary Feature Matrix

| Feature / Capability | Portal Module | Primary API Endpoint | Description |
| :--- | :--- | :--- | :--- |
| **Real-Time KPI Telemetry** | Dashboard | `/API/Read/message.php` | Active emergencies, online nodes, ready responders |
| **5s Background Polling** | Dashboard | `/API/Read/message.php` | Delta tracking detecting new field transmissions |
| **Station Mesh Monitor** | Dashboard | `/API/Read/device.php` | Visual status of all deployed field stations & batteries |
| **Google Maps Incident View** | Dashboard & Alerts | Embedded Iframe | Centers on coordinates of reporting Himalayan settlement |
| **Auto Responder Matching** | Dashboard | `/API/Read/helps.php` | Cross-references emergency code to qualified SAR teams |
| **Emergency Audit Log** | Alerts | `/API/Read/message.php` | Full historical audit log with pagination |
| **Multi-Parameter Filtering** | Alerts & Devices | Client-Side / API | Instant filter by text, node, status, and disaster code |
| **Drill / SOS Simulator** | Alerts | `/API/Create/message.php` | Manual simulation tool with device, code, and RSSI presets |
| **Incident Resolution** | Dashboard & Alerts | `/API/Update/message.php` | One-click clearance of active incidents in database |
| **LoRa Downlink Injection** | Radio Dispatch | `/API/Create/command.php` | Queue Web-to-LoRa tactical messages |
| **Gateway Downlink Polling** | Radio Dispatch | `/API/Read/pending_commands.php` | Gateway fetches queued commands every 3s |
| **Action Preset SITREPs** | Radio Dispatch | Client-Side Presets | One-click quick presets (SAR Dispatched, Medic, Evac, Ping) |
| **Fleet Lifecycle (CRUD)** | Field Nodes | `/API/Create/device.php` | Register, edit, and decommission field nodes & gateways |
| **Automated Ping Tracking** | Field Nodes | `/API/Create/message.php` | Automatically tracks node uptime from LoRa uplinks |
| **Agency Directory & ETA** | Responders | `/API/Read/helps.php` | Directory of heli units, clinics, and sherpa SAR teams |
| **Readiness Status Toggle** | Responders | `/API/Update/helps.php` | Toggle between Available, Dispatched, and Busy |
| **Click-to-Call Dispatch** | Responders | HTML `tel:` protocol | Direct phone dialing for rescue coordinators |
| **Integer Decompression** | Code Mappings | `/API/Read/index.php` | Translates raw LoRa integer codes into rich labels |
| **JSON Taxonomy Editor** | Code Mappings | `/API/Create/index.php` | Dynamic editor for locations, alerts, and help matrices |
| **Automated Email Dispatch**| Email Config | `/API/Create/emails.php` | PHPMailer SMTP high-priority HTML incident alerts |
| **Live SMTP Mail Test** | Email Config | `/API/email_helper.php` | One-click email pipeline test modal |
| **Sensor Threshold Alarms** | Email Config | `/API/config_handler.php` | Configures max temp, humidity, gas, and polling rates |
| **FCM v1 Web Push** | Portal Shell | `/API/Create/fcm_token.php` | Browser background push notifications via Service Worker |
| **Dual Theme Engine** | Global Shell | `localStorage` Sync | Instant Dark / Light mode with cross-iframe synchronization |
| **Obsidian / Mobile Nav** | Global Shell | SPA Architecture | Dynamic iframe canvas with desktop sidebar and mobile dock |

---
*Generated for the LifeLine Emergency System — [https://zenithkandel.com.np/lifeline/portal/](https://zenithkandel.com.np/lifeline/portal/)*
