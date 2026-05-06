#pragma once

#include <Arduino.h>
#include <stdint.h>

namespace mesh {
    constexpr uint8_t kProtocolVersion = 1;
    constexpr uint8_t kBroadcastNodeId = 0xFF;
    constexpr uint16_t kOnlineTimeoutMs = 15000;

    enum class NodeRole : uint8_t {
        Unknown = 0,
        Panel = 1,
        Fans = 2,
        Key = 3,
        Lights = 4,
        Any = 0xFF,
    };

    enum class MessageType : uint8_t {
        Cmd = 1,
        Event = 2,
        StatusReq = 3,
        StatusResp = 4,
        Ack = 5,
        Err = 6,
        Hello = 7,
    };

    enum class PayloadKind : uint8_t {
        None = 0,
        LightSet = 1,
        FansSet = 2,
        KeyEvent = 3,
        FansStatus = 4,
        LightStatus = 5,
    };

    enum class KeyCode : uint8_t {
        Unknown = 0,
        Power = 1,
        Water = 2,
        Light = 3,
        Wind = 4,
    };

    enum class KeyPressType : uint8_t {
        Unknown = 0,
        Short = 1,
        Long = 2,
    };

    enum class ErrorCode : uint8_t {
        None = 0,
        InvalidPayload = 1,
        InvalidTarget = 2,
        UnsupportedCommand = 3,
        Internal = 4,
    };

    struct LightSetPayload {
        uint8_t enabled = 0;
        uint8_t reserved[7] = {};
    };

    struct FansSetPayload {
        uint8_t enable = 0;
        uint8_t fan1Percent = 0;
        uint8_t fan2Percent = 0;
        uint8_t reserved[5] = {};
    };

    struct KeyEventPayload {
        uint8_t key = 0;
        uint8_t press = 0;
        uint8_t reserved[6] = {};
    };

    struct FansStatusPayload {
        uint8_t relayEnabled = 0;
        uint8_t fanEnabled = 0;
        uint8_t fan1Percent = 0;
        uint8_t fan2Percent = 0;
        uint16_t fan1Rpm = 0;
        uint16_t fan2Rpm = 0;
    };

    struct LightStatusPayload {
        uint8_t relayEnabled = 0;
        uint8_t lightEnabled = 0;
        uint8_t reserved[6] = {};
    };

    union MeshPayload {
        LightSetPayload lightSet;
        FansSetPayload fansSet;
        KeyEventPayload keyEvent;
        FansStatusPayload fansStatus;
        LightStatusPayload lightStatus;
        uint8_t raw[8];
    };

    struct MeshMessage {
        uint8_t protoVer = kProtocolVersion;
        uint8_t msgType = static_cast<uint8_t>(MessageType::Cmd);
        uint8_t payloadKind = static_cast<uint8_t>(PayloadKind::None);
        uint8_t flags = 0;
        uint8_t sourceRole = static_cast<uint8_t>(NodeRole::Unknown);
        uint8_t targetRole = static_cast<uint8_t>(NodeRole::Unknown);
        uint8_t sourceId = 0;
        uint8_t targetId = kBroadcastNodeId;
        uint32_t requestId = 0;
        uint32_t timestampMs = 0;
        MeshPayload payload = {};
        int32_t auxValue0 = 0;
        int32_t auxValue1 = 0;
    };

    static_assert(sizeof(MeshMessage) <= 32, "MeshMessage should remain compact");

    const char *roleToString(NodeRole role);

    const char *messageTypeToString(MessageType type);

    const char *payloadKindToString(PayloadKind kind);

    const char *keyCodeToString(KeyCode key);

    const char *keyPressTypeToString(KeyPressType press);

    String macToString(const uint8_t mac[6]);

    bool isZeroMac(const uint8_t mac[6]);

    MeshMessage makeBaseMessage(
        MessageType type,
        PayloadKind kind,
        NodeRole sourceRole,
        uint8_t sourceId,
        NodeRole targetRole,
        uint8_t targetId,
        uint32_t requestId
    );

    MeshMessage makeLightSetMessage(
        NodeRole sourceRole,
        uint8_t sourceId,
        NodeRole targetRole,
        uint8_t targetId,
        uint32_t requestId,
        bool enabled
    );

    MeshMessage makeFansSetMessage(
        NodeRole sourceRole,
        uint8_t sourceId,
        NodeRole targetRole,
        uint8_t targetId,
        uint32_t requestId,
        bool enabled,
        uint8_t fan1Percent,
        uint8_t fan2Percent
    );

    MeshMessage makeStatusRequestMessage(
        NodeRole sourceRole,
        uint8_t sourceId,
        NodeRole targetRole,
        uint8_t targetId,
        uint32_t requestId
    );

    MeshMessage makeKeyEventMessage(
        NodeRole sourceRole,
        uint8_t sourceId,
        uint32_t requestId,
        KeyCode key,
        KeyPressType press
    );

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
    );

    MeshMessage makeLightStatusMessage(
        NodeRole sourceRole,
        uint8_t sourceId,
        uint32_t requestId,
        bool relayEnabled,
        bool lightEnabled
    );

    MeshMessage makeAckMessage(
        NodeRole sourceRole,
        uint8_t sourceId,
        NodeRole targetRole,
        uint8_t targetId,
        uint32_t requestId
    );

    MeshMessage makeErrorMessage(
        NodeRole sourceRole,
        uint8_t sourceId,
        NodeRole targetRole,
        uint8_t targetId,
        uint32_t requestId,
        ErrorCode code
    );

    MeshMessage makeHelloMessage(
        NodeRole sourceRole,
        uint8_t sourceId,
        uint32_t requestId
    );

    bool targetsNode(const MeshMessage &message, NodeRole selfRole, uint8_t selfId);

    String describeMessage(const MeshMessage &message);
} // namespace mesh
