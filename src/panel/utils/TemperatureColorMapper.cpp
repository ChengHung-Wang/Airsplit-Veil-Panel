#include "TemperatureColorMapper.h"

#include "app/AppState.h"

namespace {
constexpr uint8_t kCoolR = 0x82;
constexpr uint8_t kCoolG = 0xDC;
constexpr uint8_t kCoolB = 0xDD;
constexpr uint8_t kWarmR = 0xFF;
constexpr uint8_t kWarmG = 0x7D;
constexpr uint8_t kWarmB = 0x00;
}

lv_color_t TemperatureColorMapper::colorForTemperature(int temperature)
{
    const int clamped = constrain(temperature, AppState::kMinTemperature, AppState::kMaxTemperature);
    if (clamped <= 25) {
        const float ratio = static_cast<float>(clamped - AppState::kMinTemperature) / 5.0f;
        return interpolateRgb(kCoolR, kCoolG, kCoolB, 0xFF, 0xFF, 0xFF, ratio);
    }

    const float ratio = static_cast<float>(clamped - 25) / 20.0f;
    return interpolateRgb(0xFF, 0xFF, 0xFF, kWarmR, kWarmG, kWarmB, ratio);
}

int TemperatureColorMapper::arcValueForTemperature(int temperature)
{
    const int clamped = constrain(temperature, AppState::kMinTemperature, AppState::kMaxTemperature);
    const float ratio = static_cast<float>(clamped - AppState::kMinTemperature)
        / static_cast<float>(AppState::kMaxTemperature - AppState::kMinTemperature);
    return static_cast<int>(ratio * static_cast<float>(kArcMaxValue));
}

int TemperatureColorMapper::visibleArcValueForTemperature(int temperature)
{
    const int mapped = arcValueForTemperature(temperature);
    return (mapped <= 0) ? 1 : mapped;
}

lv_color_t TemperatureColorMapper::interpolateRgb(
    uint8_t startR,
    uint8_t startG,
    uint8_t startB,
    uint8_t endR,
    uint8_t endG,
    uint8_t endB,
    float ratio
)
{
    const float clamped = constrain(ratio, 0.0f, 1.0f);
    const uint8_t red = static_cast<uint8_t>(startR + ((endR - startR) * clamped));
    const uint8_t green = static_cast<uint8_t>(startG + ((endG - startG) * clamped));
    const uint8_t blue = static_cast<uint8_t>(startB + ((endB - startB) * clamped));
    return lv_color_make(red, green, blue);
}
