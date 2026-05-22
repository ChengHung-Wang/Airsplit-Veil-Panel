#pragma once

#include <Arduino.h>

class PeripheralStatusLed {
public:
    explicit PeripheralStatusLed(gpio_num_t pin = GPIO_NUM_NC, bool activeLow = true);

    void begin();

    void update(uint32_t nowMs, bool forceOn, bool connected);

    bool isEnabled() const;

private:
    uint8_t breathingDuty(uint32_t nowMs) const;

    void write(uint8_t duty);

    gpio_num_t pin_ = GPIO_NUM_NC;
    bool activeLow_ = true;
    bool enabled_ = false;
    uint32_t lastUpdateMs_ = 0;
};
