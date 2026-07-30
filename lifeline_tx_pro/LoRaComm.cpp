#include "LoRaComm.h"

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

bool transmitAlert() {
    if (!loraInitialized) {
        Serial.println(F("[LORA] ERROR: Not initialized"));
        return false;
    }
    
    char alertCode = getAlertCode(selectedAlertIndex);
    char packet[16];
    sprintf(packet, "TX%03d,%c", DEVICE_ID, alertCode);
    
    Serial.printf("[LORA] TX: %s (%s)\n", packet, alertNames[selectedAlertIndex]);
    
    // Ensure LoRa is in idle state before transmitting
    LoRa.idle();
    delay(10);
    
    LoRa.beginPacket();
    LoRa.print(packet);
    bool success = LoRa.endPacket();  // Synchronous mode - wait for TX complete
    
    // Small delay to ensure radio returns to idle
    delay(50);
    
    totalTransmissions++;
    if (success) {
        successfulTransmissions++;
        lastTransmitTime = millis();
    }
    
    Serial.printf("[LORA] Result: %s\n", success ? "OK" : "FAILED");
    return success;
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

