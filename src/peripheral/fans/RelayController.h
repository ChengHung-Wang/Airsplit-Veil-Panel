#pragma once

#include <Arduino.h>

class RelayController {
public:
    explicit RelayController(int pin);

    void begin();

    void enable();

    void disable();

    bool isEnabled() const;

private:
    int pin_ = -1;
    bool enabled_ = false;
};
