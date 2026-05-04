#pragma once

#include "shared/mesh/MeshTypes.h"

namespace mesh {

struct RegistrySeedEntry {
    NodeRole role;
    uint8_t logicalId;
    uint8_t mac[6];
    const char *label;
};

constexpr uint8_t kMeshChannel = 1;
constexpr uint8_t kMeshPmk[16] = {
    'A', 'i', 'r', 'S', 'p', 'l', 'i', 't',
    'V', 'e', 'i', 'l', 'M', 'e', 's', 'h'
};
constexpr uint8_t kMeshLmk[16] = {
    'V', 'e', 'i', 'l', 'N', 'o', 'd', 'e',
    'L', 'i', 'n', 'k', 'K', 'e', 'y', '1'
};

// ESP-NOW uses the STA MAC address, not SoftAP/BLE/Ethernet MACs.
constexpr RegistrySeedEntry kRegistrySeed[] = {
    {NodeRole::Panel, 0, {0x10, 0x51, 0xDB, 0x8E, 0xE0, 0x3C}, "Node-01"},
    {NodeRole::Unknown, 0, {0x14, 0x63, 0x93, 0xC7, 0xEF, 0x8C}, "Node-02"},
    {NodeRole::Unknown, 0, {0x14, 0x63, 0x93, 0xC7, 0xFB, 0x3C}, "Node-03"},
    {NodeRole::Unknown, 0, {0x90, 0x70, 0x69, 0xC3, 0x8A, 0x80}, "Node-04"},
    {NodeRole::Unknown, 0, {0x14, 0x63, 0x93, 0xC7, 0xDE, 0x18}, "Node-05"},
    {NodeRole::Unknown, 0, {0xE4, 0xB3, 0x23, 0xB5, 0xA7, 0xC0}, "Node-06"},
    {NodeRole::Fans, 0, {0x98, 0x88, 0xE0, 0xD9, 0x22, 0x30}, "Node-07"},
};

constexpr size_t kRegistrySeedCount = sizeof(kRegistrySeed) / sizeof(kRegistrySeed[0]);

}  // namespace mesh
