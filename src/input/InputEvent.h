#pragma once

#include <stdint.h>

enum class InputEventType : uint8_t {
    KnobLeft,
    KnobRight,
    SelectPress,
    PowerToggle,
    PowerOff,
    ModeLight,
    ModeWater,
    ModeWind,
};

struct InputEvent {
    InputEventType type;
};
