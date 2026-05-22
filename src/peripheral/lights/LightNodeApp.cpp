#include "LightNodeApp.h"

#include <driver/gpio.h>

namespace {
    constexpr uint8_t kNodeId = 1;
    constexpr uint64_t kGpioPinMaskBase = 1ULL;
    constexpr uint32_t kButtonDebounceMs = 35;
    constexpr bool kStatusLedActiveLow = true;
} // namespace

LightNodeApp::LightNodeApp(
    mesh::MeshRegistry &registry,
    gpio_num_t relayPin,
    gpio_num_t statusLedPin,
    gpio_num_t toggleButtonPin
)
    : registry_(registry),
      network_(registry_, mesh::NodeRole::Lights, kNodeId, *this),
      statusLed_(statusLedPin, kStatusLedActiveLow),
      relayPin_(relayPin),
      toggleButtonPin_(toggleButtonPin) {
}

void LightNodeApp::begin() {
    Serial.begin(115200);
    delay(200);

    initRelayPin();
    statusLed_.begin();
    initToggleButtonPin();
    network_.begin();
    announceIdentity();
    sendStatus(0);
}

void LightNodeApp::loop(uint32_t nowMs) {
    network_.poll(nowMs);
    updateToggleButton(nowMs);
    statusLed_.update(nowMs, enabled_, isPanelConnected());
}

void LightNodeApp::onMeshMessageReceived(const uint8_t mac[6], const mesh::MeshMessage &message) {
    if (!mesh::targetsNode(message, mesh::NodeRole::Lights, kNodeId)) {
        return;
    }
    logMesh("RX", message);
    registry_.markSeen(mac, millis());
    notePanelLinked(message);
    if (static_cast<mesh::MessageType>(message.msgType) == mesh::MessageType::StatusReq) {
        sendStatus(message.requestId);
        return;
    }

    if ((static_cast<mesh::MessageType>(message.msgType) == mesh::MessageType::Cmd) &&
        (static_cast<mesh::PayloadKind>(message.payloadKind) == mesh::PayloadKind::LightSet)) {
        setEnabled(message.payload.lightSet.enabled != 0);
        sendStatus(message.requestId);
    }
}

void LightNodeApp::onMeshSendComplete(const uint8_t mac[6], bool success) {
    if (!success) {
        Serial.print("ESP-NOW send failed: ");
        Serial.println(mesh::macToString(mac));
    }
}

void LightNodeApp::initRelayPin() {
    gpio_reset_pin(relayPin_);

    gpio_config_t config{};
    config.pin_bit_mask = kGpioPinMaskBase << static_cast<uint32_t>(relayPin_);
    config.mode = GPIO_MODE_INPUT_OUTPUT;
    config.pull_up_en = GPIO_PULLUP_DISABLE;
    config.pull_down_en = GPIO_PULLDOWN_ENABLE;
    config.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&config);

    gpio_set_level(relayPin_, 0);
    enabled_ = false;
    Serial.printf("[LIGHT GPIO] init pin=%d level=%d\r\n", relayPin_, gpio_get_level(relayPin_));
}

void LightNodeApp::initToggleButtonPin() {
    gpio_reset_pin(toggleButtonPin_);

    gpio_config_t config{};
    config.pin_bit_mask = kGpioPinMaskBase << static_cast<uint32_t>(toggleButtonPin_);
    config.mode = GPIO_MODE_INPUT;
    config.pull_up_en = GPIO_PULLUP_ENABLE;
    config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    config.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&config);

    buttonLastSampleLow_ = gpio_get_level(toggleButtonPin_) == 0;
    buttonStableLow_ = buttonLastSampleLow_;
    buttonLastChangedAtMs_ = millis();
    Serial.printf("[LIGHT BUTTON] init pin=%d active_low=%d\r\n", toggleButtonPin_, 1);
}

void LightNodeApp::updateToggleButton(uint32_t nowMs) {
    const bool sampleLow = gpio_get_level(toggleButtonPin_) == 0;
    if (sampleLow != buttonLastSampleLow_) {
        buttonLastSampleLow_ = sampleLow;
        buttonLastChangedAtMs_ = nowMs;
        return;
    }

    if ((sampleLow != buttonStableLow_) && ((nowMs - buttonLastChangedAtMs_) >= kButtonDebounceMs)) {
        buttonStableLow_ = sampleLow;
        if (buttonStableLow_) {
            setEnabled(!enabled_);
            sendStatus(0);
        }
    }
}

void LightNodeApp::setEnabled(bool enabled) {
    enabled_ = enabled;
    gpio_set_level(relayPin_, enabled_ ? 1 : 0);
    Serial.printf(
        "[LIGHT GPIO] set pin=%d target=%d level=%d\r\n",
        relayPin_,
        enabled_ ? 1 : 0,
        gpio_get_level(relayPin_));
}

void LightNodeApp::sendStatus(uint32_t requestId) {
    const mesh::MeshMessage message = mesh::makeLightStatusMessage(
        mesh::NodeRole::Lights,
        kNodeId,
        requestId == 0 ? nextRequestId() : requestId,
        enabled_,
        enabled_
    );
    logMesh("TX", message);
    sendToPanel(message);
}

void LightNodeApp::announceIdentity() {
    if (panelLinked_) {
        return;
    }
    const mesh::MeshMessage hello = mesh::makeHelloMessage(mesh::NodeRole::Lights, kNodeId, nextRequestId());
    logMesh("TX", hello);
    network_.sendToRole(mesh::NodeRole::Panel, hello);
}

void LightNodeApp::notePanelLinked(const mesh::MeshMessage &message) {
    if (static_cast<mesh::NodeRole>(message.sourceRole) == mesh::NodeRole::Panel) {
        panelLinked_ = true;
    }
}

bool LightNodeApp::sendToPanel(const mesh::MeshMessage &message) {
    if (panelLinked_ &&network_.sendToNode(mesh::NodeRole::Panel, 1, message))
    {
        return true;
    }
    return network_.sendToRole(mesh::NodeRole::Panel, message);
}

bool LightNodeApp::isPanelConnected() const {
    for (size_t i = 0; i < registry_.size(); ++i) {
        const mesh::RegistryEntry *entry = registry_.entryAt(i);
        if ((entry == nullptr) || !entry->configured || !entry->online) {
            continue;
        }
        const mesh::NodeRole effectiveRole =
            entry->reportedRole != mesh::NodeRole::Unknown ? entry->reportedRole : entry->roleHint;
        if (effectiveRole == mesh::NodeRole::Panel) {
            return true;
        }
    }
    return false;
}

uint32_t LightNodeApp::nextRequestId() {
    return nextRequestId_++;
}

void LightNodeApp::logMesh(const char *direction, const mesh::MeshMessage &message) {
    Serial.print("[MESH ");
    Serial.print(direction);
    Serial.print("] ");
    Serial.println(mesh::describeMessage(message));
}
