#pragma once

#include <Arduino.h>

#include "shared/mesh/MeshRegistryConfig.h"

namespace mesh {

struct RegistryEntry {
    NodeRole roleHint = NodeRole::Unknown;
    uint8_t logicalIdHint = 0;
    uint8_t mac[6] = {};
    const char *label = "";
    bool configured = false;
    bool online = false;
    uint32_t lastSeenAtMs = 0;
    NodeRole reportedRole = NodeRole::Unknown;
    uint8_t reportedLogicalId = 0;
};

class MeshRegistry {
public:
    MeshRegistry();

    size_t size() const;
    const RegistryEntry *entryAt(size_t index) const;
    RegistryEntry *entryAt(size_t index);

    const RegistryEntry *findByMac(const uint8_t mac[6]) const;
    RegistryEntry *findByMac(const uint8_t mac[6]);

    void markSeen(const uint8_t mac[6], uint32_t nowMs);
    void updateIdentity(const uint8_t mac[6], NodeRole role, uint8_t logicalId, uint32_t nowMs);
    void refreshOnlineStates(uint32_t nowMs);

private:
    RegistryEntry entries_[kRegistrySeedCount] = {};
};

}  // namespace mesh
