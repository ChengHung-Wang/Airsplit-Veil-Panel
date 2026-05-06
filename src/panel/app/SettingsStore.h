#pragma once

#include <Preferences.h>

class SettingsStore {
public:
    void begin();

    void end();

    int loadTemperature(int fallbackValue) const;

    uint32_t loadFanTimer(uint32_t fallbackValue) const;

    uint8_t loadFanLevel(uint8_t fallbackValue) const;

    bool loadLightEnabled(bool fallbackValue) const;

    void saveTemperature(int value);

    void saveFanTimer(uint32_t value);

    void saveFanLevel(uint8_t value);

    void saveLightEnabled(bool value);

private:
    static constexpr const char *kNamespace = "panel";
    static constexpr const char *kTemperatureKey = "temperature";
    static constexpr const char *kFanTimerKey = "fanTimer";
    static constexpr const char *kFanLevelKey = "fanLevel";
    static constexpr const char *kLightEnabledKey = "lightEnabled";

    mutable Preferences preferences_;
    bool opened_ = false;
};
