#include "KeyNodeApp.h"

#include <driver/gpio.h>

namespace {
    constexpr uint8_t kNodeId = 1;
} // namespace

KeyNodeApp::KeyNodeApp(
    mesh::MeshRegistry &registry,
    gpio_num_t powerPin,
    gpio_num_t waterPin,
    gpio_num_t lightPin,
    gpio_num_t windPin,
    bool activeLevel,
    gpio_num_t statusLedPin
) : registry_(registry),
    network_(registry_, mesh::NodeRole::Key, kNodeId, *this),
    statusLed_(statusLedPin, true),
    powerPin_(powerPin),
    waterPin_(waterPin),
    lightPin_(lightPin),
    windPin_(windPin),
    activeLevel_(activeLevel) {
}

void KeyNodeApp::begin() {
    Serial.begin(115200);
    delay(200);
    statusLed_.begin();
    network_.begin();

    powerButton_ = new Button(powerPin_, activeLevel_);
    waterButton_ = new Button(waterPin_, activeLevel_);
    lightButton_ = new Button(lightPin_, activeLevel_);
    windButton_ = new Button(windPin_, activeLevel_);

    if (activeLevel_) {
        gpio_set_pull_mode(powerPin_, GPIO_PULLDOWN_ONLY);
        gpio_set_pull_mode(waterPin_, GPIO_PULLDOWN_ONLY);
        gpio_set_pull_mode(lightPin_, GPIO_PULLDOWN_ONLY);
        gpio_set_pull_mode(windPin_, GPIO_PULLDOWN_ONLY);
    }

    powerButton_->attachSingleClickEventCb(onPowerClick, this);
    powerButton_->attachLongPressStartEventCb(onPowerLongPress, this);
    powerButton_->attachPressDownEventCb(onPowerDown, this);
    powerButton_->attachPressUpEventCb(onPowerUp, this);
    waterButton_->attachSingleClickEventCb(onWaterClick, this);
    waterButton_->attachPressDownEventCb(onWaterDown, this);
    waterButton_->attachPressUpEventCb(onWaterUp, this);
    lightButton_->attachSingleClickEventCb(onLightClick, this);
    lightButton_->attachPressDownEventCb(onLightDown, this);
    lightButton_->attachPressUpEventCb(onLightUp, this);
    windButton_->attachSingleClickEventCb(onWindClick, this);
    windButton_->attachPressDownEventCb(onWindDown, this);
    windButton_->attachPressUpEventCb(onWindUp, this);
    announceIdentity();
}

void KeyNodeApp::loop(uint32_t nowMs) {
    network_.poll(nowMs);
    statusLed_.update(nowMs, anyButtonPressed(), isPanelConnected());
}

void KeyNodeApp::onMeshMessageReceived(const uint8_t mac[6], const mesh::MeshMessage &message) {
    if (!mesh::targetsNode(message, mesh::NodeRole::Key, kNodeId)) {
        return;
    }
    logMesh("RX", message);
    registry_.markSeen(mac, millis());
    notePanelLinked(message);
    if (static_cast<mesh::MessageType>(message.msgType) == mesh::MessageType::StatusReq) {
        const mesh::MeshMessage ack = mesh::makeAckMessage(
            mesh::NodeRole::Key,
            kNodeId,
            mesh::NodeRole::Panel,
            mesh::kBroadcastNodeId,
            message.requestId
        );
        logMesh("TX", ack);
        sendToPanel(ack);
    }
}

void KeyNodeApp::onMeshSendComplete(const uint8_t mac[6], bool success) {
    if (!success) {
        Serial.print("ESP-NOW send failed: ");
        Serial.println(mesh::macToString(mac));
    }
}

void KeyNodeApp::onPowerClick(void *, void *userData) {
    static_cast<KeyNodeApp *>(userData)->emitKey(mesh::KeyCode::Power, mesh::KeyPressType::Short);
}

void KeyNodeApp::onPowerLongPress(void *, void *userData) {
    static_cast<KeyNodeApp *>(userData)->emitKey(mesh::KeyCode::Power, mesh::KeyPressType::Long);
}

void KeyNodeApp::onPowerDown(void *, void *userData) {
    static_cast<KeyNodeApp *>(userData)->setButtonPressed(0, true);
}

void KeyNodeApp::onPowerUp(void *, void *userData) {
    static_cast<KeyNodeApp *>(userData)->setButtonPressed(0, false);
}

void KeyNodeApp::onWaterClick(void *, void *userData) {
    static_cast<KeyNodeApp *>(userData)->emitKey(mesh::KeyCode::Water, mesh::KeyPressType::Short);
}

void KeyNodeApp::onWaterDown(void *, void *userData) {
    static_cast<KeyNodeApp *>(userData)->setButtonPressed(1, true);
}

void KeyNodeApp::onWaterUp(void *, void *userData) {
    static_cast<KeyNodeApp *>(userData)->setButtonPressed(1, false);
}

void KeyNodeApp::onLightClick(void *, void *userData) {
    static_cast<KeyNodeApp *>(userData)->emitKey(mesh::KeyCode::Light, mesh::KeyPressType::Short);
}

void KeyNodeApp::onLightDown(void *, void *userData) {
    static_cast<KeyNodeApp *>(userData)->setButtonPressed(2, true);
}

void KeyNodeApp::onLightUp(void *, void *userData) {
    static_cast<KeyNodeApp *>(userData)->setButtonPressed(2, false);
}

void KeyNodeApp::onWindClick(void *, void *userData) {
    static_cast<KeyNodeApp *>(userData)->emitKey(mesh::KeyCode::Wind, mesh::KeyPressType::Short);
}

void KeyNodeApp::onWindDown(void *, void *userData) {
    static_cast<KeyNodeApp *>(userData)->setButtonPressed(3, true);
}

void KeyNodeApp::onWindUp(void *, void *userData) {
    static_cast<KeyNodeApp *>(userData)->setButtonPressed(3, false);
}

void KeyNodeApp::setButtonPressed(uint8_t index, bool pressed) {
    if (index >= 4) {
        return;
    }
    buttonPressed_[index] = pressed;
}

bool KeyNodeApp::anyButtonPressed() const {
    for (bool pressed: buttonPressed_) {
        if (pressed) {
            return true;
        }
    }
    return false;
}

void KeyNodeApp::emitKey(mesh::KeyCode key, mesh::KeyPressType press) {
    const mesh::MeshMessage message = mesh::makeKeyEventMessage(
        mesh::NodeRole::Key,
        kNodeId,
        nextRequestId(),
        key,
        press
    );
    logMesh("TX", message);
    sendToPanel(message);
}

void KeyNodeApp::announceIdentity() {
    if (panelLinked_) {
        return;
    }
    const mesh::MeshMessage hello = mesh::makeHelloMessage(mesh::NodeRole::Key, kNodeId, nextRequestId());
    logMesh("TX", hello);
    network_.sendToRole(mesh::NodeRole::Any, hello);
}

void KeyNodeApp::notePanelLinked(const mesh::MeshMessage &message) {
    if (static_cast<mesh::NodeRole>(message.sourceRole) == mesh::NodeRole::Panel) {
        panelLinked_ = true;
    }
}

bool KeyNodeApp::sendToPanel(const mesh::MeshMessage &message) {
    if (panelLinked_ &&network_.sendToNode(mesh::NodeRole::Panel, 1, message)) {
        return true;
    }
    return network_.sendToRole(mesh::NodeRole::Panel, message);
}

bool KeyNodeApp::isPanelConnected() const {
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

void KeyNodeApp::logMesh(const char *direction, const mesh::MeshMessage &message) {
    Serial.print("[MESH ");
    Serial.print(direction);
    Serial.print("] ");
    Serial.println(mesh::describeMessage(message));
}

uint32_t KeyNodeApp::nextRequestId() {
    return nextRequestId_++;
}
