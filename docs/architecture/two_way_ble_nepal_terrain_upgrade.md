# LifeLine v4.0 — Two-Way LoRa Communication, BLE Smartphone Integration & Nepal Mountain Disaster Resilience Specification

> **System Architecture & Technical Engineering Specification**  
> **Target Units:** LifeLine TX Pro (Field Communicator), LifeLine RX Pro (Base Station), LifeLine SPU/CCU (Autonomous Ridge Relay)  
> **RF Core:** SX1278 LoRa @ 433 MHz | **Microcontroller:** ESP32 DevKit V1 (Xtensa Dual-Core)  
> **Wireless Companion:** Bluetooth Low Energy (NimBLE GATT / Nordic UART Service)

---

## 📌 Executive Summary

This engineering specification defines the **v4.0 upgrade** for the **LifeLine Off-Grid Disaster Communication System**. It is custom-tailored to the unforgiving geographical, environmental, and infrastructural realities of **Nepal** — specifically deep river gorges, steep Himalayan ridges, monsoon landslide cutoffs, high-altitude freezing temperatures, and total cellular/grid collapse during major earthquakes.

### Core Upgrade Pillars:
1. **Guaranteed Two-Way LoRa Communication (Time-Division Duplexing ACK)**: Replaces one-way blind transmission with a closed-loop acknowledgment and reverse command channel between field units and base stations.
2. **Dual-Unit Bluetooth Low Energy (BLE) Mobile Integration**: Enables offline Android/iOS smartphones to connect seamlessly to both transmitters and base stations for freeform two-way emergency text chat, offline topographic map plotting, and field telemetry triage without requiring an internet connection or cellular signal.
3. **Nepal Mountain & Disaster Resilience Enhancements**: Incorporates autonomous ridge-top store-and-forward mesh repeaters, high-altitude sub-zero cold battery protection, localized Nepali UI audio/visual cues, waterproof ultrasonic GLOF/flash flood detection, and a zero-install Web Bluetooth PWA companion.

---

## 1. Deep Field Reality Analysis: Nepal Terrain & Disaster Context

```text
═════════════════════════════════════════════════════════════════════════════════
                      NEPAL GEOGRAPHICAL & RF PROFILE
═════════════════════════════════════════════════════════════════════════════════

 Elevation
  5,000m+  ▲ High Passes (Thorong La, Gokyo, Manaslu)
           │ ❄ Sub-zero (-20°C): Severe Li-ion Battery Voltage Sag & Brownouts
           │ ════════════════════════════════════════════════════════════════════
  3,000m   │       ▲ Ridge Barrier (Knife-Edge Diffraction Loss, Solid Rock NLOS)
           │      / \
           │     /   \   [Autonomous SPU Ridge Relay]
  1,500m   │    /     \                      ▲
           │   /       \                    / \         [RX Pro Base Station]
           │  /         \                  /   \       (District HQ / Army Post)
   600m    │ /           \   Deep Gorge   /     \               ┌──────┐
           │/             \  (Bhotekoshi /       \──────────────┤ █ █  │
    60m    └───────────────\  Trishuli)           \             └──────┘
            Gorge Village   \──────────────────────┴─────────────────────────────
           [TX Pro Handheld]  Monsoon Landslides / Flash Floods / Zero Cellular
═════════════════════════════════════════════════════════════════════════════════
```

### 1.1 The "Pahar" & Gorge RF Propagation Problem
* **Topographical Extremes**: Nepal transitions from 60m elevation (Terai plains) to over 8,000m (Himalayas) across barely 150 km of latitude. Settlements are frequently situated in narrow river gorges (Trishuli, Bhotekoshi, Kali Gandaki, Marshyangdi) flanked by sheer 1,000m to 2,500m rocky mountain ridges.
* **Why 433 MHz LoRa is Essential**: At 433 MHz (wavelength $\lambda \approx 69\text{ cm}$), signals undergo significantly greater knife-edge diffraction over mountain crests and penetrate wet sub-tropical jungle foliage with far lower attenuation than 868/915 MHz ($\lambda \approx 34\text{ cm}$) or 2.4 GHz Wi-Fi ($\lambda \approx 12.5\text{ cm}$).
* **Multipath Reflection & Line-of-Sight Blockage**: While 433 MHz diffracts better, solid granite mountain walls completely block direct Line-of-Sight (NLOS). Signal reflections off canyon faces produce multi-path phase cancellation. Single-hop transmission is often impossible across adjacent valleys without **Ridge-Top Repeaters** and **Closed-Loop ACK Verification**.

### 1.2 Monsoon & Winter Environmental Extremes
* **Monsoon Season (June – September)**: Extreme cloudbursts cause slope liquefaction, triggering landslides that sever road corridors (Narayanghat-Mugling, Prithvi Highway, BP Highway). Relative humidity exceeds 95–100%, causing condensation inside non-sealed enclosures and dampening RF propagation.
* **Winter Freezing (-15°C to -25°C)**: In high trekking corridors (Namche, Manang, Mustang) and mountain passes, standard Lithium-Ion (18650) and LiPo batteries suffer an internal chemical slowdown. Capacity drops by 40–50%, and internal resistance ($R_{int}$) surges.
* **The Brownout Trap**: When the SX1278 transmits at maximum boosted power (+20 dBm), it pulls instantaneous current spikes of 120–140 mA. Combined with cold-induced battery resistance, the supply voltage temporarily collapses below the ESP32's internal brownout detector threshold (2.8V), causing continuous reset loops at the exact moment an emergency alert is triggered.

### 1.3 Total Infrastructure Collapse (Earthquake Reality)
* During major seismic events (2015 Gorkha 7.8 Mw, 2023 Jajarkot/West Rukum 6.4 Mw), cellular towers collapsed or drained their backup batteries within 4–6 hours. Optical fiber trunks along highways were severed by rockfalls.
* First responders (Nepal Army, Armed Police Force, Red Cross, local ward disaster management committees - LDMC) operate in total information blackout. The emergency system **must be 100% autonomous, self-contained, and operable without the internet, cellular towers, or grid power**.

---

## 2. Upgrade Pillar 1: Guaranteed Two-Way LoRa Communication

### 2.1 The Critical Flaw in One-Way Alerting
In the legacy implementation:
```cpp
// Legacy LoRaComm.cpp
LoRa.beginPacket();
LoRa.print(packet);
bool success = LoRa.endPacket(); // Only verifies transmission buffer was flushed!
```
The transmitter displays "TRANSMITTED OK" regardless of whether the base station was in a deep radio shadow, experiencing RF interference, or powered down. In life-or-death mountain rescue scenarios, this false confirmation gives a false sense of security while no rescue is underway.

### 2.2 Time-Division Duplexing (TDD) Protocol Architecture

Because the SX1278 transceiver is half-duplex (it can transmit or receive, but not both simultaneously), two-way communication requires a synchronized Time-Division Duplexing (TDD) cycle.

```text
 ┌───────────────────────────┐                       ┌───────────────────────────┐
 │   LifeLine TX Pro         │                       │    LifeLine RX Pro        │
 │   (Field Communicator)    │                       │    (Emergency Base Hub)   │
 └─────────────┬─────────────┘                       └─────────────┬─────────────┘
               │                                                   │
               │────── [1] UPLINK ALERT PACKET (Seq: #104) ───────>│
               │   Airtime (SF12, BW125k, 24 bytes): ~1.15s        │  - Parse Alert Code
               │                                                   │  - Actuate High-dB Siren
               ├─ Switch to LoRa.receive()                         │  - Render on 16x2 LCD
               ├─ Open ACK Listen Window (2500ms)                  │  - Post to Cloud API (if WiFi OK)
               │                                                   │
               │                                                   │  - Wait 120ms (turnaround)
               │                                                   │  - Prepare Downlink ACK Frame
               │                                                   │
               │<───── [2] DOWNLINK ACK PACKET (Seq: #104) ────────│
               │   Airtime (SF12, BW125k, 12 bytes): ~0.55s        │
               │                                                   │
        [ACK Received]                                             │
        - Validate Seq Number                                      │
        - Halt Retry Timer                                         │
        - Play Double Audio Chime                                  │
        - TFT: "BASE CONFIRMED: RESCUE EN ROUTE"                   │
        - Forward confirmation to paired smartphone via BLE        │
               │                                                   │
```

### 2.3 LoRa Airtime & Turnaround Timing Calculations
At current default RF settings (**Frequency: 433.0 MHz, SF: 12, BW: 125 kHz, CR: 4/8, Preamble: 8 symbols**):
* Symbol Duration:
  $$T_{\text{sym}} = \frac{2^{SF}}{BW} = \frac{2^{12}}{125,000} = 32.768\text{ ms}$$
* Uplink Packet Time-on-Air (ToA):
  $$T_{\text{uplink}} \approx (8 + 4.25 + 8 + \text{PayloadSymbols}) \times 32.768\text{ ms} \approx 1,148\text{ ms}$$
* Downlink ACK Time-on-Air (ToA) (Compact 12-byte payload):
  $$T_{\text{ack}} \approx 557\text{ ms}$$
* **Total Round-Trip Time (RTT)**:
  $$\text{RTT} = T_{\text{uplink}} + T_{\text{turnaround}} (120\text{ms}) + T_{\text{ack}} \approx 1,825\text{ ms}$$
* **ACK Timeout Window**: Configured to **$2,500\text{ ms}$** to allow for crystal drift and processing latency.

### 2.4 Binary Frame Structures (Packed C Structs)

To minimize airtime and maximize link budget in mountainous RF conditions, v4.0 introduces unified binary structures with CRC-16-CCITT validation.

#### A. Uplink Frame Structure (`LoRaUplinkFrame`)
```c
#pragma pack(push, 1)
typedef struct {
    uint8_t  magic[2];       // 'L', 'F' (LifeLine Frame)
    uint8_t  version;        // 0x04
    uint16_t seq_num;        // Monotonic sequence number (0 - 65535)
    uint8_t  node_id;        // Transmitter ID (e.g. 003)
    uint8_t  msg_type;       // 0x01: SOS Alert, 0x02: Full Telemetry, 0x03: Mobile Text Chat
    uint8_t  alert_code;     // 'A' to 'O' (Emergency, Landslide, Medical, etc.)
    int32_t  lat_deg_e7;     // GPS Latitude * 10,000,000 (0.011m resolution)
    int32_t  lon_deg_e7;     // GPS Longitude * 10,000,000
    int16_t  alt_meters;     // Altitude in meters
    uint8_t  battery_pct;    // 0 to 100%
    uint8_t  hop_count;      // Mesh hop counter (starts at 0)
    uint8_t  relay_node_id;  // ID of repeating node (0 if direct)
    char     text_msg[32];   // Optional freeform text typed on paired smartphone
    uint16_t crc16;          // CRC-16-CCITT over preceding bytes
} LoRaUplinkFrame;
#pragma pack(pop)
```

#### B. Downlink ACK & Command Structure (`LoRaDownlinkACK`)
```c
#pragma pack(push, 1)
typedef struct {
    uint8_t  magic[2];       // 'L', 'F'
    uint8_t  version;        // 0x04
    uint16_t ack_seq_num;    // Matches uplink seq_num being acknowledged
    uint8_t  base_id;        // Base station ID (e.g. 001)
    uint8_t  target_node_id; // Targeted field unit ID
    uint8_t  ack_status;     // Status Code (see below)
    int8_t   rssi_at_base;   // Signal strength received at base station (dBm)
    int8_t   snr_at_base;    // SNR received at base station (dB)
    char     cmd_text[32];   // Optional return text from base operator (e.g., "Rescue en route")
    uint16_t crc16;          // Checksum
} LoRaDownlinkACK;
#pragma pack(pop)
```

#### ACK Status Codes:
| Code | Byte Value | Description | Transmitter LCD & Mobile Display |
| :--- | :--- | :--- | :--- |
| `ACK_LOGGED` | `0x01` | Base station recorded alert automatically | `ACK: ALERT LOGGED AT BASE` |
| `ACK_DISPATCHED` | `0x02` | Human operator dispatched rescue squad | `RESCUE TEAM DISPATCHED!` |
| `ACK_STANDBY` | `0x03` | Responders reviewing situation | `HOLD POSITION - STANDBY` |
| `ACK_EVACUATE` | `0x04` | Imminent hazard (dam breach, fire spread) | `URGENT: EVACUATE TO HIGH GROUND` |
| `ACK_MEDICAL` | `0x05` | Medical triage guidance attached | `MED TEAM EN ROUTE: KEEP WARM` |

### 2.5 Collision-Avoidance: Exponential Backoff with Jitter
During a major earthquake or landslide, multiple transmitters may attempt to signal the base station simultaneously. If nodes retransmit on fixed timers, their signals will repeatedly collide in the air (pure ALOHA collision).

v4.0 implements **Slotted Exponential Backoff with Random Jitter**:
$$\text{BackoffTime} = (\text{BASE\_BACKOFF} \times 2^{\text{retry}}) + \text{random}(200, 750)\text{ ms}$$
* Maximum retries: 3 attempts.
* If all 3 attempts fail without ACK:
  1. The transmitter displays: `ACK TIMEOUT — NO BASE CONFIRMATION`.
  2. Acoustic warning alert is sounded.
  3. Visual guidance prompt: `MOVE TO HIGHER GROUND / RIDGELINE FOR LOS`.
  4. The packet is cached in non-volatile flash storage and retried automatically every 60 seconds until confirmed.

---

## 3. Upgrade Pillar 2: Dual-Unit Bluetooth Low Energy (BLE) Integration

```text
═════════════════════════════════════════════════════════════════════════════════
                     LIFELINE DUAL-UNIT BLE TOPOLOGY
═════════════════════════════════════════════════════════════════════════════════

   FIELD SECTOR (Disaster Site)                   COMMAND SECTOR (Base Station)
  ┌─────────────────────────────┐               ┌─────────────────────────────┐
  │      Victim / Rescuer       │               │   Base Disaster Coordinator │
  │         Smartphone          │               │         Smartphone          │
  │    (Android / iOS / PWA)    │               │    (Android / iOS / PWA)    │
  └──────────────┬──────────────┘               └──────────────┬──────────────┘
                 │ BLE Nordic UART                             │ BLE Nordic UART
                 │ (Offline Chat & Maps)                       │ (Triage & Dispatch)
                 ▼                                             ▼
  ┌─────────────────────────────┐               ┌─────────────────────────────┐
  │       LifeLine TX Pro       │  LoRa 433MHz  │       LifeLine RX Pro       │
  │   (ST7789 TFT + Keypad)     │◄─────────────►│    (16x2 LCD + Loud Siren)  │
  └─────────────────────────────┘    2-Way Link └──────────────┬──────────────┘
                                                               │ HTTPS REST (if alive)
                                                               ▼
                                                ┌─────────────────────────────┐
                                                │    Central Cloud Web App    │
                                                └─────────────────────────────┘
═════════════════════════════════════════════════════════════════════════════════
```

### 3.1 Why NimBLE-Arduino Over Classic Bluetooth?
1. **iOS Compatibility**: Bluetooth Classic Serial Port Profile (SPP) is strictly blocked on Apple iOS devices without expensive Apple MFi hardware coprocessors. Bluetooth Low Energy (BLE GATT) works out-of-the-box on **both Android and iPhone**.
2. **Flash & RAM Footprint**: The ESP32's default Bluedroid stack requires ~950 KB of Flash and over 180 KB of RAM. **NimBLE-Arduino** reduces Flash consumption by ~350 KB and RAM usage to under 45 KB, leaving ample memory for Wi-Fi, SPIFFS, and graphics rendering.
3. **Radio Coexistence (Wi-Fi + BLE on Base Station)**:
   The ESP32 possesses a single shared 2.4 GHz RF circuit. While the base station uses Wi-Fi to push data to the cloud, NimBLE allows seamless time-multiplexed coexistence without dropping connections.

### 3.2 GATT Profile: Nordic UART Service (NUS)
Both units implement the industry-standard Nordic UART Service GATT specification, ensuring instant compatibility with standard BLE terminal apps (nRF Connect, Serial Bluetooth Terminal) and custom web apps:
* **Primary Service UUID**: `6E400001-B5A3-F393-E0A9-E50E24DCCA9E`
* **RX Characteristic (Write Without Response)**: `6E400002-B5A3-F393-E0A9-E50E24DCCA9E`
  * Data stream: Mobile Phone $\rightarrow$ LifeLine Unit
* **TX Characteristic (Notify)**: `6E400003-B5A3-F393-E0A9-E50E24DCCA9E`
  * Data stream: LifeLine Unit $\rightarrow$ Mobile Phone

### 3.3 Field Transmitter Unit (`TX Pro` + Mobile) Capabilities
1. **Freeform Two-Way Emergency Messaging**:
   Keypads only allow selecting broad categories (e.g. `1: EMERGENCY`, `2: MEDICAL`). With a smartphone connected:
   * The user types exact situation reports: *"3 people trapped in school collapse, 1 elderly with severe bleeding, need plasma & stretcher"*.
   * The message is packaged into `LoRaUplinkFrame.text_msg` and transmitted over 433 MHz LoRa.
2. **Offline Topographic GPS Map Rendering**:
   * TX Pro transmits its parsed GPS coordinates to the phone over BLE.
   * The phone renders the exact pin on offline cached topographic maps (OpenStreetMap / Mapbox offline pack) showing hiking trails, contour lines, nearby water sources, and distance/bearing to the nearest road.
3. **Live RF Health & Battery Telemetry**:
   * Phone displays real-time battery voltage, packet airtime, transmission counter, and LoRa signal strength.
4. **On-Device Interactive Modal Pop Screen ("MESSAGE SENDING")**:
   * When a paired smartphone issues a custom BLE message, the ST7789 display interrupts its active screen and presents a dedicated modal.
   * Renders a glowing cyan beacon, word-wrapped sitrep body, live ~20 FPS sweeping progress bar covering airtime and 5,000ms ACK wait, and closed-loop confirmation outcome badge (`[ACK CONFIRMED!]` / `[SENT (NO ACK)]`).
   * Auto-dismisses after 4 seconds or on keypress, restoring the previous screen.

### 3.4 Base Station Unit (`RX Pro` + Mobile) Capabilities
1. **Total Off-Grid Command Center**:
   * During an earthquake, when the base station's local broadband Wi-Fi router is dead, the base station is traditionally isolated from computers.
   * With BLE, the disaster coordinator (e.g., Armed Police Force post chief or ward chairman) pairs their smartphone directly to the RX Pro.
   * Incoming distress alerts, GPS locations, and sensor data appear immediately on their mobile screen.
2. **Reverse Command Dispatch**:
   * The coordinator selects an alert on their phone, selects `DISPATCHED`, types *"APF Squad airborne from Pokhara, ETA 25 minutes"*, and hits Send.
   * RX Pro broadcasts this downlink ACK over LoRa directly to the victim's field unit.
3. **Store-and-Forward Cloud Sync**:
   * The coordinator's phone caches all distress logs locally in SQLite / IndexedDB.
   * The moment the coordinator walks to a ridge with cellular signal, or connects to an emergency satellite terminal (Starlink), the mobile app automatically syncs all incident logs to the LifeLine Cloud Dashboard.
4. **Full 16×2 LCD Custom Message Mode**:
   * Dedicates both Row 0 and Row 1 (all 32 characters) entirely to incoming message text.
   * Features smart word-wrapping (`formatLCDTwoRows`), 4s hands-free auto-paging for messages > 28 chars, and manual page scroll via GPIO 14 button.

### 3.5 Companion Client Tier: Native Mobile App & Zero-Install Web PWA
* **LifeLine Companion Native Mobile App (`lifeline_companion/`)**:
  * React Native & Expo TypeScript application for iOS and Android.
  * 60 FPS sweeping Radar HUD with dynamic distance estimation and RSSI metrics.
  * 48-character LoRa SITREP composer with emergency macros and closed-loop ACK status banner.
  * Offline Mock BLE simulation engine for training without physical hardware.
* **Zero-Install Web Bluetooth Companion App (PWA)**:
  * In disaster zones where internet is down, single-file HTML5/JS PWAs (`bluefy_companion/index.html` & `docs/companion_app/`) run directly in Google Chrome (Android/Windows) and Bluefy Browser (iOS).
  * Fast optical BLE pairing via chassis QR code scanner (`portal_preview/scan_qr.html`).
  * **Zero internet connection required**.

---

## 4. Upgrade Pillar 3: Advanced Features for Nepal Mountain Conditions

### 4.1 Ridge-Top Store-and-Forward LoRa Mesh Repeater
* **The Challenge**: A village in the deep Bhotekoshi river gorge cannot reach the district base station across a 2,500m mountain ridge.
* **The Solution**: Transform autonomous SPU sensor nodes (which are typically deployed on ridge tops for landslide monitoring) into **Opportunistic Store-and-Forward Repeaters**:
  ```text
  [TX Pro: Gorge Bottom] ──(Hop 1)──> [SPU Ridge Node] ──(Hop 2)──> [RX Pro: Base Station]
  ```
  * When a node receives a frame with `hop_count < MAX_HOPS (2)`:
    1. Verifies CRC-16.
    2. Checks internal 16-entry circular cache: if `seq_num + node_id` was already repeated, it discards the frame (prevents infinite broadcast loops).
    3. Increments `hop_count`, sets `relay_node_id = THIS_NODE_ID`.
    4. Applies a randomized jitter delay (100–350 ms) to avoid colliding with other potential repeaters.
    5. Re-broadcasts the frame over LoRa.
  * Extends operational range from 5 km in gorges to **over 35 km across mountain chains**.

### 4.2 High-Altitude Cold Climate Power Management
* **Dynamic RF Power Step-Down**:
  * An ADC channel continuously monitors battery terminal voltage under load.
  * If $V_{bat} \ge 3.65\text{V}$: LoRa operates at maximum power (+20 dBm PA_BOOST).
  * If $V_{bat} < 3.40\text{V}$ (severe cold voltage sag): LoRa automatically throttles to +14 dBm and increases Spreading Factor (SF10 $\rightarrow$ SF12) to conserve link budget without drawing excessive current spikes.
* **Hardware Supercapacitor Rail Buffer**:
  * Recommendation: Place a $1000\,\mu\text{F}$ low-ESR solid electrolytic capacitor or $0.47\text{F}$ 5.5V supercapacitor directly across the SX1278 3.3V power rails.
  * Buffers the 120 mA transmit current surge, preventing brownout resets down to -20°C.
* **Low-Dropout Voltage Regulator**:
  * Replace traditional high-dropout regulators (e.g. AMS1117 with 1.2V dropout) with ultra-low dropout regulators (e.g., AP2112K or ME6211 with 250mV dropout). Allows full operation even when single-cell Li-ion drops to 3.55V.

### 4.3 Multilingual Localized UI (Nepali & English)
Many rural ward residents and village elders are not literate in English abbreviations.
* Dual-language toggle on the ST7789 display:
  * `LANDSLIDE` $\leftrightarrow$ `पहिरो (PAHIRO)`
  * `EARTHQUAKE` $\leftrightarrow$ `भूकम्प (BHUKAMPA)`
  * `FLASH FLOOD` $\leftrightarrow$ `बाढी (BAADHI)`
  * `FIRE HAZARD` $\leftrightarrow$ `आगो (AAGO)`
  * `MEDICAL SOS` $\leftrightarrow$ `बिरामी / उपचार (UPACHAAR)`
* **Acoustic Earcon Signaling**:
  Distinct acoustic buzzer cadence for illiterate users:
  * **Flood / Landslide**: 3 rising high-pitch sweeps (indicates: *Flee uphill*).
  * **Earthquake / Collapse**: 5 rapid staccato beeps.
  * **Fire**: Continuous warble tone.

### 4.4 Waterproof Ultrasonic GLOF & Flash Flood Early Warning (SPU Node)
* Glacial Lake Outburst Floods (GLOFs — e.g. Thame flood August 2024, Melamchi flood 2021) send sudden wall-of-water debris flows down mountain river beds.
* **Hardware Expansion**: Connect a weatherproof **JSN-SR04T ultrasonic sensor** or **24 GHz mmWave radar** to SPU GPIO 32/33, suspended from a mountain suspension bridge facing the river water surface.
* **Edge Detection Logic**:
  * Samples river surface distance every 10 seconds.
  * If water level rises $> 1.0\text{ meter in } 3\text{ minutes}$ or sudden high-frequency riverbed vibration is detected on the MPU6050:
  * Triggers immediate autonomous `EMERGENCY_GLOF ('W')` broadcast to all downstream base stations and handheld units, giving downstream villages 15–45 minutes of evacuation warning.

### 4.5 Reverse Broadcast Evacuation Command
* The Base Station can transmit an all-points broadcast frame:
  `TARGET_NODE = 0xFF (BROADCAST), STATUS = EVACUATE, TEXT = "DAM BREACH - HEAD TO HIGH RIDGE"`
* Every field unit within radio range intercepts this packet, turns on its loud piezo siren, flashes red LEDs, and displays the evacuation vector on both the TFT screen and connected smartphones.

---

## 5. Master Hardware Pinout & Memory Map (v4.0)

### 5.1 Flash Partition Layout (`min_spiffs.csv`)
To host the **FreeRTOS Kernel**, **LoRa Stack**, **Adafruit GFX Display Drivers**, and **NimBLE-Arduino Stack**, the standard 4MB ESP32 flash partition is re-allocated:

| Partition Name | Type | SubType | Offset | Size | Purpose |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `nvs` | data | nvs | `0x9000` | `0x5000` (20 KB) | Wi-Fi credentials & device calibration |
| `otadata` | data | ota | `0xE000` | `0x2000` (8 KB) | Dual-OTA boot selection register |
| `app0` | app | ota_0 | `0x10000` | `0x1E0000` (1.875 MB) | Primary firmware binary (Firmware A) |
| `app1` | app | ota_1 | `0x1F0000` | `0x1E0000` (1.875 MB) | Fallback firmware binary (Firmware B) |
| `spiffs` | data | spiffs | `0x3D0000` | `0x30000` (192 KB) | Web Bluetooth PWA & web portal assets |

### 5.2 LifeLine TX Pro Updated Pinout Reference

| Function | ESP32 GPIO | Connected Component | Notes |
| :--- | :--- | :--- | :--- |
| **TFT MOSI / SDA** | GPIO 23 | ST7789 Color Display | Hardware VSPI Data |
| **TFT SCLK / SCL** | GPIO 18 | ST7789 Color Display | Hardware VSPI Clock |
| **TFT CS** | GPIO 15 | ST7789 Color Display | Active LOW Chip Select |
| **TFT DC** | GPIO 4 | ST7789 Color Display | Data / Command Select |
| **TFT RESET** | GPIO 2 | ST7789 Color Display | Display Hardware Reset |
| **LoRa CS / NSS** | GPIO 5 | SX1278 LoRa Module | Active LOW Chip Select |
| **LoRa RESET** | GPIO 0 | SX1278 LoRa Module | Radio Hardware Reset |
| **LoRa DIO0** | GPIO 34 | SX1278 LoRa Module | Packet RX/TX Interrupt (Input Only) |
| **Keypad Rows (R1–R4)** | GPIO 32, 33, 25, 26 | 4×4 Matrix Keypad | Matrix Row Drive Lines |
| **Keypad Cols (C1–C4)** | GPIO 14, 12, 13, 27 | 4×4 Matrix Keypad | Matrix Column Read Lines |
| **Buzzer** | GPIO 19 | Active Piezo Buzzer | Acoustic Alert Feedback |
| **Status LEDs** | GPIO 21 (Red), GPIO 22 (Green) | Dual Status LEDs | Transmit & Receive Indicators |
| **Battery ADC** | GPIO 35 | Voltage Divider (100k / 100k) | Battery Voltage Monitoring |

---

## 6. Implementation Roadmap & Execution Phases

```text
═════════════════════════════════════════════════════════════════════════════════
                       v4.0 IMPLEMENTATION TIMELINE
═════════════════════════════════════════════════════════════════════════════════

 Phase 1: Core Protocol & ACK Engine
 ├── Update SharedProtocol.h with LoRaUplinkFrame & LoRaDownlinkACK structs
 ├── Implement LoRa half-duplex turnaround & 2500ms listen window in TX Pro
 ├── Implement automatic ACK generation & dispatch in RX Pro
 └── Implement exponential backoff & collision-avoidance retry algorithm

 Phase 2: NimBLE Integration & Mobile Gateway
 ├── Integrate NimBLE-Arduino into TX Pro and RX Pro platformio.ini
 ├── Develop BLEManager.cpp / .h implementing Nordic UART Service (NUS)
 ├── Route incoming phone messages directly into LoRa text payload
 └── Route incoming LoRa ACKs directly into phone notifications

 Phase 3: UI & Display Flow Upgrades
 ├── Update TX Pro ST7789 UI: SCREEN_WAITING_ACK & SCREEN_ACK_CONFIRMED
 ├── Add BLE connection status indicators & signal bars
 ├── Update RX Pro 16x2 LCD: ACK dispatch status & BLE phone count
 └── Add dual-language Nepali/English typography screens

 Phase 4: Nepal Disaster Resilience & Sensor Expansion
 ├── SPU Ridge-Top LoRa Store-and-Forward Relay firmware
 ├── Cold-climate battery voltage ADC monitoring & dynamic RF power throttling
 ├── Ultrasonic JSN-SR04T Flash Flood / GLOF detection logic
 └── Single-file Zero-Install Web Bluetooth Companion App (PWA)
═════════════════════════════════════════════════════════════════════════════════
```

---

## 7. Conclusion & Next Steps

This specification bridges the critical gap between theoretical IoT design and real-world deployment across the Himalayas. By replacing blind one-way transmission with guaranteed two-way acknowledgments, coupling off-grid smartphones over BLE, and introducing mountain-specific relaying and cold-power defenses, LifeLine v4.0 becomes a truly resilient, life-saving communications network.

*Proceed with Phase 1 to begin implementing the two-way protocol structures and firmware update routines.*
