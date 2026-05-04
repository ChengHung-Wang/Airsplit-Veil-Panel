#pragma once

#include "BleUartServer.h"
#include "FanController.h"
#include "RelayController.h"
#include "shared/mesh/EspNowNetwork.h"

class FansNodeApp : public mesh::EspNowNetwork::Listener, public BleUartServer::Listener {
public:
    FansNodeApp(
        FanController &fan1,
        FanController &fan2,
        RelayController &relay,
        mesh::MeshRegistry &registry
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
    void processCommand(String command, bool fromMesh);
    void applyFansState(bool relayEnabled, int fan1Percent, int fan2Percent);
    void handleSerial();
    void handleRpmUpdate(uint32_t nowMs);
    void sendStatus(uint32_t requestId);
    void announceIdentity();
    void notePanelLinked(const mesh::MeshMessage &message);
    bool sendToPanel(const mesh::MeshMessage &message);
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
    uint32_t lastRpmMs_ = 0;
    uint32_t nextRequestId_ = 1;
    bool panelLinked_ = false;
};
