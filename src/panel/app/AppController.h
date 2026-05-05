#pragma once

#include <ESP_Panel_Library.h>

#include "AppState.h"
#include "SettingsStore.h"
#include "input/InputEvent.h"
#include "ui/PanelView.h"
#include "shared/mesh/EspNowNetwork.h"

class AppController {
public:
    AppController(
        ESP_Panel *panel,
        mesh::MeshRegistry &registry,
        mesh::EspNowNetwork &network,
        mesh::NodeRole selfRole,
        uint8_t selfId
    );

    void begin();
    void handleEvent(const InputEvent &event, uint32_t nowMs);
    void handleMeshMessage(const uint8_t mac[6], const mesh::MeshMessage &message, uint32_t nowMs);
    void handleMeshSendComplete(const uint8_t mac[6], bool success);
    void update(uint32_t nowMs);
    void renderIfNeeded();
    void printStatus(Print &out, uint32_t nowMs);

private:
    void handleKnobDelta(int delta, uint32_t nowMs);
    void handleSelectPress(uint32_t nowMs);
    void enterMode(AppMode mode, uint32_t nowMs);
    void setPower(bool powerOn, uint32_t nowMs);
    void toggleLightState(uint32_t nowMs);
    int windAdjustmentDeltaForInput(int knobDirection, uint32_t nowMs);
    void startWindAdjustment(uint32_t nowMs);
    void confirmWindAdjustment(uint32_t nowMs);
    void cancelWindAdjustment();
    void cycleFanLevel(uint32_t nowMs); // fan level in wind mode
    void markSettingsDirty(
        bool temperatureChanged,
        bool fanTimerChanged,
        bool fanLevelChanged,
        bool lightChanged,
        uint32_t nowMs
    );
    void commitPendingSettings();
    void applyDisplayPower(bool powerOn);
    void syncOutputs(uint32_t nowMs);
    void sendFansCommand(bool enable, uint8_t fan1Percent, uint8_t fan2Percent, uint32_t nowMs);
    void sendLightCommand(bool enabled, uint32_t nowMs);
    void requestPeripheralStatus(uint32_t nowMs);
    void announceIdentity();
    void logMesh(const char *direction, const mesh::MeshMessage &message);
    bool shouldAcceptRemoteKey(uint32_t nowMs);
    void storePeripheralMessage(const uint8_t mac[6], const mesh::MeshMessage &message, uint32_t nowMs);
    String summarizePeripheral(const mesh::RegistryEntry &entry) const;
    uint32_t nextRequestId();

    ESP_Panel *panel_ = nullptr;
    mesh::MeshRegistry &registry_;
    mesh::EspNowNetwork &network_;
    mesh::NodeRole selfRole_ = mesh::NodeRole::Unknown;
    uint8_t selfId_ = 0;
    PanelView view_;
    SettingsStore store_;
    AppState state_;

    struct PeripheralSnapshot {
        bool seen = false;
        mesh::MeshMessage lastMessage = {};
        uint32_t updatedAtMs = 0;
    };

    bool renderDirty_ = false;
    bool displayOffPending_ = false;
    bool temperatureSavePending_ = false;
    bool fanTimerSavePending_ = false;
    bool fanLevelSavePending_ = false;
    bool lightSavePending_ = false;
    uint8_t savedFanLevel_ = 1;
    uint32_t lastSettingsChangeAtMs_ = 0;
    uint32_t lastWindTickAtMs_ = 0;
    uint32_t lastWindAdjustInputAtMs_ = 0;
    uint32_t lastWindAdjustStepAtMs_ = 0;
    uint32_t lastBlinkToggleAtMs_ = 0;
    uint32_t zeroReachedAtMs_ = 0;
    uint32_t lastStatusBroadcastAtMs_ = 0;
    uint32_t lastAcceptedRemoteKeyAtMs_ = 0;
    uint32_t nextRequestId_ = 1;
    PeripheralSnapshot snapshots_[mesh::kRegistrySeedCount] = {};
};
