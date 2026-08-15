#include "ble_service.h"
#include "alert_dispatcher.h"
#include "fall_detection/fall_detector.h"

#include <NimBLEDevice.h>

// Packet layouts are documented in BLE_PROTOCOL.md at the repository root.
// Keep the two in step: the companion app parses these byte offsets directly.

static NimBLEServer*         server      = nullptr;
static NimBLECharacteristic* chVitals    = nullptr;
static NimBLECharacteristic* chFall      = nullptr;
static NimBLECharacteristic* chStatus    = nullptr;

static volatile uint8_t pendingCommand = 0;
static unsigned long lastNotify = 0;
static unsigned long lastConnectedMs = 0;
static bool leashAlertSent = false;

class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* s) override {
        g_watchState.bleConnected = true;
        lastConnectedMs = millis();
        leashAlertSent = false;
        Serial.println(" [BLE] Phone connected.");
    }
    void onDisconnect(NimBLEServer* s) override {
        g_watchState.bleConnected = false;
        Serial.println(" [BLE] Phone disconnected, advertising again.");
        NimBLEDevice::startAdvertising();
    }
};

class CommandCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* c) override {
        std::string v = c->getValue();
        if (v.empty()) return;
        // Hand the command to the main loop rather than mutating shared state
        // from the BLE host task.
        pendingCommand = (uint8_t)v[0];
    }
};

void initBLEService() {
    NimBLEDevice::init(BLE_DEVICE_NAME);
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);
    // Disable mandatory security bonding keys so phones can reconnect instantly without needing to 'Forget Device'
    NimBLEDevice::setSecurityAuth(false, false, false);
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);

    server = NimBLEDevice::createServer();
    server->setCallbacks(new ServerCallbacks());

    NimBLEService* svc = server->createService(BLE_SERVICE_UUID);

    chVitals = svc->createCharacteristic(BLE_CHAR_VITALS_UUID, NIMBLE_PROPERTY::NOTIFY);
    chFall   = svc->createCharacteristic(BLE_CHAR_FALL_UUID,   NIMBLE_PROPERTY::INDICATE);
    chStatus = svc->createCharacteristic(BLE_CHAR_STATUS_UUID,
                                         NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);

    NimBLECharacteristic* chCommand =
        svc->createCharacteristic(BLE_CHAR_COMMAND_UUID, NIMBLE_PROPERTY::WRITE);
    chCommand->setCallbacks(new CommandCallbacks());

    svc->start();

    NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
    adv->addServiceUUID(BLE_SERVICE_UUID);
    adv->setMinInterval(0x20); // 20ms fast advertising for instant phone pairing
    adv->setMaxInterval(0x40); // 40ms
    adv->setScanResponse(true);
    adv->start();

    lastConnectedMs = millis();
    Serial.println(" -> SUCCESS: BLE GATT Server Active (advertising as " BLE_DEVICE_NAME ").");
}

void bleNotifyFallEvent(uint8_t eventType) {
    if (!chFall || !g_watchState.bleConnected) return;

    uint8_t p[12];
    p[0]  = eventType;
    p[1]  = (uint8_t)g_watchState.fallState;
    p[2]  = (uint8_t)g_watchState.hour;
    p[3]  = (uint8_t)g_watchState.minute;
    p[4]  = (uint8_t)g_watchState.second;
    p[5]  = (uint8_t)g_watchState.day;
    p[6]  = (uint8_t)(g_watchState.month + 1);
    p[7]  = (uint8_t)(g_watchState.year & 0xFF);
    p[8]  = (uint8_t)(g_watchState.year >> 8);
    p[9]  = (uint8_t)(g_watchState.heartRateBPM & 0xFF);
    p[10] = (uint8_t)(g_watchState.heartRateBPM >> 8);
    p[11] = g_watchState.batteryPercent;

    chFall->setValue(p, sizeof(p));
    chFall->indicate();
}

void updateBLEService() {
    if (!g_watchState.bleConnected && server && !server->getAdvertising()->isAdvertising()) {
        server->getAdvertising()->start();
    }
    // Commands arrive on the BLE host task; act on them here.
    uint8_t cmd = pendingCommand;
    if (cmd != 0) {
        pendingCommand = 0;
        switch (cmd) {
            case 0x01:
                if (g_watchState.fallState != FALL_STATE_NORMAL) {
                    Serial.println(" [BLE] Cancel command from phone.");
                    cancelFallAlert();
                }
                break;
            case 0x02:
                Serial.println(" [BLE] SOS command from phone.");
                triggerSimulatedFall();
                break;
            default:
                Serial.printf(" [BLE] Unknown command 0x%02X ignored.\n", cmd);
                break;
        }
    }

    if (g_watchState.bleConnected) {
        lastConnectedMs = millis();
    } else if (!leashAlertSent && millis() - lastConnectedMs > BLE_LEASH_TIMEOUT_MS) {
        // The watch and the phone have drifted apart. While Wi-Fi is still up
        // the watch can say so itself; once both links are gone it has no way
        // to reach anyone, and the phone side has to raise the alarm.
        leashAlertSent = true;
        Serial.println(" [BLE] Phone out of range beyond leash timeout.");
        queueAlertMessage("⚠️ CẢNH BÁO MẤT KẾT NỐI\nĐồng hồ đã mất kết nối Bluetooth với điện thoại quá lâu. Người đeo có thể đã đi ra xa.");
    }

    if (millis() - lastNotify < 2000) return;
    lastNotify = millis();
    if (!g_watchState.bleConnected) return;

    uint8_t vitals[8] = {0};
    vitals[0] = (g_watchState.skinContact    ? 0x01 : 0) |
                (g_watchState.hrValid        ? 0x02 : 0) |
                (g_watchState.spo2Valid      ? 0x04 : 0) |
                (g_watchState.motionArtifact ? 0x08 : 0);
    vitals[1] = (uint8_t)(g_watchState.heartRateBPM & 0xFF);
    vitals[2] = (uint8_t)(g_watchState.heartRateBPM >> 8);
    vitals[3] = g_watchState.spo2Percent;
    vitals[4] = g_watchState.signalQuality;
    vitals[5] = g_watchState.batteryPercent;
    vitals[6] = g_watchState.isCharging ? 1 : 0;
    chVitals->setValue(vitals, sizeof(vitals));
    chVitals->notify();

    uint8_t status[4];
    status[0] = (g_watchState.imuOk      ? 0x01 : 0) |
                (g_watchState.hrSensorOk ? 0x02 : 0) |
                (g_watchState.touchOk    ? 0x04 : 0);
    status[1] = g_watchState.wifiConnected ? 0x01 : 0;
    status[2] = g_watchState.queuedAlerts;
    status[3] = g_watchState.batteryPercent;
    chStatus->setValue(status, sizeof(status));
    chStatus->notify();
}
