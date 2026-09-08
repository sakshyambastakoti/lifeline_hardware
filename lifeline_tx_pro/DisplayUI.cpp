#include "DisplayUI.h"
#include "OTAManager.h"
#include "SPUReceiver.h"

// TFT Display instance (Hardware SPI using shared SPI bus)
Adafruit_ST7789 tft = Adafruit_ST7789(&SPI, TFT_CS, TFT_DC, TFT_RST);

void initDisplay() {
    pinMode(TFT_CS, OUTPUT);
    digitalWrite(TFT_CS, HIGH);
    
    if (TFT_RST >= 0) {
        pinMode(TFT_RST, OUTPUT);
        digitalWrite(TFT_RST, HIGH);
        delay(10);
        digitalWrite(TFT_RST, LOW);
        delay(10);
        digitalWrite(TFT_RST, HIGH);
        delay(50);
    }
    
    tft.init(NATIVE_WIDTH, NATIVE_HEIGHT);
    tft.setRotation(SCREEN_ROTATION);
    tft.fillScreen(COLOR_BG_PRIMARY);
    Serial.printf("[INIT] TFT: %dx%d OK\n", SCREEN_WIDTH, SCREEN_HEIGHT);
}

uint16_t getPriorityColor(uint8_t priority) {
    switch (priority) {
        case 0:  return COLOR_STATUS_CRITICAL;
        case 1:  return COLOR_STATUS_HIGH;
        case 2:  return COLOR_STATUS_MEDIUM;
        case 3:  return COLOR_STATUS_LOW;
        default: return COLOR_STATUS_NEUTRAL;
    }
}

uint16_t getAlertColor(int index) {
    if (index < 0 || index >= ALERT_COUNT) return COLOR_STATUS_NEUTRAL;
    return getPriorityColor(alertPriority[index]);
}

void updateMenuScroll() {
    if (selectedAlertIndex < menuScrollOffset) {
        menuScrollOffset = selectedAlertIndex;
    } else if (selectedAlertIndex >= menuScrollOffset + VISIBLE_MENU_ITEMS) {
        menuScrollOffset = selectedAlertIndex - VISIBLE_MENU_ITEMS + 1;
    }
    menuScrollOffset = constrain(menuScrollOffset, 0, max(0, ALERT_COUNT - VISIBLE_MENU_ITEMS));
}

void drawCenteredText(const char* text, int y, uint8_t textSize, uint16_t color) {
    tft.setTextSize(textSize);
    tft.setTextColor(color);
    int16_t x1, y1;
    uint16_t w, h;
    tft.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    tft.setCursor((SCREEN_WIDTH - w) / 2, y);
    tft.print(text);
}

void drawRightText(const char* text, int y, uint8_t textSize, uint16_t color) {
    tft.setTextSize(textSize);
    tft.setTextColor(color);
    int16_t x1, y1;
    uint16_t w, h;
    tft.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    tft.setCursor(SCREEN_WIDTH - w - MARGIN, y);
    tft.print(text);
}

void drawGradientH(int x, int y, int w, int h, uint16_t colorStart, uint16_t colorEnd) {
    for (int i = 0; i < w; i++) {
        uint8_t r1 = (colorStart >> 11) & 0x1F;
        uint8_t g1 = (colorStart >> 5) & 0x3F;
        uint8_t b1 = colorStart & 0x1F;
        uint8_t r2 = (colorEnd >> 11) & 0x1F;
        uint8_t g2 = (colorEnd >> 5) & 0x3F;
        uint8_t b2 = colorEnd & 0x1F;
        
        uint8_t r = r1 + (r2 - r1) * i / w;
        uint8_t g = g1 + (g2 - g1) * i / w;
        uint8_t b = b1 + (b2 - b1) * i / w;
        
        uint16_t color = (r << 11) | (g << 5) | b;
        tft.drawFastVLine(x + i, y, h, color);
    }
}

void drawGradientV(int x, int y, int w, int h, uint16_t colorTop, uint16_t colorBottom) {
    for (int i = 0; i < h; i++) {
        uint8_t r1 = (colorTop >> 11) & 0x1F;
        uint8_t g1 = (colorTop >> 5) & 0x3F;
        uint8_t b1 = colorTop & 0x1F;
        uint8_t r2 = (colorBottom >> 11) & 0x1F;
        uint8_t g2 = (colorBottom >> 5) & 0x3F;
        uint8_t b2 = colorBottom & 0x1F;
        
        uint8_t r = r1 + (r2 - r1) * i / h;
        uint8_t g = g1 + (g2 - g1) * i / h;
        uint8_t b = b1 + (b2 - b1) * i / h;
        
        uint16_t color = (r << 11) | (g << 5) | b;
        tft.drawFastHLine(x, y + i, w, color);
    }
}

void drawSharpCard(int x, int y, int w, int h, uint16_t bgColor, uint16_t borderColor, uint16_t accentColor, bool cornerTicks) {
    // 1. Sharp drop-shadow on bottom-right for clean depth
    tft.drawFastHLine(x + SHADOW_OFFSET, y + h, w, RGB565(5, 5, 10));
    tft.drawFastHLine(x + SHADOW_OFFSET + 1, y + h + 1, w - 1, RGB565(2, 2, 5));
    tft.drawFastVLine(x + w, y + SHADOW_OFFSET, h, RGB565(5, 5, 10));
    tft.drawFastVLine(x + w + 1, y + SHADOW_OFFSET + 1, h - 1, RGB565(2, 2, 5));
    
    // 2. Crisp rectangular fill & outer border
    tft.fillRect(x, y, w, h, bgColor);
    tft.drawRect(x, y, w, h, borderColor);
    
    // 3. Top subtle highlight line for chiseled look
    tft.drawFastHLine(x + 1, y + 1, w - 2, RGB565(60, 70, 95));
    
    // 4. Tactical corner ticks / brackets
    if (cornerTicks) {
        uint16_t tickCol = (accentColor != 0) ? accentColor : borderColor;
        int tLen = CORNER_TICK_LEN;
        if (tLen > w / 3) tLen = w / 3;
        if (tLen > h / 3) tLen = h / 3;
        
        // Top-Left
        tft.drawFastHLine(x, y, tLen, tickCol);
        tft.drawFastVLine(x, y, tLen, tickCol);
        tft.drawFastHLine(x + 1, y + 1, tLen - 1, tickCol);
        
        // Top-Right
        tft.drawFastHLine(x + w - tLen, y, tLen, tickCol);
        tft.drawFastVLine(x + w - 1, y, tLen, tickCol);
        tft.drawFastHLine(x + w - tLen, y + 1, tLen - 1, tickCol);
        
        // Bottom-Left
        tft.drawFastHLine(x, y + h - 1, tLen, tickCol);
        tft.drawFastVLine(x, y + h - tLen, tLen, tickCol);
        tft.drawFastHLine(x + 1, y + h - 2, tLen - 1, tickCol);
        
        // Bottom-Right
        tft.drawFastHLine(x + w - tLen, y + h - 1, tLen, tickCol);
        tft.drawFastVLine(x + w - 1, y + h - tLen, tLen, tickCol);
        tft.drawFastHLine(x + w - tLen, y + h - 2, tLen - 1, tickCol);
    }
}

void drawPremiumCard(int x, int y, int w, int h, uint16_t bgColor, uint16_t borderColor, bool hasGlow) {
    drawSharpCard(x, y, w, h, bgColor, borderColor, hasGlow ? COLOR_CYAN : borderColor, true);
}

void drawGlowCircle(int cx, int cy, int r, uint16_t color) {
    for (int i = GLOW_INTENSITY; i > 0; i--) {
        uint8_t cr = (color >> 11) & 0x1F;
        uint8_t cg = (color >> 5) & 0x3F;
        uint8_t cb = color & 0x1F;
        uint16_t glowColor = ((cr / (i + 1)) << 11) | ((cg / (i + 1)) << 5) | (cb / (i + 1));
        tft.drawCircle(cx, cy, r + i * 2, glowColor);
    }
    tft.fillCircle(cx, cy, r, color);
}

void drawStatusIndicator(int x, int y, int r, uint16_t color, bool active) {
    if (active) {
        tft.drawCircle(x, y, r + 3, RGB565((color >> 11) & 0x0F, (color >> 6) & 0x1F, (color >> 1) & 0x0F));
        tft.drawCircle(x, y, r + 2, RGB565((color >> 11) & 0x1F, (color >> 5) & 0x1F, (color) & 0x0F));
    }
    tft.fillCircle(x, y, r, color);
    tft.fillCircle(x - r/3, y - r/3, r/4, COLOR_TEXT_PRIMARY);
}

void drawHeader(const char* title) {
    drawGradientV(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, RGB565(20, 40, 65), COLOR_BG_HEADER);
    
    tft.drawFastHLine(0, HEADER_HEIGHT - 2, SCREEN_WIDTH, COLOR_CYAN_DARK);
    tft.drawFastHLine(0, HEADER_HEIGHT - 1, SCREEN_WIDTH, COLOR_CYAN);
    tft.drawFastHLine(0, HEADER_HEIGHT, SCREEN_WIDTH, COLOR_CYAN_DARK);
    
    tft.setTextSize(TEXT_MEDIUM);
    int16_t x1, y1;
    uint16_t w, h;
    tft.getTextBounds(title, 0, 0, &x1, &y1, &w, &h);
    int textY = (HEADER_HEIGHT - h) / 2 - 1;
    
    tft.setTextColor(RGB565(0, 0, 0));
    tft.setCursor(MARGIN + 1, textY + 1);
    tft.print(title);
    
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.setCursor(MARGIN, textY);
    tft.print(title);
}

void drawFooter(const char* hints) {
    int footerY = SCREEN_HEIGHT - FOOTER_HEIGHT;
    
    tft.drawFastHLine(0, footerY - 2, SCREEN_WIDTH, COLOR_CYAN_DARK);
    tft.drawFastHLine(0, footerY - 1, SCREEN_WIDTH, COLOR_ACCENT_LINE);
    
    drawGradientV(0, footerY, SCREEN_WIDTH, FOOTER_HEIGHT, COLOR_BG_HEADER_ALT, RGB565(5, 12, 18));
    
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.setCursor(MARGIN, footerY + (FOOTER_HEIGHT - 8) / 2);
    tft.print(hints);
}

void drawKeyBadge(int x, int y, char key, const char* label, uint16_t keyColor) {
    tft.fillRect(x, y - 2, 14, 12, COLOR_BADGE_BG);
    tft.drawRect(x, y - 2, 14, 12, keyColor);
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(keyColor);
    tft.setCursor(x + 4, y);
    tft.print(key);
    
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(x + 18, y);
    tft.print(label);
}

void drawSeparator(int y) {
    tft.drawFastHLine(MARGIN, y, SCREEN_WIDTH - (2 * MARGIN), COLOR_ACCENT_LINE);
}

void drawBootScreen() {
    // 1. Deep tactical dark background
    tft.fillScreen(RGB565(8, 12, 18));
    
    // Technical hairline outer border
    tft.drawRect(2, 2, SCREEN_WIDTH - 4, SCREEN_HEIGHT - 4, RGB565(25, 35, 52));
    tft.drawRect(4, 4, SCREEN_WIDTH - 8, SCREEN_HEIGHT - 8, RGB565(15, 22, 34));
    
    // Tactical corner registration marks
    int mDist = 8;
    // Top-Left
    tft.drawFastHLine(mDist - 3, mDist, 7, COLOR_CYAN_DARK);
    tft.drawFastVLine(mDist, mDist - 3, 7, COLOR_CYAN_DARK);
    // Top-Right
    tft.drawFastHLine(SCREEN_WIDTH - mDist - 4, mDist, 7, COLOR_CYAN_DARK);
    tft.drawFastVLine(SCREEN_WIDTH - mDist - 1, mDist - 3, 7, COLOR_CYAN_DARK);
    // Bottom-Left
    tft.drawFastHLine(mDist - 3, SCREEN_HEIGHT - mDist - 1, 7, COLOR_CYAN_DARK);
    tft.drawFastVLine(mDist, SCREEN_HEIGHT - mDist - 4, 7, COLOR_CYAN_DARK);
    // Bottom-Right
    tft.drawFastHLine(SCREEN_WIDTH - mDist - 4, SCREEN_HEIGHT - mDist - 1, 7, COLOR_CYAN_DARK);
    tft.drawFastVLine(SCREEN_WIDTH - mDist - 1, SCREEN_HEIGHT - mDist - 4, 7, COLOR_CYAN_DARK);

    // 2. Top Header Telemetry Strip
    int topStripY = 6;
    int topStripH = 15;
    tft.fillRect(6, topStripY, SCREEN_WIDTH - 12, topStripH, RGB565(14, 22, 34));
    tft.drawRect(6, topStripY, SCREEN_WIDTH - 12, topStripH, RGB565(35, 52, 75));
    tft.drawFastHLine(6, topStripY + topStripH, SCREEN_WIDTH - 12, COLOR_CYAN_DARK);
    
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_CYAN);
    tft.setCursor(14, topStripY + 4);
    tft.print(F("LIFELINE EMERGENCY SYSTEM"));
    
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(SCREEN_WIDTH - 122, topStripY + 4);
    tft.print(F("MIL-SPEC // SECURE"));

    // 3. Central Emblem: Precision Sharp Cross + Glowing Neon ECG Pulse + Reticle
    int emblemY = 54;
    int emblemX = SCREEN_WIDTH / 2;
    
    // Outer tactical reticle rings & ticks
    tft.drawCircle(emblemX, emblemY, 26, RGB565(22, 36, 54));
    tft.drawCircle(emblemX, emblemY, 28, RGB565(16, 26, 40));
    tft.drawFastVLine(emblemX, emblemY - 32, 5, COLOR_CYAN_DARK);
    tft.drawFastVLine(emblemX, emblemY + 28, 5, COLOR_CYAN_DARK);
    tft.drawFastHLine(emblemX - 32, emblemY, 5, COLOR_CYAN_DARK);
    tft.drawFastHLine(emblemX + 28, emblemY, 5, COLOR_CYAN_DARK);
    
    // Sharp Geometric Medical Cross
    int crossSize = 17;
    int crossThick = 8;
    
    // Crisp shadow behind cross
    tft.fillRect(emblemX - crossThick/2 + 1, emblemY - crossSize + 1, crossThick, crossSize * 2, RGB565(4, 4, 8));
    tft.fillRect(emblemX - crossSize + 1, emblemY - crossThick/2 + 1, crossSize * 2, crossThick, RGB565(4, 4, 8));
    
    // Vibrant Red Cross Fill
    tft.fillRect(emblemX - crossThick/2, emblemY - crossSize, crossThick, crossSize * 2, COLOR_RED);
    tft.fillRect(emblemX - crossSize, emblemY - crossThick/2, crossSize * 2, crossThick, COLOR_RED);
    
    // Chiseled Highlight Border
    tft.drawFastHLine(emblemX - crossThick/2 + 1, emblemY - crossSize + 1, crossThick - 2, COLOR_RED_BRIGHT);
    tft.drawFastVLine(emblemX - crossThick/2 + 1, emblemY - crossSize + 1, crossSize * 2 - 2, COLOR_RED_BRIGHT);
    tft.drawFastHLine(emblemX - crossSize + 1, emblemY - crossThick/2 + 1, crossSize * 2 - 2, COLOR_RED_BRIGHT);
    tft.drawFastVLine(emblemX - crossSize + 1, emblemY - crossThick/2 + 1, crossThick - 2, COLOR_RED_BRIGHT);
    
    // Dynamic Cyan & White ECG Pulse Trace
    int ecgY = emblemY;
    int ecgLeft = emblemX - 44;
    int ecgRight = emblemX + 44;
    
    tft.drawLine(ecgLeft, ecgY, emblemX - 16, ecgY, COLOR_CYAN_DARK);
    tft.drawLine(emblemX - 16, ecgY, emblemX - 10, ecgY, COLOR_CYAN_BRIGHT);
    tft.drawLine(emblemX - 10, ecgY, emblemX - 6, ecgY + 5, COLOR_CYAN_BRIGHT);
    tft.drawLine(emblemX - 6, ecgY + 5, emblemX, ecgY - 14, COLOR_CYAN_BRIGHT);
    tft.drawLine(emblemX, ecgY - 14, emblemX + 6, ecgY + 9, COLOR_CYAN_BRIGHT);
    tft.drawLine(emblemX + 6, ecgY + 9, emblemX + 10, ecgY, COLOR_CYAN_BRIGHT);
    tft.drawLine(emblemX + 10, ecgY, emblemX + 16, ecgY, COLOR_CYAN_BRIGHT);
    tft.drawLine(emblemX + 16, ecgY, ecgRight, ecgY, COLOR_CYAN_DARK);
    
    // Neon white core highlight on the peak
    tft.drawLine(emblemX - 5, ecgY + 4, emblemX, ecgY - 13, COLOR_TEXT_PRIMARY);
    tft.drawLine(emblemX, ecgY - 13, emblemX + 5, ecgY + 8, COLOR_TEXT_PRIMARY);

    // 4. Clean Brand Typography
    tft.setTextSize(TEXT_LARGE);
    int16_t x1, y1;
    uint16_t tw, th;
    tft.getTextBounds("LIFELINE", 0, 0, &x1, &y1, &tw, &th);
    int titleX = (SCREEN_WIDTH - tw) / 2;
    int titleY = 88;
    
    // Drop shadow
    tft.setTextColor(RGB565(15, 35, 55));
    tft.setCursor(titleX + 2, titleY + 2);
    tft.print(F("LIFELINE"));
    
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.setCursor(titleX, titleY);
    tft.print(F("LIFELINE"));
    
    // Technical underline
    int lineW = tw + 40;
    int lineStartX = (SCREEN_WIDTH - lineW) / 2;
    tft.drawFastHLine(lineStartX, titleY + th + 4, lineW, COLOR_CYAN_DARK);
    tft.drawFastHLine(lineStartX + 15, titleY + th + 5, lineW - 30, COLOR_CYAN);
    tft.drawFastHLine(lineStartX + 35, titleY + th + 6, lineW - 70, COLOR_PURPLE);
    
    drawCenteredText("TACTICAL EMERGENCY TRANSMITTER", 120, TEXT_SMALL, COLOR_CYAN_BRIGHT);
    drawCenteredText("FIELD SOS & TELEMETRY TERMINAL", 131, TEXT_SMALL, COLOR_TEXT_MUTED);

    // 5. Dual Sharp Telemetry Cards
    int cardY = 146;
    int cardH = 58;
    int cardW = (SCREEN_WIDTH - MARGIN * 3) / 2;
    int leftCardX = MARGIN;
    int rightCardX = MARGIN * 2 + cardW;
    
    // LEFT CARD: NODE IDENT
    drawSharpCard(leftCardX, cardY, cardW, cardH, COLOR_BG_CARD, COLOR_BORDER, COLOR_CYAN, true);
    
    int headerH = 14;
    tft.fillRect(leftCardX + 1, cardY + 1, cardW - 2, headerH, RGB565(16, 28, 44));
    tft.drawFastHLine(leftCardX + 1, cardY + headerH, cardW - 2, COLOR_CYAN_DARK);
    
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_CYAN);
    tft.setCursor(leftCardX + 8, cardY + 4);
    tft.print(F("DEVICE IDENT"));
    
    char deviceIdStr[20];
    sprintf(deviceIdStr, "TX #%03d", DEVICE_ID);
    tft.setTextSize(TEXT_MEDIUM);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.setCursor(leftCardX + 8, cardY + 21);
    tft.print(deviceIdStr);
    
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.setCursor(leftCardX + 8, cardY + 42);
    tft.print(F("LoRa 433MHz SF10"));
    
    // RIGHT CARD: SYSTEM STATUS
    drawSharpCard(rightCardX, cardY, cardW, cardH, COLOR_BG_CARD, COLOR_BORDER, COLOR_GREEN, true);
    
    tft.fillRect(rightCardX + 1, cardY + 1, cardW - 2, headerH, RGB565(14, 34, 28));
    tft.drawFastHLine(rightCardX + 1, cardY + headerH, cardW - 2, COLOR_GREEN_DARK);
    
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_GREEN_BRIGHT);
    tft.setCursor(rightCardX + 8, cardY + 4);
    tft.print(F("SYS STATUS"));
    
    // Sharp square status LED
    tft.fillRect(rightCardX + 8, cardY + 23, 7, 7, COLOR_GREEN_BRIGHT);
    tft.drawRect(rightCardX + 7, cardY + 22, 9, 9, COLOR_GREEN_DARK);
    
    tft.setTextSize(TEXT_MEDIUM);
    tft.setTextColor(COLOR_GREEN_BRIGHT);
    tft.setCursor(rightCardX + 22, cardY + 20);
    tft.print(F("ONLINE"));
    
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(rightCardX + 8, cardY + 42);
    tft.print(FIRMWARE_VERSION);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.setCursor(rightCardX + 75, cardY + 42);
    tft.print(F("SPU:OK"));

    // 6. Bottom Segmented Progress Bar & Footer Prompt
    int barY = 211;
    int barW = SCREEN_WIDTH - (MARGIN * 2);
    int barX = MARGIN;
    
    tft.drawRect(barX, barY, barW, 7, RGB565(25, 38, 55));
    tft.fillRect(barX + 1, barY + 1, barW - 2, 5, RGB565(10, 15, 22));
    
    int numSegments = 16;
    int segWidth = (barW - 4) / numSegments;
    for (int s = 0; s < numSegments; s++) {
        uint16_t segCol = (s < numSegments / 2) ? COLOR_CYAN_DARK : COLOR_CYAN;
        if (s >= numSegments - 2) segCol = COLOR_GREEN_BRIGHT;
        tft.fillRect(barX + 2 + s * segWidth, barY + 2, segWidth - 1, 3, segCol);
    }
    
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(MARGIN + 2, 224);
    tft.print(F("STANDBY // READY FOR SOS TRANSMISSION"));
    
    drawGradientH(0, SCREEN_HEIGHT - 2, SCREEN_WIDTH, 2, COLOR_CYAN_DARK, COLOR_PURPLE_DARK);
    
    bootStartTime = millis();
    Serial.println(F("[SCREEN] Tactical sharp-edge boot screen displayed"));
}


void drawHomeStatusBar() {
    // 1. Clean Deep Midnight Header (y = 0..31)
    tft.fillRect(0, 0, SCREEN_WIDTH, 31, RGB565(12, 16, 24));
    tft.drawFastHLine(0, 31, SCREEN_WIDTH, RGB565(32, 42, 60)); // Crisp 1px hairline divider
    
    // 2. Brand & Device Tag
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_WHITE);
    tft.setCursor(10, 10);
    tft.print(F("LIFELINE"));
    tft.setTextColor(COLOR_CYAN);
    tft.print(F(" TX"));
    
    // Unit ID
    char idStr[10];
    sprintf(idStr, "#%03d", DEVICE_ID);
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(84, 10);
    tft.print(idStr);
    
    // 3. Center Screen Context Title
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.setCursor(128, 10);
    tft.print(F("ALERT SELECT"));
    
    // 4. Right Status: Radio Link
    tft.setTextColor(loraInitialized ? COLOR_GREEN_BRIGHT : COLOR_RED);
    tft.setCursor(222, 10);
    tft.print(loraInitialized ? F("RF:OK") : F("RF:ERR"));
    
    // 5. Battery Gauge (Clean rectangular cell)
    int batX = 286;
    tft.drawRect(batX, 9, 22, 12, RGB565(55, 70, 95));
    tft.fillRect(batX + 22, 13, 2, 4, RGB565(55, 70, 95));  // Terminal nub
    tft.fillRect(batX + 1, 10, 20, 10, RGB565(12, 16, 24)); // Interior
    
    if (batteryPercent >= 0) {
        int fillW = constrain((batteryPercent * 18) / 100, 1, 18);
        uint16_t bCol = (batteryPercent > 50) ? COLOR_GREEN : ((batteryPercent > 20) ? COLOR_AMBER : COLOR_RED);
        tft.fillRect(batX + 2, 11, fillW, 8, bCol);
        char bStr[8];
        sprintf(bStr, "%d%%", batteryPercent);
        tft.setTextColor(bCol);
        tft.setCursor(258, 10);
        tft.print(bStr);
    } else {
        tft.fillRect(batX + 2, 11, 18, 8, RGB565(0, 60, 80));
        tft.setTextColor(COLOR_CYAN_BRIGHT);
        tft.setCursor(260, 10);
        tft.print(F("USB"));
    }
}

void drawSharpAlertCard(int index, int slotY, bool isSelected) {
    if (index < 0 || index >= ALERT_COUNT) return;
    
    int cardX = 10;
    int cardW = 292;
    int cardH = 32;
    
    uint16_t prioColor = getAlertColor(index);
    uint8_t prio = alertPriority[index];
    const char* prioTag = "NORMAL";
    if (prio == 0) prioTag = "CRITICAL";
    else if (prio == 1) prioTag = "HIGH";
    else if (prio == 2) prioTag = "MEDIUM";
    else if (prio == 3) prioTag = "NORMAL";
    
    if (!isSelected) {
        // IDLE CARD: Clean, uncluttered, high contrast
        tft.fillRect(cardX, slotY, cardW, cardH, RGB565(16, 20, 28));
        tft.drawRect(cardX, slotY, cardW, cardH, RGB565(32, 40, 56));
        
        // Left priority bar (4px)
        tft.fillRect(cardX, slotY, 4, cardH, prioColor);
        
        // Sharp Index Box
        tft.fillRect(cardX + 10, slotY + 6, 22, 20, RGB565(22, 28, 40));
        tft.drawRect(cardX + 10, slotY + 6, 22, 20, RGB565(38, 48, 68));
        tft.setTextSize(TEXT_SMALL);
        tft.setTextColor(COLOR_TEXT_MUTED);
        tft.setCursor(cardX + 14, slotY + 11);
        if (index + 1 < 10) tft.print('0');
        tft.print(index + 1);
        
        // Alert Title
        tft.setTextSize(TEXT_MEDIUM);
        tft.setTextColor(COLOR_TEXT_PRIMARY);
        tft.setCursor(cardX + 40, slotY + 9);
        tft.print(alertNamesShort[index]);
        
        // Priority Label Right-Aligned
        tft.setTextSize(TEXT_SMALL);
        tft.setTextColor(prioColor);
        int16_t x1, y1;
        uint16_t tw, th;
        tft.getTextBounds(prioTag, 0, 0, &x1, &y1, &tw, &th);
        tft.setCursor(cardX + cardW - tw - 12, slotY + 12);
        tft.print(prioTag);
        
    } else {
        // SELECTED / ARMED CARD: Eye immediately drawn here
        // 1. High-Contrast Deep Navy Body
        tft.fillRect(cardX, slotY, cardW, cardH, RGB565(14, 32, 54));
        
        // 2. Glowing Razor-Sharp Cyan Borders (cleanly bounded)
        tft.drawRect(cardX, slotY, cardW, cardH, COLOR_CYAN);
        tft.drawRect(cardX + 1, slotY + 1, cardW - 2, cardH - 2, COLOR_CYAN_DARK);
        
        // 3. Solid Left Priority Indicator (6px)
        tft.fillRect(cardX, slotY, 6, cardH, prioColor);
        
        // 4. Solid Inverted Number Badge
        tft.fillRect(cardX + 10, slotY + 6, 22, 20, COLOR_CYAN);
        tft.drawRect(cardX + 10, slotY + 6, 22, 20, COLOR_WHITE);
        tft.setTextSize(TEXT_SMALL);
        tft.setTextColor(COLOR_TEXT_DARK);
        tft.setCursor(cardX + 14, slotY + 11);
        if (index + 1 < 10) tft.print('0');
        tft.print(index + 1);
        
        // 5. Bold Pure White Title with Crisp Shadow
        tft.setTextSize(TEXT_MEDIUM);
        tft.setTextColor(RGB565(5, 12, 20));
        tft.setCursor(cardX + 41, slotY + 10);
        tft.print(alertNamesShort[index]);
        tft.setTextColor(COLOR_WHITE);
        tft.setCursor(cardX + 40, slotY + 9);
        tft.print(alertNamesShort[index]);
        
        // 6. Direct Action Tag: [* SEND >]
        int btnW = 76;
        int btnH = 22;
        int btnX = cardX + cardW - btnW - 8;
        int btnY = slotY + 5;
        tft.fillRect(btnX, btnY, btnW, btnH, RGB565(0, 65, 32));
        tft.drawRect(btnX, btnY, btnW, btnH, COLOR_GREEN_BRIGHT);
        tft.setTextSize(TEXT_SMALL);
        tft.setTextColor(COLOR_WHITE);
        tft.setCursor(btnX + 6, btnY + 6);
        tft.print(F("* SEND"));
        tft.setTextColor(COLOR_GREEN_BRIGHT);
        tft.print(F(" >"));
    }
}

void drawElevatorRail(int scrollOffset, int totalItems, int visibleCount) {
    int railX = 308;
    int railY = 36;
    int railW = 4;
    int railH = 168;
    
    // Minimalist 4px Track Line
    tft.fillRect(railX, railY, railW, railH, RGB565(24, 30, 42));
    
    // Glowing Cyan Thumb
    int thumbH = (railH * visibleCount) / totalItems;
    if (thumbH < 24) thumbH = 24;
    
    int maxOffset = max(1, totalItems - visibleCount);
    int thumbY = railY + (scrollOffset * (railH - thumbH)) / maxOffset;
    
    tft.fillRect(railX, thumbY, railW, thumbH, COLOR_CYAN);
}

void drawHomeCommandDeck() {
    // 1. Crisp Hairline Divider
    tft.drawFastHLine(0, 208, SCREEN_WIDTH, RGB565(32, 42, 60));
    
    // 2. Command Deck Background (y = 209..239)
    tft.fillRect(0, 209, SCREEN_WIDTH, 31, RGB565(12, 16, 24));
    
    // 3. Clear, Spacious Action Labels
    // Left: Scroll
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_CYAN);
    tft.setCursor(14, 219);
    tft.print(F("[A/B]"));
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.print(F(" Scroll"));
    
    // Center: Select & Send
    tft.setTextColor(COLOR_GREEN_BRIGHT);
    tft.setCursor(114, 219);
    tft.print(F("[*]"));
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.print(F(" Select & Send"));
    
    // Right: OTA
    tft.setTextColor(COLOR_AMBER);
    tft.setCursor(246, 219);
    tft.print(F("[C]"));
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.print(F(" OTA"));
}

void updateMenuSelection(int oldIndex, int newIndex) {
    const int CARD_HEIGHT = 32;
    const int CARD_SPACING = 2;
    const int LIST_START_Y = 36;
    
    // Redraw old card in idle state
    if (oldIndex >= menuScrollOffset && oldIndex < menuScrollOffset + VISIBLE_MENU_ITEMS) {
        int oldSlot = oldIndex - menuScrollOffset;
        int oldY = LIST_START_Y + (oldSlot * (CARD_HEIGHT + CARD_SPACING));
        drawSharpAlertCard(oldIndex, oldY, false);
    }
    
    // Redraw new card in armed/selected state
    if (newIndex >= menuScrollOffset && newIndex < menuScrollOffset + VISIBLE_MENU_ITEMS) {
        int newSlot = newIndex - menuScrollOffset;
        int newY = LIST_START_Y + (newSlot * (CARD_HEIGHT + CARD_SPACING));
        drawSharpAlertCard(newIndex, newY, true);
    }
    
    // Update right minimalist scroll rail
    drawElevatorRail(menuScrollOffset, ALERT_COUNT, VISIBLE_MENU_ITEMS);
}

void drawMenuCards() {
    const int CARD_HEIGHT = 32;
    const int CARD_SPACING = 2;
    const int LIST_START_Y = 36;
    
    for (int i = 0; i < VISIBLE_MENU_ITEMS; i++) {
        int alertIndex = menuScrollOffset + i;
        if (alertIndex >= ALERT_COUNT) break;
        int slotY = LIST_START_Y + (i * (CARD_HEIGHT + CARD_SPACING));
        bool isSelected = (alertIndex == selectedAlertIndex);
        drawSharpAlertCard(alertIndex, slotY, isSelected);
    }
    
    drawElevatorRail(menuScrollOffset, ALERT_COUNT, VISIBLE_MENU_ITEMS);
}

void drawMenuScreen() {
    // 1. Clean Hardware Status Bar (y = 0..31)
    drawHomeStatusBar();
    
    // 2. Clear cards canvas area once (y = 32..207)
    tft.fillRect(0, 32, SCREEN_WIDTH, 176, COLOR_BG_PRIMARY);
    
    // 3. 5 High-Legibility Alert Cards & Scroll Rail (y = 36..203)
    drawMenuCards();
    
    // 4. Clean Action Command Footer (y = 208..239)
    drawHomeCommandDeck();
    
    Serial.println(F("[SCREEN] Clean High-Legibility Home Screen displayed"));
}


void drawConfirmScreen() {
    drawGradientV(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, RGB565(60, 45, 0), COLOR_AMBER_DARK);
    tft.fillRect(0, HEADER_HEIGHT + 1, SCREEN_WIDTH, SCREEN_HEIGHT - HEADER_HEIGHT - 1, COLOR_BG_PRIMARY);
    
    tft.drawFastHLine(0, HEADER_HEIGHT - 1, SCREEN_WIDTH, COLOR_AMBER_BRIGHT);
    tft.drawFastHLine(0, HEADER_HEIGHT, SCREEN_WIDTH, COLOR_AMBER);
    
    int triX = MARGIN + 20;
    int triCenterY = HEADER_HEIGHT / 2;
    
    for (int g = 3; g > 0; g--) {
        uint16_t glowColor = RGB565(30 - g*8, 25 - g*6, 0);
        tft.fillTriangle(triX, triCenterY - 11 - g, triX - 11 - g, triCenterY + 7 + g, triX + 11 + g, triCenterY + 7 + g, glowColor);
    }
    
    tft.fillTriangle(triX, triCenterY - 9, triX - 9, triCenterY + 6, triX + 9, triCenterY + 6, COLOR_TEXT_DARK);
    tft.setTextSize(TEXT_MEDIUM);
    tft.setTextColor(COLOR_AMBER);
    tft.setCursor(triX - 5, triCenterY - 5);
    tft.print('!');
    
    int headerTextX = MARGIN + 42;
    int headerTextY = (HEADER_HEIGHT - 14) / 2;
    tft.setTextSize(TEXT_MEDIUM);
    tft.setTextColor(RGB565(40, 30, 0));
    tft.setCursor(headerTextX + 1, headerTextY + 1);
    tft.print(F("CONFIRM TRANSMISSION"));
    
    tft.setTextColor(COLOR_TEXT_DARK);
    tft.setCursor(headerTextX, headerTextY);
    tft.print(F("CONFIRM TRANSMISSION"));
    
    int contentY = HEADER_HEIGHT + 12;
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(MARGIN, contentY);
    tft.print(F("SELECTED ALERT"));
    
    tft.drawFastHLine(MARGIN + 92, contentY + 4, SCREEN_WIDTH - MARGIN * 2 - 92, COLOR_ACCENT_LINE);
    
    int alertBoxY = contentY + 18;
    int alertBoxH = 58;
    int alertBoxW = SCREEN_WIDTH - (2 * MARGIN);
    uint16_t alertColor = getAlertColor(selectedAlertIndex);
    
    tft.fillRoundRect(MARGIN + 3, alertBoxY + 3, alertBoxW, alertBoxH, 6, RGB565(5, 5, 10));
    tft.fillRoundRect(MARGIN, alertBoxY, alertBoxW, alertBoxH, 6, COLOR_BG_CARD);
    
    for (int g = 3; g >= 0; g--) {
        uint16_t barColor = (g == 0) ? alertColor : RGB565(
            ((alertColor >> 11) & 0x1F) / (g + 1),
            ((alertColor >> 5) & 0x3F) / (g + 1),
            (alertColor & 0x1F) / (g + 1)
        );
        tft.fillRect(MARGIN, alertBoxY, 6 + g, alertBoxH, barColor);
    }
    
    tft.drawFastHLine(MARGIN + 12, alertBoxY + 2, alertBoxW - 24, RGB565(50, 50, 70));
    
    tft.setTextSize(TEXT_MEDIUM);
    tft.setTextColor(alertColor);
    tft.setCursor(MARGIN + 18, alertBoxY + 12);
    tft.print(alertNames[selectedAlertIndex]);
    
    int codeBadgeW = 34;
    int codeBadgeH = 20;
    int codeBadgeX = SCREEN_WIDTH - MARGIN - codeBadgeW - 8;
    int codeBadgeY = alertBoxY + 10;
    tft.fillRoundRect(codeBadgeX, codeBadgeY, codeBadgeW, codeBadgeH, 4, COLOR_BG_INPUT);
    tft.drawRoundRect(codeBadgeX, codeBadgeY, codeBadgeW, codeBadgeH, 4, alertColor);
    tft.setTextSize(TEXT_MEDIUM);
    tft.setTextColor(alertColor);
    tft.setCursor(codeBadgeX + 10, codeBadgeY + 3);
    tft.print(getAlertCode(selectedAlertIndex));
    
    const char* prioLabels[] = {"CRITICAL", "HIGH", "MEDIUM", "OK", "INFO"};
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(MARGIN + 18, alertBoxY + 38);
    tft.print(F("Priority: "));
    tft.setTextColor(alertColor);
    tft.print(prioLabels[min((int)alertPriority[selectedAlertIndex], 4)]);
    
    int deviceInfoY = alertBoxY + alertBoxH + 10;
    int deviceInfoH = 36;
    drawPremiumCard(MARGIN, deviceInfoY, alertBoxW, deviceInfoH, COLOR_BG_CARD, COLOR_BORDER);
    
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(MARGIN + 12, deviceInfoY + 8);
    tft.print(F("FROM DEVICE:"));
    
    tft.setTextColor(COLOR_CYAN);
    char devStr[24];
    sprintf(devStr, "TX Unit #%03d", DEVICE_ID);
    tft.setCursor(MARGIN + 12, deviceInfoY + 20);
    tft.print(devStr);
    
    int loraIndicatorX = SCREEN_WIDTH - MARGIN - 22;
    int loraIndicatorY = deviceInfoY + deviceInfoH / 2;
    tft.fillCircle(loraIndicatorX, loraIndicatorY, 6, loraInitialized ? COLOR_GREEN : COLOR_RED);
    
    int btnY = SCREEN_HEIGHT - 54;
    int btnW = 105;
    int btnH = 38;
    int btnGap = 20;
    int totalBtnWidth = (btnW * 2) + btnGap;
    int btnStartX = (SCREEN_WIDTH - totalBtnWidth) / 2;
    
    for (int g = 2; g >= 0; g--) {
        uint16_t glowColor = RGB565(0, 50 - g*15, 25 - g*8);
        tft.fillRoundRect(btnStartX - g, btnY - g, btnW + g*2, btnH + g*2, 6, glowColor);
    }
    tft.fillRoundRect(btnStartX, btnY, btnW, btnH, 5, COLOR_GREEN);
    tft.drawFastHLine(btnStartX + 6, btnY + 3, btnW - 12, COLOR_GREEN_BRIGHT);
    
    tft.setTextSize(TEXT_MEDIUM);
    tft.setTextColor(COLOR_TEXT_DARK);
    int16_t tx1, ty1;
    uint16_t tw1, th1;
    tft.getTextBounds("* SEND", 0, 0, &tx1, &ty1, &tw1, &th1);
    tft.setCursor(btnStartX + (btnW - tw1) / 2, btnY + (btnH - th1) / 2);
    tft.print(F("* SEND"));
    
    int cancelX = btnStartX + btnW + btnGap;
    tft.fillRoundRect(cancelX, btnY, btnW, btnH, 5, COLOR_BG_CARD);
    tft.drawRoundRect(cancelX, btnY, btnW, btnH, 5, COLOR_RED);
    tft.drawRoundRect(cancelX + 1, btnY + 1, btnW - 2, btnH - 2, 4, COLOR_RED_DARK);
    
    tft.setTextColor(COLOR_RED);
    tft.getTextBounds("# CANCEL", 0, 0, &tx1, &ty1, &tw1, &th1);
    tft.setCursor(cancelX + (btnW - tw1) / 2, btnY + (btnH - th1) / 2);
    tft.print(F("# CANCEL"));
    
    Serial.println(F("[SCREEN] Confirm screen displayed"));
}

void drawSendingScreen() {
    drawGradientV(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, RGB565(15, 35, 55), COLOR_BG_HEADER);
    drawGradientV(0, HEADER_HEIGHT + 1, SCREEN_WIDTH, SCREEN_HEIGHT - HEADER_HEIGHT - 1, RGB565(8, 15, 25), COLOR_BG_PRIMARY);
    tft.drawFastHLine(0, HEADER_HEIGHT - 1, SCREEN_WIDTH, COLOR_CYAN_BRIGHT);
    tft.drawFastHLine(0, HEADER_HEIGHT, SCREEN_WIDTH, COLOR_CYAN);
    
    tft.setTextSize(TEXT_MEDIUM);
    tft.setTextColor(COLOR_CYAN);
    tft.setCursor(MARGIN, (HEADER_HEIGHT - 14) / 2);
    tft.print(F("TRANSMITTING"));
    
    int dotsStartX = MARGIN + 130;
    for (int d = 0; d < 3; d++) {
        tft.fillCircle(dotsStartX + d * 12, HEADER_HEIGHT / 2, 3, COLOR_CYAN);
    }
    
    int cx = SCREEN_WIDTH / 2;
    int cy = 90;
    
    for (int i = 4; i > 0; i--) {
        uint16_t ringColor = RGB565(80 - i*15, 40 - i*8, 10);
        int ringRadius = 18 + (i * 14);
        tft.drawCircle(cx, cy, ringRadius, ringColor);
        tft.drawCircle(cx, cy, ringRadius + 1, ringColor);
    }
    
    drawGlowCircle(cx, cy, 16, COLOR_ORANGE);
    tft.fillRect(cx - 3, cy - 26, 6, 12, COLOR_ORANGE);
    tft.fillCircle(cx, cy - 28, 5, COLOR_ORANGE);
    
    tft.setTextColor(COLOR_ORANGE);
    tft.setTextSize(TEXT_SMALL);
    tft.setCursor(cx - 55, cy - 3);
    tft.print(F(">>>"));
    tft.setCursor(cx + 28, cy - 3);
    tft.print(F(">>>"));
    
    int alertCardY = 145;
    int alertCardH = 54;
    int alertCardW = SCREEN_WIDTH - MARGIN * 2;
    
    drawPremiumCard(MARGIN, alertCardY, alertCardW, alertCardH, COLOR_BG_CARD, COLOR_ORANGE_DARK);
    tft.fillRect(MARGIN, alertCardY, 5, alertCardH, COLOR_ORANGE);
    
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(MARGIN + 14, alertCardY + 10);
    tft.print(F("SENDING ALERT:"));
    
    tft.setTextSize(TEXT_MEDIUM);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.setCursor(MARGIN + 14, alertCardY + 28);
    tft.print(alertNamesShort[selectedAlertIndex]);
    
    uint16_t alertColor = getAlertColor(selectedAlertIndex);
    int sendBadgeW = 32;
    int sendBadgeH = 24;
    int sendBadgeX = SCREEN_WIDTH - MARGIN - sendBadgeW - 10;
    int sendBadgeY = alertCardY + (alertCardH - sendBadgeH) / 2;
    tft.fillRoundRect(sendBadgeX, sendBadgeY, sendBadgeW, sendBadgeH, 4, alertColor);
    tft.setTextSize(TEXT_MEDIUM);
    tft.setTextColor(COLOR_TEXT_DARK);
    tft.setCursor(sendBadgeX + 9, sendBadgeY + 5);
    tft.print(getAlertCode(selectedAlertIndex));
    
    int deviceInfoY = alertCardY + alertCardH + 10;
    char devStr[24];
    sprintf(devStr, "From TX Unit #%03d", DEVICE_ID);
    drawCenteredText(devStr, deviceInfoY, TEXT_SMALL, COLOR_TEXT_MUTED);
    
    int progressY = deviceInfoY + 20;
    int progressW = SCREEN_WIDTH - MARGIN * 4;
    int progressX = (SCREEN_WIDTH - progressW) / 2;
    int progressH = 8;
    
    tft.fillRoundRect(progressX, progressY, progressW, progressH, 3, COLOR_BG_CARD);
    int fillW = progressW / 2;
    tft.fillRoundRect(progressX + 1, progressY + 1, fillW, progressH - 2, 2, COLOR_ORANGE);
    
    Serial.println(F("[SCREEN] Premium Sending screen displayed"));
}

void drawResultScreen() {
    if (lastTransmitSuccess) {
        drawGradientV(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, RGB565(15, 60, 35), COLOR_GREEN_DARK);
        drawGradientV(0, HEADER_HEIGHT + 1, SCREEN_WIDTH, SCREEN_HEIGHT - HEADER_HEIGHT - 1, RGB565(10, 40, 25), RGB565(5, 20, 12));
        tft.drawFastHLine(0, HEADER_HEIGHT - 1, SCREEN_WIDTH, COLOR_GREEN_BRIGHT);
        tft.drawFastHLine(0, HEADER_HEIGHT, SCREEN_WIDTH, COLOR_GREEN);
        
        tft.setTextSize(TEXT_MEDIUM);
        tft.setTextColor(COLOR_TEXT_PRIMARY);
        tft.setCursor(MARGIN, (HEADER_HEIGHT - 14) / 2);
        tft.print(F("TRANSMISSION SUCCESS"));
        
        int cx = SCREEN_WIDTH / 2;
        int cy = 92;
        int circleRadius = 30;
        
        for (int g = 4; g > 0; g--) {
            uint16_t glowColor = RGB565(0, 70 - g*14, 35 - g*7);
            tft.drawCircle(cx, cy, circleRadius + g*3, glowColor);
        }
        
        tft.fillCircle(cx, cy, circleRadius, COLOR_GREEN);
        tft.drawCircle(cx - 4, cy - 4, circleRadius - 4, COLOR_GREEN_BRIGHT);
        
        for (int t = -3; t <= 3; t++) {
            tft.drawLine(cx - 14, cy + t, cx - 4, cy + 12 + t, COLOR_TEXT_DARK);
            tft.drawLine(cx - 4, cy + 12 + t, cx + 16, cy - 10 + t, COLOR_TEXT_DARK);
        }
        
        int successCardY = 138;
        int successCardH = 50;
        int successCardW = SCREEN_WIDTH - MARGIN * 2;
        drawPremiumCard(MARGIN, successCardY, successCardW, successCardH, COLOR_BG_CARD, COLOR_GREEN_DARK);
        tft.fillRect(MARGIN, successCardY, 5, successCardH, COLOR_GREEN);
        
        tft.setTextSize(TEXT_MEDIUM);
        tft.setTextColor(COLOR_GREEN_BRIGHT);
        tft.setCursor(MARGIN + 16, successCardY + 10);
        tft.print(F("Message Sent!"));
        
        tft.setTextSize(TEXT_SMALL);
        tft.setTextColor(COLOR_TEXT_SECONDARY);
        tft.setCursor(MARGIN + 16, successCardY + 32);
        tft.print(F("Alert: "));
        tft.setTextColor(COLOR_TEXT_PRIMARY);
        tft.print(alertNamesShort[selectedAlertIndex]);
        
        int autoReturnY = SCREEN_HEIGHT - 38;
        int autoReturnH = 26;
        tft.fillRoundRect(MARGIN * 2, autoReturnY, SCREEN_WIDTH - MARGIN * 4, autoReturnH, 4, RGB565(15, 30, 20));
        drawCenteredText("Returning to menu...", autoReturnY + 9, TEXT_SMALL, COLOR_TEXT_MUTED);
        
        setLED(LED_GREEN, true);
        setLED(LED_RED, false);
        playSuccessTone();
        
    } else {
        drawGradientV(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, RGB565(70, 20, 20), COLOR_RED_DARK);
        drawGradientV(0, HEADER_HEIGHT + 1, SCREEN_WIDTH, SCREEN_HEIGHT - HEADER_HEIGHT - 1, RGB565(45, 15, 15), RGB565(20, 8, 8));
        tft.drawFastHLine(0, HEADER_HEIGHT - 1, SCREEN_WIDTH, COLOR_RED_BRIGHT);
        tft.drawFastHLine(0, HEADER_HEIGHT, SCREEN_WIDTH, COLOR_RED);
        
        tft.setTextSize(TEXT_MEDIUM);
        tft.setTextColor(COLOR_TEXT_PRIMARY);
        tft.setCursor(MARGIN, (HEADER_HEIGHT - 14) / 2);
        tft.print(F("TRANSMISSION FAILED"));
        
        int cx = SCREEN_WIDTH / 2;
        int cy = 88;
        int circleRadius = 30;
        
        for (int g = 4; g > 0; g--) {
            uint16_t glowColor = RGB565(70 - g*14, 18 - g*3, 18 - g*3);
            tft.drawCircle(cx, cy, circleRadius + g*3, glowColor);
        }
        
        tft.fillCircle(cx, cy, circleRadius, COLOR_CYAN);
        
        int xSize = 14;
        for (int t = -3; t <= 3; t++) {
            tft.drawLine(cx - xSize + t, cy - xSize, cx + xSize + t, cy + xSize, COLOR_TEXT_PRIMARY);
            tft.drawLine(cx + xSize + t, cy - xSize, cx - xSize + t, cy + xSize, COLOR_TEXT_PRIMARY);
        }
        
        drawCenteredText("Send Failed!", 138, TEXT_LARGE, COLOR_CYAN);
        
        int retryCardY = 166;
        int retryCardH = 32;
        int retryCardW = SCREEN_WIDTH - MARGIN * 2;
        drawPremiumCard(MARGIN, retryCardY, retryCardW, retryCardH, COLOR_BG_CARD, COLOR_BORDER);
        
        char retryStr[32];
        sprintf(retryStr, "Attempt %d of %d failed", retryCount + 1, MAX_RETRY_ATTEMPTS);
        drawCenteredText(retryStr, retryCardY + 10, TEXT_SMALL, COLOR_TEXT_SECONDARY);
        
        int failBtnY = SCREEN_HEIGHT - 52;
        int failBtnW = 100;
        int failBtnH = 36;
        int failBtnGap = 20;
        int failBtnTotalW = (failBtnW * 2) + failBtnGap;
        int failBtnStartX = (SCREEN_WIDTH - failBtnTotalW) / 2;
        
        for (int g = 2; g >= 0; g--) {
            uint16_t glowColor = RGB565(50 - g*15, 40 - g*12, 0);
            tft.fillRoundRect(failBtnStartX - g, failBtnY - g, failBtnW + g*2, failBtnH + g*2, 5, glowColor);
        }
        tft.fillRoundRect(failBtnStartX, failBtnY, failBtnW, failBtnH, 5, COLOR_AMBER);
        tft.drawFastHLine(failBtnStartX + 6, failBtnY + 3, failBtnW - 12, COLOR_AMBER_BRIGHT);
        
        tft.setTextSize(TEXT_MEDIUM);
        tft.setTextColor(COLOR_TEXT_DARK);
        int16_t rtx, rty;
        uint16_t rtw, rth;
        tft.getTextBounds("* RETRY", 0, 0, &rtx, &rty, &rtw, &rth);
        tft.setCursor(failBtnStartX + (failBtnW - rtw) / 2, failBtnY + (failBtnH - rth) / 2);
        tft.print(F("* RETRY"));
        
        int menuBtnX = failBtnStartX + failBtnW + failBtnGap;
        tft.fillRoundRect(menuBtnX, failBtnY, failBtnW, failBtnH, 5, COLOR_BG_CARD);
        tft.drawRoundRect(menuBtnX, failBtnY, failBtnW, failBtnH, 5, COLOR_TEXT_SECONDARY);
        
        tft.setTextColor(COLOR_TEXT_SECONDARY);
        tft.getTextBounds("# MENU", 0, 0, &rtx, &rty, &rtw, &rth);
        tft.setCursor(menuBtnX + (failBtnW - rtw) / 2, failBtnY + (failBtnH - rth) / 2);
        tft.print(F("# MENU"));
        
        setLED(LED_GREEN, false);
        setLED(LED_RED, true);
        playErrorTone();
    }
    
    resultStartTime = millis();
    Serial.printf("[SCREEN] Premium Result: %s\n", lastTransmitSuccess ? "SUCCESS" : "FAILED");
}

void drawSystemInfoScreen() {
    drawHeader("SYSTEM INFORMATION");
    tft.fillRect(0, HEADER_HEIGHT + 1, SCREEN_WIDTH, SCREEN_HEIGHT - HEADER_HEIGHT - 1, COLOR_BG_PRIMARY);
    
    int y = HEADER_HEIGHT + 10;
    int cardX = MARGIN;
    int cardW = SCREEN_WIDTH - MARGIN * 2;
    int cardSpacing = 8;
    
    int deviceCardH = 52;
    drawPremiumCard(cardX, y, cardW, deviceCardH, COLOR_BG_CARD, COLOR_CYAN_DARK);
    
    int iconCenterY = y + deviceCardH / 2;
    tft.fillCircle(cardX + 18, iconCenterY, 10, COLOR_CYAN);
    tft.setTextColor(COLOR_TEXT_DARK);
    tft.setTextSize(TEXT_SMALL);
    tft.setCursor(cardX + 14, iconCenterY - 3);
    tft.print(F("D"));
    
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_CYAN);
    tft.setCursor(cardX + 36, y + 10);
    tft.print(F("DEVICE"));
    
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.setTextSize(TEXT_MEDIUM);
    tft.setCursor(cardX + 36, y + 26);
    tft.print(DEVICE_NAME);
    
    char idStr[12];
    sprintf(idStr, "#%03d", DEVICE_ID);
    int badgeW = 48;
    int badgeH = 24;
    int badgeX = cardX + cardW - badgeW - 10;
    int badgeY = y + (deviceCardH - badgeH) / 2;
    tft.fillRoundRect(badgeX, badgeY, badgeW, badgeH, 4, COLOR_BG_INPUT);
    tft.drawRoundRect(badgeX, badgeY, badgeW, badgeH, 4, COLOR_CYAN);
    tft.setTextSize(TEXT_MEDIUM);
    tft.setTextColor(COLOR_CYAN);
    tft.setCursor(badgeX + 6, badgeY + 5);
    tft.print(idStr);
    
    y += deviceCardH + cardSpacing;
    
    int loraCardH = 62;
    uint16_t loraBorderColor = loraInitialized ? COLOR_GREEN_DARK : COLOR_RED_DARK;
    drawPremiumCard(cardX, y, cardW, loraCardH, COLOR_BG_CARD, loraBorderColor);
    
    int indicatorX = cardX + 18;
    int indicatorCenterY = y + loraCardH / 2;
    uint16_t statusColor = loraInitialized ? COLOR_GREEN : COLOR_RED;
    
    for (int g = 3; g > 0; g--) {
        uint16_t glowColor = loraInitialized ? 
            RGB565(0, 40 - g*10, 20 - g*5) : 
            RGB565(40 - g*10, 10 - g*2, 10 - g*2);
        tft.drawCircle(indicatorX, indicatorCenterY, 9 + g, glowColor);
    }
    tft.fillCircle(indicatorX, indicatorCenterY, 9, statusColor);
    tft.fillCircle(indicatorX - 2, indicatorCenterY - 2, 2, COLOR_TEXT_PRIMARY);
    
    int loraTextX = cardX + 38;
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_CYAN);
    tft.setCursor(loraTextX, y + 8);
    tft.print(F("LORA RADIO"));
    
    tft.setTextColor(loraInitialized ? COLOR_GREEN : COLOR_RED);
    tft.setTextSize(TEXT_MEDIUM);
    tft.setCursor(loraTextX, y + 24);
    tft.print(loraInitialized ? F("CONNECTED") : F("ERROR"));
    
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(loraTextX, y + 44);
    char freqStr[24];
    sprintf(freqStr, "%.1f MHz | SF%d", LORA_FREQUENCY / 1E6, LORA_SF);
    tft.print(freqStr);
    
    int txStatusX = cardX + cardW - 55;
    tft.setCursor(txStatusX, y + 44);
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.print(F("TX: "));
    if (totalTransmissions > 0) {
        tft.setTextColor(lastTransmitSuccess ? COLOR_GREEN : COLOR_RED);
        tft.print(lastTransmitSuccess ? F("OK") : F("--"));
    } else {
        tft.setTextColor(COLOR_TEXT_MUTED);
        tft.print(F("--"));
    }
    
    y += loraCardH + cardSpacing;
    
    int sysCardH = 54;
    drawPremiumCard(cardX, y, cardW, sysCardH, COLOR_BG_CARD, COLOR_PURPLE_DARK);
    
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_PURPLE);
    tft.setCursor(cardX + 12, y + 8);
    tft.print(F("SYSTEM"));
    
    int dataRow1Y = y + 22;
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(cardX + 12, dataRow1Y);
    tft.print(F("Power:"));
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.setCursor(cardX + 58, dataRow1Y);
    if (batteryPercent >= 0) {
        tft.print(batteryPercent);
        tft.print(F("%"));
    } else {
        tft.print(F("USB"));
    }
    
    int dataRow2Y = y + 38;
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(cardX + 12, dataRow2Y);
    tft.print(F("FW:"));
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.setCursor(cardX + 36, dataRow2Y);
    tft.print(FIRMWARE_VERSION);
    
    int statsX = cardX + cardW - 95;
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(statsX, dataRow1Y);
    tft.print(F("Sent:"));
    
    char statsStr[16];
    sprintf(statsStr, "%d/%d", successfulTransmissions, totalTransmissions);
    tft.setTextColor(COLOR_AMBER);
    tft.setCursor(statsX + 38, dataRow1Y);
    tft.print(statsStr);
    
    // SPU Sensor Node Telemetry Card
    y += sysCardH + cardSpacing;
    int spuCardH = 65;
    drawPremiumCard(cardX, y, cardW, spuCardH, COLOR_BG_CARD, COLOR_CYAN_DARK);
    
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_CYAN);
    tft.setCursor(cardX + 12, y + 6);
    tft.print(F("SPU SENSOR NODE"));
    
    if (hasSPUTelemetry()) {
        TelemetryPacket pkt = getLatestSPUTelemetry();
        
        tft.setTextColor(COLOR_GREEN_BRIGHT);
        tft.setCursor(cardX + cardW - 65, y + 6);
        tft.print(F("CONNECTED"));
        
        tft.setTextColor(COLOR_TEXT_SECONDARY);
        tft.setCursor(cardX + 12, y + 22);
        tft.printf("Temp: %.1fC  Hum: %u%%  Gas: %uPPM",
                   pkt.temp_c_x10 / 10.0, pkt.humidity_x10 / 10, pkt.gas_ppm);
        
        tft.setCursor(cardX + 12, y + 36);
        tft.printf("GPS: %.4f, %.4f (Fix: %s)",
                   pkt.lat_deg_e7 / 10000000.0, pkt.lon_deg_e7 / 10000000.0,
                   pkt.gps_fix ? "OK" : "NO");
        
        tft.setCursor(cardX + 12, y + 50);
        tft.printf("Health: %u%%  Risk: %u%%  Batt: %u%%",
                   pkt.health_score, pkt.risk_score, pkt.battery_percent);
    } else {
        tft.setTextColor(COLOR_AMBER);
        tft.setCursor(cardX + cardW - 75, y + 6);
        tft.print(F("LISTENING"));
        
        tft.setTextColor(COLOR_TEXT_MUTED);
        tft.setCursor(cardX + 12, y + 30);
        tft.print(F("Waiting for SPU telemetry (GPIO 34)..."));
    }
    
    drawFooter("Press any key to return");
    
    Serial.println(F("[SCREEN] Premium System Info displayed"));
}

void drawUserManualScreen() {
    drawHeader("USER MANUAL");
    tft.fillRect(0, HEADER_HEIGHT + 1, SCREEN_WIDTH, SCREEN_HEIGHT - HEADER_HEIGHT - FOOTER_HEIGHT - 2, COLOR_BG_PRIMARY);
    
    char pageStr[10];
    sprintf(pageStr, "%d/%d", manualPage + 1, MANUAL_TOTAL_PAGES);
    int badgeW = 36;
    int badgeX = SCREEN_WIDTH - MARGIN - badgeW;
    tft.fillRoundRect(badgeX, 10, badgeW, 18, 4, COLOR_BG_CARD);
    tft.drawRoundRect(badgeX, 10, badgeW, 18, 4, COLOR_PURPLE);
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_PURPLE);
    tft.setCursor(badgeX + 8, 14);
    tft.print(pageStr);
    
    int cardY = HEADER_HEIGHT + 6;
    int cardH = SCREEN_HEIGHT - HEADER_HEIGHT - FOOTER_HEIGHT - 14;
    drawPremiumCard(MARGIN, cardY, SCREEN_WIDTH - MARGIN * 2, cardH, COLOR_BG_CARD, COLOR_BORDER);
    
    int y = cardY + 10;
    int contentX = MARGIN + 12;
    
    tft.setTextSize(TEXT_SMALL);
    
    switch (manualPage) {
        case 0:
            tft.fillRoundRect(contentX - 4, y, 28, 20, 4, COLOR_AMBER);
            tft.setTextColor(COLOR_TEXT_DARK);
            tft.setCursor(contentX, y + 6);
            tft.print(F("123"));
            
            tft.setTextColor(COLOR_TEXT_PRIMARY);
            tft.setTextSize(TEXT_MEDIUM);
            tft.setCursor(contentX + 36, y + 3);
            tft.print(F("SELECT ALERTS"));
            y += 28;
            
            tft.drawFastHLine(contentX, y, SCREEN_WIDTH - MARGIN * 2 - 24, COLOR_AMBER_DARK);
            y += 10;
            
            tft.setTextSize(TEXT_SMALL);
            tft.setTextColor(COLOR_TEXT_SECONDARY);
            tft.setCursor(contentX, y);
            tft.print(F("Use number keys"));
            tft.setTextColor(COLOR_AMBER);
            tft.print(F(" 1-9 "));
            tft.setTextColor(COLOR_TEXT_SECONDARY);
            tft.print(F("for"));
            y += 14;
            tft.setCursor(contentX, y);
            tft.print(F("instant selection."));
            y += 20;
            
            tft.setTextColor(COLOR_TEXT_SECONDARY);
            tft.setCursor(contentX, y);
            tft.print(F("Press"));
            tft.setTextColor(COLOR_AMBER);
            tft.print(F(" 0 "));
            tft.setTextColor(COLOR_TEXT_SECONDARY);
            tft.print(F("for alert #10"));
            y += 22;
            
            tft.fillRoundRect(contentX - 4, y, SCREEN_WIDTH - MARGIN * 2 - 16, 42, 4, RGB565(30, 30, 20));
            tft.drawRoundRect(contentX - 4, y, SCREEN_WIDTH - MARGIN * 2 - 16, 42, 4, COLOR_CYAN_DARK);
            
            tft.setTextColor(COLOR_CYAN);
            tft.setCursor(contentX + 4, y + 8);
            tft.print(F("TIP:"));
            tft.setTextColor(COLOR_TEXT_SECONDARY);
            tft.print(F(" Yellow highlight"));
            tft.setCursor(contentX + 4, y + 24);
            tft.print(F("shows current selection"));
            break;
            
        case 1:
            tft.fillRoundRect(contentX - 4, y, 28, 20, 4, COLOR_CYAN);
            tft.setTextColor(COLOR_TEXT_DARK);
            tft.setCursor(contentX + 2, y + 6);
            tft.print((char)24);
            tft.print((char)25);
            
            tft.setTextColor(COLOR_TEXT_PRIMARY);
            tft.setTextSize(TEXT_MEDIUM);
            tft.setCursor(contentX + 36, y + 3);
            tft.print(F("NAVIGATION"));
            y += 28;
            
            tft.drawFastHLine(contentX, y, SCREEN_WIDTH - MARGIN * 2 - 24, COLOR_CYAN_DARK);
            y += 12;
            
            tft.setTextSize(TEXT_SMALL);
            
            drawKeyBadge(contentX, y + 2, 'A', " Scroll UP", COLOR_CYAN);
            y += 20;
            
            drawKeyBadge(contentX, y + 2, 'B', " Scroll DOWN", COLOR_CYAN);
            y += 24;
            
            tft.drawFastHLine(contentX, y, 80, COLOR_ACCENT_LINE);
            y += 8;
            
            drawKeyBadge(contentX, y + 2, 'C', " System Info", COLOR_PURPLE);
            y += 20;
            
            drawKeyBadge(contentX, y + 2, 'D', " This Help", COLOR_PURPLE);
            y += 22;
            
            tft.setTextColor(COLOR_TEXT_MUTED);
            tft.setCursor(contentX, y);
            tft.print(F("Menu wraps at top/bottom"));
            break;
            
        case 2:
            tft.fillRoundRect(contentX - 4, y, 28, 20, 4, COLOR_GREEN);
            tft.setTextColor(COLOR_TEXT_DARK);
            tft.setCursor(contentX + 2, y + 6);
            tft.print(F("TX"));
            
            tft.setTextColor(COLOR_TEXT_PRIMARY);
            tft.setTextSize(TEXT_MEDIUM);
            tft.setCursor(contentX + 36, y + 3);
            tft.print(F("SENDING"));
            y += 28;
            
            tft.drawFastHLine(contentX, y, SCREEN_WIDTH - MARGIN * 2 - 24, COLOR_GREEN_DARK);
            y += 12;
            
            tft.setTextSize(TEXT_SMALL);
            
            drawKeyBadge(contentX, y + 2, '*', " Confirm & Send", COLOR_GREEN);
            y += 24;
            
            drawKeyBadge(contentX, y + 2, '#', " Cancel & Back", COLOR_RED);
            y += 26;
            
            tft.setTextColor(COLOR_TEXT_SECONDARY);
            tft.setCursor(contentX, y);
            tft.print(F("Confirmation screen"));
            y += 12;
            tft.setCursor(contentX, y);
            tft.print(F("appears before sending"));
            y += 18;
            
            tft.fillRoundRect(contentX - 4, y, SCREEN_WIDTH - MARGIN * 2 - 16, 26, 4, COLOR_RED_DARK);
            tft.setTextColor(COLOR_TEXT_PRIMARY);
            tft.setCursor(contentX + 4, y + 8);
            tft.print(F("Failed? Press * to retry"));
            break;
            
        case 3:
            tft.fillRoundRect(contentX - 4, y, 28, 20, 4, COLOR_PURPLE);
            tft.setTextColor(COLOR_TEXT_PRIMARY);
            tft.setCursor(contentX, y + 6);
            tft.print(F("LED"));
            
            tft.setTextColor(COLOR_TEXT_PRIMARY);
            tft.setTextSize(TEXT_MEDIUM);
            tft.setCursor(contentX + 36, y + 3);
            tft.print(F("INDICATORS"));
            y += 28;
            
            tft.drawFastHLine(contentX, y, SCREEN_WIDTH - MARGIN * 2 - 24, COLOR_PURPLE_DARK);
            y += 12;
            
            tft.setTextSize(TEXT_SMALL);
            
            drawStatusIndicator(contentX + 8, y + 6, 6, COLOR_GREEN);
            tft.setTextColor(COLOR_TEXT_SECONDARY);
            tft.setCursor(contentX + 22, y + 2);
            tft.print(F("GREEN = Sent OK"));
            y += 22;
            
            drawStatusIndicator(contentX + 8, y + 6, 6, COLOR_RED);
            tft.setTextColor(COLOR_TEXT_SECONDARY);
            tft.setCursor(contentX + 22, y + 2);
            tft.print(F("RED = Send Failed"));
            y += 28;
            
            tft.fillRoundRect(contentX - 4, y, SCREEN_WIDTH - MARGIN * 2 - 16, 52, 4, RGB565(35, 30, 15));
            tft.drawRoundRect(contentX - 4, y, SCREEN_WIDTH - MARGIN * 2 - 16, 52, 4, COLOR_AMBER_DARK);
            
            tft.setTextColor(COLOR_AMBER);
            tft.setCursor(contentX + 4, y + 6);
            tft.print(F("!! EMERGENCY TIP !!"));
            tft.setTextColor(COLOR_TEXT_SECONDARY);
            tft.setCursor(contentX + 4, y + 20);
            tft.print(F("Only send alerts when"));
            tft.setCursor(contentX + 4, y + 34);
            tft.print(F("necessary."));
            break;
    }
    
    int footerY = SCREEN_HEIGHT - FOOTER_HEIGHT;
    tft.drawFastHLine(0, footerY - 2, SCREEN_WIDTH, COLOR_CYAN_DARK);
    tft.drawFastHLine(0, footerY - 1, SCREEN_WIDTH, COLOR_ACCENT_LINE);
    drawGradientV(0, footerY, SCREEN_WIDTH, FOOTER_HEIGHT, COLOR_BG_HEADER_ALT, RGB565(5, 10, 15));
    
    int hintY = footerY + (FOOTER_HEIGHT - 8) / 2;
    
    drawKeyBadge(MARGIN, hintY, 'A', "Prev", manualPage > 0 ? COLOR_CYAN : COLOR_TEXT_DISABLED);
    drawKeyBadge(MARGIN + 50, hintY, 'B', "Next", manualPage < MANUAL_TOTAL_PAGES - 1 ? COLOR_CYAN : COLOR_TEXT_DISABLED);
    
    tft.drawFastVLine(MARGIN + 100, footerY + 6, FOOTER_HEIGHT - 12, COLOR_ACCENT_LINE);
    
    drawKeyBadge(MARGIN + 110, hintY, '#', "Back", COLOR_TEXT_SECONDARY);
    
    Serial.printf("[SCREEN] Premium Manual page %d displayed\n", manualPage + 1);
}

void drawOTASelectScreen() {
    tft.fillScreen(COLOR_BG_PRIMARY);
    drawHeader("OTA FIRMWARE");
    
    int cardW = SCREEN_WIDTH - MARGIN * 2; // 300px
    
    // Card 1: Local AP Mode
    int card1Y = CONTENT_START_Y + 2; // 48
    int card1H = 72;
    drawSharpCard(MARGIN, card1Y, cardW, card1H, COLOR_BG_CARD, COLOR_BORDER, COLOR_CYAN, true);
    
    // Header strip for Card 1
    tft.fillRect(MARGIN + 1, card1Y + 1, cardW - 2, 14, RGB565(16, 28, 44));
    tft.drawFastHLine(MARGIN + 1, card1Y + 14, cardW - 2, COLOR_CYAN_DARK);
    
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_CYAN);
    tft.setCursor(MARGIN + 8, card1Y + 4);
    tft.print(F("[ OPTION 1 ] LOCAL AP MODE"));
    
    tft.setTextSize(TEXT_MEDIUM);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.setCursor(MARGIN + 10, card1Y + 20);
    tft.print(F("PRESS [ 1 ] -> LOCAL AP"));
    
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_CYAN_BRIGHT);
    tft.setCursor(MARGIN + 10, card1Y + 40);
    tft.print(F("HOTSPOT: LifeLine-TX-OTA (192.168.4.1)"));
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(MARGIN + 10, card1Y + 54);
    tft.print(F("Direct phone/PC upload (No router needed)"));
    
    // Card 2: Network Wi-Fi Mode
    int card2Y = card1Y + card1H + 6; // 126
    int card2H = 72;
    drawSharpCard(MARGIN, card2Y, cardW, card2H, COLOR_BG_CARD, COLOR_BORDER, COLOR_RED, true);
    
    // Header strip for Card 2
    tft.fillRect(MARGIN + 1, card2Y + 1, cardW - 2, 14, RGB565(36, 16, 22));
    tft.drawFastHLine(MARGIN + 1, card2Y + 14, cardW - 2, COLOR_RED);
    
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_RED_BRIGHT);
    tft.setCursor(MARGIN + 8, card2Y + 4);
    tft.print(F("[ OPTION 2 ] NETWORK WI-FI MODE"));
    
    tft.setTextSize(TEXT_MEDIUM);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.setCursor(MARGIN + 10, card2Y + 20);
    tft.print(F("PRESS [ 2 ] -> NET"));
    
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_RED_BRIGHT);
    tft.setCursor(MARGIN + 10, card2Y + 40);
    tft.print(F("Connects to Wi-Fi Network"));
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(MARGIN + 10, card2Y + 54);
    tft.print(F("Auto-fallback to Local AP if offline"));
    
    drawFooter("1:Local AP   2:Net   #:Cancel");
    Serial.println(F("[SCREEN] OTA Selection Screen displayed"));
}

void drawOTAScreen() {
    tft.fillScreen(COLOR_BG_PRIMARY);
    drawHeader("OTA FIRMWARE PORTAL");
    
    int cardY = CONTENT_START_Y + 2; // 48
    int cardW = SCREEN_WIDTH - MARGIN * 2; // 300
    int cardH = 154; // ends at 202, exactly 6px above footer at 208
    
    OTAMode mode = getCurrentOTAMode();
    uint16_t accentCol = (mode == OTA_MODE_NET) ? COLOR_GREEN : COLOR_RED;
    
    // Sharp Main Card with corner brackets
    drawSharpCard(MARGIN, cardY, cardW, cardH, COLOR_BG_CARD, COLOR_BORDER, accentCol, true);
    
    // Top Header Strip on the card
    tft.fillRect(MARGIN + 1, cardY + 1, cardW - 2, 13, RGB565(18, 22, 32));
    tft.drawFastHLine(MARGIN + 1, cardY + 13, cardW - 2, accentCol);
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(accentCol);
    tft.setCursor(MARGIN + 8, cardY + 3);
    tft.print(mode == OTA_MODE_NET ? F("NETWORK WI-FI GATEWAY") : F("LOCAL ACCESS POINT GATEWAY"));
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(MARGIN + 215, cardY + 3);
    tft.print(F("TX #001"));
    
    // 1. Telemetry Sub-Box (y = 66, h = 34, w = 284, x = 18)
    int boxX = MARGIN + 8;
    int boxW = cardW - 16;
    int boxY = cardY + 18; // 66
    int boxH = 34;
    tft.fillRect(boxX, boxY, boxW, boxH, RGB565(10, 14, 22));
    tft.drawRect(boxX, boxY, boxW, boxH, RGB565(32, 42, 58));
    
    // Row 1: SSID and Pass/Status
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(boxX + 6, boxY + 6);
    tft.print(F("SSID: "));
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.print(getOTASSID());
    
    if (mode == OTA_MODE_LOCAL) {
        tft.setTextColor(COLOR_TEXT_MUTED);
        tft.setCursor(boxX + 175, boxY + 6);
        tft.print(F("KEY: "));
        tft.setTextColor(COLOR_TEXT_PRIMARY);
        tft.print(F("12345678"));
    } else {
        tft.setTextColor(COLOR_GREEN_BRIGHT);
        tft.setCursor(boxX + 185, boxY + 6);
        tft.print(F("LINK: ONLINE"));
    }
    
    // Row 2: Portal URL
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(boxX + 6, boxY + 20);
    tft.print(F("PORTAL: "));
    tft.setTextColor(COLOR_GREEN_BRIGHT);
    tft.printf("http://%s", getOTAIPAddress().c_str());
    
    // 2. Live Status Line (y = 106)
    int statY = cardY + 58; // 106
    tft.fillRect(boxX + 6, statY + 2, 6, 6, accentCol);
    tft.drawRect(boxX + 5, statY + 1, 8, 8, RGB565(60, 60, 80));
    
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(boxX + 18, statY);
    tft.print(F("STATUS: "));
    
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.setCursor(boxX + 68, statY);
    tft.print(getOTAStatusText());
    
    // 3. Sharp Progress Bar (y = 119, h = 16, w = 284)
    int progY = cardY + 71; // 119
    int progH = 16;
    tft.fillRect(boxX, progY, boxW, progH, RGB565(8, 10, 16));
    tft.drawRect(boxX, progY, boxW, progH, RGB565(40, 50, 70));
    
    int progress = getOTAProgress();
    if (progress > 0) {
        int fillMax = boxW - 4;
        int fillW = fillMax * progress / 100;
        if (fillW > 0) {
            uint16_t col = (progress >= 100) ? COLOR_GREEN_BRIGHT : COLOR_RED;
            tft.fillRect(boxX + 2, progY + 2, fillW, progH - 4, col);
        }
    }
    
    // Center percentage text inside progress bar
    char pctBuf[8];
    snprintf(pctBuf, sizeof(pctBuf), "%d%%", progress);
    int16_t x1, y1;
    uint16_t tw, th;
    tft.getTextBounds(pctBuf, 0, 0, &x1, &y1, &tw, &th);
    int pctX = boxX + (boxW - tw) / 2;
    int pctY = progY + (progH - th) / 2;
    tft.setTextColor(COLOR_TEXT_DARK);
    tft.setCursor(pctX + 1, pctY + 1);
    tft.print(pctBuf);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.setCursor(pctX, pctY);
    tft.print(pctBuf);
    
    // 4. Instructions & Guide Box (y = 141, h = 54)
    int guideY = cardY + 93; // 141
    int guideH = 54;
    tft.fillRect(boxX, guideY, boxW, guideH, RGB565(10, 14, 20));
    tft.drawRect(boxX, guideY, boxW, guideH, RGB565(25, 35, 48));
    
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.setCursor(boxX + 6, guideY + 6);
    tft.print(F("1. WEB : Open browser to Portal URL"));
    
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.setCursor(boxX + 6, guideY + 18);
    tft.print(F("2. CLI : pio run -t upload -e esp32dev_ota"));
    
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(boxX + 6, guideY + 30);
    tft.printf("   PORT: %s", getOTAIPAddress().c_str());
    
    tft.setTextColor(COLOR_AMBER_BRIGHT);
    tft.setCursor(boxX + 6, guideY + 42);
    tft.print(F("PRESS [#] ON KEYPAD TO EXIT OTA"));
    
    drawFooter("Press [#] to Cancel & Return to Menu");
    Serial.println(F("[SCREEN] Sharp Pro OTA Screen Displayed"));
}

void updateOTAProgressBar(int progress, const char* status) {
    static int lastProgress = -1;
    static String lastStatus = "";
    
    if (currentScreen != SCREEN_OTA) return;
    
    int cardY = CONTENT_START_Y + 2; // 48
    int cardW = SCREEN_WIDTH - MARGIN * 2; // 300
    int boxX = MARGIN + 8; // 18
    int boxW = cardW - 16; // 284
    
    // Update status text only if changed
    if (status != nullptr && String(status) != lastStatus) {
        lastStatus = String(status);
        int statY = cardY + 58; // 106
        tft.fillRect(boxX + 68, statY - 1, boxW - 72, 12, COLOR_BG_CARD);
        tft.setTextSize(TEXT_SMALL);
        tft.setTextColor(progress >= 100 ? COLOR_GREEN_BRIGHT : COLOR_TEXT_PRIMARY);
        tft.setCursor(boxX + 68, statY);
        tft.print(lastStatus);
    }
    
    // Update progress bar only if integer percentage changed
    if (progress != lastProgress) {
        lastProgress = progress;
        
        int progY = cardY + 71; // 119
        int progH = 16;
        int fillMax = boxW - 4;
        int fillW = fillMax * progress / 100;
        if (fillW < 0) fillW = 0;
        if (fillW > fillMax) fillW = fillMax;
        
        // Progress fill
        if (fillW > 0) {
            uint16_t col = (progress >= 100) ? COLOR_GREEN_BRIGHT : COLOR_RED;
            tft.fillRect(boxX + 2, progY + 2, fillW, progH - 4, col);
        }
        // Unfilled remainder
        if (fillW < fillMax) {
            tft.fillRect(boxX + 2 + fillW, progY + 2, fillMax - fillW, progH - 4, RGB565(8, 10, 16));
        }
        
        // Redraw percentage text
        char pctBuf[8];
        snprintf(pctBuf, sizeof(pctBuf), "%d%%", progress);
        int16_t x1, y1;
        uint16_t tw, th;
        tft.setTextSize(TEXT_SMALL);
        tft.getTextBounds(pctBuf, 0, 0, &x1, &y1, &tw, &th);
        int pctX = boxX + (boxW - tw) / 2;
        int pctY = progY + (progH - th) / 2;
        tft.setTextColor(COLOR_TEXT_DARK);
        tft.setCursor(pctX + 1, pctY + 1);
        tft.print(pctBuf);
        tft.setTextColor(COLOR_TEXT_PRIMARY);
        tft.setCursor(pctX, pctY);
        tft.print(pctBuf);
    }
}

void drawSensorLogScreen() {
    tft.fillScreen(COLOR_BG_PRIMARY);
    drawHeader("STANDALONE MANUAL SOS UNIT");
    
    int cardY = CONTENT_START_Y + 2;
    int cardW = SCREEN_WIDTH - MARGIN * 2;
    int cardH = 175;
    
    drawPremiumCard(MARGIN, cardY, cardW, cardH, COLOR_BG_CARD, COLOR_CYAN, true);
    
    tft.setTextSize(TEXT_SMALL);
    
    tft.setTextColor(COLOR_GREEN_BRIGHT);
    tft.setCursor(MARGIN + 12, cardY + 12);
    tft.print(F("● MANUAL SOS MODE ACTIVE"));
    
    tft.drawFastHLine(MARGIN + 10, cardY + 26, cardW - 20, COLOR_ACCENT_LINE);
    
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.setCursor(MARGIN + 12, cardY + 38);
    tft.print(F("TX Module: Manual Emergency Alert Unit"));
    
    tft.setTextColor(COLOR_CYAN);
    tft.setCursor(MARGIN + 12, cardY + 58);
    tft.print(F("SPU Node Mode: Direct Cloud Upload"));
    
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.setCursor(MARGIN + 12, cardY + 78);
    tft.print(F("• SPU sends live telemetry directly to Cloud."));
    tft.setCursor(MARGIN + 12, cardY + 94);
    tft.print(F("• Cloud Server detects risk/landslide & triggers"));
    tft.setCursor(MARGIN + 12, cardY + 108);
    tft.print(F("  push notifications automatically."));
    
    tft.setTextColor(COLOR_AMBER_BRIGHT);
    tft.setCursor(MARGIN + 12, cardY + 130);
    tft.print(F("• Keypad: Select alert 1-9 to transmit LoRa SOS."));
    
    drawFooter("# Exit Screen");
    Serial.println(F("[SCREEN] Standalone Manual SOS Info displayed"));
}


