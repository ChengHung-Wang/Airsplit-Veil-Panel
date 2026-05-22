#include "BleUartServer.h"

#include <Arduino.h>
#include <BLEAdvertising.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

namespace {
    constexpr const char *kServiceUuid = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
    constexpr const char *kCharacteristicUuidRx = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";
    constexpr const char *kCharacteristicUuidTx = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";
    constexpr const char *kAdvertisedName = "AirSplit-Fans";

    class ServerCallbacks : public BLEServerCallbacks {
    public:
        explicit ServerCallbacks(BleUartServer &server) : server_(server) {
        }

        void onConnect(BLEServer *) override {
            server_.handleConnectionChanged(true);
            server_.notifyLine("BLE CONNECTED");
        }

        void onDisconnect(BLEServer *) override {
            server_.handleConnectionChanged(false);
            BLEDevice::startAdvertising();
        }

    private:
        BleUartServer &server_;
    };

    class RxCallbacks : public BLECharacteristicCallbacks {
    public:
        explicit RxCallbacks(BleUartServer &server) : server_(server) {
        }

        void onWrite(BLECharacteristic *characteristic) override {
            server_.handleIncomingCommand(String(characteristic->getValue().c_str()));
        }

    private:
        BleUartServer &server_;
    };
} // namespace

BleUartServer::BleUartServer(Listener &listener) : listener_(listener) {
}

void BleUartServer::begin(const char *deviceName) {
    if (!BLEDevice::init(deviceName)) {
        Serial.println("ERR BLE init failed");
        return;
    }
    Serial.println("BLE init ok");
    BLEServer *server = BLEDevice::createServer();
    server->setCallbacks(new ServerCallbacks(*this));

    BLEService *service = server->createService(kServiceUuid);
    txCharacteristic_ = service->createCharacteristic(
        kCharacteristicUuidTx,
        BLECharacteristic::PROPERTY_NOTIFY
    );
    BLECharacteristic *rxCharacteristic = service->createCharacteristic(
        kCharacteristicUuidRx,
        BLECharacteristic::PROPERTY_WRITE
    );
    rxCharacteristic->setCallbacks(new RxCallbacks(*this));

    service->start();
    BLEAdvertising *advertising = BLEDevice::getAdvertising();
    BLEAdvertisementData advertisementData;
    BLEAdvertisementData scanResponseData;
    advertisementData.setFlags(0x06);
    advertisementData.setName(kAdvertisedName);
    scanResponseData.setName(deviceName);
    scanResponseData.setCompleteServices(BLEUUID(kServiceUuid));
    advertising->addServiceUUID(kServiceUuid);
    const bool advOk = advertising->setAdvertisementData(advertisementData);
    const bool scanRspOk = advertising->setScanResponseData(scanResponseData);
    advertising->setScanResponse(true);
    advertising->setMinPreferred(0x06);
    advertising->setMaxPreferred(0x12);
    startAdvertising();
    started_ = advOk && scanRspOk;
    Serial.printf("BLE adv_data=%d scan_rsp=%d advertising=%d\r\n", advOk ? 1 : 0, scanRspOk ? 1 : 0, advertising->isAdvertising() ? 1 : 0);
}

void BleUartServer::poll(uint32_t nowMs) {
    if (!started_ || connected_ || ((nowMs - lastPollMs_) < 1000U)) {
        return;
    }
    lastPollMs_ = nowMs;
    BLEAdvertising *advertising = BLEDevice::getAdvertising();
    if ((advertising != nullptr) && !advertising->isAdvertising()) {
        Serial.println("BLE advertising restart");
        startAdvertising();
    }
}

void BleUartServer::notifyLine(const String &message) {
    if (!connected_ || (txCharacteristic_ == nullptr)) {
        return;
    }
    txCharacteristic_->setValue((message + "\n").c_str());
    txCharacteristic_->notify();
}

void BleUartServer::handleConnectionChanged(bool connected) {
    connected_ = connected;
    listener_.onBleConnectionChanged(connected);
}

void BleUartServer::handleIncomingCommand(const String &command) {
    listener_.onBleCommand(command);
}

void BleUartServer::startAdvertising() {
    BLEDevice::startAdvertising();
}
