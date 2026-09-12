#include "DisplayUI.h"
#include "BuzzerLED.h"
#include <Wire.h>

// Instantiate LCD
LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS);

// Define Global State Variables
ScreenState currentScreen = SCREEN_BOOT;
ScreenState previousScreenBeforePress = SCREEN_IDLE;
unsigned long bootStartTime = 0;
unsigned long noWiFiStartTime = 0;
unsigned long alertReceivedTime = 0;
unsigned long lastPulseTime = 0;
uint8_t pulseState = 0;
uint8_t bootDotState = 0;

int lastDeviceId = 0;
int lastAlertIndex = 0;
int lastRssi = 0;
float lastSnr = 0.0f;
float lastDistanceKm = 0.0f;
bool lastSentToWeb = false;

AlertRecord alertHistory[HISTORY_MAX_ITEMS];
int historyCount = 0;
int historyScrollOffset = 0;
int totalAlertsReceived = 0;

bool hasActiveChatMessage = false;
String currentChatMessage = "";
int currentChatDeviceId = 0;
int currentChatRssi = 0;
int currentChatScrollOffset = 0;

void initDisplay() {
    Wire.begin(LCD_SDA, LCD_SCL);
    lcd.init();
    lcd.backlight();
    
    // Register custom glyphs (0 to 6)
    lcd.createChar(0, (uint8_t*)glyphRadar);  // CGRAM 0: Radar
    lcd.createChar(1, (uint8_t*)glyphBell);   // CGRAM 1: Bell
    lcd.createChar(2, (uint8_t*)glyphCheck);  // CGRAM 2: Checkmark
    lcd.createChar(3, (uint8_t*)glyphSignal); // CGRAM 3: Signal bars
    lcd.createChar(4, (uint8_t*)glyphWarn);   // CGRAM 4: Warning
    lcd.createChar(5, (uint8_t*)glyphWiFi);   // CGRAM 5: WiFi / Web Sent OK
    lcd.createChar(6, (uint8_t*)glyphNoWiFi); // CGRAM 6: No WiFi / Web Failed
    
    Serial.printf("[OK] 16x2 I2C LCD initialized (SDA: GPIO %d, SCL: GPIO %d, Addr: 0x%02X)\n", 
                  LCD_SDA, LCD_SCL, LCD_ADDR);
}

char getAlertCode(int index) {
    if (index >= 0 && index < ALERT_COUNT) {
        return 'A' + index;
    }
    return 'X';
}

void printLCDLine(uint8_t row, const String& text) {
    lcd.setCursor(0, row);
    String line = text;
    if (line.length() > 16) {
        line = line.substring(0, 16);
    }
    while (line.length() < 16) {
        line += " ";
    }
    lcd.print(line);
}

void printLCDCenter(uint8_t row, const String& text) {
    String line = text;
    if (line.length() > 16) line = line.substring(0, 16);
    int spaces = (16 - line.length()) / 2;
    String fullLine = "";
    for (int i = 0; i < spaces; i++) fullLine += " ";
    fullLine += line;
    while (fullLine.length() < 16) fullLine += " ";
    lcd.setCursor(0, row);
    lcd.print(fullLine);
}

void addToHistory(int deviceId, int alertIndex, int rssi) {
    if (historyCount >= HISTORY_MAX_ITEMS) {
        for (int i = 0; i < HISTORY_MAX_ITEMS - 1; i++) {
            alertHistory[i] = alertHistory[i + 1];
        }
        historyCount = HISTORY_MAX_ITEMS - 1;
    }
    
    alertHistory[historyCount].deviceId = deviceId;
    alertHistory[historyCount].alertIndex = alertIndex;
    alertHistory[historyCount].rssi = rssi;
    alertHistory[historyCount].timestamp = millis();
    historyCount++;
    totalAlertsReceived++;
}

void drawBootScreen() {
    lcd.clear();
    printLCDCenter(0, "LIFELINE RX");
    printLCDCenter(1, "Booting...");
    bootStartTime = millis();
    bootDotState = 0;
    Serial.println(F("[SCREEN] Boot screen displayed on 16x2 LCD"));
}

bool updateBootAnimation() {
    unsigned long elapsed = millis() - bootStartTime;
    uint8_t newDotState = (elapsed / 300) % 4;
    
    if (newDotState != bootDotState) {
        bootDotState = newDotState;
        String status = "   Booting";
        for (int i = 0; i < bootDotState; i++) {
            status += ".";
        }
        printLCDLine(1, status);
    }
    
    return elapsed >= BOOT_DISPLAY_TIME;
}

#include <time.h>
#include <WiFi.h>

extern bool wifiConnected;

void initNTPTime() {
    if (wifiConnected || WiFi.status() == WL_CONNECTED) {
        configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER_1, NTP_SERVER_2, NTP_SERVER_3);
        Serial.println(F("[NTP] Initialized time synchronization"));
    }
}

bool getFormattedTimeStr(char* buffer, size_t maxLen) {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 10)) {
        return false;
    }
    snprintf(buffer, maxLen, "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    return true;
}

void drawNoWiFiScreen() {
    printLCDLine(0, " No Internet!   ");
    printLCDLine(1, "1x=Skip 3s=Setup");
    noWiFiStartTime = millis();
    Serial.println(F("[SCREEN] Displayed No Internet screen"));
}

// Distance & SNR Calculation & Formatting Helpers
float calculateDistanceKm(int rssi, double lat, double lon) {
    if (fabs(lat) > 0.001 && fabs(lon) > 0.001) {
        double lat1 = DEFAULT_BASE_LAT * DEG_TO_RAD;
        double lon1 = DEFAULT_BASE_LON * DEG_TO_RAD;
        double lat2 = lat * DEG_TO_RAD;
        double lon2 = lon * DEG_TO_RAD;
        double dlat = lat2 - lat1;
        double dlon = lon2 - lon1;
        double a = sin(dlat / 2.0) * sin(dlat / 2.0) +
                   cos(lat1) * cos(lat2) * sin(dlon / 2.0) * sin(dlon / 2.0);
        double c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));
        return (float)(6371.0 * c);
    }
    
    // Log-Distance Path Loss model for 433 MHz LoRa in Himalayan mountain terrain
    // RSSI = -(10 * n * log10(d) + A) -> d = 10^((-A - RSSI) / (10 * n))
    // A = 42 dBm reference RSSI @ 1m, n = 2.4 path loss exponent
    float exponent = (-42.0f - (float)rssi) / 24.0f;
    if (exponent < 0.0f) exponent = 0.0f;
    float distMeters = pow(10.0f, exponent);
    return distMeters / 1000.0f;
}

void formatDistance(float distKm, char* buffer, size_t maxLen) {
    if (distKm < 0.05f) {
        distKm = 0.1f;
    }
    if (distKm < 10.0f) {
        snprintf(buffer, maxLen, "%.1fkm", distKm);
    } else if (distKm < 100.0f) {
        snprintf(buffer, maxLen, "%.0fkm", distKm);
    } else {
        snprintf(buffer, maxLen, "%dkm", (int)distKm);
    }
}

void formatSNR(float snr, char* buffer, size_t maxLen) {
    int s = (int)round(snr);
    if (s >= 0) {
        snprintf(buffer, maxLen, "S:+%d", s);
    } else {
        snprintf(buffer, maxLen, "S:%d", s);
    }
}

void drawIdleScreen() {
    bool wifiOk = (WiFi.status() == WL_CONNECTED || wifiConnected);
    if (wifiOk) {
        char timeBuf[12];
        if (getFormattedTimeStr(timeBuf, sizeof(timeBuf))) {
            char row0[17];
            snprintf(row0, sizeof(row0), "Time: %-8s   ", timeBuf);
            printLCDLine(0, row0);
        } else {
            printLCDLine(0, "Time: Syncing.. ");
        }
    } else {
        printLCDLine(0, "Offline Mode    ");
    }
    
    // Column 15 Wi-Fi status symbol (CGRAM 5: WiFi OK, CGRAM 6: Offline)
    lcd.setCursor(15, 0);
    lcd.write(wifiOk ? 5 : 6);
    
    printLCDLine(1, "Waiting for TX..");
    
    lastPulseTime = millis();
    pulseState = 0;
    Serial.println(F("[SCREEN] Idle screen displayed on 16x2 LCD"));
}

void updateIdleAnimation() {
    unsigned long currentTime = millis();
    
    if (currentTime - lastPulseTime >= 500) {
        lastPulseTime = currentTime;
        pulseState = (pulseState + 1) % 4;
        
        bool wifiOk = (WiFi.status() == WL_CONNECTED || wifiConnected);
        if (wifiOk) {
            char timeBuf[12];
            if (getFormattedTimeStr(timeBuf, sizeof(timeBuf))) {
                char row0[17];
                snprintf(row0, sizeof(row0), "Time: %-8s   ", timeBuf);
                printLCDLine(0, row0);
            } else {
                printLCDLine(0, "Time: Syncing.. ");
            }
        } else {
            printLCDLine(0, "Offline Mode    ");
        }
        
        lcd.setCursor(15, 0);
        lcd.write(wifiOk ? 5 : 6);
        
        // Row 1: Waiting for TX data animated dots
        switch (pulseState) {
            case 0: printLCDLine(1, "Waiting for TX  "); break;
            case 1: printLCDLine(1, "Waiting for TX. "); break;
            case 2: printLCDLine(1, "Waiting for TX.."); break;
            case 3: printLCDLine(1, "Waiting for TX..."); break;
        }
    }
}

void drawAlertScreen(int alertIndex, int rssi, float snr, float distanceKm, bool sentToWeb, bool playSound) {
    if (alertIndex < 0 || alertIndex >= ALERT_COUNT) {
        alertIndex = ALERT_COUNT - 1;
    }
    
    uint8_t priority = alertPriority[alertIndex];
    const char* nameShort = alertNamesShort[alertIndex];
    
    // Row 0: [!] DELIVERY SOS [WiFi]
    lcd.setCursor(0, 0);
    lcd.write(4); // CGRAM 4: Warning glyph
    lcd.print(" ");
    
    char nameBuf[13];
    snprintf(nameBuf, sizeof(nameBuf), "%-12s", nameShort);
    lcd.print(nameBuf);
    
    lcd.setCursor(15, 0);
    lcd.write(sentToWeb ? 5 : 6); // CGRAM 5: WiFi connected, CGRAM 6: Offline
    
    // Row 1: RSSI, SNR, Distance (e.g. "-65dB S:+9 1.2km") - No "TX#003"
    char rssiStr[8];
    snprintf(rssiStr, sizeof(rssiStr), "%ddB", rssi);
    
    char snrStr[8];
    formatSNR(snr, snrStr, sizeof(snrStr));
    
    char distStr[8];
    formatDistance(distanceKm, distStr, sizeof(distStr));
    
    char row1[17];
    snprintf(row1, sizeof(row1), "%-5s %-4s %5s", rssiStr, snrStr, distStr);
    if (strlen(row1) > 16) {
        snprintf(row1, sizeof(row1), "%s %s %s", rssiStr, snrStr, distStr);
    }
    printLCDLine(1, row1);
    
    lastAlertIndex = alertIndex;
    lastRssi = rssi;
    lastSnr = snr;
    lastDistanceKm = distanceKm;
    lastSentToWeb = sentToWeb;
    alertReceivedTime = millis();
    
    addToHistory(lastDeviceId, alertIndex, rssi);
    if (playSound) {
        playAlertTone(priority);
    }
    
    Serial.printf("[SCREEN] Alert on LCD: Alert %d (%s), RSSI: %s, SNR: %s, Dist: %s, Web: %s\n", 
                  alertIndex, alertNames[alertIndex], rssiStr, snrStr, distStr, sentToWeb ? "YES" : "NO");
}

void drawAlertScreen(int deviceId, int alertIndex, int rssi) {
    lastDeviceId = deviceId;
    float dist = calculateDistanceKm(rssi, 0.0, 0.0);
    drawAlertScreen(alertIndex, rssi, lastSnr, dist, lastSentToWeb, true);
}

void updateAlertWebStatus(bool sentToWeb) {
    lastSentToWeb = sentToWeb;
    if (currentScreen == SCREEN_ALERT) {
        lcd.setCursor(15, 0);
        lcd.write(sentToWeb ? 5 : 6);
    }
}

bool shouldReturnToIdle() {
    if (currentScreen != SCREEN_ALERT && currentScreen != SCREEN_CUSTOM_MSG) return false;
    return (millis() - alertReceivedTime >= ALERT_DISPLAY_TIME);
}

void drawCustomMessageScreen(int deviceId, const String& message, int rssi, int scrollOffset) {
    // Row 0: M#001 -65dBm [W]  ([W] indicates Wi-Fi button scrolls message)
    char row0[17];
    snprintf(row0, sizeof(row0), "M#%03d %4ddBm [W]", deviceId % 1000, rssi);
    printLCDLine(0, row0);

    // Row 1: 16 chars from scrollOffset
    char row1[17];
    memset(row1, ' ', 16);
    row1[16] = '\0';

    int msgLen = message.length();
    if (scrollOffset < msgLen) {
        int copyLen = msgLen - scrollOffset;
        if (copyLen > 16) copyLen = 16;
        memcpy(row1, message.c_str() + scrollOffset, copyLen);
    }
    printLCDLine(1, row1);

    lastDeviceId = deviceId;
    lastRssi = rssi;
    alertReceivedTime = millis();
    Serial.printf("[SCREEN] Custom Message displayed: Dev=%d, Offset=%d, Text='%s'\n",
                  deviceId, scrollOffset, message.c_str());
}

void scrollCurrentMessage() {
    if (!hasActiveChatMessage || currentChatMessage.length() == 0) return;
    int msgLen = currentChatMessage.length();

    // Advance by 12 characters (giving 4-character overlap for continuous reading)
    if (currentChatScrollOffset + 16 < msgLen) {
        currentChatScrollOffset += 12;
    } else {
        // Wrap back to beginning
        currentChatScrollOffset = 0;
    }
    playSkipConfirmTone();
    drawCustomMessageScreen(currentChatDeviceId, currentChatMessage, currentChatRssi, currentChatScrollOffset);
}

void drawWiFiConnectingScreen(const String& ssid, int currentIdx, int totalCount) {
    printLCDLine(0, "WiFi Connecting");
    char line1[17];
    snprintf(line1, sizeof(line1), "%d/%d:%-11s", currentIdx, totalCount, ssid.c_str());
    printLCDLine(1, line1);
}

void drawWiFiConnectedScreen(const String& ip) {
    printLCDLine(0, "WiFi Connected!");
    String line1 = "IP:" + ip;
    printLCDLine(1, line1);
}

void drawWiFiFailedScreen() {
    printLCDLine(0, "WiFi Failed!    ");
    printLCDLine(1, "Open Setup AP..");
}

void drawCountdownScreen(int remainingSec) {
    printLCDLine(0, "WiFi Btn Held   ");
    char line1[17];
    snprintf(line1, sizeof(line1), "Opening in %ds...", remainingSec);
    printLCDLine(1, line1);
}

void drawProgressCountdownScreen(unsigned long elapsedMs, unsigned long totalMs) {
    printLCDLine(0, "WiFi Btn Held   ");
    
    uint8_t filled = (elapsedMs * 16) / totalMs;
    if (filled > 16) filled = 16;
    
    char bar[17];
    for (uint8_t i = 0; i < 16; i++) {
        bar[i] = (i < filled) ? (char)0xFF : ' ';
    }
    bar[16] = '\0';
    
    lcd.setCursor(0, 1);
    lcd.print(bar);
}

void drawWiFiPortalScreen() {
    printLCDLine(0, "AP:LifeLine-RX  ");
    printLCDLine(1, "192.168.4.1     ");
}

void drawOTACheckingScreen() {
    printLCDLine(0, "Checking OTA...");
    printLCDLine(1, "Connecting VPS..");
}

void drawOTAFoundScreen(const String& newVer) {
    printLCDLine(0, "New FW Found!   ");
    String line1 = "Ver: " + newVer;
    printLCDLine(1, line1);
}

void drawOTAProgressScreen(int percent) {
    static int lastDrawnPercent = -1;
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    if (percent == lastDrawnPercent && percent != 0 && percent != 100) return;
    lastDrawnPercent = percent;
    
    char line0[17];
    snprintf(line0, sizeof(line0), "Updating FW %3d%%", percent);
    printLCDLine(0, line0);
    
    uint8_t filled = (percent * 16) / 100;
    char bar[17];
    for (uint8_t i = 0; i < 16; i++) {
        bar[i] = (i < filled) ? '=' : ' ';
    }
    bar[16] = '\0';
    printLCDLine(1, bar);
}

void drawOTAFailedScreen(const String& reason) {
    printLCDLine(0, "OTA Update Fail!");
    printLCDLine(1, reason);
}

void drawOTASuccessScreen() {
    printLCDLine(0, "OTA Complete!   ");
    printLCDLine(1, "Rebooting...    ");
}

void drawLocalOTAScreen() {
    printLCDLine(0, "OTA LOCAL PORTAL");
    printLCDLine(1, "192.168.4.1     ");
}

