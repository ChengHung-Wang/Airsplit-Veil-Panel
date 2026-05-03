#pragma once

#include <ESP_Panel_Library.h>

#include "AppState.h"
#include "SettingsStore.h"
#include "input/InputEvent.h"
#include "ui/PanelView.h"

class AppController {
public:
    explicit AppController(ESP_Panel *panel);

    void begin();
    void handleEvent(const InputEvent &event, uint32_t nowMs);
    void update(uint32_t nowMs);
    void renderIfNeeded();

private:
    void handleKnobDelta(int delta, uint32_t nowMs);
    void handleSelectPress(uint32_t nowMs);
    void enterMode(AppMode mode, uint32_t nowMs);
    void setPower(bool powerOn, uint32_t nowMs);
    void toggleLightState();
    void cycleFanLevel();
    void startWindAdjustment(uint32_t nowMs);
    void confirmWindAdjustment(uint32_t nowMs);
    void cancelWindAdjustment();
    void markSettingsDirty(bool temperatureChanged, bool fanTimerChanged, uint32_t nowMs);
    void commitPendingSettings();
    void applyDisplayPower(bool powerOn);

    ESP_Panel *panel_ = nullptr;
    PanelView view_;
    SettingsStore store_;
    AppState state_;

    bool renderDirty_ = false;
    bool displayOffPending_ = false;
    bool temperatureSavePending_ = false;
    bool fanTimerSavePending_ = false;
    uint32_t lastSettingsChangeAtMs_ = 0;
    uint32_t lastWindTickAtMs_ = 0;
    uint32_t lastWindAdjustInputAtMs_ = 0;
    uint32_t lastBlinkToggleAtMs_ = 0;
    uint32_t zeroReachedAtMs_ = 0;
};
