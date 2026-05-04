#include "shared/mesh/EspNowNetwork.h"

#include <WiFi.h>
#include <esp_mac.h>
#include <esp_wifi.h>
#include <string.h>

namespace mesh {

EspNowNetwork *EspNowNetwork::instance_ = nullptr;

EspNowNetwork::EspNowNetwork(MeshRegistry &registry, NodeRole selfRole, uint8_t selfId, Listener &listener)
    : registry_(registry), selfRole_(selfRole), selfId_(selfId), listener_(listener)
{
}

bool EspNowNetwork::begin()
{
    if (initialized_) {
        return true;
    }

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(50);

    if (esp_wifi_set_channel(kMeshChannel, WIFI_SECOND_CHAN_NONE) != ESP_OK) {
        return false;
    }
    if (esp_now_init() != ESP_OK) {
        return false;
    }
    if (esp_now_set_pmk(kMeshPmk) != ESP_OK) {
        return false;
    }

    instance_ = this;
    esp_now_register_recv_cb(handleReceive);
    esp_now_register_send_cb(handleSend);
    esp_read_mac(selfMac_, ESP_MAC_WIFI_STA);

    for (size_t i = 0; i < registry_.size(); ++i) {
        const RegistryEntry *entry = registry_.entryAt(i);
        if ((entry == nullptr) || !entry->configured) {
            continue;
        }
        if (memcmp(entry->mac, selfMac_, sizeof(selfMac_)) == 0) {
            continue;
        }
        addPeer(*entry);
    }

    initialized_ = true;
    return true;
}

bool EspNowNetwork::sendToRole(NodeRole targetRole, const MeshMessage &message)
{
    bool sent = false;
    (void)targetRole;
    for (size_t i = 0; i < registry_.size(); ++i) {
        const RegistryEntry *entry = registry_.entryAt(i);
        if ((entry == nullptr) || !entry->configured) {
            continue;
        }
        if (memcmp(entry->mac, selfMac_, sizeof(selfMac_)) == 0) {
            continue;
        }
        sent = sendToMac(entry->mac, message) || sent;
    }
    return sent;
}

bool EspNowNetwork::sendToNode(NodeRole targetRole, uint8_t targetId, const MeshMessage &message)
{
    for (size_t i = 0; i < registry_.size(); ++i) {
        const RegistryEntry *entry = registry_.entryAt(i);
        if ((entry == nullptr) || !entry->configured) {
            continue;
        }
        if (memcmp(entry->mac, selfMac_, sizeof(selfMac_)) == 0) {
            continue;
        }
        if ((entry->reportedRole == targetRole) && (entry->reportedLogicalId == targetId)) {
            return sendToMac(entry->mac, message);
        }
    }
    return false;
}

void EspNowNetwork::poll(uint32_t nowMs)
{
    registry_.refreshOnlineStates(nowMs);
}

String EspNowNetwork::selfMacString() const
{
    return macToString(selfMac_);
}

bool EspNowNetwork::isSelfMac(const uint8_t mac[6]) const
{
    return memcmp(selfMac_, mac, sizeof(selfMac_)) == 0;
}

void EspNowNetwork::handleReceive(const esp_now_recv_info_t *recvInfo, const uint8_t *data, int len)
{
    if ((instance_ == nullptr) || (recvInfo == nullptr) || (recvInfo->src_addr == nullptr)) {
        return;
    }
    instance_->onReceiveInternal(recvInfo->src_addr, data, len);
}

void EspNowNetwork::handleSend(const wifi_tx_info_t *txInfo, esp_now_send_status_t status)
{
    if ((instance_ == nullptr) || (txInfo == nullptr) || (txInfo->des_addr == nullptr)) {
        return;
    }
    instance_->onSendInternal(txInfo->des_addr, status);
}

void EspNowNetwork::onReceiveInternal(const uint8_t mac[6], const uint8_t *data, int len)
{
    if (isSelfMac(mac)) {
        return;
    }
    if (registry_.findByMac(mac) == nullptr) {
        return;
    }
    if ((data == nullptr) || (len != static_cast<int>(sizeof(MeshMessage)))) {
        return;
    }

    MeshMessage message{};
    memcpy(&message, data, sizeof(message));
    if (message.protoVer != kProtocolVersion) {
        return;
    }

    registry_.markSeen(mac, millis());
    registry_.updateIdentity(
        mac,
        static_cast<NodeRole>(message.sourceRole),
        message.sourceId,
        millis()
    );
    listener_.onMeshMessageReceived(mac, message);
}

void EspNowNetwork::onSendInternal(const uint8_t mac[6], esp_now_send_status_t status)
{
    listener_.onMeshSendComplete(mac, status == ESP_NOW_SEND_SUCCESS);
}

bool EspNowNetwork::addPeer(const RegistryEntry &entry)
{
    esp_now_peer_info_t peerInfo{};
    memcpy(peerInfo.peer_addr, entry.mac, sizeof(peerInfo.peer_addr));
    peerInfo.channel = kMeshChannel;
    peerInfo.ifidx = WIFI_IF_STA;
    peerInfo.encrypt = true;
    memcpy(peerInfo.lmk, kMeshLmk, sizeof(peerInfo.lmk));

    if (esp_now_is_peer_exist(entry.mac)) {
        return true;
    }
    return esp_now_add_peer(&peerInfo) == ESP_OK;
}

bool EspNowNetwork::sendToMac(const uint8_t mac[6], const MeshMessage &message)
{
    if (!initialized_) {
        return false;
    }
    return esp_now_send(mac, reinterpret_cast<const uint8_t *>(&message), sizeof(message)) == ESP_OK;
}

}  // namespace mesh
