#include "FanController.h"

FanController::FanController(
    const String &name,
    int fgPin,
    int pwmPin,
    uint32_t pwmFreq,
    uint8_t pwmResolution,
    bool invertPwm
) : name_(name),
    fgPin_(fgPin),
    pwmPin_(pwmPin),
    pwmFreq_(pwmFreq),
    pwmResolution_(pwmResolution),
    invertPwm_(invertPwm) {
}

void FanController::begin() {
    pinMode(fgPin_, INPUT_PULLUP);
    ledcAttach(pwmPin_, pwmFreq_, pwmResolution_);
    writeDuty(0);
}

void FanController::onPulse() {
    pulseCount_ += 1;
}

void FanController::writeDuty(int percent) {
    dutyPercent_ = constrain(percent, 0, 100);
    const uint32_t pwmMax = (1UL << pwmResolution_) - 1UL;
    int pwmValue = map(dutyPercent_, 0, 100, 0, static_cast<int>(pwmMax));
    if (invertPwm_) {
        pwmValue = static_cast<int>(pwmMax) - pwmValue;
    }
    ledcWrite(pwmPin_, pwmValue);
}

void FanController::updateRpmEverySecond(uint8_t pulsesPerRev) {
    noInterrupts();
    const uint32_t pulses = pulseCount_;
    pulseCount_ = 0;
    interrupts();
    rpm_ = (pulsesPerRev == 0) ? 0 : (pulses * 60U) / pulsesPerRev;
}

const String &FanController::name() const {
    return name_;
}

int FanController::dutyPercent() const {
    return dutyPercent_;
}

uint32_t FanController::rpm() const {
    return rpm_;
}
