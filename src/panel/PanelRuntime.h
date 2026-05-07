#pragma once

#include <Arduino.h>
#include <Button.h>
#include <ESP_Knob.h>
#include <ESP_Panel_Library.h>

#include "app/AppController.h"
#include "input/InputRouter.h"
#include "shared/mesh/EspNowNetwork.h"
#include "shared/mesh/MeshRegistry.h"

class PanelRuntime {
public:
    PanelRuntime();

    void setup();

    void loop();

private:
    class MeshListener : public mesh::EspNowNetwork::Listener {
    public:
        explicit MeshListener(PanelRuntime &runtime);

        void onMeshMessageReceived(const uint8_t mac[6], const mesh::MeshMessage &message) override;

        void onMeshSendComplete(const uint8_t mac[6], bool success) override;

    private:
        PanelRuntime &runtime_;
    };

    void initializeDisplayPowerPin();

    void initializeSerial();

    void initializePanel();

    void initializeKnob();

    void initializeButton();

    void initializeLvgl();

    void initializeMesh();

    void initializeApplication();

    void processInput(uint32_t nowMs);

    void onMeshMessageReceived(const uint8_t mac[6], const mesh::MeshMessage &message, uint32_t nowMs);

    void onMeshSendComplete(const uint8_t mac[6], bool success);

    void onKnobLeft();

    void onKnobRight();

    void onButtonSingleClick();

    void onButtonLongPressStart();

    static void handleKnobLeftEvent(int count, void *usrData);

    static void handleKnobRightEvent(int count, void *usrData);

    static void handleButtonSingleClick(void *buttonHandle, void *usrData);

    static void handleButtonLongPressStart(void *buttonHandle, void *usrData);

    static PanelRuntime *instance_;

    static constexpr uint32_t kUartCommandBaudRate = 115200;
    static constexpr int kUart1RxPin = 44;
    static constexpr int kUart1TxPin = 43;

    ESP_Panel *panel_ = nullptr;
    ESP_Knob *knob_ = nullptr;
    Button *button_ = nullptr;
    HardwareSerial uart1_{1};
    InputRouter inputRouter_;
    mesh::MeshRegistry registry_;
    MeshListener meshListener_;
    mesh::EspNowNetwork network_;
    AppController *app_ = nullptr;
    bool initialized_ = false;
};
