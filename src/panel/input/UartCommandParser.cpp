#include "input/UartCommandParser.h"

String UartCommandParser::normalize(const char *command) {
    String normalized = command == nullptr ? "" : String(command);
    normalized.trim();
    normalized.toUpperCase();
    return normalized;
}

bool UartCommandParser::parse(const char *command, InputEvent &event) const {
    const String normalized = normalize(command);
    if (normalized.isEmpty()) {
        return false;
    }

    if ((normalized == "POWER") || (normalized == "POWER_SHORT")) {
        event.type = InputEventType::PowerToggle;
        return true;
    }
    if (normalized == "POWER_LONG") {
        event.type = InputEventType::PowerOff;
        return true;
    }
    if (normalized == "STATUS") {
        event.type = InputEventType::StatusRequest;
        return true;
    }
    if (normalized == "LIGHT") {
        event.type = InputEventType::ModeLight;
        return true;
    }
    if (normalized == "WATER") {
        event.type = InputEventType::ModeWater;
        return true;
    }
    if (normalized == "WIND") {
        event.type = InputEventType::ModeWind;
        return true;
    }
    if (normalized == "KNOB_LEFT") {
        event.type = InputEventType::KnobLeft;
        return true;
    }
    if (normalized == "KNOB_RIGHT") {
        event.type = InputEventType::KnobRight;
        return true;
    }
    if ((normalized == "PRESS") || (normalized == "SELECT") || (normalized == "BUTTON")) {
        event.type = InputEventType::SelectPress;
        return true;
    }

    return false;
}
