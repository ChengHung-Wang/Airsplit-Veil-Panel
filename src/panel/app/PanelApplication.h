#pragma once

#include <Arduino.h>
#include <Button.h>
#include <ESP_Knob.h>
#include <ESP_Panel_Library.h>

#include "ApplicationContext.h"
#include "providers/AppServiceProvider.h"
#include "providers/DisplayServiceProvider.h"
#include "providers/InputServiceProvider.h"
#include "providers/MeshServiceProvider.h"

class PanelApplication {
public:
    PanelApplication();

    void boot();

    void runOnce();

private:
    void processInput(uint32_t nowMs);

    static constexpr uint32_t kUartCommandBaudRate = 115200;
    static constexpr int kUart1RxPin = 44;
    static constexpr int kUart1TxPin = 43;

    ESP_Panel *panel_ = nullptr;
    ESP_Knob *knob_ = nullptr;
    Button *button_ = nullptr;
    HardwareSerial uart1_{1};
    InputRouter inputRouter_;
    InputCallbackRegistrar inputCallbacks_;
    mesh::MeshRegistry registry_;
    mesh::PanelMeshEventBridge meshBridge_;
    mesh::EspNowNetwork *network_ = nullptr;
    AppController *app_ = nullptr;
    bool booted_ = false;

    PanelApplicationContext context_;
    InputServiceProvider inputProvider_;
    DisplayServiceProvider displayProvider_;
    MeshServiceProvider meshProvider_;
    AppServiceProvider appProvider_;
};
