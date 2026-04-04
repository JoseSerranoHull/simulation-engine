#pragma once

/* parasoft-begin-suppress ALL */
#include <cstdint>
#include <glm/glm.hpp>
/* parasoft-end-suppress ALL */

namespace GE::Networking::Packets {

    // -------------------------------------------------------------------------
    // Packet type tags
    // -------------------------------------------------------------------------
    enum class PacketType : uint8_t {
        Heartbeat    = 0,
        StateUpdate  = 1,
        SceneChange  = 2,
        SpawnObject  = 3,
    };

    // -------------------------------------------------------------------------
    // Header — 4 bytes: type(1) + senderId(1) + sequence(2)
    // -------------------------------------------------------------------------
    #pragma pack(push, 1)
    struct Header {
        PacketType type     { PacketType::Heartbeat };
        uint8_t    senderId { 0 };
        uint16_t   sequence { 0 };
    };
    static_assert(sizeof(Header) == 4, "Header must be exactly 4 bytes");

    // -------------------------------------------------------------------------
    // StateUpdate — per-entity authoritative physics state from remote owner
    // -------------------------------------------------------------------------
    struct StateUpdate {
        Header    header          {};
        uint32_t  entityId        { 0 };
        glm::vec3 position        { 0.0f };
        // Quaternion stored as x,y,z,w raw floats — no gtc/quaternion.hpp needed here
        float     orientation[4]  { 0.0f, 0.0f, 0.0f, 1.0f };  ///< x y z w
        glm::vec3 linearVelocity  { 0.0f };
        glm::vec3 angularVelocity { 0.0f };
    };

    // -------------------------------------------------------------------------
    // SceneChange — instructs a peer to load a new scene path
    // -------------------------------------------------------------------------
    struct SceneChange {
        Header header           {};
        char   scenePath[128]   {};
    };

    // -------------------------------------------------------------------------
    // SpawnObject — request to all peers to instantiate a physics object
    // -------------------------------------------------------------------------
    struct SpawnObject {
        Header     header          {};
        uint32_t   entityId        { 0 };
        uint8_t    ownerPeerId     { 0 };
        uint8_t    shapeType       { 0 };  ///< 0 = sphere, 1 = box, 2 = capsule
        uint8_t    _pad[2]         {};
        glm::vec3  position        { 0.0f };
        glm::vec3  scale           { 1.0f };
        glm::vec3  linearVelocity  { 0.0f };
        float      mass            { 1.0f };
    };

    // -------------------------------------------------------------------------
    // Heartbeat — keep-alive, no payload beyond the header
    // -------------------------------------------------------------------------
    struct Heartbeat {
        Header header {};
    };

    #pragma pack(pop)

} // namespace GE::Networking::Packets
