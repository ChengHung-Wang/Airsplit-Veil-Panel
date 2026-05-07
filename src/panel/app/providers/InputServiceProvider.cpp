#include "app/providers/InputServiceProvider.h"

InputServiceProvider::InputServiceProvider(
    PanelApplicationContext &context,
    uint32_t uartCommandBaudRate,
    int uart1RxPin,
    int uart1TxPin
) : context_(context),
    uartCommandBaudRate_(uartCommandBaudRate),
    uart1RxPin_(uart1RxPin),
    uart1TxPin_(uart1TxPin) {
}

void InputServiceProvider::registerServices() {
    Serial.begin(uartCommandBaudRate_);
    context_.uart1.begin(uartCommandBaudRate_, SERIAL_8N1, uart1RxPin_, uart1TxPin_);

    Serial.println("Initialize knob device");
    context_.knob = new ESP_Knob(GPIO_NUM_KNOB_PIN_A, GPIO_NUM_KNOB_PIN_B);
    context_.knob->begin();

    Serial.println("Initialize button device");
    context_.button = new Button(GPIO_BUTTON_PIN, false);

    context_.inputCallbacks.bind(context_.inputRouter);
    context_.inputCallbacks.attach(*context_.knob, *context_.button);
}

void InputServiceProvider::boot() {
}
