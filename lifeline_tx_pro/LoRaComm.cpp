#include "LoRaComm.h"
#include "BLEManager.h"
#include "DisplayUI.h"

bool lastAckReceived = false;
String lastAckStatus = "";
String lastAckBaseId = "";
String lastAckMessage = "";
int lastAckRssi = 0;
int lastAckSnr = 0;

void initLoRa() {
    pinMode(LORA_CS, OUTPUT);
    digitalWrite(LORA_CS, HIGH);
    
    if (LORA_RST >= 0) {
        pinMode(LORA_RST, OUTPUT);
        digitalWrite(LORA_RST, HIGH);
        delay(10);
        digitalWrite(LORA_RST, LOW);
        delay(10);
        digitalWrite(LORA_RST, HIGH);
        delay(20);
    }
    
    LoRa.setPins(LORA_CS, LORA_RST, LORA_DIO0);
    if (LoRa.begin(LORA_FREQUENCY)) {
        LoRa.setSpreadingFactor(LORA_SF);
        LoRa.setSignalBandwidth(LORA_BW);
        LoRa.setCodingRate4(LORA_CR);
        LoRa.setPreambleLength(LORA_PREAMBLE);
        LoRa.setSyncWord(LORA_SYNC_WORD);
        LoRa.setTxPower(LORA_TX_POWER);
        LoRa.enableCrc();
        LoRa.receive(); // Enter continuous receive mode
        loraInitialized = true;
        Serial.printf("[INIT] LoRa OK @ %.1f MHz, SF%d, BW125kHz, CR4/%d, CRC enabled (Continuous RX)\n",
                      LORA_FREQUENCY / 1E6, LORA_SF, LORA_CR);
    } else {
        loraInitialized = false;
        Serial.println(F("[INIT] LoRa FAILED!"));
    }
}

// Helper: processes a downlink frame (ACK, CMD, or EVAC)
static bool handleParsedDownlink(const String& ackData, int rssi, int snr, bool showPopup) {
    if (ackData.startsWith("ACK") || ackData.startsWith("CMD")) {
        int targetNode = -1;
        String remainder = "";
        int firstComma = ackData.indexOf(',');

        if (firstComma == 3) {
            // Format has comma right after prefix, e.g. "ACK,003,..." or "ACK,TX003,..." or "ACK,..."
            String afterPrefix = ackData.substring(4);
            int nextComma = afterPrefix.indexOf(',');
            if (nextComma > 0) {
                String nodePart = afterPrefix.substring(0, nextComma);
                if (nodePart.startsWith("TX")) nodePart = nodePart.substring(2);
                targetNode = nodePart.toInt();
                remainder = afterPrefix.substring(nextComma + 1);
            } else {
                targetNode = 0; // Broadcast or simple ACK
                remainder = afterPrefix;
            }
        } else if (firstComma > 3) {
            // Standard format: "ACK003,..." or "CMD003,..."
            targetNode = ackData.substring(3, firstComma).toInt();
            remainder = ackData.substring(firstComma + 1);
        } else if (firstComma == -1) {
            targetNode = 0;
            remainder = "ACK";
        }

        if (targetNode == DEVICE_ID || targetNode == 0 || targetNode == -1) {
            int secondComma = remainder.indexOf(',');

            if (ackData.startsWith("ACK")) {
                // Format: <CODE>,<STATUS>,<BASE_ID>,<NOTE> or simple <STATUS>
                if (secondComma > 0) {
                    String afterCode = remainder.substring(secondComma + 1);
                    int thirdComma = afterCode.indexOf(',');
                    if (thirdComma > 0) {
                        lastAckStatus = afterCode.substring(0, thirdComma);
                        String afterStatus = afterCode.substring(thirdComma + 1);
                        int fourthComma = afterStatus.indexOf(',');
                        if (fourthComma > 0) {
                            lastAckBaseId = afterStatus.substring(0, fourthComma);
                            lastAckMessage = afterStatus.substring(fourthComma + 1);
                        } else {
                            lastAckBaseId = afterStatus;
                            lastAckMessage = "Base Confirmed";
                        }
                    } else {
                        lastAckStatus = afterCode;
                        lastAckBaseId = "BASE-01";
                        lastAckMessage = "Received";
                    }
                } else {
                    lastAckStatus = remainder.length() > 0 ? remainder : "OK";
                    lastAckBaseId = "BASE-01";
                    lastAckMessage = "Acknowledged";
                }
            } else {
                // CMD: <ACTION>,<MESSAGE>
                if (secondComma > 0) {
                    lastAckStatus = remainder.substring(0, secondComma);
                    lastAckMessage = remainder.substring(secondComma + 1);
                    lastAckBaseId = "COMMAND-BASE";
                } else {
                    lastAckStatus = remainder;
                    lastAckMessage = "Command Received";
                    lastAckBaseId = "COMMAND-BASE";
                }
            }

            lastAckReceived = true;
            lastAckRssi = rssi;
            lastAckSnr = snr;

            Serial.printf("[LORA DOWNLINK MATCH] Status: %s, Base: %s, Msg: %s\n",
                          lastAckStatus.c_str(), lastAckBaseId.c_str(), lastAckMessage.c_str());

            // 1. Record into message history
            addReceivedMessageToHistory(lastAckBaseId, lastAckStatus, lastAckMessage, rssi);

            // 2. Forward to mobile phone via BLE
            notifyBLEAck(lastAckStatus.c_str(), lastAckBaseId.c_str(), lastAckMessage.c_str(), rssi, snr);

            // 3. Trigger popup if requested
            if (showPopup) {
                triggerMessagePopup("BASE STATION DISPATCH", lastAckBaseId, lastAckMessage, rssi, lastAckStatus);
            }
            return true;
        }
    } else if (ackData.startsWith("EVAC")) {
        lastAckStatus = "EVACUATE";
        lastAckBaseId = "EMERGENCY_BROADCAST";
        int comma = ackData.indexOf(',', 5);
        lastAckMessage = (comma > 0) ? ackData.substring(comma + 1) : "Evacuate to high ground immediately";
        lastAckReceived = true;
        lastAckRssi = rssi;
        lastAckSnr = snr;

        Serial.printf("[LORA EVAC MATCH] %s\n", lastAckMessage.c_str());
        addReceivedMessageToHistory(lastAckBaseId, lastAckStatus, lastAckMessage, rssi);
        notifyBLEAck(lastAckStatus.c_str(), lastAckBaseId.c_str(), lastAckMessage.c_str(), rssi, snr);

        if (showPopup) {
            triggerMessagePopup("EMERGENCY EVACUATION", lastAckBaseId, lastAckMessage, rssi, "EVACUATE");
        }
        return true;
    }
    return false;
}

// Background LoRa downlink listener (monitors channel continuously)
bool checkIncomingDownlinkLoRa() {
    if (!loraInitialized) return false;

    int packetSize = LoRa.parsePacket();
    if (packetSize == 0) return false;

    String ackData = "";
    while (LoRa.available()) {
        ackData += (char)LoRa.read();
    }
    ackData.trim();
    int rssi = LoRa.packetRssi();
    int snr = LoRa.packetSnr();

    Serial.printf("[LORA BG RECV] Raw (%d bytes): '%s', RSSI: %d, SNR: %d\n", packetSize, ackData.c_str(), rssi, snr);
    bool matched = handleParsedDownlink(ackData, rssi, snr, true);
    LoRa.receive();
    return matched;
}

// Internal helper: listens for ACK packet on LoRa for up to timeoutMs
static bool waitForDownlinkACK(unsigned long timeoutMs) {
    LoRa.receive();
    unsigned long startTime = millis();
    Serial.printf("[LORA ACK] Listening for downlink ACK window (%lu ms)...\n", timeoutMs);

    while (millis() - startTime < timeoutMs) {
        int packetSize = LoRa.parsePacket();
        if (packetSize > 0) {
            String ackData = "";
            while (LoRa.available()) {
                ackData += (char)LoRa.read();
            }
            ackData.trim();
            int rssi = LoRa.packetRssi();
            int snr = LoRa.packetSnr();

            Serial.printf("[LORA ACK RECV] Raw (%d bytes): '%s', RSSI: %d, SNR: %d\n", packetSize, ackData.c_str(), rssi, snr);
            bool matched = handleParsedDownlink(ackData, rssi, snr, false);
            if (matched) {
                LoRa.receive();
                return true;
            }
            LoRa.receive();
        }
        delay(10);
    }

    lastAckReceived = false;
    Serial.println(F("[LORA ACK] Timeout - no ACK frame received from base station."));
    notifyBLEAckTimeout();
    LoRa.receive();
    return false;
}

bool transmitAlertWithAck(int alertIdx) {
    if (!loraInitialized) {
        Serial.println(F("[LORA] ERROR: Radio not initialized"));
        return false;
    }

    int idx = (alertIdx >= 0) ? alertIdx : selectedAlertIndex;
    char alertCode = getAlertCode(idx);
    char packet[32];
    snprintf(packet, sizeof(packet), "TX%03d,%c", DEVICE_ID, alertCode);

    Serial.printf("[LORA TX] Uplink Alert: '%s' (%s)\n", packet, alertNames[idx]);
    notifyBLEAlertSent(alertCode, alertNames[idx]);

    lastAckReceived = false;
    lastAckStatus = "";
    lastAckMessage = "";

    bool ackSuccess = false;
    for (int attempt = 1; attempt <= MAX_RETRY_ATTEMPTS; attempt++) {
        retryCount = attempt - 1;
        Serial.printf("[LORA TX] Attempt %d/%d...\n", attempt, MAX_RETRY_ATTEMPTS);

        LoRa.idle();
        delay(10);

        LoRa.beginPacket();
        LoRa.print(packet);
        bool txOk = LoRa.endPacket();

        totalTransmissions++;
        if (!txOk) {
            Serial.println(F("[LORA TX] Transmission buffer flush failed!"));
            delay(100);
            continue;
        }

        successfulTransmissions++;
        lastTransmitTime = millis();

        // Half-duplex turnaround: wait for Downlink ACK (5,000ms window)
        ackSuccess = waitForDownlinkACK(DOWNLINK_ACK_TIMEOUT_MS);
        if (ackSuccess) {
            Serial.printf("[LORA] Closed-loop ACK confirmed on attempt %d!\n", attempt);
            break;
        }

        if (attempt < MAX_RETRY_ATTEMPTS) {
            // Collision-avoidance backoff with randomized jitter
            unsigned long backoff = 400 + (attempt * 300) + random(100, 400);
            Serial.printf("[LORA] No ACK. Retrying in %lu ms...\n", backoff);
            delay(backoff);
        }
    }

    return ackSuccess;
}

bool transmitChatMessage(const String& message) {
    if (!loraInitialized) {
        Serial.println(F("[LORA] ERROR: Radio not initialized"));
        return false;
    }

    String cleanMsg = message;
    cleanMsg.replace(",", " "); // Avoid CSV delimiters
    if (cleanMsg.length() > 48) cleanMsg = cleanMsg.substring(0, 48);

    char packet[96];
    snprintf(packet, sizeof(packet), "TX%03d,CHAT,%s", DEVICE_ID, cleanMsg.c_str());

    Serial.printf("[LORA CHAT TX] '%s'\n", packet);

    LoRa.idle();
    delay(10);

    LoRa.beginPacket();
    LoRa.print(packet);
    bool txOk = LoRa.endPacket();

    totalTransmissions++;
    if (!txOk) return false;

    successfulTransmissions++;
    lastTransmitTime = millis();

    // Listen for ACK from base
    return waitForDownlinkACK(DOWNLINK_ACK_TIMEOUT_MS);
}

bool transmitAlert() {
    return transmitAlertWithAck(selectedAlertIndex);
}

bool transmitSPUTelemetry(const TelemetryPacket& pkt) {
    if (!loraInitialized) {
        Serial.println(F("[LORA] ERROR: Not initialized"));
        return false;
    }
    
    char payloadStr[96];
    snprintf(payloadStr, sizeof(payloadStr),
             "TX%03d,%c,%d,%u,%u,%ld,%ld,%d,%u,%u",
             pkt.node_id,
             pkt.emergency_code,
             pkt.temp_c_x10,
             pkt.humidity_x10,
             pkt.gas_ppm,
             (long)pkt.lat_deg_e7,
             (long)pkt.lon_deg_e7,
             pkt.alt_meters,
             pkt.health_score,
             pkt.risk_score);

    Serial.printf("[LORA SPU TX] Packet: '%s'\n", payloadStr);

    LoRa.idle();
    delay(10);
    
    LoRa.beginPacket();
    LoRa.print(payloadStr);
    bool success = LoRa.endPacket();
    
    delay(50);
    
    totalTransmissions++;
    if (success) {
        successfulTransmissions++;
        lastTransmitTime = millis();
    }
    
    Serial.printf("[LORA SPU TX] Result: %s\n", success ? "OK" : "FAILED");
    return success;
}
