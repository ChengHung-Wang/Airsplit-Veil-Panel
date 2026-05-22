#pragma once

#include "BleUartServer.h"
#include "FanController.h"
#include "RelayController.h"
#include "shared/mesh/EspNowNetwork.h"
#include "shared/peripheral/PeripheralStatusLed.h"

class FansNodeApp : public mesh::EspNowNetwork::Listener, public BleUartServer::Listener {
public:
    FansNodeApp(
        FanController &fan1,
        FanController &fan2,
        RelayController &relay,
        mesh::MeshRegistry &registry,
        gpio_num_t statusLedPin = GPIO_NUM_NC
    );

    void begin();

    void loop(uint32_t nowMs);

    void onFan1Pulse();

    void onFan2Pulse();

    void onMeshMessageReceived(const uint8_t mac[6], const mesh::MeshMessage &message) override;

    void onMeshSendComplete(const uint8_t mac[6], bool success) override;

    void onBleCommand(const String &command) override;

    void onBleConnectionChanged(bool connected) override;

private:
    struct PendingMeshMessage {
        uint8_t mac[6] = {};
        mesh::MeshMessage message = {};
    };

    void processCommand(String command, bool fromMesh);

    void applyFansState(bool relayEnabled, int fan1Percent, int fan2Percent);

    void handleSerial();

    void processPendingMeshMessages();

    void handleRpmUpdate(uint32_t nowMs);

    void ensureNetworkStarted(uint32_t nowMs);

    void sendStatus(uint32_t requestId);

    void announceIdentity();

    void notePanelLinked(const mesh::MeshMessage &message);

    bool sendToPanel(const mesh::MeshMessage &message);

    bool isPanelConnected() const;

    bool hasActiveFanOutput() const;

    void logMesh(const char *direction, const mesh::MeshMessage &message, bool mirrorToBle);

    String statusString() const;

    void printHelp();

    uint32_t nextRequestId();

    FanController &fan1_;
    FanController &fan2_;
    RelayController &relay_;
    mesh::MeshRegistry &registry_;
    BleUartServer ble_;
    mesh::EspNowNetwork network_;
    PeripheralStatusLed statusLed_;
    static constexpr uint8_t kPendingMeshCapacity = 8;
    PendingMeshMessage pendingMesh_[kPendingMeshCapacity];
    volatile uint8_t pendingMeshHead_ = 0;
    volatile uint8_t pendingMeshTail_ = 0;
    char serialBuffer_[64] = {};
    size_t serialLength_ = 0;
    uint32_t lastRpmMs_ = 0;
    uint32_t bootMs_ = 0;
    uint32_t nextRequestId_ = 1;
    bool networkStarted_ = false;
    bool panelLinked_ = false;
};
