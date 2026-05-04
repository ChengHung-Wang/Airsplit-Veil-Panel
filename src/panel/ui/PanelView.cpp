#include "config/lvgl_port_v8.h"
#include "ui/PanelView.h"
#include "ui/UiAssets.h"
#include "utils/TemperatureColorMapper.h"

namespace {
constexpr lv_coord_t kTemperatureCenterY = -2;
constexpr lv_coord_t kWindCenterY = -25;
constexpr lv_coord_t kIconY = 130;
constexpr lv_coord_t kLightUnderlineY = 166;
constexpr lv_coord_t kDotY = -160;
constexpr lv_coord_t kScaleY = 104;
constexpr lv_coord_t kScaleX = 140;
constexpr lv_coord_t kTemperatureArcSize = ESP_PANEL_LCD_HEIGHT;
constexpr lv_coord_t kWindArcSize = ESP_PANEL_LCD_HEIGHT;
constexpr int kTemperatureArcRotation = 150;
constexpr int kTemperatureArcSweep = 240;
constexpr int kWindArcRange = 1000;
constexpr uint32_t kArcAnimDurationMs = 150;

lv_color_t warmLightColor()
{
    return lv_color_make(0xF1, 0xD3, 0x9D);
}

void setHidden(lv_obj_t *obj, bool hidden)
{
    if (hidden) {
        lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
    }
}

void setArcValueExec(void *obj, int32_t value)
{
    lv_arc_set_value(static_cast<lv_obj_t *>(obj), static_cast<int16_t>(value));
}
}

void PanelView::begin()
{
    if (initialized_) {
        return;
    }

    screen_ = lv_obj_create(nullptr);
    lv_obj_remove_style_all(screen_);
    lv_obj_set_size(screen_, lv_disp_get_hor_res(nullptr), lv_disp_get_ver_res(nullptr));
    lv_obj_set_style_bg_color(screen_, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(screen_, LV_OPA_COVER, 0);

    static const lv_style_prop_t transitionProps[] = {
        LV_STYLE_TEXT_COLOR,
        LV_STYLE_IMG_RECOLOR,
        LV_STYLE_ARC_COLOR,
        static_cast<lv_style_prop_t>(0)
    };
    static lv_style_transition_dsc_t transition;
    static bool transitionReady = false;
    if (!transitionReady) {
        lv_style_transition_dsc_init(&transition, transitionProps, lv_anim_path_ease_in_out, 220, 0, nullptr);
        transitionReady = true;
    }

    temperatureArc_ = lv_arc_create(screen_);
    lv_obj_set_size(temperatureArc_, kTemperatureArcSize, kTemperatureArcSize);
    lv_obj_align(temperatureArc_, LV_ALIGN_CENTER, 0, 0);
    lv_arc_set_rotation(temperatureArc_, kTemperatureArcRotation);
    lv_arc_set_bg_angles(temperatureArc_, 0, kTemperatureArcSweep);
    lv_arc_set_range(
        temperatureArc_,
        TemperatureColorMapper::kArcMinValue,
        TemperatureColorMapper::kArcMaxValue
    );
    lv_arc_set_value(temperatureArc_, currentTemperatureArcValue_);
    lv_obj_set_style_arc_width(temperatureArc_, 30, LV_PART_MAIN);
    lv_obj_set_style_arc_width(temperatureArc_, 30, LV_PART_INDICATOR);
    lv_obj_set_style_arc_opa(temperatureArc_, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_arc_color(
        temperatureArc_,
        TemperatureColorMapper::colorForTemperature(AppState::kMinTemperature),
        LV_PART_INDICATOR
    );
    lv_obj_set_style_arc_rounded(temperatureArc_, true, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(temperatureArc_, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_clear_flag(temperatureArc_, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_transition(temperatureArc_, &transition, LV_PART_INDICATOR);

    windProgressArc_ = lv_arc_create(screen_);
    lv_obj_set_size(windProgressArc_, kWindArcSize, kWindArcSize);
    lv_obj_align(windProgressArc_, LV_ALIGN_CENTER, 0, 0);
    lv_arc_set_rotation(windProgressArc_, 270);
    lv_arc_set_bg_angles(windProgressArc_, 0, 360);
    lv_arc_set_range(windProgressArc_, 0, kWindArcRange);
    lv_arc_set_value(windProgressArc_, currentWindArcValue_);
    lv_obj_set_style_arc_width(windProgressArc_, 30, LV_PART_MAIN);
    lv_obj_set_style_arc_width(windProgressArc_, 30, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(windProgressArc_, lv_color_make(0x4E, 0x4E, 0x4E), LV_PART_MAIN);
    lv_obj_set_style_arc_color(windProgressArc_, lv_color_make(0xD9, 0xD9, 0xD9), LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(windProgressArc_, true, LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(windProgressArc_, true, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(windProgressArc_, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_clear_flag(windProgressArc_, LV_OBJ_FLAG_CLICKABLE);

    centerLabel_ = lv_label_create(screen_);
    lv_obj_set_style_text_font(centerLabel_, &AppleSDGothicNeo_ExtraBold_200px, 0);
    lv_obj_set_style_text_color(centerLabel_, lv_color_white(), 0);
    lv_obj_set_style_transition(centerLabel_, &transition, 0);
    lv_label_set_text(centerLabel_, "25");
    lv_obj_align(centerLabel_, LV_ALIGN_CENTER, 0, kTemperatureCenterY);

    leftLabel_ = lv_label_create(screen_);
    lv_obj_set_style_text_font(leftLabel_, &AppleSDGothicNeo_ExtraBold_40px, 0);
    lv_obj_set_style_text_color(leftLabel_, lv_color_white(), 0);
    lv_label_set_text(leftLabel_, "20");
    lv_obj_align(leftLabel_, LV_ALIGN_CENTER, -kScaleX, kScaleY);

    rightLabel_ = lv_label_create(screen_);
    lv_obj_set_style_text_font(rightLabel_, &AppleSDGothicNeo_ExtraBold_40px, 0);
    lv_obj_set_style_text_color(rightLabel_, lv_color_white(), 0);
    lv_label_set_text(rightLabel_, "45");
    lv_obj_align(rightLabel_, LV_ALIGN_CENTER, kScaleX, kScaleY);

    iconImage_ = lv_img_create(screen_);
    lv_obj_align(iconImage_, LV_ALIGN_CENTER, 0, kIconY);
    lv_obj_set_style_img_recolor_opa(iconImage_, LV_OPA_COVER, 0);
    lv_obj_set_style_transition(iconImage_, &transition, 0);
    
    // The light mode underline design is removed in the current UI design, but we keep the code here for easy re-enable in the future if needed.
    // --------
    // lightUnderline_ = lv_obj_create(screen_);
    // lv_obj_remove_style_all(lightUnderline_);
    // lv_obj_set_size(lightUnderline_, 96, 7);
    // lv_obj_align(lightUnderline_, LV_ALIGN_CENTER, 0, kLightUnderlineY);
    // lv_obj_set_style_bg_color(lightUnderline_, warmLightColor(), 0);
    // lv_obj_set_style_bg_opa(lightUnderline_, LV_OPA_COVER, 0);
    // lv_obj_set_style_radius(lightUnderline_, 0, 0);

    for (uint8_t i = 0; i < 3; ++i) {
        fanDots_[i] = lv_obj_create(screen_);
        lv_obj_remove_style_all(fanDots_[i]);
        lv_obj_set_size(fanDots_[i], 20, 20);
        lv_obj_align(fanDots_[i], LV_ALIGN_CENTER, static_cast<lv_coord_t>((static_cast<int>(i) - 1) * 38), kDotY);
        lv_obj_set_style_radius(fanDots_[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_opa(fanDots_[i], LV_OPA_COVER, 0);
    }

    lv_scr_load(screen_);
    initialized_ = true;
}

void PanelView::render(const AppState &state)
{
    if (!initialized_) {
        begin();
    }

    setPowerOff(!state.powerOn);
    if (!state.powerOn) {
        return;
    }

    switch (state.currentMode) {
    case AppMode::Idle:
        renderTemperatureMode(state.temperature, nullptr, lv_color_white(), false);
        setHidden(leftLabel_, true);
        setHidden(rightLabel_, true);
        // setHidden(temperatureArc_, true);
        break;
    case AppMode::Light:
        renderLightMode(state);
        break;
    case AppMode::Water:
        renderWaterMode(state);
        break;
    case AppMode::Wind:
        renderWindMode(state);
        break;
    }
}

void PanelView::setPowerOff(bool powerOff)
{
    setHidden(temperatureArc_, powerOff);
    setHidden(windProgressArc_, powerOff);
    setHidden(centerLabel_, powerOff);
    setHidden(leftLabel_, powerOff);
    setHidden(rightLabel_, powerOff);
    setHidden(iconImage_, powerOff);
    // setHidden(lightUnderline_, powerOff);
    for (lv_obj_t *dot : fanDots_) {
        setHidden(dot, powerOff);
    }
}

void PanelView::renderTemperatureMode(
    int temperature,
    const lv_img_dsc_t *icon,
    lv_color_t iconColor,
    bool showLightUnderline
)
{
    lv_obj_set_style_text_font(centerLabel_, &AppleSDGothicNeo_ExtraBold_200px, 0);
    lv_label_set_text_fmt(centerLabel_, "%d", temperature);
    lv_obj_align(centerLabel_, LV_ALIGN_CENTER, 0, kTemperatureCenterY);
    lv_obj_set_style_text_color(centerLabel_, TemperatureColorMapper::colorForTemperature(temperature), 0);

    setTemperatureScaleVisible(true);
    setTemperatureArc(temperature, true);
    setProgress(0.0f, false);
    setFanDots(0);
    setIcon(icon, iconColor);

    // The light mode underline design is removed in the current UI design, but we keep the code here for easy re-enable in the future if needed.
    // --------
    // setHidden(lightUnderline_, !showLightUnderline);
}

void PanelView::renderLightMode(const AppState &state)
{
    renderTemperatureMode(
        state.temperature,
        &light_80x80,
        state.lightEnabled ? warmLightColor() : lv_color_white(),
        state.lightEnabled
    );
}

void PanelView::renderWaterMode(const AppState &state)
{
    renderTemperatureMode(state.temperature, &water_80x80, lv_color_white(), false);
}

void PanelView::renderWindMode(const AppState &state)
{
    setTemperatureScaleVisible(false);
    setTemperatureArc(state.temperature, false);
    // The light mode underline design is removed in the current UI design, but we keep the code here for easy re-enable in the future if needed.
    // --------
    // setHidden(lightUnderline_, true);
    setIcon(&wind_80x80, lv_color_white());
    setFanDots(state.fanLevel);

    lv_obj_set_style_text_font(centerLabel_, &AppleSDGothicNeo_ExtraBold_128px, 0);
    const uint32_t displaySeconds = state.windAdjusting ? state.windAdjustCandidateSeconds : state.fanRemainingSeconds;
    if (state.windAdjusting && !state.windAdjustmentBlinkOn) {
        lv_label_set_text(centerLabel_, "");
    } else {
        lv_label_set_text_fmt(centerLabel_, "%02lu:%02lu", displaySeconds / 60UL, displaySeconds % 60UL);
    }
    lv_obj_align(centerLabel_, LV_ALIGN_CENTER, 0, kWindCenterY);
    lv_obj_set_style_text_color(centerLabel_, lv_color_white(), 0);

    const float progress = (state.fanTimerSeconds == 0U)
        ? 0.0f
        : static_cast<float>(state.fanRemainingSeconds) / static_cast<float>(state.fanTimerSeconds);
    setProgress(progress, true);
}

void PanelView::setIcon(const lv_img_dsc_t *icon, lv_color_t color)
{
    if (icon == nullptr) {
        setHidden(iconImage_, true);
        return;
    }

    lv_img_set_src(iconImage_, icon);
    lv_obj_set_style_img_recolor(iconImage_, color, 0);
    lv_obj_align(iconImage_, LV_ALIGN_CENTER, 0, kIconY);
    setHidden(iconImage_, false);
}

void PanelView::setFanDots(uint8_t activeLevel)
{
    for (uint8_t i = 0; i < 3; ++i) {
        const bool visible = activeLevel > 0;
        setHidden(fanDots_[i], !visible);
        if (!visible) {
            continue;
        }

        const bool active = (i + 1U) <= activeLevel;
        lv_obj_set_style_bg_color(
            fanDots_[i],
            active ? lv_color_white() : lv_color_make(0x62, 0x62, 0x62),
            0
        );
    }
}

void PanelView::setProgress(float progressRatio, bool visible)
{
    setHidden(windProgressArc_, !visible);
    if (!visible) {
        return;
    }

    const float clamped = constrain(progressRatio, 0.0f, 1.0f);
    int value = static_cast<int>(clamped * static_cast<float>(kWindArcRange));
    if (value <= 0) {
        value = 1;
    }
    animateArcTo(windProgressArc_, currentWindArcValue_, value, kArcAnimDurationMs);
}

void PanelView::setTemperatureArc(int temperature, bool visible)
{
    setHidden(temperatureArc_, !visible);
    if (!visible) {
        return;
    }

    lv_obj_set_style_arc_color(
        temperatureArc_,
        TemperatureColorMapper::colorForTemperature(temperature),
        LV_PART_INDICATOR
    );
    animateArcTo(
        temperatureArc_,
        currentTemperatureArcValue_,
        TemperatureColorMapper::visibleArcValueForTemperature(temperature),
        kArcAnimDurationMs
    );
}

void PanelView::setTemperatureScaleVisible(bool visible)
{
    // setHidden(leftLabel_, !visible);
    // setHidden(rightLabel_, !visible);
    
    // For the current design we decided to hide the temperature scales labels.
    setHidden(leftLabel_, true);
    setHidden(rightLabel_, true);
}

void PanelView::animateArcTo(lv_obj_t *arc, int &currentValue, int targetValue, uint32_t durationMs)
{
    lv_anim_del(arc, setArcValueExec);

    lv_anim_t animation;
    lv_anim_init(&animation);
    lv_anim_set_var(&animation, arc);
    lv_anim_set_exec_cb(&animation, setArcValueExec);
    lv_anim_set_values(&animation, currentValue, targetValue);
    lv_anim_set_time(&animation, durationMs);
    lv_anim_set_path_cb(&animation, lv_anim_path_ease_out);
    lv_anim_start(&animation);

    currentValue = targetValue;
}
