#pragma once

#include <lvgl.h>

#include "panel/app/AppState.h"

class PanelView {
public:
    void begin();
    void render(const AppState &state);

private:
    void setPowerOff(bool powerOff);
    void renderTemperatureMode(int temperature, const lv_img_dsc_t *icon, lv_color_t iconColor, bool showLightUnderline);
    void renderLightMode(const AppState &state);
    void renderWaterMode(const AppState &state);
    void renderWindMode(const AppState &state);
    void setIcon(const lv_img_dsc_t *icon, lv_color_t color);
    void setFanDots(uint8_t activeLevel);
    void setProgress(float progressRatio, bool visible);
    void setTemperatureArc(int temperature, bool visible);
    void setTemperatureScaleVisible(bool visible);
    void animateArcTo(lv_obj_t *arc, int &currentValue, int targetValue, uint32_t durationMs);

    bool initialized_ = false;
    lv_obj_t *screen_ = nullptr;
    lv_obj_t *temperatureArc_ = nullptr;
    lv_obj_t *centerLabel_ = nullptr;
    lv_obj_t *leftLabel_ = nullptr;
    lv_obj_t *rightLabel_ = nullptr;
    lv_obj_t *iconImage_ = nullptr;
    lv_obj_t *lightUnderline_ = nullptr;
    lv_obj_t *windProgressArc_ = nullptr;
    lv_obj_t *fanDots_[3] = {nullptr, nullptr, nullptr};
    int currentTemperatureArcValue_ = 1;
    int currentWindArcValue_ = 1000;
};
