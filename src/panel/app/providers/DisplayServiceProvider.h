#pragma once

#include "ServiceProvider.h"
#include "app/ApplicationContext.h"

class DisplayServiceProvider : public ServiceProvider {
public:
    explicit DisplayServiceProvider(PanelApplicationContext &context);

    void registerServices() override;

    void boot() override;

private:
    void initializeDisplayPowerPin();

    PanelApplicationContext &context_;
};
