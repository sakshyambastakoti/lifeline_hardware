#include "LoRaComm.h"
#include <SPI.h>
#include <LoRa.h>

bool loraInitialized = false;

bool initLoRa() {
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
    Serial.println(F("[OK] SPI initialized for LoRa"));
    
    LoRa.setPins(LORA_CS, LORA_RST, LORA_DIO0);
    if (!LoRa.begin(LORA_FREQUENCY)) {
        Serial.println(F("[ERROR] LoRa initialization failed!"));
        loraInitialized = false;
        return false;
    }
    
    LoRa.setSpreadingFactor(LORA_SF);
    LoRa.setSignalBandwidth(LORA_BW);
    LoRa.enableCrc();
    loraInitialized = true;
    Serial.println(F("[OK] LoRa initialized @ 433MHz, SF12, BW125kHz, CRC enabled"));
    Serial.println(F("[OK] LoRa in continuous receive mode"));
    return true;
}

static int mapEmergencyCodeToAlertIndex(char code) {
    switch (code) {
        case 'N': return 4;  // STATUS OK
        case 'F': return 0;  // EMERGENCY (Fire)
        case 'L': return 11; // LANDSLIDE
        case 'Q': return 0;  // EMERGENCY (Earthquake)
        case 'S': return 0;  // EMERGENCY (Manual SOS)
        case 'M': return 1;  // MEDICAL
        case 'H': return 8;  // WEATHER ALERT
        case 'C': return 12; // SNOW STORM
        case 'W': return 8;  // WEATHER ALERT
        case 'G': return 0;  // EMERGENCY (Gas)
        case 'B': return 13; // EQUIPMENT FAILURE
        default:
            if (code >= 'A' && code <= 'O') return code - 'A';
            if (code >= 'a' && code <= 'o') return code - 'a';
            return 14; // OTHER
    }
}

bool parseLoRaPacket(int& deviceId, int& alertIndex, int& rssi) {
    int packetSize = LoRa.parsePacket();
    if (packetSize == 0) return false;
    
    String data = "";
    while (LoRa.available()) {
        data += (char)LoRa.read();
    }
    
    rssi = LoRa.packetRssi();
    
    Serial.printf("[RX] Raw packet (%d bytes): '%s', RSSI: %d\n", packetSize, data.c_str(), rssi);
    
    if (data.startsWith("TX")) {
        data = data.substring(2);
    }
    
    int comma = data.indexOf(',');
    if (comma <= 0) {
        Serial.println(F("[RX] Invalid packet format"));
        return false;
    }
    
    deviceId = data.substring(0, comma).toInt();
    
    String alertPart = data.substring(comma + 1);
    alertPart.trim();
    
    if (alertPart.length() == 1 && alertPart[0] >= 'A' && alertPart[0] <= 'O') {
        alertIndex = alertPart[0] - 'A';
    } else if (alertPart.length() == 1 && alertPart[0] >= 'a' && alertPart[0] <= 'o') {
        alertIndex = alertPart[0] - 'a';
    } else {
        alertIndex = alertPart.toInt();
    }
    
    if (alertIndex < 0 || alertIndex >= ALERT_COUNT) {
        Serial.printf("[RX] Invalid alert index %d, defaulting to OTHER\n", alertIndex);
        alertIndex = ALERT_COUNT - 1;
    }
    
    Serial.printf("[RX] Parsed: Device=%d, Alert=%d (%s)\n", deviceId, alertIndex, alertNames[alertIndex]);
    
    LoRa.receive();
    return true;
}

bool parseLoRaPacketExtended(FullTelemetryData& telemetry) {
    int packetSize = LoRa.parsePacket();
    if (packetSize == 0) return false;

    String data = "";
    while (LoRa.available()) {
        data += (char)LoRa.read();
    }
    data.trim();

    telemetry.rssi = LoRa.packetRssi();
    telemetry.isChatMessage = false;
    telemetry.chatMessage = "";

    Serial.printf("[RX EX] Raw packet (%d bytes): '%s', RSSI: %d\n", packetSize, data.c_str(), telemetry.rssi);

    if (data.startsWith("TX")) {
        data = data.substring(2);
    }

    // Check for freeform Mobile Chat format: <ID>,CHAT,<text>
    if (data.indexOf(",CHAT,") != -1) {
        int chatComma = data.indexOf(",CHAT,");
        telemetry.deviceId = data.substring(0, chatComma).toInt();
        telemetry.emergencyCode = 'M';
        telemetry.alertIndex = 0; // Highlight as emergency attention
        telemetry.isFullTelemetry = false;
        telemetry.isChatMessage = true;
        telemetry.chatMessage = data.substring(chatComma + 6);
        telemetry.chatMessage.trim();
        Serial.printf("[RX CHAT] Device #%d Chat: '%s'\n", telemetry.deviceId, telemetry.chatMessage.c_str());
        LoRa.receive();
        return true;
    }

    // Split payload by commas
    int commaIndices[12];
    int tokenCount = 0;
    int searchPos = 0;

    while (tokenCount < 12) {
        int idx = data.indexOf(',', searchPos);
        if (idx == -1) break;
        commaIndices[tokenCount++] = idx;
        searchPos = idx + 1;
    }

    if (tokenCount == 0) {
        Serial.println(F("[RX EX] Invalid packet: No commas found"));
        return false;
    }

    // 1. Device ID
    telemetry.deviceId = data.substring(0, commaIndices[0]).toInt();

    // 2. Emergency Code
    String codeStr = (tokenCount >= 1) ? data.substring(commaIndices[0] + 1, (tokenCount > 1) ? commaIndices[1] : data.length()) : "N";
    codeStr.trim();
    telemetry.emergencyCode = codeStr.length() > 0 ? codeStr[0] : 'N';
    telemetry.alertIndex = mapEmergencyCodeToAlertIndex(telemetry.emergencyCode);

    // If extended CSV packet (has 9 comma separators -> 10 tokens)
    if (tokenCount >= 9) {
        telemetry.isFullTelemetry = true;

        // Token 2: Temp x10
        int temp_x10 = data.substring(commaIndices[1] + 1, commaIndices[2]).toInt();
        telemetry.temperature = temp_x10 / 10.0f;

        // Token 3: Hum x10
        int hum_x10 = data.substring(commaIndices[2] + 1, commaIndices[3]).toInt();
        telemetry.humidity = hum_x10 / 10.0f;

        // Token 4: Gas PPM
        telemetry.gasPpm = data.substring(commaIndices[3] + 1, commaIndices[4]).toInt();

        // Token 5: Lat e7
        long lat_e7 = data.substring(commaIndices[4] + 1, commaIndices[5]).toInt();
        telemetry.latitude = (double)lat_e7 / 1e7;

        // Token 6: Lon e7
        long lon_e7 = data.substring(commaIndices[5] + 1, commaIndices[6]).toInt();
        telemetry.longitude = (double)lon_e7 / 1e7;

        // Token 7: Alt meters
        telemetry.altitude = data.substring(commaIndices[6] + 1, commaIndices[7]).toInt();

        // Token 8: Health Score
        telemetry.healthScore = data.substring(commaIndices[7] + 1, commaIndices[8]).toInt();

        // Token 9: Risk Score
        telemetry.riskScore = data.substring(commaIndices[8] + 1).toInt();
    } else {
        // Legacy single string alert (TX003,F)
        telemetry.isFullTelemetry = false;
        telemetry.temperature = 0;
        telemetry.humidity = 0;
        telemetry.gasPpm = 0;
        telemetry.latitude = 0;
        telemetry.longitude = 0;
        telemetry.altitude = 0;
        telemetry.healthScore = 100;
        telemetry.riskScore = 0;
    }

    Serial.printf("[RX EX] Parsed: Device=%d, Code=%c, FullData=%s, Temp=%.1f, Lat=%.6f, Lon=%.6f\n",
                  telemetry.deviceId, telemetry.emergencyCode, telemetry.isFullTelemetry ? "YES" : "NO",
                  telemetry.temperature, telemetry.latitude, telemetry.longitude);

    LoRa.receive();
    return true;
}

bool sendDownlinkACK(int targetDeviceId, char emergencyCode, const char* status, const char* message) {
    if (!loraInitialized) {
        Serial.println(F("[DOWNLINK] LoRa not initialized"));
        return false;
    }

    // 120ms turnaround delay to let field unit transition to receive mode
    delay(120);

    char packet[96];
    snprintf(packet, sizeof(packet), "ACK%03d,%c,%s,BASE%02d,%s",
             targetDeviceId, emergencyCode, status, DEVICE_ID, message);

    Serial.printf("[LORA DOWNLINK ACK] '%s'\n", packet);

    LoRa.idle();
    delay(5);
    LoRa.beginPacket();
    LoRa.print(packet);
    bool ok = LoRa.endPacket();

    LoRa.receive();
    Serial.printf("[LORA DOWNLINK ACK] Result: %s\n", ok ? "OK" : "FAILED");
    return ok;
}

bool sendDownlinkCommand(int targetDeviceId, const char* action, const char* message) {
    if (!loraInitialized) return false;

    delay(120);
    char packet[96];
    snprintf(packet, sizeof(packet), "CMD%03d,%s,%s",
             targetDeviceId, action, message);

    Serial.printf("[LORA DOWNLINK CMD] '%s'\n", packet);

    LoRa.idle();
    delay(5);
    LoRa.beginPacket();
    LoRa.print(packet);
    bool ok = LoRa.endPacket();

    LoRa.receive();
    return ok;
}

bool sendBroadcastEvacuation(const char* message) {
    if (!loraInitialized) return false;

    delay(120);
    char packet[96];
    snprintf(packet, sizeof(packet), "EVAC,ALL,%s", message);

    Serial.printf("[LORA EVAC BROADCAST] '%s'\n", packet);

    LoRa.idle();
    delay(5);
    LoRa.beginPacket();
    LoRa.print(packet);
    bool ok = LoRa.endPacket();

    LoRa.receive();
    return ok;
}
