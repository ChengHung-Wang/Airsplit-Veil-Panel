#pragma once

#include <lvgl.h>

class TemperatureColorMapper {
public:
    static constexpr int kArcMinValue = 0;
    static constexpr int kArcMaxValue = 1000;

    static lv_color_t colorForTemperature(int temperature);

    static int arcValueForTemperature(int temperature);

    static int visibleArcValueForTemperature(int temperature);

private:
    static lv_color_t interpolateRgb(
        uint8_t startR,
        uint8_t startG,
        uint8_t startB,
        uint8_t endR,
        uint8_t endG,
        uint8_t endB,
        float ratio
    );
};
