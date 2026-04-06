#pragma once

#include <Arduino.h>

#include "input/InputEvent.h"

class UartCommandParser {
public:
    bool parse(const char *command, InputEvent &event) const;

private:
    static String normalize(const char *command);
};
