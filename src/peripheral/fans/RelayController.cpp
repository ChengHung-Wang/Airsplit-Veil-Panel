#include "RelayController.h"

RelayController::RelayController(int pin): pin_(pin)
{
}

void RelayController::begin()
{
    pinMode(pin_, OUTPUT);
    disable();
}

void RelayController::enable()
{
    enabled_ = true;
    digitalWrite(pin_, HIGH);
}

void RelayController::disable()
{
    enabled_ = false;
    digitalWrite(pin_, LOW);
}

bool RelayController::isEnabled() const
{
    return enabled_;
}
