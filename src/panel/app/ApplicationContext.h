#pragma once

#include <Arduino.h>
#include <Button.h>
#include <ESP_Knob.h>
#include <ESP_Panel_Library.h>

#include "AppController.h"
#include "input/InputCallbackRegistrar.h"
#include "input/InputRouter.h"
#include "shared/mesh/MeshRegistry.h"
#include "shared/mesh/PanelMeshEventBridge.h"

struct PanelApplicationContext {
    ESP_Panel *&panel;
    ESP_Knob *&knob;
    Button *&button;
    HardwareSerial &uart1;
    InputRouter &inputRouter;
    InputCallbackRegistrar &inputCallbacks;
    mesh::MeshRegistry &registry;
    mesh::PanelMeshEventBridge &meshBridge;
    mesh::EspNowNetwork *&network;
    AppController *&app;
};
