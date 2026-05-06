#include "BleUartServer.h"

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

namespace {
    constexpr const char *kServiceUuid = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
    constexpr const char *kCharacteristicUuidRx = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";
    constexpr const char *kCharacteristicUuidTx = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";

    class ServerCallbacks : public BLEServerCallbacks {
    public:
        explicit ServerCallbacks(BleUartServer &server) : server_(server) {
        }

        void onConnect(BLEServer *) override {
            server_.notifyLine("BLE CONNECTED");
            server_.handleConnectionChanged(true);
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
    BLEDevice::init(deviceName);
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
    advertising->addServiceUUID(kServiceUuid);
    advertising->start();
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
