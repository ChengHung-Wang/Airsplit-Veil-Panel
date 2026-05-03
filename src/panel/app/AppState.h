#pragma once

#include <Arduino.h>

enum class AppMode : uint8_t {
    Idle,
    Light,
    Water,
    Wind,
};

enum class LightState : uint8_t {
    Primary,
    Secondary,
};

struct AppState {
    static constexpr int kMinTemperature = 20;
    static constexpr int kMaxTemperature = 45;
    static constexpr uint32_t kMinFanTimerSeconds = 15;
    static constexpr uint32_t kDefaultFanTimerSeconds = 15 * 60;

    bool powerOn = true;
    AppMode currentMode = AppMode::Idle;
    LightState lightState = LightState::Primary;
    int temperature = 25;
    uint32_t fanTimerSeconds = kDefaultFanTimerSeconds;
    uint32_t fanRemainingSeconds = kDefaultFanTimerSeconds;
    uint32_t windAdjustCandidateSeconds = kDefaultFanTimerSeconds;
    uint8_t fanLevel = 1;
    bool windAdjusting = false;
    bool windAdjustmentBlinkOn = true;
};
