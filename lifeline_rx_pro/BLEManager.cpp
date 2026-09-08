#include "BLEManager.h"
#include <NimBLEDevice.h>

static NimBLEServer* pServer = nullptr;
static NimBLECharacteristic* pTxCharacteristic = nullptr;
static NimBLECharacteristic* pRxCharacteristic = nullptr;

static bool deviceConnected = false;
static bool oldDeviceConnected = false;

static bool hasPendingReplyFlag = false;
static int pendingReplyDevId = 0;
static String pendingReplyAction = "";
static String pendingReplyMsg = "";

static bool hasPendingEvacFlag = false;
static String pendingEvacMsg = "";

static void processIncomingBLECommand(const String& cmd) {
    if (cmd.startsWith("REPLY:") || cmd.startsWith("CMD:")) {
        // Format: REPLY:<devId>,<action>,<message>
        String payload = cmd.substring(cmd.indexOf(':') + 1);
        int c1 = payload.indexOf(',');
        if (c1 > 0) {
            pendingReplyDevId = payload.substring(0, c1).toInt();
            String rem = payload.substring(c1 + 1);
            int c2 = rem.indexOf(',');
            if (c2 > 0) {
                pendingReplyAction = rem.substring(0, c2);
                pendingReplyMsg = rem.substring(c2 + 1);
            } else {
                pendingReplyAction = "DISPATCHED";
                pendingReplyMsg = rem;
            }
            hasPendingReplyFlag = true;
            Serial.printf("[BLE CMD] Queued Reply to Dev #%d: Action=%s, Msg=%s\n",
                          pendingReplyDevId, pendingReplyAction.c_str(), pendingReplyMsg.c_str());
        }
    } else if (cmd.startsWith("EVAC:")) {
        pendingEvacMsg = cmd.substring(5);
        pendingEvacMsg.trim();
        hasPendingEvacFlag = true;
        Serial.printf("[BLE CMD] Queued Broadcast EVAC: '%s'\n", pendingEvacMsg.c_str());
    } else if (cmd.equalsIgnoreCase("STATUS") || cmd.equalsIgnoreCase("PING")) {
        notifyBLEStatus();
    } else {
        Serial.printf("[BLE CMD] Unknown Base command: '%s'\n", cmd.c_str());
    }
}

class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer) override {
        deviceConnected = true;
        Serial.println(F("[BLE BASE] Commander phone paired and connected!"));
    }

    void onDisconnect(NimBLEServer* pServer) override {
        deviceConnected = false;
        Serial.println(F("[BLE BASE] Commander phone disconnected. Advertising active."));
        NimBLEDevice::startAdvertising();
    }
};

class RxCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic) override {
        std::string rxValue = pCharacteristic->getValue();
        if (rxValue.length() > 0) {
            String val = String(rxValue.c_str());
            val.trim();
            Serial.printf("[BLE BASE RX] '%s'\n", val.c_str());
            processIncomingBLECommand(val);
        }
    }
};

void initBLE() {
    Serial.println(F("[BLE INIT] Starting LifeLine-RX-Base..."));
    NimBLEDevice::init("LifeLine-RX-Base");
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);

    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    NimBLEService* pService = pServer->createService(BLE_SERVICE_UUID);

    pTxCharacteristic = pService->createCharacteristic(
        BLE_CHAR_TX_UUID,
        NIMBLE_PROPERTY::NOTIFY
    );

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

    Serial.println(F("[BLE INIT] LifeLine-RX-Base BLE NUS advertising ready."));
}

void updateBLE() {
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
        Serial.printf("[BLE BASE TX] Sent: '%s'\n", data.c_str());
    }
}

void notifyBLEStatus() {
    char buf[128];
    snprintf(buf, sizeof(buf), "STATUS:ROLE=BASE,LORA=OK,VER=%s", FIRMWARE_VERSION);
    sendBLEString(String(buf));
}

void notifyBLEAlert(int devId, char code, const char* name, int rssi) {
    char buf[128];
    snprintf(buf, sizeof(buf), "ALERT:DEV=%03d,CODE=%c,NAME=%s,RSSI=%d", devId, code, name, rssi);
    sendBLEString(String(buf));
}

void notifyBLEChat(int devId, const char* text, int rssi) {
    char buf[160];
    snprintf(buf, sizeof(buf), "CHAT:DEV=%03d,TEXT=%s,RSSI=%d", devId, text, rssi);
    sendBLEString(String(buf));
}

void notifyBLETelemetry(int devId, float temp, float hum, double lat, double lon, int rssi) {
    char buf[160];
    snprintf(buf, sizeof(buf), "TELEMETRY:DEV=%03d,TEMP=%.1f,HUM=%.1f,LAT=%.6f,LON=%.6f,RSSI=%d",
             devId, temp, hum, lat, lon, rssi);
    sendBLEString(String(buf));
}

bool hasPendingBLEReply() {
    return hasPendingReplyFlag;
}

void getPendingBLEReply(int& devId, String& action, String& message) {
    hasPendingReplyFlag = false;
    devId = pendingReplyDevId;
    action = pendingReplyAction;
    message = pendingReplyMsg;
}

bool hasPendingBLEEvac() {
    return hasPendingEvacFlag;
}

String getPendingBLEEvacMessage() {
    hasPendingEvacFlag = false;
    return pendingEvacMsg;
}
