#pragma once

#include <lvgl.h>

#include "app/AppState.h"

class PanelView {
public:
    void begin();

    void render(const AppState &state);

    void setIdleDimPercent(uint8_t dimPercent, uint32_t durationMs);

private:
    void setPowerOff(bool powerOff);

    void renderTemperatureMode(int temperature, const lv_img_dsc_t *icon, lv_color_t iconColor,
                               bool showLightUnderline);

    void renderLightMode(const AppState &state);

    void renderWaterMode(const AppState &state);

    void renderWindMode(const AppState &state);

    void setIcon(const lv_img_dsc_t *icon, lv_color_t color, bool recolorEnabled = true);

    void setProgress(float progressRatio, bool visible);

    void setTemperatureArc(int temperature, bool visible);

    void setTemperatureScaleVisible(bool visible);

    void animateArcTo(lv_obj_t *arc, int &currentValue, int targetValue, uint32_t durationMs);

    void animateOverlayOpacityTo(lv_opa_t targetOpacity, uint32_t durationMs);

    bool initialized_ = false;
    lv_obj_t *screen_ = nullptr;
    lv_obj_t *idleDimOverlay_ = nullptr;
    lv_obj_t *temperatureArc_ = nullptr;
    lv_obj_t *centerLabel_ = nullptr;
    lv_obj_t *leftLabel_ = nullptr;
    lv_obj_t *rightLabel_ = nullptr;
    lv_obj_t *iconImage_ = nullptr;
    lv_obj_t *lightUnderline_ = nullptr;
    lv_obj_t *windProgressArc_ = nullptr;
    lv_opa_t currentDimOverlayOpacity_ = LV_OPA_TRANSP;
    int currentTemperatureArcValue_ = 1;
    int currentWindArcValue_ = 1000;
};
