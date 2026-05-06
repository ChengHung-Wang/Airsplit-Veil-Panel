#pragma once

#include <Arduino.h>

#include "shared/mesh/EspNowNetwork.h"

class LightNodeApp : public mesh::EspNowNetwork::Listener {
public:
    LightNodeApp(mesh::MeshRegistry &registry, gpio_num_t relayPin);

    void begin();
    void loop(uint32_t nowMs);

    void onMeshMessageReceived(const uint8_t mac[6], const mesh::MeshMessage &message) override;
    void onMeshSendComplete(const uint8_t mac[6], bool success) override;

private:
    void initRelayPin();
    void setEnabled(bool enabled);
    void sendStatus(uint32_t requestId);
    void announceIdentity();
    void notePanelLinked(const mesh::MeshMessage &message);
    bool sendToPanel(const mesh::MeshMessage &message);
    void logMesh(const char *direction, const mesh::MeshMessage &message);
    uint32_t nextRequestId();

    mesh::MeshRegistry &registry_;
    mesh::EspNowNetwork network_;
    gpio_num_t relayPin_;
    bool enabled_ = false;
    uint32_t nextRequestId_ = 1;
    bool panelLinked_ = false;
};
