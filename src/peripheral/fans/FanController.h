#pragma once

#include <Arduino.h>

class FanController {
public:
    FanController(const String &name, int fgPin, int pwmPin, uint32_t pwmFreq, uint8_t pwmResolution, bool invertPwm);

    void begin();

    void onPulse();

    void writeDuty(int percent);

    void updateRpmEverySecond(uint8_t pulsesPerRev);

    const String &name() const;

    int dutyPercent() const;

    uint32_t rpm() const;

private:
    String name_;
    int fgPin_ = -1;
    int pwmPin_ = -1;
    uint32_t pwmFreq_ = 0;
    uint8_t pwmResolution_ = 0;
    bool invertPwm_ = false;
    volatile uint32_t pulseCount_ = 0;
    uint32_t rpm_ = 0;
    int dutyPercent_ = 0;
};
