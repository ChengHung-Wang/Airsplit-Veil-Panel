#pragma once

#include "ServiceProvider.h"
#include "app/ApplicationContext.h"

class AppServiceProvider : public ServiceProvider {
public:
    explicit AppServiceProvider(PanelApplicationContext &context);

    void registerServices() override;

    void boot() override;

private:
    static constexpr mesh::NodeRole kSelfRole = mesh::NodeRole::Panel;
    static constexpr uint8_t kSelfId = 1;

    PanelApplicationContext &context_;
};
