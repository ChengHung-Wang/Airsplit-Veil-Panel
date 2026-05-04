#include "shared/mesh/MeshRegistry.h"

#include <string.h>

namespace mesh {

MeshRegistry::MeshRegistry()
{
    for (size_t i = 0; i < kRegistrySeedCount; ++i) {
        entries_[i].roleHint = kRegistrySeed[i].role;
        entries_[i].logicalIdHint = kRegistrySeed[i].logicalId;
        memcpy(entries_[i].mac, kRegistrySeed[i].mac, sizeof(entries_[i].mac));
        entries_[i].label = kRegistrySeed[i].label;
        entries_[i].configured = !isZeroMac(entries_[i].mac);
    }
}

size_t MeshRegistry::size() const
{
    return kRegistrySeedCount;
}

const RegistryEntry *MeshRegistry::entryAt(size_t index) const
{
    return index < size() ? &entries_[index] : nullptr;
}

RegistryEntry *MeshRegistry::entryAt(size_t index)
{
    return index < size() ? &entries_[index] : nullptr;
}

const RegistryEntry *MeshRegistry::findByMac(const uint8_t mac[6]) const
{
    for (const RegistryEntry &entry : entries_) {
        if (memcmp(entry.mac, mac, sizeof(entry.mac)) == 0) {
            return &entry;
        }
    }
    return nullptr;
}

RegistryEntry *MeshRegistry::findByMac(const uint8_t mac[6])
{
    return const_cast<RegistryEntry *>(
        static_cast<const MeshRegistry *>(this)->findByMac(mac)
    );
}

void MeshRegistry::markSeen(const uint8_t mac[6], uint32_t nowMs)
{
    RegistryEntry *entry = findByMac(mac);
    if (entry == nullptr) {
        return;
    }
    entry->lastSeenAtMs = nowMs;
    entry->online = true;
}

void MeshRegistry::updateIdentity(const uint8_t mac[6], NodeRole role, uint8_t logicalId, uint32_t nowMs)
{
    RegistryEntry *entry = findByMac(mac);
    if (entry == nullptr) {
        return;
    }
    entry->reportedRole = role;
    entry->reportedLogicalId = logicalId;
    entry->lastSeenAtMs = nowMs;
    entry->online = true;
}

void MeshRegistry::refreshOnlineStates(uint32_t nowMs)
{
    for (RegistryEntry &entry : entries_) {
        if (!entry.configured) {
            entry.online = false;
            continue;
        }
        entry.online = (entry.lastSeenAtMs != 0U) && ((nowMs - entry.lastSeenAtMs) <= kOnlineTimeoutMs);
    }
}

}  // namespace mesh
