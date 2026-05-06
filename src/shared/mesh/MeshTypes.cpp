#include "shared/mesh/MeshTypes.h"

#include <stdio.h>
#include <string.h>

namespace mesh {
    namespace {
        template<typename T>
        uint8_t asByte(T value) {
            return static_cast<uint8_t>(value);
        }
    } // namespace

    const char *roleToString(NodeRole role) {
        switch (role) {
            case NodeRole::Panel:
                return "Panel";
            case NodeRole::Fans:
                return "Fans";
            case NodeRole::Key:
                return "Key";
            case NodeRole::Lights:
                return "Lights";
            case NodeRole::Any:
                return "Any";
            case NodeRole::Unknown:
            default:
                return "Unknown";
        }
    }

    const char *messageTypeToString(MessageType type) {
        switch (type) {
            case MessageType::Cmd:
                return "CMD";
            case MessageType::Event:
                return "EVENT";
            case MessageType::StatusReq:
                return "STATUS_REQ";
            case MessageType::StatusResp:
                return "STATUS_RESP";
            case MessageType::Ack:
                return "ACK";
            case MessageType::Err:
                return "ERR";
            case MessageType::Hello:
                return "HELLO";
            default:
                return "UNKNOWN";
        }
    }

    const char *payloadKindToString(PayloadKind kind) {
        switch (kind) {
            case PayloadKind::LightSet:
                return "LIGHT_SET";
            case PayloadKind::FansSet:
                return "FANS_SET";
            case PayloadKind::KeyEvent:
                return "KEY_EVENT";
            case PayloadKind::FansStatus:
                return "FANS_STATUS";
            case PayloadKind::LightStatus:
                return "LIGHT_STATUS";
            case PayloadKind::None:
            default:
                return "NONE";
        }
    }

    const char *keyCodeToString(KeyCode key) {
        switch (key) {
            case KeyCode::Power:
                return "POWER";
            case KeyCode::Water:
                return "WATER";
            case KeyCode::Light:
                return "LIGHT";
            case KeyCode::Wind:
                return "WIND";
            case KeyCode::Unknown:
            default:
                return "UNKNOWN";
        }
    }

    const char *keyPressTypeToString(KeyPressType press) {
        switch (press) {
            case KeyPressType::Short:
                return "SHORT";
            case KeyPressType::Long:
                return "LONG";
            case KeyPressType::Unknown:
            default:
                return "UNKNOWN";
        }
    }

    String macToString(const uint8_t mac[6]) {
        char buffer[18] = {};
        snprintf(
            buffer,
            sizeof(buffer),
            "%02X:%02X:%02X:%02X:%02X:%02X",
            mac[0],
            mac[1],
            mac[2],
            mac[3],
            mac[4],
            mac[5]
        );
        return String(buffer);
    }

    bool isZeroMac(const uint8_t mac[6]) {
        for (size_t i = 0; i < 6; ++i) {
            if (mac[i] != 0) {
                return false;
            }
        }
        return true;
    }

    MeshMessage makeBaseMessage(
        MessageType type,
        PayloadKind kind,
        NodeRole sourceRole,
        uint8_t sourceId,
        NodeRole targetRole,
        uint8_t targetId,
        uint32_t requestId
    ) {
        MeshMessage message{};
        message.protoVer = kProtocolVersion;
        message.msgType = asByte(type);
        message.payloadKind = asByte(kind);
        message.sourceRole = asByte(sourceRole);
        message.targetRole = asByte(targetRole);
        message.sourceId = sourceId;
        message.targetId = targetId;
        message.requestId = requestId;
        message.timestampMs = millis();
        return message;
    }

    MeshMessage makeLightSetMessage(
        NodeRole sourceRole,
        uint8_t sourceId,
        NodeRole targetRole,
        uint8_t targetId,
        uint32_t requestId,
        bool enabled
    ) {
        MeshMessage message = makeBaseMessage(
            MessageType::Cmd,
            PayloadKind::LightSet,
            sourceRole,
            sourceId,
            targetRole,
            targetId,
            requestId
        );
        message.payload.lightSet.enabled = enabled ? 1 : 0;
        return message;
    }

    MeshMessage makeFansSetMessage(
        NodeRole sourceRole,
        uint8_t sourceId,
        NodeRole targetRole,
        uint8_t targetId,
        uint32_t requestId,
        bool enabled,
        uint8_t fan1Percent,
        uint8_t fan2Percent
    ) {
        MeshMessage message = makeBaseMessage(
            MessageType::Cmd,
            PayloadKind::FansSet,
            sourceRole,
            sourceId,
            targetRole,
            targetId,
            requestId
        );
        message.payload.fansSet.enable = enabled ? 1 : 0;
        message.payload.fansSet.fan1Percent = constrain(fan1Percent, 0, 100);
        message.payload.fansSet.fan2Percent = constrain(fan2Percent, 0, 100);
        return message;
    }

    MeshMessage makeStatusRequestMessage(
        NodeRole sourceRole,
        uint8_t sourceId,
        NodeRole targetRole,
        uint8_t targetId,
        uint32_t requestId
    ) {
        return makeBaseMessage(
            MessageType::StatusReq,
            PayloadKind::None,
            sourceRole,
            sourceId,
            targetRole,
            targetId,
            requestId
        );
    }

    MeshMessage makeKeyEventMessage(
        NodeRole sourceRole,
        uint8_t sourceId,
        uint32_t requestId,
        KeyCode key,
        KeyPressType press
    ) {
        MeshMessage message = makeBaseMessage(
            MessageType::Event,
            PayloadKind::KeyEvent,
            sourceRole,
            sourceId,
            NodeRole::Panel,
            kBroadcastNodeId,
            requestId
        );
        message.payload.keyEvent.key = asByte(key);
        message.payload.keyEvent.press = asByte(press);
        return message;
    }

    MeshMessage makeFansStatusMessage(
        NodeRole sourceRole,
        uint8_t sourceId,
        uint32_t requestId,
        bool relayEnabled,
        bool fanEnabled,
        uint8_t fan1Percent,
        uint8_t fan2Percent,
        uint16_t fan1Rpm,
        uint16_t fan2Rpm
    ) {
        MeshMessage message = makeBaseMessage(
            MessageType::StatusResp,
            PayloadKind::FansStatus,
            sourceRole,
            sourceId,
            NodeRole::Panel,
            kBroadcastNodeId,
            requestId
        );
        message.payload.fansStatus.relayEnabled = relayEnabled ? 1 : 0;
        message.payload.fansStatus.fanEnabled = fanEnabled ? 1 : 0;
        message.payload.fansStatus.fan1Percent = fan1Percent;
        message.payload.fansStatus.fan2Percent = fan2Percent;
        message.payload.fansStatus.fan1Rpm = fan1Rpm;
        message.payload.fansStatus.fan2Rpm = fan2Rpm;
        return message;
    }

    MeshMessage makeLightStatusMessage(
        NodeRole sourceRole,
        uint8_t sourceId,
        uint32_t requestId,
        bool relayEnabled,
        bool lightEnabled
    ) {
        MeshMessage message = makeBaseMessage(
            MessageType::StatusResp,
            PayloadKind::LightStatus,
            sourceRole,
            sourceId,
            NodeRole::Panel,
            kBroadcastNodeId,
            requestId
        );
        message.payload.lightStatus.relayEnabled = relayEnabled ? 1 : 0;
        message.payload.lightStatus.lightEnabled = lightEnabled ? 1 : 0;
        return message;
    }

    MeshMessage makeAckMessage(
        NodeRole sourceRole,
        uint8_t sourceId,
        NodeRole targetRole,
        uint8_t targetId,
        uint32_t requestId
    ) {
        return makeBaseMessage(
            MessageType::Ack,
            PayloadKind::None,
            sourceRole,
            sourceId,
            targetRole,
            targetId,
            requestId
        );
    }

    MeshMessage makeErrorMessage(
        NodeRole sourceRole,
        uint8_t sourceId,
        NodeRole targetRole,
        uint8_t targetId,
        uint32_t requestId,
        ErrorCode code
    ) {
        MeshMessage message = makeBaseMessage(
            MessageType::Err,
            PayloadKind::None,
            sourceRole,
            sourceId,
            targetRole,
            targetId,
            requestId
        );
        message.auxValue0 = static_cast<int32_t>(code);
        return message;
    }

    MeshMessage makeHelloMessage(
        NodeRole sourceRole,
        uint8_t sourceId,
        uint32_t requestId
    ) {
        return makeBaseMessage(
            MessageType::Hello,
            PayloadKind::None,
            sourceRole,
            sourceId,
            NodeRole::Any,
            kBroadcastNodeId,
            requestId
        );
    }

    bool targetsNode(const MeshMessage &message, NodeRole selfRole, uint8_t selfId) {
        const NodeRole targetRole = static_cast<NodeRole>(message.targetRole);
        if ((targetRole != NodeRole::Any) && (targetRole != selfRole)) {
            return false;
        }
        return (message.targetId == kBroadcastNodeId) || (message.targetId == selfId);
    }

    String describeMessage(const MeshMessage &message) {
        String text;
        text.reserve(128);
        text += "ver=";
        text += String(message.protoVer);
        text += ",type=";
        text += messageTypeToString(static_cast<MessageType>(message.msgType));
        text += ",payload=";
        text += payloadKindToString(static_cast<PayloadKind>(message.payloadKind));
        text += ",src=";
        text += roleToString(static_cast<NodeRole>(message.sourceRole));
        text += "#";
        text += String(message.sourceId);
        text += ",dst=";
        text += roleToString(static_cast<NodeRole>(message.targetRole));
        text += "#";
        if (message.targetId == kBroadcastNodeId) {
            text += "ALL";
        } else {
            text += String(message.targetId);
        }
        text += ",req=";
        text += String(message.requestId);

        switch (static_cast<PayloadKind>(message.payloadKind)) {
            case PayloadKind::LightSet:
                text += ",enabled=";
                text += message.payload.lightSet.enabled ? "1" : "0";
                break;
            case PayloadKind::FansSet:
                text += ",enable=";
                text += message.payload.fansSet.enable ? "1" : "0";
                text += ",f1=";
                text += String(message.payload.fansSet.fan1Percent);
                text += ",f2=";
                text += String(message.payload.fansSet.fan2Percent);
                break;
            case PayloadKind::KeyEvent:
                text += ",key=";
                text += keyCodeToString(static_cast<KeyCode>(message.payload.keyEvent.key));
                text += ",press=";
                text += keyPressTypeToString(static_cast<KeyPressType>(message.payload.keyEvent.press));
                break;
            case PayloadKind::FansStatus:
                text += ",relay=";
                text += message.payload.fansStatus.relayEnabled ? "1" : "0";
                text += ",enable=";
                text += message.payload.fansStatus.fanEnabled ? "1" : "0";
                text += ",f1=";
                text += String(message.payload.fansStatus.fan1Percent);
                text += ",rpm1=";
                text += String(message.payload.fansStatus.fan1Rpm);
                text += ",f2=";
                text += String(message.payload.fansStatus.fan2Percent);
                text += ",rpm2=";
                text += String(message.payload.fansStatus.fan2Rpm);
                break;
            case PayloadKind::LightStatus:
                text += ",relay=";
                text += message.payload.lightStatus.relayEnabled ? "1" : "0";
                text += ",enabled=";
                text += message.payload.lightStatus.lightEnabled ? "1" : "0";
                break;
            case PayloadKind::None:
            default:
                break;
        }

        return text;
    }
} // namespace mesh
