#pragma once

#include "ServiceProvider.h"
#include "app/ApplicationContext.h"

class InputServiceProvider : public ServiceProvider {
public:
    InputServiceProvider(
        PanelApplicationContext &context,
        uint32_t uartCommandBaudRate,
        int uart1RxPin,
        int uart1TxPin
    );

    void registerServices() override;

    void boot() override;

private:
    PanelApplicationContext &context_;
    uint32_t uartCommandBaudRate_ = 0;
    int uart1RxPin_ = -1;
    int uart1TxPin_ = -1;
};
