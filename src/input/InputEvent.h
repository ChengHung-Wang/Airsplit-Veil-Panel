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

enum class InputEventSource : uint8_t {
    Local,
    Uart0,
    Uart1,
};

struct InputEvent {
    InputEventType type;
    InputEventSource source = InputEventSource::Local;
};
