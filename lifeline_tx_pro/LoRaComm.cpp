#include "LoRaComm.h"
#include "BLEManager.h"

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
        LoRa.enableCrc();
        loraInitialized = true;
        Serial.printf("[INIT] LoRa OK @ %.1f MHz, SF%d, BW125kHz, CRC enabled\n", LORA_FREQUENCY / 1E6, LORA_SF);
    } else {
        loraInitialized = false;
        Serial.println(F("[INIT] LoRa FAILED!"));
    }
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
            lastAckRssi = LoRa.packetRssi();
            lastAckSnr = LoRa.packetSnr();

            Serial.printf("[LORA ACK RECV] Raw (%d bytes): '%s', RSSI: %d, SNR: %d\n",
                          packetSize, ackData.c_str(), lastAckRssi, lastAckSnr);

            // Check if this is an ACK, CMD, or EVAC frame
            // Expected formats:
            // 1) ACK003,A,LOGGED,BASE01,Alert recorded
            // 2) CMD003,DISPATCHED,Rescue squad dispatched ETA 20m
            // 3) EVAC,ALL,Flash flood warning evacuate immediately
            if (ackData.startsWith("ACK") || ackData.startsWith("CMD")) {
                int firstComma = ackData.indexOf(',');
                if (firstComma > 3) {
                    int targetNode = ackData.substring(3, firstComma).toInt();
                    if (targetNode == DEVICE_ID || targetNode == 0) {
                        // Extract remaining tokens
                        String remainder = ackData.substring(firstComma + 1);
                        int secondComma = remainder.indexOf(',');

                        if (ackData.startsWith("ACK")) {
                            // Format: <CODE>,<STATUS>,<BASE_ID>,<NOTE>
                            if (secondComma > 0) {
                                String code = remainder.substring(0, secondComma);
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
                                    lastAckBaseId = "BASE";
                                    lastAckMessage = "Received";
                                }
                            } else {
                                lastAckStatus = remainder;
                                lastAckBaseId = "BASE";
                                lastAckMessage = "Acknowledged";
                            }
                        } else {
                            // CMD: <ACTION>,<MESSAGE>
                            if (secondComma > 0) {
                                lastAckStatus = remainder.substring(0, secondComma);
                                lastAckMessage = remainder.substring(secondComma + 1);
                                lastAckBaseId = "COMMAND";
                            } else {
                                lastAckStatus = remainder;
                                lastAckMessage = "Command Received";
                                lastAckBaseId = "COMMAND";
                            }
                        }

                        lastAckReceived = true;
                        Serial.printf("[LORA ACK MATCH] Status: %s, Base: %s, Msg: %s\n",
                                      lastAckStatus.c_str(), lastAckBaseId.c_str(), lastAckMessage.c_str());

                        // Forward to connected phone via BLE
                        notifyBLEAck(lastAckStatus.c_str(), lastAckBaseId.c_str(), lastAckMessage.c_str(), lastAckRssi, lastAckSnr);
                        return true;
                    }
                }
            } else if (ackData.startsWith("EVAC")) {
                lastAckStatus = "EVACUATE";
                lastAckBaseId = "EMERGENCY_BROADCAST";
                int comma = ackData.indexOf(',', 5);
                lastAckMessage = (comma > 0) ? ackData.substring(comma + 1) : "Move to high ground";
                lastAckReceived = true;
                notifyBLEAck(lastAckStatus.c_str(), lastAckBaseId.c_str(), lastAckMessage.c_str(), lastAckRssi, lastAckSnr);
                return true;
            }
        }
        delay(10);
    }

    lastAckReceived = false;
    Serial.println(F("[LORA ACK] Timeout - no ACK frame received from base station."));
    notifyBLEAckTimeout();
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

        // Half-duplex turnaround: wait for Downlink ACK (2,500ms window)
        ackSuccess = waitForDownlinkACK(2500);
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
    return waitForDownlinkACK(2500);
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
