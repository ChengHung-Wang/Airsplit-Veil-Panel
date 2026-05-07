#include "app/providers/MeshServiceProvider.h"

#include "shared/mesh/EspNowNetwork.h"

MeshServiceProvider::MeshServiceProvider(PanelApplicationContext &context) : context_(context) {
}

void MeshServiceProvider::registerServices() {
    context_.network = new mesh::EspNowNetwork(context_.registry, kSelfRole, kSelfId, context_.meshBridge);
}

void MeshServiceProvider::boot() {
    Serial.println("Initialize ESP-NOW");
    if ((context_.network != nullptr) && !context_.network->begin()) {
        Serial.println("ESP-NOW init failed");
    }
}
