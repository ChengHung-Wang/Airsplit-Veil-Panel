#include "PeripheralStatusLed.h"

#include <math.h>

namespace {
    constexpr uint32_t kPwmFreq = 1000;
    constexpr uint8_t kPwmResolution = 8;
    constexpr uint8_t kDutyMax = 255;
    constexpr uint32_t kUpdateIntervalMs = 16;
    constexpr uint32_t kBreathingPeriodMs = 3200;
    constexpr uint32_t kConnectedBlinkPeriodMs = 850;
    constexpr uint32_t kConnectedBlinkOffMs = 800;
    constexpr float kPi = 3.14159265358979323846F;
} // namespace

PeripheralStatusLed::PeripheralStatusLed(gpio_num_t pin, bool activeLow)
    : pin_(pin),
      activeLow_(activeLow) {
}

void PeripheralStatusLed::begin() {
    enabled_ = pin_ != GPIO_NUM_NC;
    if (!enabled_) {
        return;
    }
    ledcAttach(pin_, kPwmFreq, kPwmResolution);
    write(0);
}

void PeripheralStatusLed::update(uint32_t nowMs, bool forceOn, bool connected) {
    if (!enabled_ || ((nowMs - lastUpdateMs_) < kUpdateIntervalMs)) {
        return;
    }
    lastUpdateMs_ = nowMs;

    if (forceOn) {
        write(kDutyMax);
        return;
    }

    if (!connected) {
        write(breathingDuty(nowMs));
        return;
    }

    const uint32_t phaseMs = nowMs % kConnectedBlinkPeriodMs;
    write(phaseMs < kConnectedBlinkOffMs ? 0 : kDutyMax);
}

bool PeripheralStatusLed::isEnabled() const {
    return enabled_;
}

uint8_t PeripheralStatusLed::breathingDuty(uint32_t nowMs) const {
    const float phase = static_cast<float>(nowMs % kBreathingPeriodMs) /
        static_cast<float>(kBreathingPeriodMs);
    const float wave = 0.5F - (0.5F * cosf(phase * 2.0F * kPi));
    const float perceptual = powf(wave, 2.2F);
    constexpr float kFloorDuty = 3.0F;
    constexpr float kCeilingDuty = static_cast<float>(kDutyMax);
    return static_cast<uint8_t>(kFloorDuty + (perceptual * (kCeilingDuty - kFloorDuty)));
}

void PeripheralStatusLed::write(uint8_t duty) {
    ledcWrite(pin_, activeLow_ ? (kDutyMax - duty) : duty);
}
