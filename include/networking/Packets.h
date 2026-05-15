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
        Heartbeat         = 0,
        StateUpdate       = 1,
        SceneChange       = 2,
        SpawnObject       = 3,
        AnimationSync     = 4,
        DiscoveryHello    = 5,  ///< LAN auto-connect probe (new peer → existing peers)
        DiscoveryResponse = 6,  ///< Reply from existing peer (existing → new peer temp socket)
        PeerAnnounce      = 7,  ///< "I just joined as Peer N" broadcast after auto-connect
        PeerLeave         = 8,  ///< "I am disconnecting" — sent before Shutdown so peers free the slot
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
    static_assert(sizeof(StateUpdate) == 60, "StateUpdate wire size changed — update all peers");

    // -------------------------------------------------------------------------
    // SceneChange — instructs a peer to load a new scene path
    // -------------------------------------------------------------------------
    struct SceneChange {
        Header header           {};
        char   scenePath[128]   {};
    };
    static_assert(sizeof(SceneChange) == 132, "SceneChange wire size changed — update all peers");

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
    static_assert(sizeof(SpawnObject) == 52, "SpawnObject wire size changed — update all peers");

    // -------------------------------------------------------------------------
    // AnimationSync — syncs animated object timer state to remote peers
    // -------------------------------------------------------------------------
    struct AnimationSync {
        Header   header   {};
        uint32_t entityId { 0 };
        float    elapsed  { 0.0f };
        uint8_t  reversed { 0 };   // 0 = forward, 1 = reversed
        uint8_t  _pad[3]  {};
    };
    static_assert(sizeof(AnimationSync) == 16, "AnimationSync wire size changed — update all peers");

    // -------------------------------------------------------------------------
    // Heartbeat — keep-alive, no payload beyond the header
    // -------------------------------------------------------------------------
    struct Heartbeat {
        Header header {};
    };
    static_assert(sizeof(Heartbeat) == 4, "Heartbeat wire size changed — update all peers");

    // -------------------------------------------------------------------------
    // DiscoveryHello — broadcast probe sent by a new peer during auto-connect
    // -------------------------------------------------------------------------
    struct DiscoveryHello {
        Header header { PacketType::DiscoveryHello };
        char   scenePath[128] {};  ///< scene this peer is currently in; empty = any
    };
    static_assert(sizeof(DiscoveryHello) == 132, "DiscoveryHello wire size changed — update all peers");

    // -------------------------------------------------------------------------
    // DiscoveryResponse — unicast reply from an already-connected peer
    // -------------------------------------------------------------------------
    struct DiscoveryResponse {
        Header   header  { PacketType::DiscoveryResponse };
        uint8_t  peerID  { 0 };       ///< 1–4: slot this peer occupies
        uint8_t  _pad[3] {};
        uint32_t peerAddr { 0 };      ///< NBO IPv4 of the relayed peer; 0 = use packet sender's IP
        char     scenePath[128] {};   ///< scene this peer is currently in
    };
    static_assert(sizeof(DiscoveryResponse) == 140, "DiscoveryResponse wire size changed — update all peers");

    // -------------------------------------------------------------------------
    // PeerAnnounce — broadcast after auto-connect so existing peers can add us
    // -------------------------------------------------------------------------
    struct PeerAnnounce {
        Header   header   { PacketType::PeerAnnounce };
        uint8_t  peerID   { 0 };      ///< slot this peer just claimed (1–4)
        uint8_t  _pad[3]  {};
        uint32_t peerAddr { 0 };      ///< NBO IPv4 of the announced peer; 0 = use packet sender's IP
    };
    static_assert(sizeof(PeerAnnounce) == 12, "PeerAnnounce wire size changed — update all peers");

    // -------------------------------------------------------------------------
    // PeerLeave — broadcast just before Shutdown so peers can free the slot
    // -------------------------------------------------------------------------
    struct PeerLeave {
        Header header { PacketType::PeerLeave };  ///< senderId in header identifies who left
    };
    static_assert(sizeof(PeerLeave) == 4, "PeerLeave wire size changed — update all peers");

    #pragma pack(pop)

} // namespace GE::Networking::Packets
