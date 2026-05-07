#pragma once

#include <Button.h>
#include <ESP_Knob.h>

#include "input/InputRouter.h"

class InputCallbackRegistrar {
public:
    void bind(InputRouter &inputRouter);

    void attach(ESP_Knob &knob, Button &button);

private:
    void onKnobLeft();

    void onKnobRight();

    void onButtonSingleClick();

    void onButtonLongPressStart();

    static void handleKnobLeftEvent(int count, void *usrData);

    static void handleKnobRightEvent(int count, void *usrData);

    static void handleButtonSingleClick(void *buttonHandle, void *usrData);

    static void handleButtonLongPressStart(void *buttonHandle, void *usrData);

    static InputCallbackRegistrar *instance_;

    InputRouter *inputRouter_ = nullptr;
};
