#include "input/InputCallbackRegistrar.h"

InputCallbackRegistrar *InputCallbackRegistrar::instance_ = nullptr;

void InputCallbackRegistrar::bind(InputRouter &inputRouter) {
    inputRouter_ = &inputRouter;
    instance_ = this;
}

void InputCallbackRegistrar::attach(ESP_Knob &knob, Button &button) {
    knob.attachLeftEventCallback(&InputCallbackRegistrar::handleKnobLeftEvent);
    knob.attachRightEventCallback(&InputCallbackRegistrar::handleKnobRightEvent);
    button.attachSingleClickEventCb(&InputCallbackRegistrar::handleButtonSingleClick, nullptr);
    button.attachLongPressStartEventCb(&InputCallbackRegistrar::handleButtonLongPressStart, nullptr);
}

void InputCallbackRegistrar::onKnobLeft() {
    if (inputRouter_ != nullptr) {
        inputRouter_->emitKnobRight();
    }
}

void InputCallbackRegistrar::onKnobRight() {
    if (inputRouter_ != nullptr) {
        inputRouter_->emitKnobLeft();
    }
}

void InputCallbackRegistrar::onButtonSingleClick() {
    if (inputRouter_ != nullptr) {
        inputRouter_->emitSelectPress();
    }
}

void InputCallbackRegistrar::onButtonLongPressStart() {
    if (inputRouter_ != nullptr) {
        inputRouter_->emitPowerOff();
    }
}

void InputCallbackRegistrar::handleKnobLeftEvent(int count, void *usrData) {
    (void) count;
    (void) usrData;
    if (instance_ != nullptr) {
        instance_->onKnobLeft();
    }
}

void InputCallbackRegistrar::handleKnobRightEvent(int count, void *usrData) {
    (void) count;
    (void) usrData;
    if (instance_ != nullptr) {
        instance_->onKnobRight();
    }
}

void InputCallbackRegistrar::handleButtonSingleClick(void *buttonHandle, void *usrData) {
    (void) buttonHandle;
    (void) usrData;
    if (instance_ != nullptr) {
        instance_->onButtonSingleClick();
    }
}

void InputCallbackRegistrar::handleButtonLongPressStart(void *buttonHandle, void *usrData) {
    (void) buttonHandle;
    (void) usrData;
    if (instance_ != nullptr) {
        instance_->onButtonLongPressStart();
    }
}
