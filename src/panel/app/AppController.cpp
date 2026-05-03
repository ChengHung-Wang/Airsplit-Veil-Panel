#include "AppController.h"

#include "config/lvgl_port_v8.h"

namespace {
constexpr uint32_t kBlinkIntervalMs = 500;
constexpr uint32_t kAdjustTimeoutMs = 5000;
constexpr uint32_t kSettingsCommitDelayMs = 1000;
constexpr uint32_t kAutoPowerOffDelayMs = 5000;
constexpr uint32_t kMaxFanTimerSeconds = (99 * 60) + 59;
}

AppController::AppController(ESP_Panel *panel): panel_(panel)
{
}

void AppController::begin()
{
    store_.begin();
    state_.temperature = constrain(
        store_.loadTemperature(state_.temperature),
        AppState::kMinTemperature,
        AppState::kMaxTemperature
    );
    state_.fanTimerSeconds = max(
        store_.loadFanTimer(state_.fanTimerSeconds),
        AppState::kMinFanTimerSeconds
    );
    state_.fanRemainingSeconds = state_.fanTimerSeconds;
    state_.windAdjustCandidateSeconds = state_.fanTimerSeconds;

    applyDisplayPower(true);
    lvgl_port_lock(-1);
    view_.begin();
    view_.render(state_);
    lvgl_port_unlock();
}

void AppController::handleEvent(const InputEvent &event, uint32_t nowMs)
{
    switch (event.type) {
    case InputEventType::PowerToggle:
        setPower(!state_.powerOn, nowMs);
        return;
    case InputEventType::PowerOff:
        setPower(false, nowMs);
        return;
    default:
        break;
    }

    if (!state_.powerOn) {
        return;
    }

    switch (event.type) {
    case InputEventType::KnobLeft:
        handleKnobDelta(-1, nowMs);
        break;
    case InputEventType::KnobRight:
        handleKnobDelta(1, nowMs);
        break;
    case InputEventType::SelectPress:
        handleSelectPress(nowMs);
        break;
    case InputEventType::ModeLight:
        if (state_.currentMode == AppMode::Light) {
            toggleLightState();
        } else {
            enterMode(AppMode::Light, nowMs);
        }
        break;
    case InputEventType::ModeWater:
        enterMode(AppMode::Water, nowMs);
        break;
    case InputEventType::ModeWind:
        if (state_.currentMode == AppMode::Wind) {
            if (!state_.windAdjusting) {
                cycleFanLevel();
            }
        } else {
            enterMode(AppMode::Wind, nowMs);
        }
        break;
    case InputEventType::PowerToggle:
    case InputEventType::PowerOff:
        break;
    }
}

void AppController::update(uint32_t nowMs)
{
    if (temperatureSavePending_ || fanTimerSavePending_) {
        if ((nowMs - lastSettingsChangeAtMs_) >= kSettingsCommitDelayMs) {
            commitPendingSettings();
        }
    }

    if (!state_.powerOn) {
        return;
    }

    if (state_.currentMode == AppMode::Wind) {
        if (state_.windAdjusting) {
            if ((nowMs - lastBlinkToggleAtMs_) >= kBlinkIntervalMs) {
                state_.windAdjustmentBlinkOn = !state_.windAdjustmentBlinkOn;
                lastBlinkToggleAtMs_ = nowMs;
                renderDirty_ = true;
            }
            if ((nowMs - lastWindAdjustInputAtMs_) >= kAdjustTimeoutMs) {
                cancelWindAdjustment();
            }
        } else if (state_.fanRemainingSeconds > 0U) {
            while ((nowMs - lastWindTickAtMs_) >= 1000U) {
                lastWindTickAtMs_ += 1000U;
                if (state_.fanRemainingSeconds > 0U) {
                    --state_.fanRemainingSeconds;
                    renderDirty_ = true;
                }
                if (state_.fanRemainingSeconds == 0U) {
                    zeroReachedAtMs_ = nowMs;
                    break;
                }
            }
        } else if ((zeroReachedAtMs_ != 0U) && ((nowMs - zeroReachedAtMs_) >= kAutoPowerOffDelayMs)) {
            setPower(false, nowMs);
        }
    }
}

void AppController::renderIfNeeded()
{
    if (!renderDirty_) {
        return;
    }

    lvgl_port_lock(-1);
    view_.render(state_);
    lvgl_port_unlock();
    renderDirty_ = false;

    if (displayOffPending_) {
        applyDisplayPower(false);
        displayOffPending_ = false;
    }
}

void AppController::handleKnobDelta(int delta, uint32_t nowMs)
{
    if (state_.currentMode == AppMode::Wind) {
        startWindAdjustment(nowMs);
        const int nextCandidate = static_cast<int>(state_.windAdjustCandidateSeconds) + delta;
        state_.windAdjustCandidateSeconds = constrain(
            nextCandidate,
            static_cast<int>(AppState::kMinFanTimerSeconds),
            static_cast<int>(kMaxFanTimerSeconds)
        );
        state_.windAdjustmentBlinkOn = true;
        lastWindAdjustInputAtMs_ = nowMs;
        renderDirty_ = true;
        return;
    }

    const int nextTemperature = constrain(
        state_.temperature + delta,
        AppState::kMinTemperature,
        AppState::kMaxTemperature
    );
    if (nextTemperature != state_.temperature) {
        state_.temperature = nextTemperature;
        markSettingsDirty(true, false, nowMs);
        renderDirty_ = true;
    }
}

void AppController::handleSelectPress(uint32_t nowMs)
{
    if (state_.currentMode != AppMode::Wind) {
        return;
    }

    if (state_.windAdjusting) {
        confirmWindAdjustment(nowMs);
        return;
    }

    cycleFanLevel();
}

void AppController::enterMode(AppMode mode, uint32_t nowMs)
{
    state_.currentMode = mode;
    state_.windAdjusting = false;
    state_.windAdjustmentBlinkOn = true;
    zeroReachedAtMs_ = 0;

    if (mode == AppMode::Wind) {
        state_.fanRemainingSeconds = state_.fanTimerSeconds;
        state_.windAdjustCandidateSeconds = state_.fanTimerSeconds;
        lastWindTickAtMs_ = nowMs;
    }

    renderDirty_ = true;
}

void AppController::setPower(bool powerOn, uint32_t nowMs)
{
    if (state_.powerOn == powerOn) {
        return;
    }

    commitPendingSettings();
    state_.powerOn = powerOn;
    state_.windAdjusting = false;
    state_.windAdjustmentBlinkOn = true;
    zeroReachedAtMs_ = 0;

    if (powerOn) {
        applyDisplayPower(true);
        state_.currentMode = AppMode::Idle;
        state_.fanRemainingSeconds = state_.fanTimerSeconds;
        state_.windAdjustCandidateSeconds = state_.fanTimerSeconds;
        lastWindTickAtMs_ = nowMs;
    } else {
        state_.currentMode = AppMode::Idle;
        displayOffPending_ = true;
    }

    renderDirty_ = true;
}

void AppController::toggleLightState()
{
    state_.lightState = (state_.lightState == LightState::Primary)
        ? LightState::Secondary
        : LightState::Primary;
    renderDirty_ = true;
}

void AppController::cycleFanLevel()
{
    state_.fanLevel = (state_.fanLevel % 3U) + 1U;
    renderDirty_ = true;
}

void AppController::startWindAdjustment(uint32_t nowMs)
{
    if (!state_.windAdjusting) {
        state_.windAdjusting = true;
        state_.windAdjustCandidateSeconds = max(state_.fanRemainingSeconds, AppState::kMinFanTimerSeconds);
        state_.windAdjustmentBlinkOn = true;
        lastBlinkToggleAtMs_ = nowMs;
    }
    lastWindAdjustInputAtMs_ = nowMs;
}

void AppController::confirmWindAdjustment(uint32_t nowMs)
{
    state_.windAdjusting = false;
    state_.windAdjustmentBlinkOn = true;
    state_.fanTimerSeconds = state_.windAdjustCandidateSeconds;
    state_.fanRemainingSeconds = state_.windAdjustCandidateSeconds;
    lastWindTickAtMs_ = nowMs;
    zeroReachedAtMs_ = 0;
    markSettingsDirty(false, true, nowMs);
    renderDirty_ = true;
}

void AppController::cancelWindAdjustment()
{
    state_.windAdjusting = false;
    state_.windAdjustmentBlinkOn = true;
    state_.windAdjustCandidateSeconds = max(state_.fanRemainingSeconds, AppState::kMinFanTimerSeconds);
    renderDirty_ = true;
}

void AppController::markSettingsDirty(bool temperatureChanged, bool fanTimerChanged, uint32_t nowMs)
{
    temperatureSavePending_ = temperatureSavePending_ || temperatureChanged;
    fanTimerSavePending_ = fanTimerSavePending_ || fanTimerChanged;
    lastSettingsChangeAtMs_ = nowMs;
}

void AppController::commitPendingSettings()
{
    if (temperatureSavePending_) {
        store_.saveTemperature(state_.temperature);
        temperatureSavePending_ = false;
    }
    if (fanTimerSavePending_) {
        store_.saveFanTimer(state_.fanTimerSeconds);
        fanTimerSavePending_ = false;
    }
}

void AppController::applyDisplayPower(bool powerOn)
{
    if (panel_ == nullptr) {
        return;
    }

    if (powerOn) {
        if (panel_->getLcd() != nullptr) {
            panel_->getLcd()->displayOn();
        }
        if (panel_->getBacklight() != nullptr) {
            panel_->getBacklight()->on();
        }
    } else {
        if (panel_->getBacklight() != nullptr) {
            panel_->getBacklight()->off();
        }
        if (panel_->getLcd() != nullptr) {
            panel_->getLcd()->displayOff();
        }
    }
}
