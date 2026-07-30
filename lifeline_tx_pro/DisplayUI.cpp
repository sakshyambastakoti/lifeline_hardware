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

void drawPremiumCard(int x, int y, int w, int h, uint16_t bgColor, uint16_t borderColor, bool hasGlow) {
    tft.fillRoundRect(x + SHADOW_OFFSET, y + SHADOW_OFFSET, w, h, BORDER_RADIUS, RGB565(5, 5, 10));
    tft.fillRoundRect(x, y, w, h, BORDER_RADIUS, bgColor);
    tft.drawRoundRect(x, y, w, h, BORDER_RADIUS, borderColor);
    tft.drawFastHLine(x + 4, y + 1, w - 8, RGB565(60, 60, 80));
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
    tft.fillRoundRect(x, y - 2, 14, 12, 2, COLOR_BADGE_BG);
    tft.drawRoundRect(x, y - 2, 14, 12, 2, keyColor);
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
    drawGradientV(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, RGB565(15, 20, 35), COLOR_BG_PRIMARY);
    drawGradientH(0, 0, SCREEN_WIDTH, 4, COLOR_CYAN_DARK, COLOR_PURPLE_DARK);
    
    int crossCenterX = SCREEN_WIDTH / 2;
    int crossCenterY = 50;
    int crossSize = 22;
    int crossThick = 10;
    
    for (int g = 4; g > 0; g--) {
        uint16_t glowColor = RGB565(80 - g*15, 20 - g*4, 20 - g*4);
        tft.fillRect(crossCenterX - crossThick/2 - g, crossCenterY - crossSize - g, 
                     crossThick + g*2, crossSize * 2 + g*2, glowColor);
        tft.fillRect(crossCenterX - crossSize - g, crossCenterY - crossThick/2 - g, 
                     crossSize * 2 + g*2, crossThick + g*2, glowColor);
    }
    
    tft.fillRect(crossCenterX - crossThick/2, crossCenterY - crossSize, 
                 crossThick, crossSize * 2, COLOR_RED);
    tft.fillRect(crossCenterX - crossSize, crossCenterY - crossThick/2, 
                 crossSize * 2, crossThick, COLOR_RED);
    
    tft.fillRect(crossCenterX - crossThick/2 + 2, crossCenterY - crossSize + 2, 
                 2, crossSize - 4, COLOR_RED_BRIGHT);
    
    tft.setTextSize(TEXT_XLARGE);
    int16_t x1, y1;
    uint16_t tw, th;
    tft.getTextBounds("LifeLine", 0, 0, &x1, &y1, &tw, &th);
    int titleX = (SCREEN_WIDTH - tw) / 2;
    int titleY = 85;
    
    tft.setTextColor(COLOR_CYAN_DARK);
    tft.setCursor(titleX + 2, titleY + 2);
    tft.print(F("LifeLine"));
    
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.setCursor(titleX, titleY);
    tft.print(F("LifeLine"));
    
    for (int i = 0; i < tw; i++) {
        uint16_t lineColor = (i < tw/2) ? COLOR_CYAN : COLOR_PURPLE;
        tft.drawPixel(titleX + i, titleY + th + 6, lineColor);
        tft.drawPixel(titleX + i, titleY + th + 7, lineColor);
    }
    
    drawCenteredText("EMERGENCY COMMUNICATION", 130, TEXT_SMALL, COLOR_CYAN);
    tft.fillCircle(SCREEN_WIDTH/2 - 60, 144, 2, COLOR_ACCENT_LINE);
    drawCenteredText("TRANSMITTER", 140, TEXT_MEDIUM, COLOR_TEXT_SECONDARY);
    tft.fillCircle(SCREEN_WIDTH/2 + 60, 144, 2, COLOR_ACCENT_LINE);
    
    int cardY = SCREEN_HEIGHT - 65;
    int cardW = (SCREEN_WIDTH - MARGIN * 3) / 2;
    
    drawPremiumCard(MARGIN, cardY, cardW, 50, COLOR_BG_CARD, COLOR_BORDER);
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_CYAN);
    tft.setCursor(MARGIN + 8, cardY + 8);
    tft.print(F("DEVICE"));
    
    char deviceIdStr[16];
    sprintf(deviceIdStr, "TX #%03d", DEVICE_ID);
    tft.setTextSize(TEXT_MEDIUM);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.setCursor(MARGIN + 8, cardY + 26);
    tft.print(deviceIdStr);
    
    int rightCardX = MARGIN * 2 + cardW;
    drawPremiumCard(rightCardX, cardY, cardW, 50, COLOR_BG_CARD, COLOR_BORDER);
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(rightCardX + 8, cardY + 8);
    tft.print(F("STATUS"));
    
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_GREEN);
    tft.setCursor(rightCardX + 8, cardY + 24);
    tft.print(F("Initializing..."));
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(rightCardX + 8, cardY + 36);
    tft.print(FIRMWARE_VERSION);
    
    drawGradientH(0, SCREEN_HEIGHT - 3, SCREEN_WIDTH, 3, COLOR_PURPLE_DARK, COLOR_CYAN_DARK);
    
    bootStartTime = millis();
    Serial.println(F("[SCREEN] Premium boot screen displayed"));
}

void updateMenuSelection(int oldIndex, int newIndex) {
    int listStartY = HEADER_HEIGHT + 26;
    int itemHeight = MENU_ITEM_HEIGHT;
    
    if (oldIndex >= menuScrollOffset && oldIndex < menuScrollOffset + VISIBLE_MENU_ITEMS) {
        int i = oldIndex - menuScrollOffset;
        int itemY = listStartY + (i * itemHeight);
        int itemWidth = SCREEN_WIDTH - (2 * MARGIN);
        
        tft.fillRect(MARGIN, itemY, itemWidth, itemHeight - 2, COLOR_BG_PRIMARY);
        tft.drawRoundRect(MARGIN, itemY, itemWidth, itemHeight - 3, 4, COLOR_BORDER);
        
        tft.fillRoundRect(MARGIN + 4, itemY + 6, 22, 18, 3, COLOR_BG_CARD);
        tft.setTextSize(TEXT_MEDIUM);
        tft.setTextColor(COLOR_TEXT_MUTED);
        tft.setCursor(MARGIN + 8, itemY + 9);
        if (oldIndex + 1 < 10) tft.print(' ');
        tft.print(oldIndex + 1);
        
        tft.setTextColor(COLOR_TEXT_SECONDARY);
        tft.setCursor(MARGIN + 32, itemY + 9);
        tft.print(alertNamesShort[oldIndex]);
        
        uint16_t prioColor = getAlertColor(oldIndex);
        tft.fillCircle(SCREEN_WIDTH - MARGIN - 14, itemY + 15, 5, prioColor);
    }
    
    if (newIndex >= menuScrollOffset && newIndex < menuScrollOffset + VISIBLE_MENU_ITEMS) {
        int i = newIndex - menuScrollOffset;
        int itemY = listStartY + (i * itemHeight);
        int itemWidth = SCREEN_WIDTH - (2 * MARGIN);
        
        tft.fillRoundRect(MARGIN + 2, itemY + 2, itemWidth, itemHeight - 3, 5, RGB565(60, 55, 0));
        tft.fillRoundRect(MARGIN, itemY, itemWidth, itemHeight - 3, 5, COLOR_AMBER);
        tft.fillRect(MARGIN, itemY, 4, itemHeight - 3, COLOR_AMBER_BRIGHT);
        tft.drawFastHLine(MARGIN + 6, itemY + 2, itemWidth - 20, COLOR_AMBER_BRIGHT);
        
        tft.fillRoundRect(MARGIN + 6, itemY + 5, 24, 20, 3, COLOR_TEXT_DARK);
        tft.setTextSize(TEXT_MEDIUM);
        tft.setTextColor(COLOR_AMBER);
        tft.setCursor(MARGIN + 10, itemY + 8);
        if (newIndex + 1 < 10) tft.print(' ');
        tft.print(newIndex + 1);
        
        tft.setTextColor(COLOR_TEXT_DARK);
        tft.setCursor(MARGIN + 36, itemY + 8);
        tft.print(alertNamesShort[newIndex]);
        
        uint16_t prioColor = getAlertColor(newIndex);
        if (prioColor == COLOR_AMBER) prioColor = COLOR_ORANGE_DARK;
        tft.fillCircle(SCREEN_WIDTH - MARGIN - 14, itemY + 14, 6, prioColor);
        tft.drawCircle(SCREEN_WIDTH - MARGIN - 14, itemY + 14, 7, COLOR_TEXT_DARK);
        tft.drawCircle(SCREEN_WIDTH - MARGIN - 14, itemY + 14, 8, COLOR_TEXT_DARK);
    }
}

void drawMenuScreen() {
    drawHeader("SELECT ALERT TYPE");
    tft.fillRect(0, HEADER_HEIGHT + 1, SCREEN_WIDTH, SCREEN_HEIGHT - HEADER_HEIGHT - FOOTER_HEIGHT - 2, COLOR_BG_PRIMARY);
    
    char idStr[10];
    sprintf(idStr, "#%03d", DEVICE_ID);
    tft.fillRoundRect(SCREEN_WIDTH - 48, 8, 42, 18, 4, COLOR_BG_CARD);
    tft.drawRoundRect(SCREEN_WIDTH - 48, 8, 42, 18, 4, COLOR_CYAN_DARK);
    drawRightText(idStr, 12, TEXT_SMALL, COLOR_CYAN);
    
    int indicatorY = HEADER_HEIGHT + 8;
    int barWidth = 80;
    int barX = SCREEN_WIDTH - MARGIN - barWidth;
    tft.fillRoundRect(barX, indicatorY, barWidth, 10, 3, COLOR_BG_CARD);
    
    int fillWidth = (menuScrollOffset + VISIBLE_MENU_ITEMS) * barWidth / ALERT_COUNT;
    int startFill = menuScrollOffset * barWidth / ALERT_COUNT;
    tft.fillRoundRect(barX + startFill, indicatorY + 1, fillWidth - startFill, 8, 2, COLOR_CYAN_DARK);
    
    int firstItem = menuScrollOffset + 1;
    int lastItem = min(menuScrollOffset + VISIBLE_MENU_ITEMS, ALERT_COUNT);
    char rangeStr[16];
    sprintf(rangeStr, "%d-%d of %d", firstItem, lastItem, ALERT_COUNT);
    
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(MARGIN, indicatorY + 1);
    tft.print(rangeStr);
    
    int listStartY = HEADER_HEIGHT + 26;
    int itemHeight = MENU_ITEM_HEIGHT;
    
    for (int i = 0; i < VISIBLE_MENU_ITEMS; i++) {
        int alertIndex = menuScrollOffset + i;
        if (alertIndex >= ALERT_COUNT) break;
        
        int itemY = listStartY + (i * itemHeight);
        int itemWidth = SCREEN_WIDTH - (2 * MARGIN);
        bool isSelected = (alertIndex == selectedAlertIndex);
        
        if (isSelected) {
            tft.fillRoundRect(MARGIN + 2, itemY + 2, itemWidth, itemHeight - 3, 5, RGB565(60, 55, 0));
            tft.fillRoundRect(MARGIN, itemY, itemWidth, itemHeight - 3, 5, COLOR_AMBER);
            tft.fillRect(MARGIN, itemY, 4, itemHeight - 3, COLOR_AMBER_BRIGHT);
            tft.drawFastHLine(MARGIN + 6, itemY + 2, itemWidth - 20, COLOR_AMBER_BRIGHT);
            
            tft.fillRoundRect(MARGIN + 6, itemY + 5, 24, 20, 3, COLOR_TEXT_DARK);
            tft.setTextSize(TEXT_MEDIUM);
            tft.setTextColor(COLOR_AMBER);
            tft.setCursor(MARGIN + 10, itemY + 8);
            if (alertIndex + 1 < 10) tft.print(' ');
            tft.print(alertIndex + 1);
            
            tft.setTextColor(COLOR_TEXT_DARK);
            tft.setCursor(MARGIN + 36, itemY + 8);
            tft.print(alertNamesShort[alertIndex]);
            
            uint16_t prioColor = getAlertColor(alertIndex);
            if (prioColor == COLOR_AMBER) prioColor = COLOR_ORANGE_DARK;
            tft.fillCircle(SCREEN_WIDTH - MARGIN - 14, itemY + 14, 6, prioColor);
            tft.drawCircle(SCREEN_WIDTH - MARGIN - 14, itemY + 14, 7, COLOR_TEXT_DARK);
            tft.drawCircle(SCREEN_WIDTH - MARGIN - 14, itemY + 14, 8, COLOR_TEXT_DARK);
            
        } else {
            tft.drawRoundRect(MARGIN, itemY, itemWidth, itemHeight - 3, 4, COLOR_BORDER);
            
            tft.fillRoundRect(MARGIN + 4, itemY + 6, 22, 18, 3, COLOR_BG_CARD);
            tft.setTextSize(TEXT_MEDIUM);
            tft.setTextColor(COLOR_TEXT_MUTED);
            tft.setCursor(MARGIN + 8, itemY + 9);
            if (alertIndex + 1 < 10) tft.print(' ');
            tft.print(alertIndex + 1);
            
            tft.setTextColor(COLOR_TEXT_SECONDARY);
            tft.setCursor(MARGIN + 32, itemY + 9);
            tft.print(alertNamesShort[alertIndex]);
            
            uint16_t prioColor = getAlertColor(alertIndex);
            tft.fillCircle(SCREEN_WIDTH - MARGIN - 14, itemY + 15, 5, prioColor);
        }
    }
    
    int footerY = SCREEN_HEIGHT - FOOTER_HEIGHT;
    tft.drawFastHLine(0, footerY - 2, SCREEN_WIDTH, COLOR_CYAN_DARK);
    tft.drawFastHLine(0, footerY - 1, SCREEN_WIDTH, COLOR_ACCENT_LINE);
    drawGradientV(0, footerY, SCREEN_WIDTH, FOOTER_HEIGHT, COLOR_BG_HEADER_ALT, RGB565(5, 10, 15));
    
    int hintY = footerY + (FOOTER_HEIGHT - 8) / 2;
    int hintX = MARGIN;
    
    drawKeyBadge(hintX, hintY, 'A', "", COLOR_CYAN);
    tft.print((char)24);
    hintX += 22;
    
    drawKeyBadge(hintX, hintY, 'B', "", COLOR_CYAN);
    tft.print((char)25);
    hintX += 26;
    
    tft.drawFastVLine(hintX, footerY + 6, FOOTER_HEIGHT - 12, COLOR_ACCENT_LINE);
    hintX += 6;
    
    drawKeyBadge(hintX, hintY, '*', "SEL", COLOR_GREEN);
    hintX += 44;
    
    tft.drawFastVLine(hintX, footerY + 6, FOOTER_HEIGHT - 12, COLOR_ACCENT_LINE);
    hintX += 6;
    
    drawKeyBadge(hintX, hintY, 'C', "Info", COLOR_TEXT_SECONDARY);
    hintX += 44;
    
    drawKeyBadge(hintX, hintY, 'D', "Help", COLOR_TEXT_SECONDARY);
    
    Serial.println(F("[SCREEN] Premium Menu displayed"));
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

void drawOTAScreen() {
    tft.fillScreen(COLOR_BG_PRIMARY);
    drawHeader("WIRELESS OTA FIRMWARE PORTAL");
    
    int cardY = CONTENT_START_Y + 2;
    int cardW = SCREEN_WIDTH - MARGIN * 2;
    int cardH = 175;
    
    // Main Glassmorphic OTA Card
    drawPremiumCard(MARGIN, cardY, cardW, cardH, COLOR_BG_CARD, COLOR_CYAN, true);
    
    // Glowing Access Point Info Box
    tft.fillRoundRect(MARGIN + 6, cardY + 6, cardW - 12, 54, 4, COLOR_BG_HEADER_ALT);
    tft.drawRoundRect(MARGIN + 6, cardY + 6, cardW - 12, 54, 4, COLOR_ACCENT_BRIGHT);
    
    // Row 1 Left: AP SSID
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_CYAN);
    tft.setCursor(MARGIN + 12, cardY + 14);
    tft.print(F("WI-FI AP : "));
    tft.fillRoundRect(MARGIN + 80, cardY + 11, 125, 16, 3, RGB565(15, 45, 65));
    tft.setTextColor(COLOR_CYAN_BRIGHT);
    tft.setCursor(MARGIN + 86, cardY + 15);
    tft.print(F("LifeLine-TX-OTA"));
    
    // Row 1 Right: Password
    tft.setTextColor(COLOR_AMBER);
    tft.setCursor(MARGIN + 215, cardY + 14);
    tft.print(F("PASS: "));
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.print(F("12345678"));
    
    // Row 2: WEB PORTAL URL
    tft.setTextColor(COLOR_GREEN_BRIGHT);
    tft.setCursor(MARGIN + 12, cardY + 36);
    tft.print(F("WEB URL  : "));
    tft.fillRoundRect(MARGIN + 80, cardY + 33, 200, 16, 3, RGB565(10, 50, 30));
    tft.setTextColor(COLOR_GREEN_BRIGHT);
    tft.setCursor(MARGIN + 86, cardY + 37);
    tft.print(F("http://192.168.4.1"));
    
    // Separator line
    tft.drawFastHLine(MARGIN + 10, cardY + 66, cardW - 20, COLOR_ACCENT_LINE);
    
    // Status text label & live status string
    int progress = getOTAProgress();
    String statusStr = getOTAStatusText();
    
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(MARGIN + 12, cardY + 76);
    tft.print(F("STATUS: "));
    
    if (progress > 0 && progress < 100) {
        tft.setTextColor(COLOR_CYAN_BRIGHT);
    } else if (progress >= 100) {
        tft.setTextColor(COLOR_GREEN_BRIGHT);
    } else {
        tft.setTextColor(COLOR_AMBER_BRIGHT);
    }
    tft.print(statusStr);
    
    // Premium Glowing Progress Bar Container
    int progressY = cardY + 96;
    int progressW = cardW - 24;
    int progressH = 22;
    int progressX = MARGIN + 12;
    
    tft.fillRoundRect(progressX, progressY, progressW, progressH, 5, COLOR_BG_INPUT);
    tft.drawRoundRect(progressX, progressY, progressW, progressH, 5, progress > 0 ? COLOR_CYAN : COLOR_BORDER);
    
    if (progress > 0) {
        int fillW = (progressW - 4) * progress / 100;
        if (fillW > 0) {
            drawGradientH(progressX + 2, progressY + 2, fillW, progressH - 4, COLOR_CYAN_DARK, COLOR_GREEN_BRIGHT);
        }
    }
    
    // Percentage text inside progress bar
    char pctBuf[16];
    snprintf(pctBuf, sizeof(pctBuf), "%d%%", progress);
    tft.setTextSize(TEXT_SMALL);
    int16_t x1, y1;
    uint16_t tw, th;
    tft.getTextBounds(pctBuf, 0, 0, &x1, &y1, &tw, &th);
    
    int pctX = progressX + (progressW - tw) / 2;
    int pctY = progressY + (progressH - th) / 2;
    
    tft.setTextColor(COLOR_TEXT_DARK);
    tft.setCursor(pctX + 1, pctY + 1); // Drop shadow
    tft.print(pctBuf);
    tft.setTextColor(progress > 45 ? COLOR_TEXT_DARK : COLOR_TEXT_PRIMARY);
    tft.setCursor(pctX, pctY);
    tft.print(pctBuf);
    
    // Bottom command hints card
    tft.setTextSize(TEXT_SMALL);
    tft.setTextColor(COLOR_TEXT_MUTED);
    tft.setCursor(MARGIN + 12, cardY + 126);
    tft.print(F("PIO CLI: pio run -e esp32dev_ota -t upload"));
    tft.setCursor(MARGIN + 12, cardY + 142);
    tft.print(F("WEB OTA: Open 192.168.4.1 to upload .bin file"));
    
    drawFooter("# Exit OTA Mode");
    Serial.println(F("[SCREEN] Enhanced OTA Screen Displayed"));
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


