#include "Config.h"

// Alert messages - MUST match receiver configuration
const char* alertNames[ALERT_COUNT] = {
    "CRITICAL SOS",         // 0  - A - CRITICAL
    "DELIVERY / LABOR",     // 1  - B - CRITICAL (Maternal Childbirth Emergency)
    "HELI RESCUE NEEDED",   // 2  - C - CRITICAL (Medical Air Evacuation)
    "MEDICINE SHORTAGE",    // 3  - D - MEDIUM   (Essential Drugs / Antibiotics / IV)
    "OXYGEN SHORTAGE",      // 4  - E - CRITICAL (Cylinders Depleted)
    "SEVERE INJURY",        // 5  - F - HIGH     (Trauma / Fracture / Bleeding)
    "BLOOD NEEDED",         // 6  - G - CRITICAL (Urgent Transfusion)
    "ALTITUDE SICKNESS",    // 7  - H - HIGH     (Severe AMS / HAPE / HACE)
    "FOOD SHORTAGE",        // 8  - I - MEDIUM   (Rations Depleted)
    "WATER SHORTAGE",       // 9  - J - MEDIUM   (Drinking Water Crisis)
    "DISEASE OUTBREAK",     // 10 - K - HIGH     (Epidemic / Infection Cluster)
    "FREEZING / SHELTER",   // 11 - L - MEDIUM   (Extreme Cold / Blankets Needed)
    "LANDSLIDE / HAZARD",   // 12 - M - HIGH     (Slope / Trail Collapsed)
    "DOCTOR / NURSE NEED",  // 13 - N - MEDIUM   (Medical Personnel Required)
    "STATUS OK / ALL SAFE"  // 14 - O - OK       (Routine Nominal Check-in)
};

// Shorter names for compact display (all <= 12 characters for 16x2 LCD)
const char* alertNamesShort[ALERT_COUNT] = {
    "CRITICAL SOS",
    "DELIVERY SOS",
    "HELI RESCUE",
    "MED SHORTAGE",
    "OXYGEN SHORT",
    "SEVERE INJUR",
    "BLOOD NEEDED",
    "ALTITUDE AMS",
    "FOOD SHORT",
    "WATER SHORT",
    "OUTBREAK",
    "COLD SHELTER",
    "LANDSLIDE",
    "DOCTOR REQ",
    "STATUS OK"
};

// Priority levels: 0=CRITICAL, 1=HIGH, 2=MEDIUM, 3=OK, 4=NEUTRAL
const uint8_t alertPriority[ALERT_COUNT] = {
    0, 0, 0, 2, 0, 1, 0, 1, 2, 2, 1, 2, 1, 2, 3
};

// Alert codes for LoRa transmission (A-O)
char getAlertCode(int index) {
    if (index >= 0 && index < ALERT_COUNT) {
        return 'A' + index;
    }
    return 'X';  // Invalid
}

// Current application state
ScreenState currentScreen = SCREEN_BOOT;
ScreenState previousScreen = SCREEN_BOOT;

// Menu navigation state
int selectedAlertIndex = 0;
int menuScrollOffset = 0;

// Manual screen state  
int manualPage = 0;

// Timing state
unsigned long bootStartTime = 0;
unsigned long resultStartTime = 0;
unsigned long lastKeyPressTime = 0;
unsigned long lastTransmitTime = 0;

// Transmission state
bool lastTransmitSuccess = false;
int retryCount = 0;
int totalTransmissions = 0;
int successfulTransmissions = 0;

// System status
bool loraInitialized = false;
int batteryPercent = -1;  // -1 = not available

// BLE Portal & Message History
RxMessageItem rxMessageHistory[RX_MESSAGE_HISTORY_MAX];
int rxMessageCount = 0;
int blePortalScrollIndex = 0;
bool bleRadioEnabled = true;

// Popup Modal State
String popupTitle = "";
String popupSender = "";
String popupMessage = "";
String popupStatus = "";
int popupRssi = 0;
unsigned long popupStartTime = 0;

void addReceivedMessageToHistory(const String& sender, const String& status, const String& text, int rssi) {
    if (rxMessageCount >= RX_MESSAGE_HISTORY_MAX) {
        for (int i = 0; i < RX_MESSAGE_HISTORY_MAX - 1; i++) {
            rxMessageHistory[i] = rxMessageHistory[i + 1];
        }
        rxMessageCount = RX_MESSAGE_HISTORY_MAX - 1;
    }
    rxMessageHistory[rxMessageCount].sender = sender;
    rxMessageHistory[rxMessageCount].status = status;
    rxMessageHistory[rxMessageCount].text = text;
    rxMessageHistory[rxMessageCount].timestamp = millis();
    rxMessageHistory[rxMessageCount].rssi = rssi;
    rxMessageCount++;
    blePortalScrollIndex = max(0, rxMessageCount - 1);
}
