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

    void pollSerial(Stream &serial, Print &reply, InputEventSource source);
    bool dequeue(InputEvent &event);

private:
    static constexpr size_t kQueueSize = 32;
    static constexpr size_t kLineBufferSize = 48;
    static constexpr size_t kSerialSourceCount = 2;

    struct SerialState {
        static constexpr size_t kBufferSize = 48;
        char lineBuffer_[kBufferSize] = {};
        size_t lineLength_ = 0;
    };

    bool enqueue(const InputEvent &event);
    bool enqueue(InputEventType type);
    SerialState *getSerialState(InputEventSource source);
    void parseLine(SerialState &state, Print &reply, InputEventSource source);

    InputEvent queue_[kQueueSize] = {};
    size_t head_ = 0;
    size_t tail_ = 0;
    SerialState serialStates_[kSerialSourceCount] = {};
    UartCommandParser parser_;
    portMUX_TYPE queueMux_ = portMUX_INITIALIZER_UNLOCKED;
};
