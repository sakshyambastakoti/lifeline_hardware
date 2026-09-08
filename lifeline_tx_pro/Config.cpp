#include "Config.h"

// Alert messages - MUST match receiver configuration
const char* alertNames[ALERT_COUNT] = {
    "EMERGENCY",            // 0  - A - CRITICAL
    "MEDICAL EMERGENCY",    // 1  - B - CRITICAL
    "MEDICINE SHORTAGE",    // 2  - C - MEDIUM
    "EVACUATION NEEDED",    // 3  - D - CRITICAL
    "STATUS OK",            // 4  - E - OK
    "INJURY REPORTED",      // 5  - F - HIGH
    "FOOD SHORTAGE",        // 6  - G - MEDIUM
    "WATER SHORTAGE",       // 7  - H - MEDIUM
    "WEATHER ALERT",        // 8  - I - MEDIUM
    "LOST PERSON",          // 9  - J - HIGH
    "ANIMAL ATTACK",        // 10 - K - HIGH
    "LANDSLIDE",            // 11 - L - HIGH
    "SNOW STORM",           // 12 - M - HIGH
    "EQUIPMENT FAILURE",    // 13 - N - MEDIUM
    "OTHER EMERGENCY"       // 14 - O - NEUTRAL
};

// Shorter names for compact display
const char* alertNamesShort[ALERT_COUNT] = {
    "EMERGENCY",
    "MEDICAL EMERG",
    "MED SHORTAGE",
    "EVACUATION",
    "STATUS OK",
    "INJURY",
    "FOOD SHORT",
    "WATER SHORT",
    "WEATHER",
    "LOST PERSON",
    "ANIMAL ATTK",
    "LANDSLIDE",
    "SNOW STORM",
    "EQUIP FAIL",
    "OTHER"
};

// Priority levels: 0=CRITICAL, 1=HIGH, 2=MEDIUM, 3=OK, 4=NEUTRAL
const uint8_t alertPriority[ALERT_COUNT] = {
    0, 0, 2, 0, 3, 1, 2, 2, 2, 1, 1, 1, 1, 2, 4
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
