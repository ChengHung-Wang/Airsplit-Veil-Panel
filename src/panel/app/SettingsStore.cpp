#include "app/SettingsStore.h"

void SettingsStore::begin()
{
    if (!opened_) {
        opened_ = preferences_.begin(kNamespace, false);
    }
}

void SettingsStore::end()
{
    if (opened_) {
        preferences_.end();
        opened_ = false;
    }
}

int SettingsStore::loadTemperature(int fallbackValue) const
{
    return opened_ ? preferences_.getInt(kTemperatureKey, fallbackValue) : fallbackValue;
}

uint32_t SettingsStore::loadFanTimer(uint32_t fallbackValue) const
{
    return opened_ ? preferences_.getUInt(kFanTimerKey, fallbackValue) : fallbackValue;
}

bool SettingsStore::loadLightEnabled(bool fallbackValue) const
{
    return opened_ ? preferences_.getBool(kLightEnabledKey, fallbackValue) : fallbackValue;
}

void SettingsStore::saveTemperature(int value)
{
    if (opened_) {
        preferences_.putInt(kTemperatureKey, value);
    }
}

void SettingsStore::saveFanTimer(uint32_t value)
{
    if (opened_) {
        preferences_.putUInt(kFanTimerKey, value);
    }
}

void SettingsStore::saveLightEnabled(bool value)
{
    if (opened_) {
        preferences_.putBool(kLightEnabledKey, value);
    }
}
