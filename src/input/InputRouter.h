#pragma once

#include <Arduino.h>

#include "input/InputEvent.h"
#include "input/UartCommandParser.h"

class InputRouter {
public:
    void emitKnobLeft();
    void emitKnobRight();
    void emitSelectPress();
    void emitPowerToggle();
    void emitPowerOff();

    void pollSerial(Stream &serial, Print &log);
    bool dequeue(InputEvent &event);

private:
    static constexpr size_t kQueueSize = 32;
    static constexpr size_t kLineBufferSize = 48;

    bool enqueue(InputEventType type);
    void parseLine(Print &log);

    InputEvent queue_[kQueueSize] = {};
    size_t head_ = 0;
    size_t tail_ = 0;
    char lineBuffer_[kLineBufferSize] = {};
    size_t lineLength_ = 0;
    UartCommandParser parser_;
    portMUX_TYPE queueMux_ = portMUX_INITIALIZER_UNLOCKED;
};
