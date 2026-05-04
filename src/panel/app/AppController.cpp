#include "AppController.h"

#include "config/lvgl_port_v8.h"

namespace {
constexpr uint32_t kBlinkIntervalMs = 500;
constexpr uint32_t kAdjustTimeoutMs = 5000;
constexpr uint32_t kSettingsCommitDelayMs = 1000;
constexpr uint32_t kAutoPowerOffDelayMs = 5000;
constexpr uint32_t kStatusRefreshMs = 3000;
constexpr uint32_t kRemoteKeyDebounceMs = 150;
constexpr uint32_t kMaxFanTimerSeconds = (99 * 60) + 59;
}

AppController::AppController(
    ESP_Panel *panel,
    mesh::MeshRegistry &registry,
    mesh::EspNowNetwork &network,
    mesh::NodeRole selfRole,
    uint8_t selfId
): panel_(panel),
   registry_(registry),
   network_(network),
   selfRole_(selfRole),
   selfId_(selfId)
{
}

void AppController::begin()
{
    store_.begin();
    state_.temperature = constrain(
        store_.loadTemperature(state_.temperature),
        AppState::kMinTemperature,
        AppState::kMaxTemperature
    );
    state_.fanTimerSeconds = max(
        store_.loadFanTimer(state_.fanTimerSeconds),
        AppState::kMinFanTimerSeconds
    );
    state_.lightEnabled = store_.loadLightEnabled(state_.lightEnabled);
    state_.fanRemainingSeconds = state_.fanTimerSeconds;
    state_.windAdjustCandidateSeconds = state_.fanTimerSeconds;
    state_.fanLevel = 1;

    applyDisplayPower(true);
    lvgl_port_lock(-1);
    view_.begin();
    view_.render(state_);
    lvgl_port_unlock();

    announceIdentity();
    syncOutputs(millis());
}

void AppController::handleEvent(const InputEvent &event, uint32_t nowMs)
{
    switch (event.type) {
    case InputEventType::PowerToggle:
        setPower(!state_.powerOn, nowMs);
        return;
    case InputEventType::PowerOff:
        setPower(false, nowMs);
        return;
    case InputEventType::StatusRequest:
        return;
    default:
        break;
    }

    if (!state_.powerOn) {
        return;
    }

    switch (event.type) {
    case InputEventType::KnobLeft:
        handleKnobDelta(-1, nowMs);
        break;
    case InputEventType::KnobRight:
        handleKnobDelta(1, nowMs);
        break;
    case InputEventType::SelectPress:
        handleSelectPress(nowMs);
        break;
    case InputEventType::ModeLight:
        if (state_.currentMode == AppMode::Light) {
            toggleLightState(nowMs);
        } else {
            enterMode(AppMode::Light, nowMs);
        }
        break;
    case InputEventType::ModeWater:
        if (state_.currentMode == AppMode::Water) {
            state_.currentMode = AppMode::Idle;
            state_.fanLevel = 0;
            sendFansCommand(true, 0, 0, nowMs);
            renderDirty_ = true;
        } else {
            enterMode(AppMode::Water, nowMs);
        }
        break;
    case InputEventType::ModeWind:
        if (state_.currentMode != AppMode::Wind) {
            enterMode(AppMode::Wind, nowMs);
        }
        break;
    case InputEventType::StatusRequest:
    case InputEventType::PowerToggle:
    case InputEventType::PowerOff:
        break;
    }
}

void AppController::handleMeshMessage(const uint8_t mac[6], const mesh::MeshMessage &message, uint32_t nowMs)
{
    if (!mesh::targetsNode(message, selfRole_, selfId_)) {
        return;
    }
    logMesh("RX", message);
    registry_.markSeen(mac, nowMs);
    storePeripheralMessage(mac, message, nowMs);

    switch (static_cast<mesh::PayloadKind>(message.payloadKind)) {
    case mesh::PayloadKind::KeyEvent:
        if (!shouldAcceptRemoteKey(nowMs)) {
            return;
        }
        lastAcceptedRemoteKeyAtMs_ = nowMs;
        switch (static_cast<mesh::KeyCode>(message.payload.keyEvent.key)) {
        case mesh::KeyCode::Power:
            if (static_cast<mesh::KeyPressType>(message.payload.keyEvent.press) == mesh::KeyPressType::Long) {
                handleEvent(InputEvent{InputEventType::PowerOff, InputEventSource::Local}, nowMs);
            } else {
                handleEvent(InputEvent{InputEventType::PowerToggle, InputEventSource::Local}, nowMs);
            }
            break;
        case mesh::KeyCode::Water:
            handleEvent(InputEvent{InputEventType::ModeWater, InputEventSource::Local}, nowMs);
            break;
        case mesh::KeyCode::Light:
            handleEvent(InputEvent{InputEventType::ModeLight, InputEventSource::Local}, nowMs);
            break;
        case mesh::KeyCode::Wind:
            handleEvent(InputEvent{InputEventType::ModeWind, InputEventSource::Local}, nowMs);
            break;
        case mesh::KeyCode::Unknown:
        default:
            break;
        }
        break;
    case mesh::PayloadKind::FansStatus:
    case mesh::PayloadKind::LightStatus:
    case mesh::PayloadKind::None:
    case mesh::PayloadKind::LightSet:
    case mesh::PayloadKind::FansSet:
    default:
        break;
    }

    if (static_cast<mesh::MessageType>(message.msgType) == mesh::MessageType::StatusReq) {
        announceIdentity();
    }
}

void AppController::handleMeshSendComplete(const uint8_t mac[6], bool success)
{
    if (!success) {
        // Serial.print("ESP-NOW send failed: ");
        // Serial.println(mesh::macToString(mac));
    }
}

void AppController::update(uint32_t nowMs)
{
    if (temperatureSavePending_ || fanTimerSavePending_ || lightSavePending_) {
        if ((nowMs - lastSettingsChangeAtMs_) >= kSettingsCommitDelayMs) {
            commitPendingSettings();
        }
    }

    registry_.refreshOnlineStates(nowMs);
    if ((nowMs - lastStatusBroadcastAtMs_) >= kStatusRefreshMs) {
        requestPeripheralStatus(nowMs);
        lastStatusBroadcastAtMs_ = nowMs;
    }

    if (!state_.powerOn) {
        return;
    }

    if (state_.currentMode == AppMode::Wind) {
        if (state_.windAdjusting) {
            if ((nowMs - lastBlinkToggleAtMs_) >= kBlinkIntervalMs) {
                state_.windAdjustmentBlinkOn = !state_.windAdjustmentBlinkOn;
                lastBlinkToggleAtMs_ = nowMs;
                renderDirty_ = true;
            }
            if ((nowMs - lastWindAdjustInputAtMs_) >= kAdjustTimeoutMs) {
                cancelWindAdjustment();
            }
        } else if (state_.fanRemainingSeconds > 0U) {
            while ((nowMs - lastWindTickAtMs_) >= 1000U) {
                lastWindTickAtMs_ += 1000U;
                if (state_.fanRemainingSeconds > 0U) {
                    --state_.fanRemainingSeconds;
                    renderDirty_ = true;
                }
                if (state_.fanRemainingSeconds == 0U) {
                    zeroReachedAtMs_ = nowMs;
                    break;
                }
            }
        } else if ((zeroReachedAtMs_ != 0U) && ((nowMs - zeroReachedAtMs_) >= kAutoPowerOffDelayMs)) {
            setPower(false, nowMs);
        }
    }
}

void AppController::renderIfNeeded()
{
    if (!renderDirty_) {
        return;
    }

    lvgl_port_lock(-1);
    view_.render(state_);
    lvgl_port_unlock();
    renderDirty_ = false;

    if (displayOffPending_) {
        applyDisplayPower(false);
        displayOffPending_ = false;
    }
}

void AppController::handleKnobDelta(int delta, uint32_t nowMs)
{
    if (state_.currentMode == AppMode::Wind) {
        startWindAdjustment(nowMs);
        const int nextCandidate = static_cast<int>(state_.windAdjustCandidateSeconds) + delta;
        state_.windAdjustCandidateSeconds = constrain(
            nextCandidate,
            static_cast<int>(AppState::kMinFanTimerSeconds),
            static_cast<int>(kMaxFanTimerSeconds)
        );
        state_.windAdjustmentBlinkOn = true;
        lastWindAdjustInputAtMs_ = nowMs;
        renderDirty_ = true;
        return;
    }

    const int nextTemperature = constrain(
        state_.temperature + delta,
        AppState::kMinTemperature,
        AppState::kMaxTemperature
    );
    if (nextTemperature != state_.temperature) {
        state_.temperature = nextTemperature;
        markSettingsDirty(true, false, false, nowMs);
        renderDirty_ = true;
    }
}

void AppController::handleSelectPress(uint32_t nowMs)
{
    if (state_.currentMode != AppMode::Wind) {
        return;
    }

    if (state_.windAdjusting) {
        confirmWindAdjustment(nowMs);
        return;
    }
}

void AppController::enterMode(AppMode mode, uint32_t nowMs)
{
    state_.currentMode = mode;
    state_.windAdjusting = false;
    state_.windAdjustmentBlinkOn = true;
    zeroReachedAtMs_ = 0;

    if (mode == AppMode::Wind) {
        state_.fanRemainingSeconds = state_.fanTimerSeconds;
        state_.windAdjustCandidateSeconds = state_.fanTimerSeconds;
        lastWindTickAtMs_ = nowMs;
        state_.fanLevel = 1;
    } else if (mode == AppMode::Water) {
        state_.fanLevel = 3;
    } else {
        state_.fanLevel = 0;
    }

    syncOutputs(nowMs);
    renderDirty_ = true;
}

void AppController::setPower(bool powerOn, uint32_t nowMs)
{
    if (state_.powerOn == powerOn) {
        return;
    }

    commitPendingSettings();
    state_.powerOn = powerOn;
    state_.windAdjusting = false;
    state_.windAdjustmentBlinkOn = true;
    zeroReachedAtMs_ = 0;

    if (powerOn) {
        applyDisplayPower(true);
        state_.currentMode = AppMode::Idle;
        state_.fanRemainingSeconds = state_.fanTimerSeconds;
        state_.windAdjustCandidateSeconds = state_.fanTimerSeconds;
        lastWindTickAtMs_ = nowMs;
        state_.fanLevel = 0;
    } else {
        state_.currentMode = AppMode::Idle;
        state_.fanLevel = 0;
        displayOffPending_ = true;
    }

    syncOutputs(nowMs);
    renderDirty_ = true;
}

void AppController::toggleLightState(uint32_t nowMs)
{
    state_.lightEnabled = !state_.lightEnabled;
    sendLightCommand(state_.lightEnabled, nowMs);
    markSettingsDirty(false, false, true, nowMs);
    renderDirty_ = true;
}

void AppController::startWindAdjustment(uint32_t nowMs)
{
    if (!state_.windAdjusting) {
        state_.windAdjusting = true;
        state_.windAdjustCandidateSeconds = max(state_.fanRemainingSeconds, AppState::kMinFanTimerSeconds);
        state_.windAdjustmentBlinkOn = true;
        lastBlinkToggleAtMs_ = nowMs;
    }
    lastWindAdjustInputAtMs_ = nowMs;
}

void AppController::confirmWindAdjustment(uint32_t nowMs)
{
    state_.windAdjusting = false;
    state_.windAdjustmentBlinkOn = true;
    state_.fanTimerSeconds = state_.windAdjustCandidateSeconds;
    state_.fanRemainingSeconds = state_.windAdjustCandidateSeconds;
    lastWindTickAtMs_ = nowMs;
    zeroReachedAtMs_ = 0;
    markSettingsDirty(false, true, false, nowMs);
    renderDirty_ = true;
}

void AppController::cancelWindAdjustment()
{
    state_.windAdjusting = false;
    state_.windAdjustmentBlinkOn = true;
    state_.windAdjustCandidateSeconds = max(state_.fanRemainingSeconds, AppState::kMinFanTimerSeconds);
    renderDirty_ = true;
}

void AppController::markSettingsDirty(bool temperatureChanged, bool fanTimerChanged, bool lightChanged, uint32_t nowMs)
{
    temperatureSavePending_ = temperatureSavePending_ || temperatureChanged;
    fanTimerSavePending_ = fanTimerSavePending_ || fanTimerChanged;
    lightSavePending_ = lightSavePending_ || lightChanged;
    lastSettingsChangeAtMs_ = nowMs;
}

void AppController::commitPendingSettings()
{
    if (temperatureSavePending_) {
        store_.saveTemperature(state_.temperature);
        temperatureSavePending_ = false;
    }
    if (fanTimerSavePending_) {
        store_.saveFanTimer(state_.fanTimerSeconds);
        fanTimerSavePending_ = false;
    }
    if (lightSavePending_) {
        store_.saveLightEnabled(state_.lightEnabled);
        lightSavePending_ = false;
    }
}

void AppController::applyDisplayPower(bool powerOn)
{
    if (panel_ == nullptr) {
        return;
    }

    if (powerOn) {
        if (panel_->getLcd() != nullptr) {
            panel_->getLcd()->displayOn();
        }
        if (panel_->getBacklight() != nullptr) {
            panel_->getBacklight()->on();
        }
    } else {
        if (panel_->getBacklight() != nullptr) {
            panel_->getBacklight()->off();
        }
        if (panel_->getLcd() != nullptr) {
            panel_->getLcd()->displayOff();
        }
    }
}

void AppController::syncOutputs(uint32_t nowMs)
{
    if (!state_.powerOn) {
        sendLightCommand(false, nowMs);
        sendFansCommand(false, 0, 0, nowMs);
        return;
    }

    sendLightCommand(state_.lightEnabled, nowMs);

    switch (state_.currentMode) {
    case AppMode::Idle:
    case AppMode::Light:
        sendFansCommand(true, 15, 15, nowMs);
        break;
    case AppMode::Water:
        sendFansCommand(true, 100, 100, nowMs);
        break;
    case AppMode::Wind:
        sendFansCommand(true, 35, 35, nowMs);
        break;
    }
}

void AppController::sendFansCommand(bool enable, uint8_t fan1Percent, uint8_t fan2Percent, uint32_t nowMs)
{
    const mesh::MeshMessage message = mesh::makeFansSetMessage(
        selfRole_,
        selfId_,
        mesh::NodeRole::Fans,
        mesh::kBroadcastNodeId,
        nextRequestId(),
        enable,
        fan1Percent,
        fan2Percent
    );
    logMesh("TX", message);
    network_.sendToRole(mesh::NodeRole::Fans, message);
    lastStatusBroadcastAtMs_ = nowMs;
}

void AppController::sendLightCommand(bool enabled, uint32_t nowMs)
{
    const mesh::MeshMessage message = mesh::makeLightSetMessage(
        selfRole_,
        selfId_,
        mesh::NodeRole::Lights,
        mesh::kBroadcastNodeId,
        nextRequestId(),
        enabled
    );
    logMesh("TX", message);
    network_.sendToRole(mesh::NodeRole::Lights, message);
    lastStatusBroadcastAtMs_ = nowMs;
}

void AppController::requestPeripheralStatus(uint32_t nowMs)
{
    const mesh::MeshMessage fansReq = mesh::makeStatusRequestMessage(
        selfRole_, selfId_, mesh::NodeRole::Fans, mesh::kBroadcastNodeId, nextRequestId()
    );
    const mesh::MeshMessage lightsReq = mesh::makeStatusRequestMessage(
        selfRole_, selfId_, mesh::NodeRole::Lights, mesh::kBroadcastNodeId, nextRequestId()
    );
    const mesh::MeshMessage keyReq = mesh::makeStatusRequestMessage(
        selfRole_, selfId_, mesh::NodeRole::Key, mesh::kBroadcastNodeId, nextRequestId()
    );
    logMesh("TX", fansReq);
    network_.sendToRole(mesh::NodeRole::Fans, fansReq);
    logMesh("TX", lightsReq);
    network_.sendToRole(mesh::NodeRole::Lights, lightsReq);
    logMesh("TX", keyReq);
    network_.sendToRole(mesh::NodeRole::Key, keyReq);
    lastStatusBroadcastAtMs_ = nowMs;
}

void AppController::announceIdentity()
{
    const mesh::MeshMessage hello = mesh::makeHelloMessage(selfRole_, selfId_, nextRequestId());
    logMesh("TX", hello);
    network_.sendToRole(mesh::NodeRole::Any, hello);
}

bool AppController::shouldAcceptRemoteKey(uint32_t nowMs)
{
    return (lastAcceptedRemoteKeyAtMs_ == 0U) || ((nowMs - lastAcceptedRemoteKeyAtMs_) > kRemoteKeyDebounceMs);
}

void AppController::storePeripheralMessage(const uint8_t mac[6], const mesh::MeshMessage &message, uint32_t nowMs)
{
    const mesh::RegistryEntry *entry = registry_.findByMac(mac);
    if (entry == nullptr) {
        return;
    }

    for (size_t i = 0; i < registry_.size(); ++i) {
        const mesh::RegistryEntry *candidate = registry_.entryAt(i);
        if ((candidate == nullptr) || (candidate != entry)) {
            continue;
        }
        snapshots_[i].seen = true;
        snapshots_[i].lastMessage = message;
        snapshots_[i].updatedAtMs = nowMs;
        return;
    }
}

String AppController::summarizePeripheral(const mesh::RegistryEntry &entry) const
{
    for (size_t i = 0; i < registry_.size(); ++i) {
        const mesh::RegistryEntry *candidate = registry_.entryAt(i);
        if ((candidate == nullptr) || (candidate != &entry) || !snapshots_[i].seen) {
            continue;
        }
        return mesh::describeMessage(snapshots_[i].lastMessage);
    }
    return String("no-status");
}

uint32_t AppController::nextRequestId()
{
    return nextRequestId_++;
}

void AppController::logMesh(const char *direction, const mesh::MeshMessage &message)
{
    Serial.print("[MESH ");
    Serial.print(direction);
    Serial.print("] ");
    Serial.println(mesh::describeMessage(message));
}

void AppController::printStatus(Print &out, uint32_t nowMs)
{
    registry_.refreshOnlineStates(nowMs);
    out.println("STATUS BEGIN");
    out.print("panel.mac=");
    out.println(network_.selfMacString());
    out.print("panel.power=");
    out.println(state_.powerOn ? "ON" : "OFF");
    out.print("panel.mode=");
    switch (state_.currentMode) {
    case AppMode::Idle:
        out.println("Idle");
        break;
    case AppMode::Light:
        out.println("Light");
        break;
    case AppMode::Water:
        out.println("Water");
        break;
    case AppMode::Wind:
        out.println("Wind");
        break;
    }
    out.print("panel.temperature=");
    out.println(state_.temperature);
    out.print("panel.fanTimer=");
    out.println(state_.fanTimerSeconds);
    out.print("panel.lightEnabled=");
    out.println(state_.lightEnabled ? "1" : "0");

    for (size_t i = 0; i < registry_.size(); ++i) {
        const mesh::RegistryEntry *entry = registry_.entryAt(i);
        if (entry == nullptr) {
            continue;
        }
        out.print("device=");
        out.print(entry->label);
        out.print(",mac=");
        out.print(mesh::macToString(entry->mac));
        out.print(",configured=");
        out.print(entry->configured ? "1" : "0");
        out.print(",online=");
        out.print(entry->online ? "1" : "0");
        out.print(",reported=");
        out.print(mesh::roleToString(entry->reportedRole));
        out.print("#");
        out.print(entry->reportedLogicalId);
        out.print(",summary=");
        out.println(summarizePeripheral(*entry));
    }
    out.println("STATUS END");
}
