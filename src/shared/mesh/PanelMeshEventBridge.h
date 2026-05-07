#pragma once

#include "shared/mesh/EspNowNetwork.h"

class AppController;

namespace mesh {
    class PanelMeshEventBridge : public EspNowNetwork::Listener {
    public:
        void setApplication(AppController *app);

        void onMeshMessageReceived(const uint8_t mac[6], const MeshMessage &message) override;

        void onMeshSendComplete(const uint8_t mac[6], bool success) override;

    private:
        AppController *app_ = nullptr;
    };
} // namespace mesh
