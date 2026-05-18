#include "mesh/PanelMeshEventBridge.h"

#include "app/AppController.h"

namespace mesh {
    void PanelMeshEventBridge::setApplication(AppController *app) {
        app_ = app;
    }

    void PanelMeshEventBridge::onMeshMessageReceived(const uint8_t mac[6], const MeshMessage &message) {
        if (app_ != nullptr) {
            app_->handleMeshMessage(mac, message, millis());
        }
    }

    void PanelMeshEventBridge::onMeshSendComplete(const uint8_t mac[6], bool success) {
        if (app_ != nullptr) {
            app_->handleMeshSendComplete(mac, success);
        }
    }
} // namespace mesh
