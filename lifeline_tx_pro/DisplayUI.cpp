#include "DisplayUI.h"
#include "OTAManager.h"
#include "SPUReceiver.h"
#include "BLEManager.h"
#include "LoRaComm.h"

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
    
    // BLE Companion Status Badge
    int bleBadgeW = 44;
    int bleBadgeH = 14;
    int bleBadgeX = SCREEN_WIDTH - bleBadgeW - 8;
    int bleBadgeY = (HEADER_HEIGHT - bleBadgeH) / 2;
    if (isBLEConnected()) {
        tft.fillRect(bleBadgeX, bleBadgeY, bleBadgeW, bleBadgeH, RGB565(8, 42, 65));
        tft.drawRect(bleBadgeX, bleBadgeY, bleBadgeW, bleBadgeH, COLOR_CYAN_BRIGHT);
        tft.setTextSize(TEXT_SMALL);
        tft.setTextColor(COLOR_CYAN_BRIGHT);
        tft.setCursor(bleBadgeX + 5, bleBadgeY + 3);
        tft.print(F("BLE ON"));
    } else {
        tft.fillRect(bleBadgeX, bleBadgeY, bleBadgeW, bleBadgeH, RGB565(20, 25, 35));
        tft.drawRect(bleBadgeX, bleBadgeY, bleBadgeW, bleBadgeH, RGB565(55, 65, 85));
        tft.setTextSize(TEXT_SMALL);
        tft.setTextColor(COLOR_TEXT_MUTED);
        tft.setCursor(bleBadgeX + 3, bleBadgeY + 3);
        tft.print(F("BLE ADV"));
    }
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
    // 1. Deep tactical dark background (matching boot screen RGB565(8, 12, 18))
    tft.fillRect(0, 0, SCREEN_WIDTH, 35, RGB565(8, 12, 18));
    
    // Technical hairline outer border (top segment of boot screen HUD border)
    tft.drawFastHLine(2, 2, SCREEN_WIDTH - 4, RGB565(25, 35, 52));
    tft.drawFastHLine(4, 4, SCREEN_WIDTH - 8, RGB565(15, 22, 34));
    tft.drawFastVLine(2, 2, 33, RGB565(25, 35, 52));
    tft.drawFastVLine(SCREEN_WIDTH - 3, 2, 33, RGB565(25, 35, 52));
    tft.drawFastVLine(4, 4, 31, RGB565(15, 22, 34));
    tft.drawFastVLine(SCREEN_WIDTH - 5, 4, 31, RGB565(15, 22, 34));
    
    // Tactical Top-Left & Top-Right Registration Marks (matching boot screen)
    int mDist = 8;
    tft.drawFastHLine(mDist - 3, mDist, 7, COLOR_CYAN_DARK);
    tft.drawFastVLine(mDist, mDist - 3, 7, COLOR_CYAN_DARK);
    tft.drawFastHLine(SCREEN_WIDTH - mDist - 4, mDist, 7, COLOR_CYAN_DARK);
    tft.drawFastVLine(SCREEN_WIDTH - mDist - 1, mDist - 3, 7, COLOR_CYAN_DARK);
    
    // 2. Top Header Telemetry Strip (exact match to boot screen topStrip)
    int topStripY = 6;
    int topStripH = 16;
    tft.fillRect(6, topStripY, SCREEN_WIDTH - 12, topStripH, RGB565(14, 22, 34));
    tft.drawRect(6, topStripY, SCREEN_WIDTH - 12, topStripH, RGB565(35, 52, 75));
    tft.drawFastHLine(6, topStripY + topStripH, SCREEN_WIDTH - 12, COLOR_CYAN_DARK);
    
    // Medical Cross Icon (5x5)
    tft.fillRect(14, topStripY + 3, 2, 10, COLOR_RED);
    tft.fillRect(10, topStripY + 7, 10, 2, COLOR_RED);
    tft.drawPixel(14, topStripY + 7, COLOR_WHITE);
    
    // Device & Model branding
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_CYAN);
    tft.setCursor(24, topStripY + 4);
    tft.print(F("LIFELINE // TX #003"));
    
    // LoRa RF Status
    tft.setTextColor(loraInitialized ? COLOR_GREEN_BRIGHT : COLOR_RED);
    tft.setCursor(264, topStripY + 4);
    tft.print(loraInitialized ? F("RF:OK") : F("RF:ERR"));
    
    // 3. Sub-Header Navigation Breadcrumb (y = 25)
    tft.setTextSize(TEXT_SMALL);
    char frameStr[24];
    int firstItem = menuScrollOffset + 1;
    int lastItem = min(menuScrollOffset + VISIBLE_MENU_ITEMS, ALERT_COUNT);
    sprintf(frameStr, "FRAME [%02d-%02d]/%02d", firstItem, lastItem, ALERT_COUNT);
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(204, 25);
    tft.print(frameStr);
    tft.setTextColor(menuScrollOffset > 0 ? COLOR_CYAN : COLOR_TEXT_DISABLED);
    tft.print((char)24);
    tft.setTextColor(menuScrollOffset + VISIBLE_MENU_ITEMS < ALERT_COUNT ? COLOR_CYAN : COLOR_TEXT_DISABLED);
    tft.print((char)25);
}

void drawSharpAlertCard(int index, int slotY, bool isSelected) {
    if (index < 0 || index >= ALERT_COUNT) return;
    
    int cardX = 8;
    int cardW = 288;
    int cardH = 31;
    
    uint16_t prioColor = getAlertColor(index);
    uint8_t prio = alertPriority[index];
    const char* prioTag = "NORMAL";
    if (prio == 0) prioTag = "CRIT";
    else if (prio == 1) prioTag = "HIGH";
    else if (prio == 2) prioTag = "MED";
    else if (prio == 3) prioTag = " OK ";
    
    if (!isSelected) {
        // IDLE CARD: Tactical stealth card (matching boot screen drawSharpCard)
        tft.fillRect(cardX, slotY, cardW, cardH, RGB565(14, 20, 30));
        tft.drawRect(cardX, slotY, cardW, cardH, RGB565(32, 45, 65));
        tft.drawFastHLine(cardX + 1, slotY + 1, cardW - 2, RGB565(45, 60, 85)); // Chiseled top edge
        
        // Tactical corner brackets (3px ticks matching boot screen)
        tft.drawFastHLine(cardX, slotY, 4, RGB565(45, 65, 90));
        tft.drawFastVLine(cardX, slotY, 4, RGB565(45, 65, 90));
        tft.drawFastHLine(cardX + cardW - 4, slotY, 4, RGB565(45, 65, 90));
        tft.drawFastVLine(cardX + cardW - 1, slotY, 4, RGB565(45, 65, 90));
        tft.drawFastHLine(cardX, slotY + cardH - 1, 4, RGB565(45, 65, 90));
        tft.drawFastVLine(cardX, slotY + cardH - 4, 4, RGB565(45, 65, 90));
        tft.drawFastHLine(cardX + cardW - 4, slotY + cardH - 1, 4, RGB565(45, 65, 90));
        tft.drawFastVLine(cardX + cardW - 1, slotY + cardH - 4, 4, RGB565(45, 65, 90));
        
        // Left priority bar (4px)
        tft.fillRect(cardX, slotY, 4, cardH, prioColor);
        
        // Sharp Index Box
        tft.fillRect(cardX + 8, slotY + 5, 22, 21, RGB565(20, 28, 42));
        tft.drawRect(cardX + 8, slotY + 5, 22, 21, RGB565(40, 56, 80));
        tft.setTextSize(TEXT_SMALL);
        tft.setTextColor(COLOR_CYAN);
        tft.setCursor(cardX + 12, slotY + 11);
        if (index + 1 < 10) tft.print('0');
        tft.print(index + 1);
        
        // Alert Title
        tft.setTextSize(TEXT_MEDIUM);
        tft.setTextColor(COLOR_TEXT_PRIMARY);
        tft.setCursor(cardX + 38, slotY + 8);
        tft.print(alertNamesShort[index]);
        
        // Right Priority Badge
        tft.fillRect(cardX + cardW - 74, slotY + 6, 40, 19, RGB565(18, 26, 38));
        tft.drawRect(cardX + cardW - 74, slotY + 6, 40, 19, prioColor);
        tft.setTextSize(TEXT_SMALL);
        tft.setTextColor(prioColor);
        tft.setCursor(cardX + cardW - 68, slotY + 11);
        tft.print(prioTag);
        
        // Hotkey Badge
        tft.setTextColor(COLOR_TEXT_MUTED);
        tft.setCursor(cardX + cardW - 24, slotY + 11);
        if (index < 9) {
            tft.print('['); tft.print(index + 1); tft.print(']');
        } else if (index == 9) {
            tft.print(F("[0]"));
        } else {
            tft.print('['); tft.print(getAlertCode(index)); tft.print(']');
        }
        
    } else {
        // SELECTED / ARMED CARD: Boot screen SYS STATUS inspired!
        // 1. High-Contrast Deep Navy Cockpit Fill
        tft.fillRect(cardX, slotY, cardW, cardH, RGB565(16, 32, 52));
        
        // 2. Glowing Razor-Sharp Neon Cyan Borders
        tft.drawRect(cardX, slotY, cardW, cardH, COLOR_CYAN);
        tft.drawRect(cardX + 1, slotY + 1, cardW - 2, cardH - 2, COLOR_CYAN_DARK);
        
        // 3. Tactical Corner Brackets (6px bright white ticks)
        tft.drawFastHLine(cardX, slotY, 6, COLOR_WHITE);
        tft.drawFastVLine(cardX, slotY, 6, COLOR_WHITE);
        tft.drawFastHLine(cardX + cardW - 6, slotY, 6, COLOR_WHITE);
        tft.drawFastVLine(cardX + cardW - 1, slotY, 6, COLOR_WHITE);
        tft.drawFastHLine(cardX, slotY + cardH - 1, 6, COLOR_WHITE);
        tft.drawFastVLine(cardX, slotY + cardH - 6, 6, COLOR_WHITE);
        tft.drawFastHLine(cardX + cardW - 6, slotY + cardH - 1, 6, COLOR_WHITE);
        tft.drawFastVLine(cardX + cardW - 1, slotY + cardH - 6, 6, COLOR_WHITE);
        
        // 4. Solid Left Priority Indicator (5px)
        tft.fillRect(cardX, slotY, 5, cardH, prioColor);
        
        // 5. Tactical Pointer Chevron
        tft.drawLine(cardX + 6, slotY + 9, cardX + 10, slotY + 15, COLOR_CYAN_BRIGHT);
        tft.drawLine(cardX + 6, slotY + 21, cardX + 10, slotY + 15, COLOR_CYAN_BRIGHT);
        tft.drawLine(cardX + 7, slotY + 9, cardX + 11, slotY + 15, COLOR_WHITE);
        tft.drawLine(cardX + 7, slotY + 21, cardX + 11, slotY + 15, COLOR_WHITE);
        
        // 6. Solid Inverted Number Badge
        tft.fillRect(cardX + 13, slotY + 5, 22, 21, COLOR_CYAN);
        tft.drawRect(cardX + 13, slotY + 5, 22, 21, COLOR_WHITE);
        tft.setTextSize(TEXT_SMALL);
        tft.setTextColor(COLOR_TEXT_DARK);
        tft.setCursor(cardX + 17, slotY + 11);
        if (index + 1 < 10) tft.print('0');
        tft.print(index + 1);
        
        // 7. Bold Pure White Title with Crisp Shadow
        tft.setTextSize(TEXT_MEDIUM);
        tft.setTextColor(RGB565(5, 12, 20));
        tft.setCursor(cardX + 41, slotY + 9);
        tft.print(alertNamesShort[index]);
        tft.setTextColor(COLOR_WHITE);
        tft.setCursor(cardX + 40, slotY + 8);
        tft.print(alertNamesShort[index]);
        
        // 8. Boot-Screen SYS STATUS Inspired Action Card [* ARM]
        int btnW = 86;
        int btnH = 21;
        int btnX = cardX + cardW - btnW - 6;
        int btnY = slotY + 5;
        tft.fillRect(btnX, btnY, btnW, btnH, RGB565(10, 36, 26));
        tft.drawRect(btnX, btnY, btnW, btnH, COLOR_GREEN_BRIGHT);
        
        // Square status LED (exact match to boot screen rightCardX + 8, cardY + 23)
        tft.fillRect(btnX + 5, btnY + 7, 6, 6, COLOR_GREEN_BRIGHT);
        tft.drawRect(btnX + 4, btnY + 6, 8, 8, COLOR_GREEN_DARK);
        
        tft.setTextSize(TEXT_SMALL);
        tft.setTextColor(COLOR_WHITE);
        tft.setCursor(btnX + 17, btnY + 7);
        tft.print(F("* ARM"));
        tft.setTextColor(COLOR_GREEN_BRIGHT);
        tft.print(F(" >"));
    }
}

void drawElevatorRail(int scrollOffset, int totalItems, int visibleCount) {
    int railX = 302;
    int railY = 36;
    int railW = 9;
    int railH = 163;
    
    // Segmented Telemetry Track (matching boot screen segmented bar)
    tft.fillRect(railX, railY, railW, railH, RGB565(10, 15, 22));
    tft.drawRect(railX, railY, railW, railH, RGB565(25, 38, 55));
    
    // Glowing Cyan Thumb with Center Tick
    int thumbH = (railH * visibleCount) / totalItems;
    if (thumbH < 24) thumbH = 24;
    
    int maxOffset = max(1, totalItems - visibleCount);
    int thumbY = railY + (scrollOffset * (railH - thumbH)) / maxOffset;
    
    tft.fillRect(railX + 1, thumbY, railW - 2, thumbH, RGB565(16, 42, 60));
    tft.drawRect(railX + 1, thumbY, railW - 2, thumbH, COLOR_CYAN);
    tft.drawFastHLine(railX + 2, thumbY + thumbH / 2, railW - 4, COLOR_WHITE);
}

void drawHomeCommandDeck() {
    int footerY = 205;
    int footerW = SCREEN_WIDTH - 12;
    int footerH = 25;
    
    // 1. Technical Separator Lines & Strip (matching boot screen bottom strip)
    tft.drawFastHLine(6, footerY - 1, footerW, COLOR_CYAN_DARK);
    tft.fillRect(6, footerY, footerW, footerH, RGB565(12, 18, 28));
    tft.drawRect(6, footerY, footerW, footerH, RGB565(30, 44, 65));
    
    // 2. Action Key Badges
    tft.setTextSize(TEXT_SMALL);
    
    // [A] UP
    tft.setTextColor(COLOR_CYAN);
    tft.setCursor(14, footerY + 8);
    tft.print(F("[A"));
    tft.print((char)24);
    tft.print(F("]"));
    
    // [B] DOWN
    tft.setCursor(44, footerY + 8);
    tft.print(F("[B"));
    tft.print((char)25);
    tft.print(F("]"));
    
    tft.drawFastVLine(78, footerY + 4, footerH - 8, RGB565(30, 44, 65));
    
    // [*] ARM & TRANSMIT
    tft.setTextColor(COLOR_GREEN_BRIGHT);
    tft.setCursor(88, footerY + 8);
    tft.print(F("[*] ARM & TRANSMIT"));
    
    tft.drawFastVLine(206, footerY + 4, footerH - 8, RGB565(30, 44, 65));
    
    // [C] OTA
    tft.setTextColor(COLOR_CYAN);
    tft.setCursor(214, footerY + 8);
    tft.print(F("[C] OTA"));
    
    tft.drawFastVLine(262, footerY + 4, footerH - 8, RGB565(30, 44, 65));
    
    // [1-9]
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(270, footerY + 8);
    tft.print(F("1-9"));
    
    // 3. Signature Bottom Gradient Hairline (exact match to boot screen line 422!)
    drawGradientH(6, SCREEN_HEIGHT - 4, footerW, 2, COLOR_CYAN_DARK, COLOR_PURPLE_DARK);
    
    // Bottom-Left & Bottom-Right Registration Marks (matching boot screen!)
    int mDist = 8;
    tft.drawFastHLine(mDist - 3, SCREEN_HEIGHT - mDist - 1, 7, COLOR_CYAN_DARK);
    tft.drawFastVLine(mDist, SCREEN_HEIGHT - mDist - 4, 7, COLOR_CYAN_DARK);
    tft.drawFastHLine(SCREEN_WIDTH - mDist - 4, SCREEN_HEIGHT - mDist - 1, 7, COLOR_CYAN_DARK);
    tft.drawFastVLine(SCREEN_WIDTH - mDist - 1, SCREEN_HEIGHT - mDist - 4, 7, COLOR_CYAN_DARK);
}

void updateMenuSelection(int oldIndex, int newIndex) {
    const int CARD_HEIGHT = 31;
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
    
    // Update right elevator rail
    drawElevatorRail(menuScrollOffset, ALERT_COUNT, VISIBLE_MENU_ITEMS);
}

void drawMenuCards() {
    const int CARD_HEIGHT = 31;
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
    // 1. Draw outer HUD framing, background, & status telemetry strip
    drawHomeStatusBar();
    
    // 2. Clear cards canvas area once with tactical background
    tft.fillRect(6, 35, SCREEN_WIDTH - 12, 169, RGB565(8, 12, 18));
    
    // 3. 5 Boot-Screen Inspired Sharp Alert Cards & Scroll Rail
    drawMenuCards();
    
    // 4. Boot-Screen Command Deck Footer with signature gradient
    drawHomeCommandDeck();
    
    Serial.println(F("[SCREEN] Boot-Screen Inspired Tactical Home Screen displayed"));
}


void drawConfirmScreen() {
    // 1. Deep tactical dark background
    tft.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, RGB565(8, 12, 18));
    
    // Technical hairline outer border (dual perimeter hairlines)
    tft.drawFastHLine(2, 2, SCREEN_WIDTH - 4, RGB565(25, 35, 52));
    tft.drawFastHLine(4, 4, SCREEN_WIDTH - 8, RGB565(15, 22, 34));
    tft.drawFastHLine(2, SCREEN_HEIGHT - 3, SCREEN_WIDTH - 4, RGB565(25, 35, 52));
    tft.drawFastHLine(4, SCREEN_HEIGHT - 5, SCREEN_WIDTH - 8, RGB565(15, 22, 34));
    tft.drawFastVLine(2, 2, SCREEN_HEIGHT - 4, RGB565(25, 35, 52));
    tft.drawFastVLine(SCREEN_WIDTH - 3, 2, SCREEN_HEIGHT - 4, RGB565(25, 35, 52));
    tft.drawFastVLine(4, 4, SCREEN_HEIGHT - 8, RGB565(15, 22, 34));
    tft.drawFastVLine(SCREEN_WIDTH - 5, 4, SCREEN_HEIGHT - 8, RGB565(15, 22, 34));
    
    // Amber Tactical Corner Registration Marks (L-ticks)
    int mDist = 8;
    tft.drawFastHLine(mDist - 3, mDist, 7, COLOR_AMBER_DARK);
    tft.drawFastVLine(mDist, mDist - 3, 7, COLOR_AMBER_DARK);
    tft.drawFastHLine(SCREEN_WIDTH - mDist - 4, mDist, 7, COLOR_AMBER_DARK);
    tft.drawFastVLine(SCREEN_WIDTH - mDist - 1, mDist - 3, 7, COLOR_AMBER_DARK);
    tft.drawFastHLine(mDist - 3, SCREEN_HEIGHT - mDist - 1, 7, COLOR_AMBER_DARK);
    tft.drawFastVLine(mDist, SCREEN_HEIGHT - mDist - 4, 7, COLOR_AMBER_DARK);
    tft.drawFastHLine(SCREEN_WIDTH - mDist - 4, SCREEN_HEIGHT - mDist - 1, 7, COLOR_AMBER_DARK);
    tft.drawFastVLine(SCREEN_WIDTH - mDist - 1, SCREEN_HEIGHT - mDist - 4, 7, COLOR_AMBER_DARK);
    
    // 2. Top Header Telemetry Strip
    int topStripY = 6;
    int topStripH = 16;
    tft.fillRect(6, topStripY, SCREEN_WIDTH - 12, topStripH, RGB565(24, 18, 10));
    tft.drawRect(6, topStripY, SCREEN_WIDTH - 12, topStripH, RGB565(65, 48, 20));
    tft.drawFastHLine(6, topStripY + topStripH, SCREEN_WIDTH - 12, COLOR_AMBER_DARK);
    
    // Amber warning glyph badge
    tft.fillRect(10, topStripY + 2, 12, 12, COLOR_AMBER);
    tft.drawRect(10, topStripY + 2, 12, 12, COLOR_WHITE);
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_TEXT_DARK);
    tft.setCursor(14, topStripY + 4);
    tft.print('!');
    
    tft.setTextColor(COLOR_AMBER_BRIGHT);
    tft.setCursor(28, topStripY + 4);
    char unitTag[24];
    sprintf(unitTag, "LIFELINE // TX #%03d", DEVICE_ID);
    tft.print(unitTag);
    
    tft.setTextColor(loraInitialized ? COLOR_GREEN_BRIGHT : COLOR_RED);
    tft.setCursor(264, topStripY + 4);
    tft.print(loraInitialized ? F("RF:RDY") : F("RF:ERR"));
    
    // 4. Primary Tactical Alert Focus Card (y = 37 to 101)
    int alertCardX = 8;
    int alertCardY = 37;
    int alertCardW = 304;
    int alertCardH = 64;
    uint16_t alertColor = getAlertColor(selectedAlertIndex);
    
    // Deep cockpit card fill & double border
    tft.fillRect(alertCardX, alertCardY, alertCardW, alertCardH, RGB565(16, 24, 36));
    tft.drawRect(alertCardX, alertCardY, alertCardW, alertCardH, RGB565(40, 56, 80));
    tft.drawFastHLine(alertCardX + 1, alertCardY + 1, alertCardW - 2, RGB565(60, 75, 105));
    
    // 6px White corner brackets
    tft.drawFastHLine(alertCardX, alertCardY, 6, COLOR_WHITE);
    tft.drawFastVLine(alertCardX, alertCardY, 6, COLOR_WHITE);
    tft.drawFastHLine(alertCardX + alertCardW - 6, alertCardY, 6, COLOR_WHITE);
    tft.drawFastVLine(alertCardX + alertCardW - 1, alertCardY, 6, COLOR_WHITE);
    tft.drawFastHLine(alertCardX, alertCardY + alertCardH - 1, 6, COLOR_WHITE);
    tft.drawFastVLine(alertCardX, alertCardY + alertCardH - 6, 6, COLOR_WHITE);
    tft.drawFastHLine(alertCardX + alertCardW - 6, alertCardY + alertCardH - 1, 6, COLOR_WHITE);
    tft.drawFastVLine(alertCardX + alertCardW - 1, alertCardY + alertCardH - 6, 6, COLOR_WHITE);
    
    // Left 6px solid priority bar
    tft.fillRect(alertCardX, alertCardY, 6, alertCardH, alertColor);
    
    // Tactical Chevron
    tft.drawLine(alertCardX + 10, alertCardY + 14, alertCardX + 14, alertCardY + 20, COLOR_AMBER_BRIGHT);
    tft.drawLine(alertCardX + 10, alertCardY + 26, alertCardX + 14, alertCardY + 20, COLOR_AMBER_BRIGHT);
    tft.drawLine(alertCardX + 11, alertCardY + 14, alertCardX + 15, alertCardY + 20, COLOR_WHITE);
    tft.drawLine(alertCardX + 11, alertCardY + 26, alertCardX + 15, alertCardY + 20, COLOR_WHITE);
    
    // Alert Title with shadow
    tft.setTextSize(TEXT_MEDIUM);
    tft.setTextColor(RGB565(5, 10, 16));
    tft.setCursor(alertCardX + 23, alertCardY + 13);
    tft.print(alertNames[selectedAlertIndex]);
    tft.setTextColor(COLOR_WHITE);
    tft.setCursor(alertCardX + 22, alertCardY + 12);
    tft.print(alertNames[selectedAlertIndex]);
    
    // Right Alert Code Badge
    int codeBadgeW = 44;
    int codeBadgeH = 22;
    int codeBadgeX = alertCardX + alertCardW - codeBadgeW - 8;
    int codeBadgeY = alertCardY + 10;
    tft.fillRect(codeBadgeX, codeBadgeY, codeBadgeW, codeBadgeH, RGB565(20, 28, 42));
    tft.drawRect(codeBadgeX, codeBadgeY, codeBadgeW, codeBadgeH, alertColor);
    tft.drawFastHLine(codeBadgeX + 1, codeBadgeY + 1, codeBadgeW - 2, COLOR_WHITE);
    tft.setTextSize(TEXT_MEDIUM);
    tft.setTextColor(alertColor);
    tft.setCursor(codeBadgeX + 8, codeBadgeY + 3);
    tft.print('[');
    tft.print(getAlertCode(selectedAlertIndex));
    tft.print(']');
    
    // Lower Status Strip inside Alert Card
    tft.drawFastHLine(alertCardX + 6, alertCardY + 38, alertCardW - 12, RGB565(30, 44, 65));
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(alertCardX + 14, alertCardY + 45);
    tft.print(F("SEVERITY: "));
    
    const char* prioFullLabels[] = {"CRITICAL (P0)", "HIGH PRIORITY (P1)", "MEDIUM (P2)", "ROUTINE/OK (P3)", "INFO (P4)"};
    int pIdx = min((int)alertPriority[selectedAlertIndex], 4);
    tft.fillRect(alertCardX + 76, alertCardY + 46, 6, 6, alertColor);
    tft.drawRect(alertCardX + 75, alertCardY + 45, 8, 8, COLOR_WHITE);
    tft.setTextColor(alertColor);
    tft.setCursor(alertCardX + 88, alertCardY + 45);
    tft.print(prioFullLabels[pIdx]);
    
    tft.setTextColor(COLOR_CYAN);
    tft.setCursor(alertCardX + alertCardW - 68, alertCardY + 45);
    char slotStr[12];
    sprintf(slotStr, "SLOT #%02d", selectedAlertIndex + 1);
    tft.print(slotStr);
    
    // 5. Secondary Dual-Column Telemetry & Payload Card (y = 106 to 156)
    int telCardY = 106;
    int telCardH = 50;
    tft.fillRect(alertCardX, telCardY, alertCardW, telCardH, RGB565(12, 18, 28));
    tft.drawRect(alertCardX, telCardY, alertCardW, telCardH, RGB565(30, 44, 65));
    tft.drawFastHLine(alertCardX + 1, telCardY + 1, alertCardW - 2, RGB565(45, 60, 85));
    
    // Corner ticks (3px)
    tft.drawFastHLine(alertCardX, telCardY, 4, RGB565(45, 65, 90));
    tft.drawFastVLine(alertCardX, telCardY, 4, RGB565(45, 65, 90));
    tft.drawFastHLine(alertCardX + alertCardW - 4, telCardY, 4, RGB565(45, 65, 90));
    tft.drawFastVLine(alertCardX + alertCardW - 1, telCardY, 4, RGB565(45, 65, 90));
    tft.drawFastHLine(alertCardX, telCardY + telCardH - 1, 4, RGB565(45, 65, 90));
    tft.drawFastVLine(alertCardX, telCardY + telCardH - 4, 4, RGB565(45, 65, 90));
    tft.drawFastHLine(alertCardX + alertCardW - 4, telCardY + telCardH - 1, 4, RGB565(45, 65, 90));
    tft.drawFastVLine(alertCardX + alertCardW - 1, telCardY + telCardH - 4, 4, RGB565(45, 65, 90));
    
    // Left Column
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(alertCardX + 12, telCardY + 8);
    tft.print(F("ORIGIN NODE:"));
    tft.setTextColor(COLOR_CYAN_BRIGHT);
    char devStr[24];
    sprintf(devStr, "TX UNIT #%03d", DEVICE_ID);
    tft.setCursor(alertCardX + 12, telCardY + 20);
    tft.print(devStr);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.setCursor(alertCardX + 12, telCardY + 33);
    tft.print(F("FREQ: 868.00 MHz // SF7"));
    
    // Vertical Separator
    tft.drawFastVLine(alertCardX + 148, telCardY + 6, telCardH - 12, RGB565(25, 38, 55));
    
    // Right Column
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(alertCardX + 158, telCardY + 8);
    tft.print(F("TARGET GATEWAY:"));
    tft.setTextColor(COLOR_GREEN_BRIGHT);
    tft.setCursor(alertCardX + 158, telCardY + 20);
    tft.print(F("BASE SPU (ALL CH)"));
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.setCursor(alertCardX + 158, telCardY + 33);
    tft.print(F("MOD: LoRa CHIRP // 20dBm"));
    
    // 6. Tactical Warning Banner (y = 161 to 184)
    int warnY = 161;
    int warnH = 23;
    tft.fillRect(alertCardX, warnY, alertCardW, warnH, RGB565(32, 20, 8));
    tft.drawRect(alertCardX, warnY, alertCardW, warnH, COLOR_AMBER_DARK);
    tft.drawFastHLine(alertCardX + 1, warnY + 1, alertCardW - 2, COLOR_AMBER);
    
    tft.fillRect(alertCardX + 8, warnY + 5, 12, 12, COLOR_AMBER);
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_TEXT_DARK);
    tft.setCursor(alertCardX + 12, warnY + 7);
    tft.print('!');
    
    tft.setTextColor(COLOR_AMBER_BRIGHT);
    tft.setCursor(alertCardX + 26, warnY + 7);
    tft.print(F("WARNING: BROADCAST PACKET CANNOT BE RECALLED"));
    
    // 7. Tactical Command Deck Action Buttons (y = 190, h = 38)
    int btnY = 190;
    int btnW = 148;
    int btnH = 38;
    
    // Left Button: [* CONFIRM & BROADCAST]
    int sendBtnX = 8;
    tft.fillRect(sendBtnX, btnY, btnW, btnH, RGB565(10, 40, 24));
    tft.drawRect(sendBtnX, btnY, btnW, btnH, COLOR_GREEN_BRIGHT);
    tft.drawRect(sendBtnX + 1, btnY + 1, btnW - 2, btnH - 2, COLOR_GREEN_DARK);
    tft.drawFastHLine(sendBtnX + 2, btnY + 2, btnW - 4, COLOR_WHITE);
    
    // Square green status LED
    tft.fillRect(sendBtnX + 8, btnY + 14, 8, 8, COLOR_GREEN_BRIGHT);
    tft.drawRect(sendBtnX + 7, btnY + 13, 10, 10, COLOR_WHITE);
    
    tft.setTextSize(TEXT_MEDIUM);
    tft.setTextColor(RGB565(5, 16, 10));
    tft.setCursor(sendBtnX + 25, btnY + 8);
    tft.print(F("* SEND"));
    tft.setTextColor(COLOR_WHITE);
    tft.setCursor(sendBtnX + 24, btnY + 7);
    tft.print(F("* SEND"));
    
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_GREEN_BRIGHT);
    tft.setCursor(sendBtnX + 24, btnY + 23);
    tft.print(F("BROADCAST RF"));
    
    // Right Button: [# ABORT / CANCEL]
    int cancelBtnX = 164;
    tft.fillRect(cancelBtnX, btnY, btnW, btnH, RGB565(42, 12, 16));
    tft.drawRect(cancelBtnX, btnY, btnW, btnH, COLOR_RED);
    tft.drawRect(cancelBtnX + 1, btnY + 1, btnW - 2, btnH - 2, COLOR_RED_DARK);
    tft.drawFastHLine(cancelBtnX + 2, btnY + 2, btnW - 4, RGB565(255, 140, 140));
    
    // Square red status LED
    tft.fillRect(cancelBtnX + 8, btnY + 14, 8, 8, COLOR_RED);
    tft.drawRect(cancelBtnX + 7, btnY + 13, 10, 10, COLOR_WHITE);
    
    tft.setTextSize(TEXT_MEDIUM);
    tft.setTextColor(RGB565(20, 5, 6));
    tft.setCursor(cancelBtnX + 25, btnY + 8);
    tft.print(F("# CANCEL"));
    tft.setTextColor(COLOR_WHITE);
    tft.setCursor(cancelBtnX + 24, btnY + 7);
    tft.print(F("# CANCEL"));
    
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_RED_BRIGHT);
    tft.setCursor(cancelBtnX + 24, btnY + 23);
    tft.print(F("ABORT & RETURN"));
    
    // Signature bottom gradient hairline
    drawGradientH(6, SCREEN_HEIGHT - 3, SCREEN_WIDTH - 12, 2, COLOR_AMBER_DARK, COLOR_PURPLE_DARK);
    
    Serial.println(F("[SCREEN] Tactical Confirm screen displayed"));
}

void drawSendingScreen() {
    // 1. Deep tactical dark background
    tft.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, RGB565(8, 12, 18));
    
    // Dual perimeter hairlines
    tft.drawFastHLine(2, 2, SCREEN_WIDTH - 4, RGB565(25, 35, 52));
    tft.drawFastHLine(4, 4, SCREEN_WIDTH - 8, RGB565(15, 22, 34));
    tft.drawFastHLine(2, SCREEN_HEIGHT - 3, SCREEN_WIDTH - 4, RGB565(25, 35, 52));
    tft.drawFastHLine(4, SCREEN_HEIGHT - 5, SCREEN_WIDTH - 8, RGB565(15, 22, 34));
    tft.drawFastVLine(2, 2, SCREEN_HEIGHT - 4, RGB565(25, 35, 52));
    tft.drawFastVLine(SCREEN_WIDTH - 3, 2, SCREEN_HEIGHT - 4, RGB565(25, 35, 52));
    tft.drawFastVLine(4, 4, SCREEN_HEIGHT - 8, RGB565(15, 22, 34));
    tft.drawFastVLine(SCREEN_WIDTH - 5, 4, SCREEN_HEIGHT - 8, RGB565(15, 22, 34));
    
    // Cyan Tactical Corner Registration Marks (L-ticks)
    int mDist = 8;
    tft.drawFastHLine(mDist - 3, mDist, 7, COLOR_CYAN_DARK);
    tft.drawFastVLine(mDist, mDist - 3, 7, COLOR_CYAN_DARK);
    tft.drawFastHLine(SCREEN_WIDTH - mDist - 4, mDist, 7, COLOR_CYAN_DARK);
    tft.drawFastVLine(SCREEN_WIDTH - mDist - 1, mDist - 3, 7, COLOR_CYAN_DARK);
    tft.drawFastHLine(mDist - 3, SCREEN_HEIGHT - mDist - 1, 7, COLOR_CYAN_DARK);
    tft.drawFastVLine(mDist, SCREEN_HEIGHT - mDist - 4, 7, COLOR_CYAN_DARK);
    tft.drawFastHLine(SCREEN_WIDTH - mDist - 4, SCREEN_HEIGHT - mDist - 1, 7, COLOR_CYAN_DARK);
    tft.drawFastVLine(SCREEN_WIDTH - mDist - 1, SCREEN_HEIGHT - mDist - 4, 7, COLOR_CYAN_DARK);
    
    // 2. Top Header Telemetry Strip
    int topStripY = 6;
    int topStripH = 16;
    tft.fillRect(6, topStripY, SCREEN_WIDTH - 12, topStripH, RGB565(12, 26, 40));
    tft.drawRect(6, topStripY, SCREEN_WIDTH - 12, topStripH, RGB565(35, 65, 95));
    tft.drawFastHLine(6, topStripY + topStripH, SCREEN_WIDTH - 12, COLOR_CYAN);
    
    // Transmitting cyan square beacon
    tft.fillRect(10, topStripY + 4, 8, 8, COLOR_CYAN_BRIGHT);
    tft.drawRect(9, topStripY + 3, 10, 10, COLOR_WHITE);
    
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_CYAN_BRIGHT);
    tft.setCursor(24, topStripY + 4);
    char unitTag[24];
    sprintf(unitTag, "LIFELINE // TX #%03d", DEVICE_ID);
    tft.print(unitTag);
    
    tft.setTextColor(COLOR_AMBER_BRIGHT);
    tft.setCursor(266, topStripY + 4);
    tft.print(F("TX:20dBm"));
    
    // 3. Sub-header Breadcrumb (y = 25)
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_CYAN);
    tft.setCursor(8, 25);
    tft.print(F("UPLINK FREQ: 868.000 MHz"));
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(248, 25);
    tft.print(F("AIRTIME ~82ms"));
    
    // 4. Hero Cybernetic Radar & Sonar Array (y = 36 to 126)
    int cx = SCREEN_WIDTH / 2; // 160
    int cy = 80;
    
    // Crosshairs
    tft.drawFastHLine(cx - 90, cy, 180, RGB565(20, 34, 50));
    tft.drawFastVLine(cx, cy - 42, 84, RGB565(20, 34, 50));
    for (int d = -80; d <= 80; d += 20) {
        if (d != 0) {
            tft.drawFastVLine(cx + d, cy - 2, 5, RGB565(30, 50, 75));
            tft.drawFastHLine(cx - 2, cy + d/2, 5, RGB565(30, 50, 75));
        }
    }
    
    // Concentric Radar Rings
    const uint16_t ringCols[] = {
        RGB565(0, 40, 65),
        RGB565(0, 80, 120),
        RGB565(0, 140, 190),
        COLOR_CYAN_BRIGHT
    };
    const int ringRadii[] = { 46, 34, 23, 12 };
    
    for (int r = 0; r < 4; r++) {
        tft.drawCircle(cx, cy, ringRadii[r], ringCols[r]);
        tft.drawCircle(cx, cy, ringRadii[r] + 1, ringCols[r]);
    }
    
    // Diagonal tick marks
    int tOff = 26;
    tft.drawLine(cx - tOff - 4, cy - tOff - 4, cx - tOff, cy - tOff, COLOR_CYAN_DARK);
    tft.drawLine(cx + tOff, cy - tOff, cx + tOff + 4, cy - tOff - 4, COLOR_CYAN_DARK);
    tft.drawLine(cx - tOff - 4, cy + tOff + 4, cx - tOff, cy + tOff, COLOR_CYAN_DARK);
    tft.drawLine(cx + tOff, cy + tOff, cx + tOff + 4, cy + tOff + 4, COLOR_CYAN_DARK);
    
    // Center Transmitter Beacon Core
    tft.fillRect(cx - 4, cy - 4, 9, 9, COLOR_CYAN_BRIGHT);
    tft.drawRect(cx - 5, cy - 5, 11, 11, COLOR_WHITE);
    tft.drawPixel(cx, cy, COLOR_WHITE);
    
    // Vertical Antenna mast
    tft.fillRect(cx - 1, cy - 14, 3, 9, COLOR_WHITE);
    tft.drawPixel(cx, cy - 16, COLOR_CYAN_BRIGHT);
    
    // Directional Vector Wings
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_CYAN_DARK);
    tft.setCursor(cx - 110, cy - 4);
    tft.print(F("<<< LORA RF"));
    tft.setTextColor(COLOR_CYAN);
    tft.setCursor(cx + 42, cy - 4);
    tft.print(F("UPLINK >>>"));
    
    // Flanking Telemetry Badges
    // Left Badge
    tft.fillRect(8, 48, 54, 28, RGB565(14, 22, 34));
    tft.drawRect(8, 48, 54, 28, RGB565(30, 48, 70));
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(12, 52);
    tft.print(F("MOD"));
    tft.setTextColor(COLOR_CYAN);
    tft.setCursor(12, 63);
    tft.print(F("CSS125"));
    
    // Right Badge
    tft.fillRect(SCREEN_WIDTH - 8 - 54, 48, 54, 28, RGB565(14, 22, 34));
    tft.drawRect(SCREEN_WIDTH - 8 - 54, 48, 54, 28, RGB565(30, 48, 70));
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(SCREEN_WIDTH - 8 - 50, 52);
    tft.print(F("CRC"));
    tft.setTextColor(COLOR_GREEN_BRIGHT);
    tft.setCursor(SCREEN_WIDTH - 8 - 50, 63);
    tft.print(F("VALID"));
    
    // 5. Tactical Payload Card (y = 130 to 178)
    int pCardY = 130;
    int pCardH = 48;
    uint16_t alertColor = getAlertColor(selectedAlertIndex);
    
    tft.fillRect(8, pCardY, 304, pCardH, RGB565(14, 22, 34));
    tft.drawRect(8, pCardY, 304, pCardH, COLOR_CYAN_DARK);
    tft.drawFastHLine(9, pCardY + 1, 302, RGB565(50, 80, 110));
    
    // Left 5px priority bar
    tft.fillRect(8, pCardY, 5, pCardH, alertColor);
    
    // Corner ticks
    tft.drawFastHLine(8, pCardY, 4, COLOR_WHITE);
    tft.drawFastVLine(8, pCardY, 4, COLOR_WHITE);
    tft.drawFastHLine(308, pCardY, 4, COLOR_WHITE);
    tft.drawFastVLine(311, pCardY, 4, COLOR_WHITE);
    tft.drawFastHLine(8, pCardY + pCardH - 1, 4, COLOR_WHITE);
    tft.drawFastVLine(8, pCardY + pCardH - 4, 4, COLOR_WHITE);
    tft.drawFastHLine(308, pCardY + pCardH - 1, 4, COLOR_WHITE);
    tft.drawFastVLine(311, pCardY + pCardH - 4, 4, COLOR_WHITE);
    
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(18, pCardY + 8);
    tft.print(F("ACTIVE PAYLOAD DISPATCH:"));
    
    tft.setTextSize(TEXT_MEDIUM);
    tft.setTextColor(COLOR_WHITE);
    tft.setCursor(18, pCardY + 23);
    tft.print(alertNames[selectedAlertIndex]);
    
    // Code badge
    int sndBadgeW = 44;
    int sndBadgeH = 24;
    int sndBadgeX = 304 - sndBadgeW;
    int sndBadgeY = pCardY + 12;
    tft.fillRect(sndBadgeX, sndBadgeY, sndBadgeW, sndBadgeH, RGB565(18, 28, 44));
    tft.drawRect(sndBadgeX, sndBadgeY, sndBadgeW, sndBadgeH, alertColor);
    tft.setTextSize(TEXT_MEDIUM);
    tft.setTextColor(alertColor);
    tft.setCursor(sndBadgeX + 9, sndBadgeY + 4);
    tft.print('[');
    tft.print(getAlertCode(selectedAlertIndex));
    tft.print(']');
    
    // 6. Segmented High-Tech Progress Bar (y = 184 to 204)
    int progY = 184;
    int progH = 20;
    tft.fillRect(8, progY, 304, progH, RGB565(10, 15, 22));
    tft.drawRect(8, progY, 304, progH, RGB565(25, 45, 65));
    
    // 20 Discrete Tactical Segments
    int numSegments = 20;
    int segSpacing = 2;
    int totalInnerW = 304 - 6;
    int segW = (totalInnerW - (numSegments - 1) * segSpacing) / numSegments;
    int activeSegments = 14;
    
    for (int s = 0; s < numSegments; s++) {
        int segX = 11 + s * (segW + segSpacing);
        if (s < activeSegments - 1) {
            tft.fillRect(segX, progY + 3, segW, progH - 6, RGB565(0, 120 + s * 8, 180 + s * 4));
        } else if (s == activeSegments - 1) {
            tft.fillRect(segX, progY + 3, segW, progH - 6, COLOR_WHITE);
        } else {
            tft.fillRect(segX, progY + 3, segW, progH - 6, RGB565(14, 22, 32));
        }
    }
    
    // 7. Bottom Safety Banner (y = 208 to 232)
    int botY = 208;
    int botH = 24;
    tft.fillRect(8, botY, 304, botH, RGB565(12, 18, 28));
    tft.drawRect(8, botY, 304, botH, RGB565(30, 44, 65));
    
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_AMBER);
    tft.setCursor(14, botY + 7);
    tft.print(F("[TX ENERGIZED]"));
    tft.setTextColor(COLOR_WHITE);
    tft.setCursor(102, botY + 7);
    tft.print(F("AIRTIME ACTIVE - DO NOT POWER OFF"));
    
    // Signature bottom gradient hairline
    drawGradientH(6, SCREEN_HEIGHT - 3, SCREEN_WIDTH - 12, 2, COLOR_CYAN_DARK, COLOR_PURPLE_DARK);
    
    Serial.println(F("[SCREEN] Tactical Sending screen displayed"));
}

void drawResultScreen() {
    if (lastTransmitSuccess) {
        // ═══════════════════════════════════════════════════════════════════════
        //                      SUCCESS RESULT SCREEN
        // ═══════════════════════════════════════════════════════════════════════
        tft.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, RGB565(6, 16, 12));
        
        // Dual perimeter hairlines
        tft.drawFastHLine(2, 2, SCREEN_WIDTH - 4, RGB565(15, 45, 30));
        tft.drawFastHLine(4, 4, SCREEN_WIDTH - 8, RGB565(10, 30, 20));
        tft.drawFastHLine(2, SCREEN_HEIGHT - 3, SCREEN_WIDTH - 4, RGB565(15, 45, 30));
        tft.drawFastHLine(4, SCREEN_HEIGHT - 5, SCREEN_WIDTH - 8, RGB565(10, 30, 20));
        tft.drawFastVLine(2, 2, SCREEN_HEIGHT - 4, RGB565(15, 45, 30));
        tft.drawFastVLine(SCREEN_WIDTH - 3, 2, SCREEN_HEIGHT - 4, RGB565(15, 45, 30));
        tft.drawFastVLine(4, 4, SCREEN_HEIGHT - 8, RGB565(10, 30, 20));
        tft.drawFastVLine(SCREEN_WIDTH - 5, 4, SCREEN_HEIGHT - 8, RGB565(10, 30, 20));
        
        // Emerald Corner Registration Marks
        int mDist = 8;
        tft.drawFastHLine(mDist - 3, mDist, 7, COLOR_GREEN_DARK);
        tft.drawFastVLine(mDist, mDist - 3, 7, COLOR_GREEN_DARK);
        tft.drawFastHLine(SCREEN_WIDTH - mDist - 4, mDist, 7, COLOR_GREEN_DARK);
        tft.drawFastVLine(SCREEN_WIDTH - mDist - 1, mDist - 3, 7, COLOR_GREEN_DARK);
        tft.drawFastHLine(mDist - 3, SCREEN_HEIGHT - mDist - 1, 7, COLOR_GREEN_DARK);
        tft.drawFastVLine(mDist, SCREEN_HEIGHT - mDist - 4, 7, COLOR_GREEN_DARK);
        tft.drawFastHLine(SCREEN_WIDTH - mDist - 4, SCREEN_HEIGHT - mDist - 1, 7, COLOR_GREEN_DARK);
        tft.drawFastVLine(SCREEN_WIDTH - mDist - 1, SCREEN_HEIGHT - mDist - 4, 7, COLOR_GREEN_DARK);
        
        // Top Header Telemetry Strip
        int topStripY = 6;
        int topStripH = 16;
        tft.fillRect(6, topStripY, SCREEN_WIDTH - 12, topStripH, RGB565(10, 34, 20));
        tft.drawRect(6, topStripY, SCREEN_WIDTH - 12, topStripH, RGB565(25, 75, 45));
        tft.drawFastHLine(6, topStripY + topStripH, SCREEN_WIDTH - 12, COLOR_GREEN_BRIGHT);
        
        // Green LED
        tft.fillRect(10, topStripY + 4, 8, 8, COLOR_GREEN_BRIGHT);
        tft.drawRect(9, topStripY + 3, 10, 10, COLOR_WHITE);
        
        tft.setTextSize(TEXT_SMALL);
        tft.setTextColor(COLOR_GREEN_BRIGHT);
        tft.setCursor(24, topStripY + 4);
        char unitTag[24];
        sprintf(unitTag, "LIFELINE // TX #%03d", DEVICE_ID);
        tft.print(unitTag);
        
        tft.setTextColor(COLOR_WHITE);
        tft.setCursor(144, topStripY + 4);
        tft.print(F("BROADCAST CONFIRMED // ACK"));
        
        tft.setTextColor(COLOR_GREEN_BRIGHT);
        tft.setCursor(266, topStripY + 4);
        tft.print(F("STATUS:OK"));
        
        // Sub-header Breadcrumb (y = 25)
        tft.setTextSize(TEXT_SMALL);
        tft.setTextColor(COLOR_GREEN_BRIGHT);
        tft.setCursor(8, 25);
        tft.print(F("LoRa SPU BASE STATION ACKNOWLEDGED // 2-WAY HANDSHAKE"));
        tft.setTextColor(COLOR_WHITE);
        tft.setCursor(252, 25);
        if (lastAckSnr != 0) {
            char snrBuf[16];
            snprintf(snrBuf, sizeof(snrBuf), "SNR:%+ddB", lastAckSnr);
            tft.print(snrBuf);
        } else {
            tft.print(F("SNR:+10dB"));
        }
        
        // Hero Tactical Success Reticle & Checkmark Emblem (y = 36 to 116)
        int cx = SCREEN_WIDTH / 2; // 160
        int cy = 66;
        
        // Crosshairs
        tft.drawFastHLine(cx - 60, cy, 120, RGB565(15, 45, 30));
        tft.drawFastVLine(cx, cy - 28, 56, RGB565(15, 45, 30));
        
        // Reticle rings
        tft.drawCircle(cx, cy, 32, RGB565(0, 70, 40));
        tft.drawCircle(cx, cy, 33, RGB565(0, 90, 50));
        tft.drawCircle(cx, cy, 24, COLOR_GREEN_DARK);
        tft.drawCircle(cx, cy, 25, COLOR_GREEN_BRIGHT);
        
        // Center Sharp Emerald Badge
        tft.fillRect(cx - 16, cy - 16, 33, 33, RGB565(8, 42, 24));
        tft.drawRect(cx - 16, cy - 16, 33, 33, COLOR_GREEN_BRIGHT);
        tft.drawRect(cx - 15, cy - 15, 31, 31, COLOR_WHITE);
        
        // Razor-sharp military checkmark
        for (int t = -2; t <= 2; t++) {
            tft.drawLine(cx - 9, cy + t, cx - 3, cy + 7 + t, COLOR_WHITE);
            tft.drawLine(cx - 3, cy + 7 + t, cx + 10, cy - 7 + t, COLOR_WHITE);
        }
        
        // Headline with shadow
        tft.setTextSize(TEXT_MEDIUM);
        tft.setTextColor(RGB565(0, 20, 10));
        tft.setCursor(cx - 105, 96);
        tft.print(F("BASE ACK CONFIRMED!"));
        tft.setTextColor(COLOR_GREEN_BRIGHT);
        tft.setCursor(cx - 106, 95);
        tft.print(F("BASE ACK CONFIRMED!"));
        
        tft.setTextSize(TEXT_SMALL);
        tft.setTextColor(COLOR_TEXT_SECONDARY);
        tft.setCursor(cx - 96, 111);
        tft.print(F("EMERGENCY 2-WAY HANDSHAKE VERIFIED"));
        
        // Dispatch Telemetry Card (y = 124 to 178)
        int sCardY = 124;
        int sCardH = 54;
        tft.fillRect(8, sCardY, 304, sCardH, RGB565(10, 24, 18));
        tft.drawRect(8, sCardY, 304, sCardH, RGB565(25, 65, 45));
        tft.drawFastHLine(9, sCardY + 1, 302, COLOR_GREEN_DARK);
        
        // Left 5px priority bar
        tft.fillRect(8, sCardY, 5, sCardH, COLOR_GREEN_BRIGHT);
        
        // Corner ticks
        tft.drawFastHLine(8, sCardY, 4, COLOR_WHITE);
        tft.drawFastVLine(8, sCardY, 4, COLOR_WHITE);
        tft.drawFastHLine(308, sCardY, 4, COLOR_WHITE);
        tft.drawFastVLine(311, sCardY, 4, COLOR_WHITE);
        tft.drawFastHLine(8, sCardY + sCardH - 1, 4, COLOR_WHITE);
        tft.drawFastVLine(8, sCardY + sCardH - 4, 4, COLOR_WHITE);
        tft.drawFastHLine(308, sCardY + sCardH - 1, 4, COLOR_WHITE);
        tft.drawFastVLine(311, sCardY + sCardH - 4, 4, COLOR_WHITE);
        
        tft.setTextSize(TEXT_SMALL);
        tft.setTextColor(COLOR_TEXT_MUTED);
        tft.setCursor(18, sCardY + 8);
        tft.print(F("ALERT CONFIRMED: "));
        tft.setTextColor(COLOR_WHITE);
        tft.print(alertNames[selectedAlertIndex]);
        
        tft.setTextColor(COLOR_TEXT_MUTED);
        tft.setCursor(18, sCardY + 23);
        tft.print(F("GATEWAY ACK ID:  "));
        tft.setTextColor(COLOR_CYAN_BRIGHT);
        char ackBuf[40];
        snprintf(ackBuf, sizeof(ackBuf), "%s (RSSI %d dBm)",
                 lastAckBaseId.length() > 0 ? lastAckBaseId.c_str() : "BASE-01",
                 lastAckRssi != 0 ? lastAckRssi : -68);
        tft.print(ackBuf);
        
        tft.setTextColor(COLOR_TEXT_MUTED);
        tft.setCursor(18, sCardY + 38);
        tft.print(F("RESPONSE MSG:    "));
        tft.setTextColor(COLOR_GREEN_BRIGHT);
        tft.print(lastAckMessage.length() > 0 ? lastAckMessage.c_str() : "DISPATCHED / VERIFIED");
        
        // Right code badge
        int rBadgeW = 38;
        int rBadgeH = 22;
        int rBadgeX = 304 - rBadgeW;
        int rBadgeY = sCardY + 8;
        tft.fillRect(rBadgeX, rBadgeY, rBadgeW, rBadgeH, RGB565(15, 36, 26));
        tft.drawRect(rBadgeX, rBadgeY, rBadgeW, rBadgeH, COLOR_GREEN_BRIGHT);
        tft.setTextSize(TEXT_MEDIUM);
        tft.setTextColor(COLOR_GREEN_BRIGHT);
        tft.setCursor(rBadgeX + 6, rBadgeY + 3);
        tft.print('[');
        tft.print(getAlertCode(selectedAlertIndex));
        tft.print(']');
        
        // Command Deck & Auto-Return Countdown Rail (y = 184 to 232)
        int autoY = 184;
        int autoH = 20;
        tft.fillRect(8, autoY, 304, autoH, RGB565(8, 24, 16));
        tft.drawRect(8, autoY, 304, autoH, COLOR_GREEN_DARK);
        
        int segs = 20;
        int segW = (304 - 6 - (segs - 1) * 2) / segs;
        for (int s = 0; s < segs; s++) {
            tft.fillRect(11 + s * (segW + 2), autoY + 3, segW, autoH - 6, RGB565(0, 140, 75));
        }
        
        int promptY = 208;
        int promptH = 24;
        tft.fillRect(8, promptY, 304, promptH, RGB565(10, 26, 18));
        tft.drawRect(8, promptY, 304, promptH, RGB565(25, 65, 45));
        
        tft.setTextSize(TEXT_SMALL);
        tft.setTextColor(COLOR_GREEN_BRIGHT);
        tft.setCursor(14, promptY + 7);
        tft.print(F("[ANY KEY]"));
        tft.setTextColor(COLOR_WHITE);
        tft.setCursor(80, promptY + 7);
        tft.print(F("RETURN TO MENU (AUTO IN 4s)"));
        
        // Signature bottom gradient hairline
        drawGradientH(6, SCREEN_HEIGHT - 3, SCREEN_WIDTH - 12, 2, COLOR_GREEN_DARK, COLOR_CYAN_DARK);
        
        setLED(LED_GREEN, true);
        setLED(LED_RED, false);
        playSuccessTone();
        
    } else {
        // ═══════════════════════════════════════════════════════════════════════
        //                      FAILURE RESULT SCREEN
        // ═══════════════════════════════════════════════════════════════════════
        tft.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, RGB565(18, 8, 10));
        
        // Dual perimeter hairlines
        tft.drawFastHLine(2, 2, SCREEN_WIDTH - 4, RGB565(55, 20, 25));
        tft.drawFastHLine(4, 4, SCREEN_WIDTH - 8, RGB565(35, 12, 16));
        tft.drawFastHLine(2, SCREEN_HEIGHT - 3, SCREEN_WIDTH - 4, RGB565(55, 20, 25));
        tft.drawFastHLine(4, SCREEN_HEIGHT - 5, SCREEN_WIDTH - 8, RGB565(35, 12, 16));
        tft.drawFastVLine(2, 2, SCREEN_HEIGHT - 4, RGB565(55, 20, 25));
        tft.drawFastVLine(SCREEN_WIDTH - 3, 2, SCREEN_HEIGHT - 4, RGB565(55, 20, 25));
        tft.drawFastVLine(4, 4, SCREEN_HEIGHT - 8, RGB565(35, 12, 16));
        tft.drawFastVLine(SCREEN_WIDTH - 5, 4, SCREEN_HEIGHT - 8, RGB565(35, 12, 16));
        
        // Red Corner Registration Marks
        int mDist = 8;
        tft.drawFastHLine(mDist - 3, mDist, 7, COLOR_RED_DARK);
        tft.drawFastVLine(mDist, mDist - 3, 7, COLOR_RED_DARK);
        tft.drawFastHLine(SCREEN_WIDTH - mDist - 4, mDist, 7, COLOR_RED_DARK);
        tft.drawFastVLine(SCREEN_WIDTH - mDist - 1, mDist - 3, 7, COLOR_RED_DARK);
        tft.drawFastHLine(mDist - 3, SCREEN_HEIGHT - mDist - 1, 7, COLOR_RED_DARK);
        tft.drawFastVLine(mDist, SCREEN_HEIGHT - mDist - 4, 7, COLOR_RED_DARK);
        tft.drawFastHLine(SCREEN_WIDTH - mDist - 4, SCREEN_HEIGHT - mDist - 1, 7, COLOR_RED_DARK);
        tft.drawFastVLine(SCREEN_WIDTH - mDist - 1, SCREEN_HEIGHT - mDist - 4, 7, COLOR_RED_DARK);
        
        // Top Header Telemetry Strip
        int topStripY = 6;
        int topStripH = 16;
        tft.fillRect(6, topStripY, SCREEN_WIDTH - 12, topStripH, RGB565(36, 12, 16));
        tft.drawRect(6, topStripY, SCREEN_WIDTH - 12, topStripH, RGB565(85, 25, 30));
        tft.drawFastHLine(6, topStripY + topStripH, SCREEN_WIDTH - 12, COLOR_RED);
        
        // Red LED
        tft.fillRect(10, topStripY + 4, 8, 8, COLOR_RED);
        tft.drawRect(9, topStripY + 3, 10, 10, COLOR_WHITE);
        
        tft.setTextSize(TEXT_SMALL);
        tft.setTextColor(COLOR_RED_BRIGHT);
        tft.setCursor(24, topStripY + 4);
        char unitTag[24];
        sprintf(unitTag, "LIFELINE // TX #%03d", DEVICE_ID);
        tft.print(unitTag);
        
        tft.setTextColor(COLOR_RED_BRIGHT);
        tft.setCursor(266, topStripY + 4);
        tft.print(F("STATUS:ERR"));
        
        // Hero Tactical Hazard Reticle & Cross Emblem (y = 36 to 116)
        int cx = SCREEN_WIDTH / 2; // 160
        int cy = 66;
        
        // Crosshairs
        tft.drawFastHLine(cx - 60, cy, 120, RGB565(45, 18, 20));
        tft.drawFastVLine(cx, cy - 28, 56, RGB565(45, 18, 20));
        
        // Reticle rings
        tft.drawCircle(cx, cy, 32, RGB565(75, 20, 25));
        tft.drawCircle(cx, cy, 33, RGB565(95, 25, 30));
        tft.drawCircle(cx, cy, 24, COLOR_RED_DARK);
        tft.drawCircle(cx, cy, 25, COLOR_RED);
        
        // Center Sharp Red Badge
        tft.fillRect(cx - 16, cy - 16, 33, 33, RGB565(48, 10, 14));
        tft.drawRect(cx - 16, cy - 16, 33, 33, COLOR_RED);
        tft.drawRect(cx - 15, cy - 15, 31, 31, COLOR_WHITE);
        
        // Razor-sharp military Cross [X]
        for (int t = -2; t <= 2; t++) {
            tft.drawLine(cx - 9 + t, cy - 9, cx + 9 + t, cy + 9, COLOR_WHITE);
            tft.drawLine(cx - 9 + t, cy + 9, cx + 9 + t, cy - 9, COLOR_WHITE);
        }
        
        // Headline with shadow
        tft.setTextSize(TEXT_MEDIUM);
        tft.setTextColor(RGB565(25, 5, 8));
        tft.setCursor(cx - 100, 96);
        tft.print(F("TRANSMISSION FAILED!"));
        tft.setTextColor(COLOR_RED_BRIGHT);
        tft.setCursor(cx - 101, 95);
        tft.print(F("TRANSMISSION FAILED!"));
        
        tft.setTextSize(TEXT_SMALL);
        tft.setTextColor(COLOR_AMBER);
        tft.setCursor(cx - 110, 111);
        tft.print(F("GATEWAY DID NOT ACKNOWLEDGE PACKET BROADCAST"));
        
        // Diagnostic & Attempt Card (y = 124 to 178)
        int fCardY = 124;
        int fCardH = 54;
        tft.fillRect(8, fCardY, 304, fCardH, RGB565(26, 14, 18));
        tft.drawRect(8, fCardY, 304, fCardH, RGB565(75, 25, 30));
        tft.drawFastHLine(9, fCardY + 1, 302, COLOR_RED_DARK);
        
        // Left 5px solid red bar
        tft.fillRect(8, fCardY, 5, fCardH, COLOR_RED);
        
        // Corner ticks
        tft.drawFastHLine(8, fCardY, 4, COLOR_WHITE);
        tft.drawFastVLine(8, fCardY, 4, COLOR_WHITE);
        tft.drawFastHLine(308, fCardY, 4, COLOR_WHITE);
        tft.drawFastVLine(311, fCardY, 4, COLOR_WHITE);
        tft.drawFastHLine(8, fCardY + fCardH - 1, 4, COLOR_WHITE);
        tft.drawFastVLine(8, fCardY + fCardH - 4, 4, COLOR_WHITE);
        tft.drawFastHLine(308, fCardY + fCardH - 1, 4, COLOR_WHITE);
        tft.drawFastVLine(311, fCardY + fCardH - 4, 4, COLOR_WHITE);
        
        tft.setTextSize(TEXT_SMALL);
        tft.setTextColor(COLOR_TEXT_MUTED);
        tft.setCursor(18, fCardY + 8);
        tft.print(F("ATTEMPT:         "));
        char attStr[32];
        sprintf(attStr, "ATTEMPT %d OF %d FAILED", retryCount + 1, MAX_RETRY_ATTEMPTS);
        tft.setTextColor(COLOR_RED_BRIGHT);
        tft.print(attStr);
        
        tft.setTextColor(COLOR_TEXT_MUTED);
        tft.setCursor(18, fCardY + 23);
        tft.print(F("RF DIAGNOSTIC:   "));
        tft.setTextColor(COLOR_AMBER);
        tft.print(F("UPLINK TIMEOUT // NO BASE ACK"));
        
        tft.setTextColor(COLOR_TEXT_MUTED);
        tft.setCursor(18, fCardY + 38);
        tft.print(F("TACTICAL ADVICE: "));
        tft.setTextColor(COLOR_WHITE);
        tft.print(F("EXTEND ANTENNA / MOVE TO HIGHER GROUND"));
        
        // Command Deck Action Buttons (y = 184, h = 42)
        int failBtnY = 184;
        int failBtnW = 148;
        int failBtnH = 42;
        
        // Left: [* RETRY SEND]
        int retryBtnX = 8;
        tft.fillRect(retryBtnX, failBtnY, failBtnW, failBtnH, RGB565(45, 30, 8));
        tft.drawRect(retryBtnX, failBtnY, failBtnW, failBtnH, COLOR_AMBER_BRIGHT);
        tft.drawRect(retryBtnX + 1, failBtnY + 1, failBtnW - 2, failBtnH - 2, COLOR_AMBER_DARK);
        tft.drawFastHLine(retryBtnX + 2, failBtnY + 2, failBtnW - 4, COLOR_WHITE);
        
        tft.fillRect(retryBtnX + 8, failBtnY + 16, 8, 8, COLOR_AMBER_BRIGHT);
        tft.drawRect(retryBtnX + 7, failBtnY + 15, 10, 10, COLOR_WHITE);
        
        tft.setTextSize(TEXT_MEDIUM);
        tft.setTextColor(RGB565(25, 15, 0));
        tft.setCursor(retryBtnX + 25, failBtnY + 8);
        tft.print(F("* RETRY"));
        tft.setTextColor(COLOR_WHITE);
        tft.setCursor(retryBtnX + 24, failBtnY + 7);
        tft.print(F("* RETRY"));
        
        tft.setTextSize(TEXT_SMALL);
        tft.setTextColor(COLOR_AMBER_BRIGHT);
        tft.setCursor(retryBtnX + 24, failBtnY + 25);
        char retrySub[24];
        sprintf(retrySub, "TRY %d OF %d", min(retryCount + 2, MAX_RETRY_ATTEMPTS), MAX_RETRY_ATTEMPTS);
        tft.print(retrySub);
        
        // Right: [# ABORT / MENU]
        int menuBtnX = 164;
        tft.fillRect(menuBtnX, failBtnY, failBtnW, failBtnH, RGB565(18, 24, 34));
        tft.drawRect(menuBtnX, failBtnY, failBtnW, failBtnH, RGB565(60, 80, 110));
        tft.drawRect(menuBtnX + 1, failBtnY + 1, failBtnW - 2, failBtnH - 2, RGB565(35, 50, 75));
        tft.drawFastHLine(menuBtnX + 2, failBtnY + 2, failBtnW - 4, COLOR_WHITE);
        
        tft.fillRect(menuBtnX + 8, failBtnY + 16, 8, 8, COLOR_TEXT_MUTED);
        tft.drawRect(menuBtnX + 7, failBtnY + 15, 10, 10, COLOR_WHITE);
        
        tft.setTextSize(TEXT_MEDIUM);
        tft.setTextColor(RGB565(5, 10, 15));
        tft.setCursor(menuBtnX + 25, failBtnY + 8);
        tft.print(F("# MENU"));
        tft.setTextColor(COLOR_WHITE);
        tft.setCursor(menuBtnX + 24, failBtnY + 7);
        tft.print(F("# MENU"));
        
        tft.setTextSize(TEXT_SMALL);
        tft.setTextColor(COLOR_TEXT_SECONDARY);
        tft.setCursor(menuBtnX + 24, failBtnY + 25);
        tft.print(F("ABORT DISPATCH"));
        
        // Signature bottom gradient hairline
        drawGradientH(6, SCREEN_HEIGHT - 3, SCREEN_WIDTH - 12, 2, COLOR_RED_DARK, COLOR_AMBER_DARK);
        
        setLED(LED_GREEN, false);
        setLED(LED_RED, true);
        playErrorTone();
    }
    
    resultStartTime = millis();
    Serial.printf("[SCREEN] Tactical Result: %s\n", lastTransmitSuccess ? "SUCCESS" : "FAILED");
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

void drawBLEPortalScreen() {
    tft.fillScreen(COLOR_BG_PRIMARY);
    drawHeader("BLUETOOTH PORTAL");

    int card1Y = CONTENT_START_Y + 2;
    int cardW = SCREEN_WIDTH - MARGIN * 2;
    int card1H = 46;

    // Card 1: BLE Radio Power & Service
    bool bleOn = isBLERadioEnabled();
    drawSharpCard(MARGIN, card1Y, cardW, card1H, COLOR_BG_CARD, bleOn ? COLOR_CYAN : RGB565(50, 60, 80), bleOn ? COLOR_CYAN : COLOR_RED);
    
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(MARGIN + 12, card1Y + 8);
    tft.print(F("BLE RADIO POWER: "));
    tft.setTextColor(bleOn ? COLOR_GREEN_BRIGHT : COLOR_RED_BRIGHT);
    tft.print(bleOn ? F("ENABLED [ON]") : F("DISABLED [OFF]"));

    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.setCursor(MARGIN + 12, card1Y + 24);
    char nameBuf[40];
    snprintf(nameBuf, sizeof(nameBuf), "NAME: LifeLine-TX-%03d  [KEY 1: TOGGLE]", DEVICE_ID);
    tft.print(nameBuf);

    // Card 2: Connected Device
    int card2Y = card1Y + card1H + 6;
    int card2H = 44;
    bool connected = isBLEConnected();
    drawSharpCard(MARGIN, card2Y, cardW, card2H, COLOR_BG_CARD, connected ? COLOR_GREEN : RGB565(40, 50, 70), connected ? COLOR_GREEN : COLOR_CYAN);
    
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(MARGIN + 12, card2Y + 8);
    tft.print(F("MOBILE COMPANION: "));
    tft.setTextColor(connected ? COLOR_GREEN_BRIGHT : (bleOn ? COLOR_AMBER_BRIGHT : COLOR_TEXT_MUTED));
    tft.print(connected ? F("CONNECTED (ACTIVE)") : (bleOn ? F("ADVERTISING (WAITING)") : F("RADIO OFF")));

    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.setCursor(MARGIN + 12, card2Y + 24);
    char clientBuf[48];
    snprintf(clientBuf, sizeof(clientBuf), "DEVICE: %s", getConnectedClientInfo().c_str());
    tft.print(clientBuf);

    // Card 3: Received Base Station Messages
    int card3Y = card2Y + card2H + 6;
    int card3H = 68;
    drawSharpCard(MARGIN, card3Y, cardW, card3H, COLOR_BG_CARD, RGB565(25, 45, 65), COLOR_CYAN);

    tft.setTextColor(COLOR_CYAN_BRIGHT);
    tft.setCursor(MARGIN + 12, card3Y + 7);
    if (rxMessageCount > 0) {
        char msgHdr[48];
        snprintf(msgHdr, sizeof(msgHdr), "BASE MSG [%d/%d]  FROM: %s (%d dBm)",
                 blePortalScrollIndex + 1, rxMessageCount,
                 rxMessageHistory[blePortalScrollIndex].sender.c_str(),
                 rxMessageHistory[blePortalScrollIndex].rssi);
        tft.print(msgHdr);

        // Status badge line
        tft.setTextColor(COLOR_AMBER_BRIGHT);
        tft.setCursor(MARGIN + 12, card3Y + 22);
        char stBuf[32];
        snprintf(stBuf, sizeof(stBuf), "STATUS: %s", rxMessageHistory[blePortalScrollIndex].status.c_str());
        tft.print(stBuf);

        // Message body
        tft.setTextSize(TEXT_MEDIUM);
        tft.setTextColor(COLOR_WHITE);
        tft.setCursor(MARGIN + 12, card3Y + 38);
        String msgText = rxMessageHistory[blePortalScrollIndex].text;
        if (msgText.length() > 24) msgText = msgText.substring(0, 24) + "...";
        tft.print(msgText);
    } else {
        tft.print(F("BASE STATION DOWNLINK FEED"));
        tft.setTextColor(COLOR_TEXT_MUTED);
        tft.setCursor(MARGIN + 12, card3Y + 26);
        tft.print(F("No messages received from Base Station yet."));
        tft.setCursor(MARGIN + 12, card3Y + 44);
        tft.print(F("Incoming commands and dispatch will appear here."));
    }

    drawFooter("[1] TOGGLE BLE   [B] NEXT MSG   [#] BACK");
    Serial.println(F("[SCREEN] BLE Portal screen displayed"));
}

void drawMessagePopupScreen() {
    // Semi-modal blackout with cyan emergency border
    tft.fillScreen(RGB565(12, 14, 20));
    
    // Outer border
    tft.drawRect(4, 4, SCREEN_WIDTH - 8, SCREEN_HEIGHT - 8, COLOR_CYAN);
    tft.drawRect(6, 6, SCREEN_WIDTH - 12, SCREEN_HEIGHT - 12, COLOR_WHITE);
    
    // Header Banner
    int bannerH = 34;
    tft.fillRect(8, 8, SCREEN_WIDTH - 16, bannerH, RGB565(15, 35, 55));
    tft.drawFastHLine(8, 8 + bannerH, SCREEN_WIDTH - 16, COLOR_CYAN_BRIGHT);
    
    tft.setTextSize(TEXT_MEDIUM);
    tft.setTextColor(COLOR_CYAN_BRIGHT);
    tft.setCursor(20, 16);
    tft.print(F("BASE STATION INSTRUCTION"));
    
    // Status & Source Strip
    int stripY = 8 + bannerH + 8;
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(20, stripY);
    tft.print(F("SOURCE: "));
    tft.setTextColor(COLOR_WHITE);
    tft.print(popupSender.length() > 0 ? popupSender : "BASE STATION #01");
    tft.print(F("  |  RSSI: "));
    tft.setTextColor(COLOR_CYAN_BRIGHT);
    char rBuf[16];
    snprintf(rBuf, sizeof(rBuf), "%d dBm", popupRssi);
    tft.print(rBuf);
    
    // Status Badge
    int badgeY = stripY + 18;
    tft.fillRect(20, badgeY, 130, 20, RGB565(10, 45, 30));
    tft.drawRect(20, badgeY, 130, 20, COLOR_GREEN_BRIGHT);
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_GREEN_BRIGHT);
    tft.setCursor(26, badgeY + 6);
    tft.print(popupStatus.length() > 0 ? popupStatus : "DISPATCHED");
    
    // Message Body Card
    int msgCardY = badgeY + 28;
    int msgCardH = 75;
    tft.fillRect(16, msgCardY, SCREEN_WIDTH - 32, msgCardH, RGB565(20, 24, 34));
    tft.drawRect(16, msgCardY, SCREEN_WIDTH - 32, msgCardH, COLOR_BORDER);
    
    tft.setTextSize(TEXT_MEDIUM);
    tft.setTextColor(COLOR_WHITE);
    
    String msg = popupMessage;
    if (msg.length() <= 24) {
        tft.setCursor(26, msgCardY + 24);
        tft.print(msg);
    } else {
        String l1 = msg.substring(0, 24);
        String l2 = msg.substring(24);
        if (l2.length() > 24) l2 = l2.substring(0, 22) + "..";
        tft.setCursor(26, msgCardY + 16);
        tft.print(l1);
        tft.setCursor(26, msgCardY + 40);
        tft.print(l2);
    }
    
    // Dismiss action prompt
    int footerY = SCREEN_HEIGHT - 32;
    tft.fillRect(16, footerY, SCREEN_WIDTH - 32, 24, RGB565(10, 30, 45));
    tft.drawRect(16, footerY, SCREEN_WIDTH - 32, 24, COLOR_CYAN);
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_CYAN_BRIGHT);
    tft.setCursor(32, footerY + 8);
    tft.print(F("PRESS ANY KEY [*] OR [#] TO DISMISS"));
    
    Serial.println(F("[SCREEN] Emergency Message Popup displayed"));
}

void triggerMessagePopup(const String& title, const String& sender, const String& message, int rssi, const String& status) {
    popupTitle = title;
    popupSender = sender;
    popupMessage = message;
    popupStatus = status;
    popupRssi = rssi;
    popupStartTime = millis();

    // Audible alarm alert
    playConfirmTone();
    delay(100);
    playConfirmTone();

    previousScreen = currentScreen;
    currentScreen = SCREEN_MESSAGE_POPUP;
    drawMessagePopupScreen();
}


