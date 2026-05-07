#include "app/PanelApplication.h"

PanelApplication::PanelApplication() : context_{
                                           panel_,
                                           knob_,
                                           button_,
                                           uart1_,
                                           inputRouter_,
                                           inputCallbacks_,
                                           registry_,
                                           meshBridge_,
                                           network_,
                                           app_,
                                       },
                                       inputProvider_(context_, kUartCommandBaudRate, kUart1RxPin, kUart1TxPin),
                                       displayProvider_(context_),
                                       meshProvider_(context_),
                                       appProvider_(context_) {
}

void PanelApplication::boot() {
    if (booted_) {
        return;
    }

    static const String kTitle = "Airsplit Veil Panel";

    inputProvider_.registerServices();
    Serial.println(kTitle + " start");

    displayProvider_.registerServices();
    meshProvider_.registerServices();
    appProvider_.registerServices();

    inputProvider_.boot();
    displayProvider_.boot();
    meshProvider_.boot();
    appProvider_.boot();

    Serial.println(kTitle + " ready");
    booted_ = true;
}

void PanelApplication::runOnce() {
    if (!booted_ || (app_ == nullptr)) {
        delay(10);
        return;
    }

    const uint32_t nowMs = millis();
    processInput(nowMs);
    if (network_ != nullptr) {
        network_->poll(nowMs);
    }
    app_->update(nowMs);
    app_->renderIfNeeded();
}

void PanelApplication::processInput(uint32_t nowMs) {
    inputRouter_.pollSerial(Serial, Serial, InputEventSource::Uart0);
    inputRouter_.pollSerial(uart1_, uart1_, InputEventSource::Uart1);

    InputEvent event{};
    while (inputRouter_.dequeue(event)) {
        if (event.type == InputEventType::StatusRequest) {
            app_->printStatus(Serial, nowMs);
            app_->printStatus(uart1_, nowMs);
            continue;
        }

        app_->handleEvent(event, nowMs);
    }
}
