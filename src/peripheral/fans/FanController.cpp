#include "FanController.h"

FanController::FanController(
    const String &name,
    int fgPin,
    int pwmPin,
    uint32_t pwmFreq,
    uint8_t pwmResolution,
    bool invertPwm
): name_(name),
   fgPin_(fgPin),
   pwmPin_(pwmPin),
   pwmFreq_(pwmFreq),
   pwmResolution_(pwmResolution),
   invertPwm_(invertPwm)
{
}

void FanController::begin()
{
    pinMode(fgPin_, INPUT_PULLUP);
    ledcAttach(pwmPin_, pwmFreq_, pwmResolution_);
    writeDuty(0);
}

void FanController::onPulse()
{
    pulseCount_ += 1;
}

void FanController::writeDuty(int percent)
{
    dutyPercent_ = constrain(percent, 0, 100);
    int pwmValue = map(dutyPercent_, 0, 100, 0, 255);
    if (invertPwm_) {
        pwmValue = 255 - pwmValue;
    }
    ledcWrite(pwmPin_, pwmValue);
}

void FanController::updateRpmEverySecond(uint8_t pulsesPerRev)
{
    noInterrupts();
    const uint32_t pulses = pulseCount_;
    pulseCount_ = 0;
    interrupts();
    rpm_ = (pulsesPerRev == 0) ? 0 : (pulses * 60U) / pulsesPerRev;
}

const String &FanController::name() const
{
    return name_;
}

int FanController::dutyPercent() const
{
    return dutyPercent_;
}

uint32_t FanController::rpm() const
{
    return rpm_;
}
