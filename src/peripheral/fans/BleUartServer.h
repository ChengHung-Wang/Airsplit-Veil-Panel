#pragma once

#include <Arduino.h>

class BLECharacteristic;

class BleUartServer {
public:
    class Listener {
    public:
        virtual ~Listener() = default;

        virtual void onBleCommand(const String &command) = 0;

        virtual void onBleConnectionChanged(bool connected) = 0;
    };

    explicit BleUartServer(Listener &listener);

    void begin(const char *deviceName);

    void poll(uint32_t nowMs);

    void notifyLine(const String &message);

    void handleConnectionChanged(bool connected);

    void handleIncomingCommand(const String &command);

private:
    void startAdvertising();

    Listener &listener_;
    BLECharacteristic *txCharacteristic_ = nullptr;
    uint32_t lastPollMs_ = 0;
    bool connected_ = false;
    bool started_ = false;
};
