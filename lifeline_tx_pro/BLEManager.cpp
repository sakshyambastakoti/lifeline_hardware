#include "BLEManager.h"
#include <NimBLEDevice.h>

static NimBLEServer* pServer = nullptr;
static NimBLECharacteristic* pTxCharacteristic = nullptr;
static NimBLECharacteristic* pRxCharacteristic = nullptr;

static bool deviceConnected = false;
static bool oldDeviceConnected = false;

static bool hasPendingChat = false;
static String pendingChatMessage = "";
static bool hasPendingAlertFlag = false;
static char pendingAlertCodeChar = 0;

static void processIncomingBLECommand(const String& cmd) {
    if (cmd.startsWith("MSG:")) {
        pendingChatMessage = cmd.substring(4);
        pendingChatMessage.trim();
        hasPendingChat = true;
        Serial.printf("[BLE CMD] New Chat Message queued: '%s'\n", pendingChatMessage.c_str());
    } else if (cmd.startsWith("ALERT:")) {
        String codeStr = cmd.substring(6);
        codeStr.trim();
        if (codeStr.length() > 0) {
            pendingAlertCodeChar = codeStr[0];
            hasPendingAlertFlag = true;
            Serial.printf("[BLE CMD] New Alert queued: '%c'\n", pendingAlertCodeChar);
        }
    } else if (cmd.equalsIgnoreCase("STATUS") || cmd.equalsIgnoreCase("PING")) {
        notifyBLEStatus();
    } else {
        Serial.printf("[BLE CMD] Unrecognized command: '%s'\n", cmd.c_str());
    }
}

class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer) override {
        deviceConnected = true;
        Serial.println(F("[BLE] Mobile client paired and connected!"));
    }

    void onDisconnect(NimBLEServer* pServer) override {
        deviceConnected = false;
        Serial.println(F("[BLE] Mobile client disconnected. Restarting advertising..."));
        NimBLEDevice::startAdvertising();
    }
};

class RxCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic) override {
        std::string rxValue = pCharacteristic->getValue();
        if (rxValue.length() > 0) {
            String val = String(rxValue.c_str());
            val.trim();
            Serial.printf("[BLE RAW RX] '%s'\n", val.c_str());
            processIncomingBLECommand(val);
        }
    }
};

void initBLE() {
    char devName[32];
    snprintf(devName, sizeof(devName), "LifeLine-TX-%03d", DEVICE_ID);

    Serial.printf("[BLE INIT] Initializing NimBLE: %s\n", devName);
    NimBLEDevice::init(devName);
    NimBLEDevice::setPower(ESP_PWR_LVL_P9); // High RF output power for mountain range

    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    NimBLEService* pService = pServer->createService(BLE_SERVICE_UUID);

    // TX Characteristic: LifeLine TX -> Smartphone (Notify)
    pTxCharacteristic = pService->createCharacteristic(
        BLE_CHAR_TX_UUID,
        NIMBLE_PROPERTY::NOTIFY
    );

    // RX Characteristic: Smartphone -> LifeLine TX (Write / Write Without Response)
    pRxCharacteristic = pService->createCharacteristic(
        BLE_CHAR_RX_UUID,
        NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
    );
    pRxCharacteristic->setCallbacks(new RxCallbacks());

    pService->start();

    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(BLE_SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->start();

    Serial.println(F("[BLE INIT] BLE Nordic UART Service advertising active."));
}

void updateBLE() {
    // Handle disconnect re-advertising if needed
    if (!deviceConnected && oldDeviceConnected) {
        delay(10);
        oldDeviceConnected = deviceConnected;
    }
    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
        notifyBLEStatus();
    }
}

bool isBLEConnected() {
    return deviceConnected;
}

void sendBLEString(const String& data) {
    if (deviceConnected && pTxCharacteristic != nullptr) {
        pTxCharacteristic->setValue(data.c_str());
        pTxCharacteristic->notify();
        Serial.printf("[BLE TX] Sent: '%s'\n", data.c_str());
    }
}

void notifyBLEStatus() {
    char buf[128];
    snprintf(buf, sizeof(buf), "STATUS:DEV=%03d,BAT=%d,LORA=%s,VER=%s",
             DEVICE_ID, batteryPercent, loraInitialized ? "OK" : "ERR", FIRMWARE_VERSION);
    sendBLEString(String(buf));
}

void notifyBLEAlertSent(char code, const char* name) {
    char buf[96];
    snprintf(buf, sizeof(buf), "ALERT_SENT:CODE=%c,NAME=%s,TIME=%lu", code, name, millis());
    sendBLEString(String(buf));
}

void notifyBLEAck(const char* status, const char* baseId, const char* note, int rssi, int snr) {
    char buf[160];
    snprintf(buf, sizeof(buf), "ACK_RECV:STATUS=%s,BASE=%s,NOTE=%s,RSSI=%d,SNR=%d",
             status, baseId, note, rssi, snr);
    sendBLEString(String(buf));
}

void notifyBLEAckTimeout() {
    sendBLEString("ACK_TIMEOUT:NO_BASE_CONFIRMATION");
}

bool hasPendingBLEChatMessage() {
    return hasPendingChat;
}

String getPendingBLEChatMessage() {
    hasPendingChat = false;
    return pendingChatMessage;
}

bool hasPendingBLEAlert() {
    return hasPendingAlertFlag;
}

char getPendingBLEAlert() {
    hasPendingAlertFlag = false;
    return pendingAlertCodeChar;
}
