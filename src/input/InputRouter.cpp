#include "input/InputRouter.h"

bool InputRouter::enqueue(InputEventType type)
{
    portENTER_CRITICAL(&queueMux_);
    const size_t nextTail = (tail_ + 1U) % kQueueSize;
    if (nextTail == head_) {
        portEXIT_CRITICAL(&queueMux_);
        return false;
    }

    queue_[tail_].type = type;
    tail_ = nextTail;
    portEXIT_CRITICAL(&queueMux_);
    return true;
}

void InputRouter::emitKnobLeft()
{
    enqueue(InputEventType::KnobLeft);
}

void InputRouter::emitKnobRight()
{
    enqueue(InputEventType::KnobRight);
}

void InputRouter::emitSelectPress()
{
    enqueue(InputEventType::SelectPress);
}

void InputRouter::emitPowerToggle()
{
    enqueue(InputEventType::PowerToggle);
}

void InputRouter::emitPowerOff()
{
    enqueue(InputEventType::PowerOff);
}

bool InputRouter::dequeue(InputEvent &event)
{
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

void InputRouter::pollSerial(Stream &serial, Print &log)
{
    while (serial.available() > 0) {
        const char ch = static_cast<char>(serial.read());
        if ((ch == '\n') || (ch == '\r')) {
            parseLine(log);
            continue;
        }

        if (lineLength_ < (kLineBufferSize - 1U)) {
            lineBuffer_[lineLength_++] = ch;
            lineBuffer_[lineLength_] = '\0';
        }
    }
}

void InputRouter::parseLine(Print &log)
{
    if (lineLength_ == 0U) {
        return;
    }

    InputEvent event{};
    if (parser_.parse(lineBuffer_, event)) {
        enqueue(event.type);
        log.print("UART command accepted: ");
        log.println(lineBuffer_);
    } else {
        log.print("Unknown UART command: ");
        log.println(lineBuffer_);
    }

    lineLength_ = 0;
    lineBuffer_[0] = '\0';
}
