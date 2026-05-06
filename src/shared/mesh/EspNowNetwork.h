#pragma once

#include <Arduino.h>
#include <esp_now.h>

#include "shared/mesh/MeshRegistry.h"

namespace mesh {
    class EspNowNetwork {
    public:
        class Listener {
        public:
            virtual ~Listener() = default;

            virtual void onMeshMessageReceived(const uint8_t mac[6], const MeshMessage &message) = 0;

            virtual void onMeshSendComplete(const uint8_t mac[6], bool success) = 0;
        };

        EspNowNetwork(MeshRegistry &registry, NodeRole selfRole, uint8_t selfId, Listener &listener);

        bool begin();

        bool sendToRole(NodeRole targetRole, const MeshMessage &message);

        bool sendToNode(NodeRole targetRole, uint8_t targetId, const MeshMessage &message);

        void poll(uint32_t nowMs);

        String selfMacString() const;

        bool isSelfMac(const uint8_t mac[6]) const;

    private:
        static void handleReceive(const esp_now_recv_info_t *recvInfo, const uint8_t *data, int len);

        static void handleSend(const wifi_tx_info_t *txInfo, esp_now_send_status_t status);

        void onReceiveInternal(const uint8_t mac[6], const uint8_t *data, int len);

        void onSendInternal(const uint8_t mac[6], esp_now_send_status_t status);

        bool addPeer(const RegistryEntry &entry);

        bool sendToMac(const uint8_t mac[6], const MeshMessage &message);

        MeshRegistry &registry_;
        NodeRole selfRole_;
        uint8_t selfId_ = 0;
        Listener &listener_;
        uint8_t selfMac_[6] = {};
        bool initialized_ = false;

        static EspNowNetwork *instance_;
    };
} // namespace mesh
