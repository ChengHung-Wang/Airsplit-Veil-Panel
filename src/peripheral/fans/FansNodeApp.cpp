#include "FansNodeApp.h"

namespace {
    constexpr uint8_t kNodeId = 1;
    constexpr uint8_t kPulsesPerRev = 2;
} // namespace

FansNodeApp::FansNodeApp(
    FanController &fan1,
    FanController &fan2,
    RelayController &relay,
    mesh::MeshRegistry &registry
) : fan1_(fan1),
    fan2_(fan2),
    relay_(relay),
    registry_(registry),
    ble_(*this),
    network_(registry_, mesh::NodeRole::Fans, kNodeId, *this) {
}

void FansNodeApp::begin() {
    Serial.begin(115200);
    delay(200);

    fan1_.begin();
    fan2_.begin();
    relay_.begin();
    ble_.begin("AirSplit Veil Fan Controller");
    network_.begin();
    announceIdentity();
    printHelp();
    sendStatus(0);
}

void FansNodeApp::loop(uint32_t nowMs) {
    handleSerial();
    handleRpmUpdate(nowMs);
    network_.poll(nowMs);
}

void FansNodeApp::onFan1Pulse() {
    fan1_.onPulse();
}

void FansNodeApp::onFan2Pulse() {
    fan2_.onPulse();
}

void FansNodeApp::onMeshMessageReceived(const uint8_t mac[6], const mesh::MeshMessage &message) {
    if (!mesh::targetsNode(message, mesh::NodeRole::Fans, kNodeId)) {
        return;
    }
    logMesh("RX", message, true);
    registry_.markSeen(mac, millis());
    notePanelLinked(message);
    switch (static_cast<mesh::MessageType>(message.msgType)) {
        case mesh::MessageType::Hello:
            break;
        case mesh::MessageType::Cmd:
            if (static_cast<mesh::PayloadKind>(message.payloadKind) == mesh::PayloadKind::FansSet) {
                applyFansState(
                    message.payload.fansSet.enable != 0,
                    message.payload.fansSet.fan1Percent,
                    message.payload.fansSet.fan2Percent
                );
                sendStatus(message.requestId);
            }
            break;
        case mesh::MessageType::StatusReq:
            sendStatus(message.requestId);
            break;
        default:
            break;
    }
}

void FansNodeApp::onMeshSendComplete(const uint8_t mac[6], bool success) {
    if (!success) {
        Serial.print("ESP-NOW send failed: ");
        Serial.println(mesh::macToString(mac));
    }
}

void FansNodeApp::onBleCommand(const String &command) {
    processCommand(command, false);
}

void FansNodeApp::onBleConnectionChanged(bool) {
}

void FansNodeApp::processCommand(String command, bool fromMesh) {
    command.trim();
    command.toUpperCase();
    if (command.isEmpty()) {
        return;
    }

    if (command == "HELP") {
        printHelp();
        return;
    }
    if (command == "STATUS") {
        Serial.println(statusString());
        ble_.notifyLine(statusString());
        return;
    }
    if ((command == "RELAY=1") || (command == "ENABLE")) {
        applyFansState(true, fan1_.dutyPercent(), fan2_.dutyPercent());
        sendStatus(nextRequestId());
        return;
    }
    if ((command == "RELAY=0") || (command == "DISABLE")) {
        applyFansState(false, fan1_.dutyPercent(), fan2_.dutyPercent());
        sendStatus(nextRequestId());
        return;
    }
    if (command.startsWith("F1=")) {
        applyFansState(relay_.isEnabled(), command.substring(3).toInt(), fan2_.dutyPercent());
        sendStatus(nextRequestId());
        return;
    }
    if (command.startsWith("F2=")) {
        applyFansState(relay_.isEnabled(), fan1_.dutyPercent(), command.substring(3).toInt());
        sendStatus(nextRequestId());
        return;
    }
    if (command.startsWith("ALL=")) {
        const int value = command.substring(4).toInt();
        applyFansState(relay_.isEnabled(), value, value);
        sendStatus(nextRequestId());
        return;
    }
    if (command == "STOP") {
        applyFansState(relay_.isEnabled(), 0, 0);
        sendStatus(nextRequestId());
        return;
    }

    if (!fromMesh) {
        Serial.println("ERR Unknown command: " + command);
        ble_.notifyLine("ERR Unknown command: " + command);
    }
}

void FansNodeApp::applyFansState(bool relayEnabled, int fan1Percent, int fan2Percent) {
    fan1_.writeDuty(fan1Percent);
    fan2_.writeDuty(fan2Percent);
    if (relayEnabled) {
        relay_.enable();
    } else {
        relay_.disable();
    }
}

void FansNodeApp::handleSerial() {
    if (!Serial.available()) {
        return;
    }
    processCommand(Serial.readStringUntil('\n'), false);
}

void FansNodeApp::handleRpmUpdate(uint32_t nowMs) {
    if ((nowMs - lastRpmMs_) < 1000U) {
        return;
    }
    fan1_.updateRpmEverySecond(kPulsesPerRev);
    fan2_.updateRpmEverySecond(kPulsesPerRev);
    lastRpmMs_ = nowMs;
}

void FansNodeApp::sendStatus(uint32_t requestId) {
    const mesh::MeshMessage message = mesh::makeFansStatusMessage(
        mesh::NodeRole::Fans,
        kNodeId,
        requestId == 0 ? nextRequestId() : requestId,
        relay_.isEnabled(),
        relay_.isEnabled(),
        fan1_.dutyPercent(),
        fan2_.dutyPercent(),
        fan1_.rpm(),
        fan2_.rpm()
    );
    logMesh("TX", message, true);
    sendToPanel(message);
    const String status = statusString();
    Serial.println(status);
    ble_.notifyLine(status);
}

void FansNodeApp::announceIdentity() {
    if (panelLinked_) {
        return;
    }
    const mesh::MeshMessage hello = mesh::makeHelloMessage(mesh::NodeRole::Fans, kNodeId, nextRequestId());
    logMesh("TX", hello, true);
    network_.sendToRole(mesh::NodeRole::Any, hello);
}

void FansNodeApp::notePanelLinked(const mesh::MeshMessage &message) {
    if (static_cast<mesh::NodeRole>(message.sourceRole) == mesh::NodeRole::Panel) {
        panelLinked_ = true;
    }
}

bool FansNodeApp::sendToPanel(const mesh::MeshMessage &message) {
    if (panelLinked_ && network_.sendToNode(mesh::NodeRole::Panel, 1, message)) {
        return true;
    }
    return network_.sendToRole(mesh::NodeRole::Panel, message);
}

String FansNodeApp::statusString() const {
    String s;
    s += "ROLE=Fans,ID=1,RELAY=";
    s += relay_.isEnabled() ? "ON" : "OFF";
    s += ",ENABLE=";
    s += relay_.isEnabled() ? "1" : "0";
    s += ",F1=" + String(fan1_.dutyPercent()) + "%";
    s += ",RPM1=" + String(fan1_.rpm());
    s += ",F2=" + String(fan2_.dutyPercent()) + "%";
    s += ",RPM2=" + String(fan2_.rpm());
    return s;
}

void FansNodeApp::printHelp() {
    Serial.println("ENABLE|DISABLE|F1=0~100|F2=0~100|ALL=0~100|STOP|STATUS|HELP");
}

uint32_t FansNodeApp::nextRequestId() {
    return nextRequestId_++;
}

void FansNodeApp::logMesh(const char *direction, const mesh::MeshMessage &message, bool mirrorToBle) {
    const String line = String("[MESH ") + direction + "] " + mesh::describeMessage(message);
    Serial.println(line);
    if (mirrorToBle) {
        ble_.notifyLine(line);
    }
}
