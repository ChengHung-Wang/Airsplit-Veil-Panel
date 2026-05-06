#pragma once

#include <Arduino.h>
#include <Button.h>

#include "shared/mesh/EspNowNetwork.h"

class KeyNodeApp : public mesh::EspNowNetwork::Listener {
public:
    KeyNodeApp(
        mesh::MeshRegistry &registry,
        gpio_num_t powerPin,
        gpio_num_t waterPin,
        gpio_num_t lightPin,
        gpio_num_t windPin,
        bool activeLevel
    );

    void begin();

    void loop(uint32_t nowMs);

    void onMeshMessageReceived(const uint8_t mac[6], const mesh::MeshMessage &message) override;

    void onMeshSendComplete(const uint8_t mac[6], bool success) override;

private:
    static void onPowerClick(void *buttonHandle, void *userData);

    static void onPowerLongPress(void *buttonHandle, void *userData);

    static void onWaterClick(void *buttonHandle, void *userData);

    static void onLightClick(void *buttonHandle, void *userData);

    static void onWindClick(void *buttonHandle, void *userData);

    void emitKey(mesh::KeyCode key, mesh::KeyPressType press);

    void announceIdentity();

    void notePanelLinked(const mesh::MeshMessage &message);

    bool sendToPanel(const mesh::MeshMessage &message);

    void logMesh(const char *direction, const mesh::MeshMessage &message);

    uint32_t nextRequestId();

    mesh::MeshRegistry &registry_;
    mesh::EspNowNetwork network_;
    Button *powerButton_ = nullptr;
    Button *waterButton_ = nullptr;
    Button *lightButton_ = nullptr;
    Button *windButton_ = nullptr;
    gpio_num_t powerPin_ = GPIO_NUM_NC;
    gpio_num_t waterPin_ = GPIO_NUM_NC;
    gpio_num_t lightPin_ = GPIO_NUM_NC;
    gpio_num_t windPin_ = GPIO_NUM_NC;
    bool activeLevel_ = false;
    uint32_t nextRequestId_ = 1;
    bool panelLinked_ = false;
};
