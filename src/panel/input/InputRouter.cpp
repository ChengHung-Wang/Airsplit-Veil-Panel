#include "input/InputRouter.h"

bool InputRouter::enqueue(const InputEvent &event) {
    portENTER_CRITICAL(&queueMux_);
    const size_t nextTail = (tail_ + 1U) % kQueueSize;
    if (nextTail == head_) {
        portEXIT_CRITICAL(&queueMux_);
        return false;
    }

    queue_[tail_] = event;
    tail_ = nextTail;
    portEXIT_CRITICAL(&queueMux_);
    return true;
}

bool InputRouter::enqueue(InputEventType type) {
    return enqueue(InputEvent{type, InputEventSource::Local});
}

void InputRouter::emitKnobLeft() {
    enqueue(InputEventType::KnobLeft);
}

void InputRouter::emitKnobRight() {
    enqueue(InputEventType::KnobRight);
}

void InputRouter::emitSelectPress() {
    enqueue(InputEventType::SelectPress);
}

void InputRouter::emitPowerToggle() {
    enqueue(InputEventType::PowerToggle);
}

void InputRouter::emitPowerOff() {
    enqueue(InputEventType::PowerOff);
}

bool InputRouter::dequeue(InputEvent &event) {
    portENTER_CRITICAL(&queueMux_);
    if (head_ == tail_) {
        portEXIT_CRITICAL(&queueMux_);
        return false;
    }

    event = queue_[head_];
    head_ = (head_ + 1U) % kQueueSize;
    portEXIT_CRITICAL(&queueMux_);
    return true;
}

InputRouter::SerialState *InputRouter::getSerialState(InputEventSource source) {
    switch (source) {
        case InputEventSource::Uart0:
            return &serialStates_[0];
        case InputEventSource::Uart1:
            return &serialStates_[1];
        case InputEventSource::Local:
        default:
            return nullptr;
    }
}

void InputRouter::pollSerial(Stream &serial, Print &reply, InputEventSource source) {
    SerialState *state = getSerialState(source);
    if (state == nullptr) {
        return;
    }

    while (serial.available() > 0) {
        const char ch = static_cast<char>(serial.read());
        if ((ch == '\n') || (ch == '\r')) {
            parseLine(*state, reply, source);
            continue;
        }

        if (state->lineLength_ < (kLineBufferSize - 1U)) {
            state->lineBuffer_[state->lineLength_++] = ch;
            state->lineBuffer_[state->lineLength_] = '\0';
        }
    }
}

void InputRouter::parseLine(SerialState &state, Print &reply, InputEventSource source) {
    if (state.lineLength_ == 0U) {
        return;
    }

    InputEvent event{};
    if (parser_.parse(state.lineBuffer_, event)) {
        event.source = source;
        enqueue(event);
        reply.print("UART command accepted: ");
        reply.println(state.lineBuffer_);
    } else {
        reply.print("Unknown UART command: ");
        reply.println(state.lineBuffer_);
    }

    state.lineLength_ = 0;
    state.lineBuffer_[0] = '\0';
}
