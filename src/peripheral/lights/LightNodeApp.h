#pragma once

#include <Arduino.h>

#include "shared/peripheral/PeripheralStatusLed.h"
#include "shared/mesh/EspNowNetwork.h"

class LightNodeApp : public mesh::EspNowNetwork::Listener {
public:
    LightNodeApp(
        mesh::MeshRegistry &registry,
        gpio_num_t relayPin,
        gpio_num_t statusLedPin,
        gpio_num_t toggleButtonPin
    );

    void begin();

    void loop(uint32_t nowMs);

    void onMeshMessageReceived(const uint8_t mac[6], const mesh::MeshMessage &message) override;

    void onMeshSendComplete(const uint8_t mac[6], bool success) override;

private:
    void initRelayPin();

    void initToggleButtonPin();

    void updateToggleButton(uint32_t nowMs);

    void setEnabled(bool enabled);

    void sendStatus(uint32_t requestId);

    void announceIdentity();

    void notePanelLinked(const mesh::MeshMessage &message);

    bool sendToPanel(const mesh::MeshMessage &message);

    bool isPanelConnected() const;

    void logMesh(const char *direction, const mesh::MeshMessage &message);

    uint32_t nextRequestId();

    mesh::MeshRegistry &registry_;
    mesh::EspNowNetwork network_;
    PeripheralStatusLed statusLed_;
    gpio_num_t relayPin_;
    gpio_num_t toggleButtonPin_;
    bool enabled_ = false;
    uint32_t nextRequestId_ = 1;
    bool panelLinked_ = false;
    bool buttonStableLow_ = false;
    bool buttonLastSampleLow_ = false;
    uint32_t buttonLastChangedAtMs_ = 0;
};
