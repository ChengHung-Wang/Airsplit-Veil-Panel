#pragma once

#include "shared/mesh/MeshTypes.h"

namespace mesh {
    struct RegistrySeedEntry {
        NodeRole role;
        uint8_t logicalId;
        uint8_t mac[6];
        const char *label;
    };
} // namespace mesh

#include "MeshSecrets.generated.h"
