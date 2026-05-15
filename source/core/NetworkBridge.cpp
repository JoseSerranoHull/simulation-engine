// Winsock2 must come before windows.h (needed for BeginAutoConnect raw socket)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
/* parasoft-begin-suppress ALL */
#include <winsock2.h>
#include <ws2tcpip.h>
/* parasoft-end-suppress ALL */

#include "core/NetworkBridge.h"
#include "core/Logger.h"

/* parasoft-begin-suppress ALL */
#include <cstring>
#include <ctime>
#include <algorithm>
#include <chrono>
#include <map>
#include <random>
#include <sstream>
#include <glm/gtc/quaternion.hpp>
/* parasoft-end-suppress ALL */

namespace GE {

// ---------------------------------------------------------------------------
// Diagnostic helpers
// ---------------------------------------------------------------------------

namespace {
    std::string wallClockStr() {
        const std::time_t t = std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now());
        struct tm tm_info {};
        localtime_s(&tm_info, &t);
        char buf[12]{};
        std::strftime(buf, sizeof(buf), "%H:%M:%S", &tm_info);
        return buf;
    }
} // anonymous namespace

void NetworkBridge::logDiscovery(const std::string& text)
{
    GE_LOG_INFO("NetworkBridge [Discovery]: " + text);
    std::lock_guard<std::mutex> lock(m_discoveryLogMutex);
    if (m_discoveryLog.size() >= MAX_DISCOVERY_LOG) { m_discoveryLog.pop_front(); }
    m_discoveryLog.push_back({ wallClockStr(), text });
}

std::vector<NetworkBridge::DiscoveryEvent> NetworkBridge::GetDiscoveryLogSnapshot() const
{
    std::lock_guard<std::mutex> lock(m_discoveryLogMutex);
    return { m_discoveryLog.begin(), m_discoveryLog.end() };
}

void NetworkBridge::ClearDiscoveryLog()
{
    std::lock_guard<std::mutex> lock(m_discoveryLogMutex);
    m_discoveryLog.clear();
}

uint64_t NetworkBridge::GetPeerLastPacketMs(uint8_t peerId) const
{
    if (peerId < 1U || peerId > Networking::NetworkService::MAX_PEERS) { return 0U; }
    return m_peerLastPacketMs[static_cast<std::size_t>(peerId - 1U)]
        .load(std::memory_order_relaxed);
}

// ---------------------------------------------------------------------------
// BroadcastOwnedStates
// ---------------------------------------------------------------------------

void NetworkBridge::BroadcastOwnedStates() {
    if ((m_service == nullptr) || !m_service->IsConnected()) { return; }
    if (m_entityManager == nullptr) { return; }

    // Throttle to ~60 broadcasts/sec
    constexpr float BROADCAST_INTERVAL_SEC = 1.0f / 60.0f;
    const auto now = std::chrono::steady_clock::now();
    const std::chrono::duration<float> elapsed = now - m_lastBroadcast;
    if (elapsed.count() < BROADCAST_INTERVAL_SEC) { return; }
    m_lastBroadcast = now;

    const uint8_t localId  = m_service->GetLocalPeerId();

    // Map local peer ID (1–4) to the OwnerType enum (0-based index)
    // OwnerType::ONE==0, TWO==1, THREE==2, FOUR==3
    if (localId < 1 || localId > 4) { return; }
    const auto localOwner = static_cast<GE::Components::OwnerType>(localId - 1U);

    auto& ownerArr     = m_entityManager->GetCompArr<GE::Components::OwnerComponent>();
    auto& transformArr = m_entityManager->GetCompArr<GE::Components::Transform>();
    auto& rigidArr     = m_entityManager->GetCompArr<GE::Components::RigidBody>();

    const uint32_t count = ownerArr.GetCount();
    for (uint32_t i = 0U; i < count; ++i) {
        if (ownerArr.Data()[i].owner != localOwner) { continue; }

        const uint32_t entityId = ownerArr.Index()[i];

        auto* tr = m_entityManager->TryGetTIComponent<GE::Components::Transform>(entityId);
        auto* rb = m_entityManager->TryGetTIComponent<GE::Components::RigidBody>(entityId);
        if ((tr == nullptr) || (rb == nullptr)) { continue; }

        Networking::Packets::StateUpdate pkt{};
        pkt.header.type     = Networking::Packets::PacketType::StateUpdate;
        pkt.header.senderId = localId;
        pkt.header.sequence = m_outSequence++;

        pkt.entityId        = entityId;
        pkt.position        = tr->m_worldPosition;
        // Derive orientation from world matrix (rotation part), pack as x y z w
        {
            const glm::quat q = glm::quat_cast(glm::mat3(tr->m_worldMatrix));
            pkt.orientation[0] = q.x;
            pkt.orientation[1] = q.y;
            pkt.orientation[2] = q.z;
            pkt.orientation[3] = q.w;
        }
        pkt.linearVelocity  = rb->velocity;
        pkt.angularVelocity = rb->angularVelocity;

        m_service->Broadcast(&pkt, sizeof(pkt));
    }
}

// ---------------------------------------------------------------------------
// ApplyReceivedState — dispatch on packet type
// ---------------------------------------------------------------------------

void NetworkBridge::ApplyReceivedState(uint8_t senderId,
                                       const uint8_t* data,
                                       std::size_t    size,
                                       uint32_t       senderAddr,
                                       uint16_t       senderPort)
{
    if (m_entityManager == nullptr) { return; }
    if (size < sizeof(Networking::Packets::Header)) { return; }

    Networking::Packets::Header hdr{};
    std::memcpy(&hdr, data, sizeof(hdr));

    switch (hdr.type) {
    case Networking::Packets::PacketType::StateUpdate:
        handleStateUpdate(senderId, data, size);
        break;
    case Networking::Packets::PacketType::SceneChange:
        handleSceneChange(data, size);
        break;
    case Networking::Packets::PacketType::SpawnObject:
        handleSpawnObject(data, size);
        break;
    case Networking::Packets::PacketType::AnimationSync:
        handleAnimationSync(data, size);
        break;
    case Networking::Packets::PacketType::DiscoveryHello:
        handleDiscoveryHello(senderAddr, senderPort, data, size);
        break;
    case Networking::Packets::PacketType::PeerAnnounce:
        if (size >= sizeof(Networking::Packets::PeerAnnounce)) {
            Networking::Packets::PeerAnnounce pa{};
            std::memcpy(&pa, data, sizeof(pa));
            // peerAddr carries the actual peer IP when this packet is a relay from another node.
            // Without it, senderAddr would be the relay node's IP, corrupting peer routing.
            const uint32_t actualAddr = (pa.peerAddr != 0U) ? pa.peerAddr : senderAddr;
            handlePeerAnnounce(pa.peerID, actualAddr);
        }
        break;
    default:
        break;  // Heartbeat, DiscoveryResponse (handled in jthread), and unknowns ignored
    }
}

// ---------------------------------------------------------------------------
// handleStateUpdate (was the body of ApplyReceivedState)
// ---------------------------------------------------------------------------

void NetworkBridge::handleStateUpdate(uint8_t senderId,
                                      const uint8_t* data,
                                      std::size_t    size)
{
    if (size < sizeof(Networking::Packets::StateUpdate)) { return; }

    Networking::Packets::StateUpdate pkt{};
    std::memcpy(&pkt, data, sizeof(pkt));

    // Drop packets with invalid sender IDs before any arithmetic
    if (senderId < 1U || senderId > Networking::NetworkService::MAX_PEERS) { return; }
    const uint8_t peerIdx = senderId - 1U;

    // Drop packets from peers not registered for the current scene
    if (!((m_acceptedPeerMask.load(std::memory_order_relaxed) >> peerIdx) & 1U)) { return; }

    if (pkt.header.sequence <= m_lastSeenSequence[peerIdx]) { return; }
    m_lastSeenSequence[peerIdx] = pkt.header.sequence;

    // Stamp arrival time for ImGui per-peer diagnostics
    m_peerLastPacketMs[peerIdx].store(
        static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count()),
        std::memory_order_relaxed);

    // Dead reckoning: store authoritative state; blend will be applied
    // by UpdateRemoteEntities() on the physics thread.
    {
        std::lock_guard<std::mutex> lock(m_remoteStatesMutex);
        RemoteEntityState& rs = m_remoteStates[pkt.entityId];
        rs.authPosition  = pkt.position;
        rs.authVelocity  = pkt.linearVelocity;
        rs.authTimeSec   = std::chrono::duration<double>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        rs.blendTimer    = RemoteEntityState::BLEND_DURATION;
    }

    // Apply angularVelocity and orientation immediately
    // (rotation is not dead-reckoned — too complex for rotation blending).
    auto* rb = m_entityManager->TryGetTIComponent<GE::Components::RigidBody>(pkt.entityId);
    if (rb != nullptr) {
        rb->angularVelocity = pkt.angularVelocity;
    }
    auto* tr = m_entityManager->TryGetTIComponent<GE::Components::Transform>(pkt.entityId);
    if (tr != nullptr) {
        const glm::quat q(pkt.orientation[3],   // w
                          pkt.orientation[0],   // x
                          pkt.orientation[1],   // y
                          pkt.orientation[2]);  // z
        const glm::mat3 rotMat = glm::mat3_cast(q);
        const glm::vec3 scale  = tr->m_localScale;
        tr->m_worldMatrix = glm::mat4(
            glm::vec4(rotMat[0] * scale.x, 0.0f),
            glm::vec4(rotMat[1] * scale.y, 0.0f),
            glm::vec4(rotMat[2] * scale.z, 0.0f),
            glm::vec4(pkt.position,         1.0f)
        );
    }
}

// ---------------------------------------------------------------------------
// UpdateRemoteEntities — dead reckoning + smooth blend correction
// ---------------------------------------------------------------------------

void NetworkBridge::UpdateRemoteEntities(float dt)
{
    if (m_entityManager == nullptr) { return; }
    if (m_service == nullptr) { return; }

    const uint8_t localId = m_service->GetLocalPeerId();
    const auto localOwner = (localId >= 1 && localId <= 4)
                             ? static_cast<GE::Components::OwnerType>(localId - 1U)
                             : GE::Components::OwnerType::ONE;

    const double nowSec = std::chrono::duration<double>(
        std::chrono::steady_clock::now().time_since_epoch()).count();

    std::lock_guard<std::mutex> lock(m_remoteStatesMutex);

    for (auto& [entityId, rs] : m_remoteStates) {
        // Skip entities owned by this local peer
        auto* owner = m_entityManager->TryGetTIComponent<GE::Components::OwnerComponent>(entityId);
        if (owner != nullptr && owner->owner == localOwner) { continue; }

        auto* tr = m_entityManager->TryGetTIComponent<GE::Components::Transform>(entityId);
        auto* rb = m_entityManager->TryGetTIComponent<GE::Components::RigidBody>(entityId);
        if (tr == nullptr || rb == nullptr) { continue; }

        // Guard: if packet is stale (peer likely disconnected), stop dead reckoning
        const float dtSincePacket = static_cast<float>(nowSec - rs.authTimeSec);
        if (dtSincePacket > 2.0f) { continue; }

        // Dead reckoning: predict position based on last known velocity
        const glm::vec3 predictedPos = rs.authPosition + rs.authVelocity * dtSincePacket;

        if (rs.blendTimer > 0.0f) {
            // Active blend: lerp from current rendered position toward predicted
            const float alpha = dt / rs.blendTimer;
            const float t     = glm::clamp(alpha, 0.0f, 1.0f);
            rs.renderPosition = glm::mix(tr->m_worldPosition, predictedPos, t);
            rs.blendTimer    -= dt;
            if (rs.blendTimer < 0.0f) { rs.blendTimer = 0.0f; }
        } else {
            // No active blend: pure dead reckoning
            rs.renderPosition = predictedPos;
        }

        // Apply to entity (overwrites whatever PhysicsSystem did this tick)
        tr->m_localPosition   = rs.renderPosition;
        tr->m_worldPosition   = rs.renderPosition;
        rb->velocity          = rs.authVelocity;
        tr->m_worldMatrix[3]  = glm::vec4(rs.renderPosition, 1.0f);
    }
}

// ---------------------------------------------------------------------------
// ClearRemoteStates — call on scene change to purge stale entity IDs
// ---------------------------------------------------------------------------

void NetworkBridge::ClearRemoteStates()
{
    std::lock_guard<std::mutex> lock(m_remoteStatesMutex);
    m_remoteStates.clear();
    m_acceptedPeerMask.store(0, std::memory_order_relaxed);
}

// ---------------------------------------------------------------------------
// handleSceneChange — thread-safe write to m_pendingNetworkScene
// ---------------------------------------------------------------------------

void NetworkBridge::handleSceneChange(const uint8_t* data, std::size_t size)
{
    if (size < sizeof(Networking::Packets::SceneChange)) { return; }

    Networking::Packets::SceneChange pkt{};
    std::memcpy(&pkt, data, sizeof(pkt));
    pkt.scenePath[sizeof(pkt.scenePath) - 1] = '\0';   // paranoid null-terminate

    const std::string path(pkt.scenePath);
    if (path.empty()) { return; }

    GE_LOG_INFO("NetworkBridge: SCENE_CHANGE received → '" + path + "'");

    std::lock_guard<std::mutex> lock(m_pendingNetworkSceneMutex);
    m_pendingNetworkScene = path;   // main thread picks this up via PollPendingSceneChange()
}

// ---------------------------------------------------------------------------
// handleSpawnObject — activate a pre-created pool entity on a remote peer
// ---------------------------------------------------------------------------

void NetworkBridge::handleSpawnObject(const uint8_t* data, std::size_t size)
{
    if (size < sizeof(Networking::Packets::SpawnObject)) { return; }

    Networking::Packets::SpawnObject pkt{};
    std::memcpy(&pkt, data, sizeof(pkt));

    // Drop packets from peers not registered for the current scene
    {
        const uint8_t sid = pkt.header.senderId;
        if (sid < 1U || sid > Networking::NetworkService::MAX_PEERS) { return; }
        const uint8_t peerIdx = sid - 1U;
        if (!((m_acceptedPeerMask.load(std::memory_order_relaxed) >> peerIdx) & 1U)) { return; }
    }

    auto* tr = m_entityManager->TryGetTIComponent<GE::Components::Transform>(pkt.entityId);
    auto* rb = m_entityManager->TryGetTIComponent<GE::Components::RigidBody>(pkt.entityId);

    if ((tr == nullptr) || (rb == nullptr)) {
        GE_LOG_WARN("NetworkBridge: SPAWN_OBJECT for unknown entity "
                    + std::to_string(pkt.entityId) + " — ignored.");
        return;
    }

    // Activate the entity: move it into the world and enable physics
    tr->m_localPosition = pkt.position;
    tr->m_worldPosition = pkt.position;
    tr->m_localScale    = pkt.scale;
    tr->m_worldMatrix = glm::mat4(
        glm::vec4(pkt.scale.x, 0.0f,      0.0f,      0.0f),
        glm::vec4(0.0f,      pkt.scale.y, 0.0f,      0.0f),
        glm::vec4(0.0f,      0.0f,      pkt.scale.z, 0.0f),
        glm::vec4(pkt.position,                       1.0f)
    );

    rb->isStatic   = false;
    rb->useGravity = true;
    rb->velocity   = pkt.linearVelocity;
}

// ---------------------------------------------------------------------------
// BroadcastSceneChange — send 3× for UDP reliability
// ---------------------------------------------------------------------------

void NetworkBridge::BroadcastSceneChange(const std::string& path)
{
    if ((m_service == nullptr) || !m_service->IsConnected()) { return; }

    Networking::Packets::SceneChange pkt{};
    pkt.header.type     = Networking::Packets::PacketType::SceneChange;
    pkt.header.senderId = m_service->GetLocalPeerId();
    pkt.header.sequence = m_outSequence++;

    const std::size_t copyLen = std::min(path.size(), sizeof(pkt.scenePath) - 1U);
    std::memcpy(pkt.scenePath, path.c_str(), copyLen);
    pkt.scenePath[copyLen] = '\0';

    // Send 3 times to tolerate single UDP packet loss
    for (int i = 0; i < 3; ++i) {
        m_service->Broadcast(&pkt, sizeof(pkt));
    }

    GE_LOG_INFO("NetworkBridge: broadcasted SCENE_CHANGE → '" + path + "'");
}

// ---------------------------------------------------------------------------
// BroadcastSpawnObject
// ---------------------------------------------------------------------------

void NetworkBridge::BroadcastSpawnObject(uint32_t entityId, uint8_t ownerPeerId,
                                         uint8_t shapeType,
                                         const glm::vec3& position,
                                         const glm::vec3& scale,
                                         const glm::vec3& linearVelocity,
                                         float mass)
{
    if ((m_service == nullptr) || !m_service->IsConnected()) { return; }

    Networking::Packets::SpawnObject pkt{};
    pkt.header.type     = Networking::Packets::PacketType::SpawnObject;
    pkt.header.senderId = m_service->GetLocalPeerId();
    pkt.header.sequence = m_outSequence++;
    pkt.entityId        = entityId;
    pkt.ownerPeerId     = ownerPeerId;
    pkt.shapeType       = shapeType;
    pkt.position        = position;
    pkt.scale           = scale;
    pkt.linearVelocity  = linearVelocity;
    pkt.mass            = mass;

    m_service->Broadcast(&pkt, sizeof(pkt));
}

// ---------------------------------------------------------------------------
// PollPendingSceneChange — called by main thread each frame
// ---------------------------------------------------------------------------

std::optional<std::string> NetworkBridge::PollPendingSceneChange()
{
    std::lock_guard<std::mutex> lock(m_pendingNetworkSceneMutex);
    if (m_pendingNetworkScene.empty()) { return std::nullopt; }
    std::string path = std::move(m_pendingNetworkScene);
    m_pendingNetworkScene.clear();
    return path;
}

// ---------------------------------------------------------------------------
// BroadcastAnimationStates — one-shot sync of all animated object timers
// ---------------------------------------------------------------------------

void NetworkBridge::BroadcastAnimationStates()
{
    if ((m_service == nullptr) || !m_service->IsConnected()) { return; }
    if (m_entityManager == nullptr) { return; }

    // Only send to peers that are registered for the current scene (same mask used by receive path)
    const uint8_t mask = m_acceptedPeerMask.load(std::memory_order_relaxed);
    if (mask == 0U) { return; }

    auto& animArr = m_entityManager->GetCompArr<GE::Components::AnimatedObjectComponent>();
    for (uint32_t i = 0U; i < animArr.GetCount(); ++i) {
        const auto& ac = animArr.Data()[i];
        Networking::Packets::AnimationSync pkt{};
        pkt.header.type     = Networking::Packets::PacketType::AnimationSync;
        pkt.header.senderId = m_service->GetLocalPeerId();
        pkt.header.sequence = m_outSequence++;
        pkt.entityId        = animArr.Index()[i];
        pkt.elapsed         = ac.elapsed;
        pkt.reversed        = ac.reversed ? 1U : 0U;
        for (uint8_t p = 0U; p < Networking::NetworkService::MAX_PEERS; ++p) {
            if ((mask >> p) & 1U) {
                m_service->Send(p + 1U, &pkt, sizeof(pkt));
            }
        }
    }
}

// ---------------------------------------------------------------------------
// handleAnimationSync — apply remote animation timer to local component
// ---------------------------------------------------------------------------

void NetworkBridge::handleAnimationSync(const uint8_t* data, std::size_t size)
{
    if (size < sizeof(Networking::Packets::AnimationSync)) { return; }
    Networking::Packets::AnimationSync pkt{};
    std::memcpy(&pkt, data, sizeof(pkt));

    // Drop packets from peers not registered for the current scene
    {
        const uint8_t sid = pkt.header.senderId;
        if (sid < 1U || sid > Networking::NetworkService::MAX_PEERS) { return; }
        const uint8_t peerIdx = sid - 1U;
        if (!((m_acceptedPeerMask.load(std::memory_order_relaxed) >> peerIdx) & 1U)) { return; }
    }

    auto* ac = m_entityManager->TryGetTIComponent<
        GE::Components::AnimatedObjectComponent>(pkt.entityId);
    if (ac == nullptr) { return; }
    ac->elapsed  = pkt.elapsed;
    ac->reversed = (pkt.reversed != 0U);
}

// ---------------------------------------------------------------------------
// SetCurrentScene — thread-safe write of the active scene path
// ---------------------------------------------------------------------------

void NetworkBridge::SetCurrentScene(const std::string& path)
{
    std::lock_guard<std::mutex> lock(m_scenePathMutex);
    m_currentScenePath = path;
}

void NetworkBridge::RegisterScenePeer(uint8_t peerId)
{
    if (peerId < 1U || peerId > Networking::NetworkService::MAX_PEERS) { return; }
    m_acceptedPeerMask.fetch_or(
        static_cast<uint8_t>(1U << (peerId - 1U)), std::memory_order_relaxed);
    GE_LOG_INFO("NetworkBridge: registered scene peer " + std::to_string(peerId));
}

// ---------------------------------------------------------------------------
// handleDiscoveryHello — existing peer responds to a new peer's broadcast probe
// ---------------------------------------------------------------------------

void NetworkBridge::handleDiscoveryHello(uint32_t senderAddr, uint16_t senderPort,
                                          const uint8_t* data, std::size_t size)
{
    // Log receipt BEFORE any early-return check — absence of this log in the terminal
    // confirms the packet never reached the game socket (firewall / timing race).
    {
        char senderIp[INET_ADDRSTRLEN]{};
        inet_ntop(AF_INET, &senderAddr, senderIp, sizeof(senderIp));
        logDiscovery("<- DiscoveryHello from " + std::string(senderIp));
    }

    if ((m_service == nullptr) || !m_service->IsConnected()) {
        logDiscovery("   (ignored — game socket not connected yet)");
        return;
    }
    const uint8_t myId = m_service->GetLocalPeerId();
    if (myId < 1U || myId > 4U) { return; }

    // Parse incoming scene path
    std::string incomingScene;
    if (size >= sizeof(Networking::Packets::DiscoveryHello)) {
        Networking::Packets::DiscoveryHello hello{};
        std::memcpy(&hello, data, sizeof(hello));
        hello.scenePath[sizeof(hello.scenePath) - 1] = '\0';
        incomingScene = hello.scenePath;
    }

    // Reject peers on a different scene (both sides must have a scene set to filter)
    {
        std::lock_guard<std::mutex> lock(m_scenePathMutex);
        if (!incomingScene.empty() && !m_currentScenePath.empty()
            && incomingScene != m_currentScenePath) {
            logDiscovery("   rejected — scene mismatch (theirs: " + incomingScene + ")");
            return;
        }
    }

    Networking::Packets::DiscoveryResponse resp{};
    resp.header.type     = Networking::Packets::PacketType::DiscoveryResponse;
    resp.header.senderId = myId;
    resp.peerID          = myId;
    {
        std::lock_guard<std::mutex> lock(m_scenePathMutex);
        const std::size_t len = std::min(m_currentScenePath.size(),
                                         sizeof(resp.scenePath) - 1U);
        std::memcpy(resp.scenePath, m_currentScenePath.c_str(), len);
        resp.scenePath[len] = '\0';
    }
    m_service->SendRaw(senderAddr, senderPort, &resp, sizeof(resp));

    logDiscovery("-> DiscoveryResponse sent (I am Peer " + std::to_string(myId) + ")");

    // Relay DiscoveryResponse for every other registered peer so the joiner learns the
    // full peer topology from a single DiscoveryHello. peerAddr carries each peer's real
    // IP (necessary because the relay packet is sent from THIS socket, not the peer's socket).
    {
        const std::string sceneCopy = [this]() {
            std::lock_guard<std::mutex> lk(m_scenePathMutex);
            return m_currentScenePath;
        }();

        for (const auto& peer : m_service->GetActivePeers()) {
            if (peer.peerId == myId) { continue; }  // already sent our own above

            Networking::Packets::DiscoveryResponse relay{};
            relay.header.type     = Networking::Packets::PacketType::DiscoveryResponse;
            relay.header.senderId = peer.peerId;
            relay.peerID          = peer.peerId;
            relay.peerAddr        = peer.addr;   // real NBO IP of the relayed peer
            const std::size_t len = std::min(sceneCopy.size(), sizeof(relay.scenePath) - 1U);
            std::memcpy(relay.scenePath, sceneCopy.c_str(), len);
            relay.scenePath[len] = '\0';

            m_service->SendRaw(senderAddr, senderPort, &relay, sizeof(relay));
            logDiscovery("-> Relayed DiscoveryResponse for Peer " + std::to_string(peer.peerId));
        }
    }
}

// ---------------------------------------------------------------------------
// handlePeerAnnounce — a peer that just auto-connected announces itself
// ---------------------------------------------------------------------------

void NetworkBridge::handlePeerAnnounce(uint8_t peerID, uint32_t senderAddr)
{
    if ((m_service == nullptr) || !m_service->IsConnected()) { return; }
    if (peerID < 1U || peerID > 4U) { return; }
    if (peerID == m_service->GetLocalPeerId()) { return; }  // don't add ourselves

    char ipBuf[INET_ADDRSTRLEN]{};
    inet_ntop(AF_INET, &senderAddr, ipBuf, sizeof(ipBuf));
    const uint16_t peerPort = static_cast<uint16_t>(54000U + peerID - 1U);

    // Check BEFORE adding so we can detect genuinely new peers.
    const bool wasNew = !m_service->HasPeer(peerID);

    m_service->AddPeer(peerID, ipBuf, peerPort);
    RegisterScenePeer(peerID);

    GE_LOG_INFO("NetworkBridge: peer " + std::to_string(peerID)
                + " announced itself from " + ipBuf);

    // If this is a new peer, unicast our own PeerAnnounce back so they add us too.
    // This closes the loop for assume-host joiners that skipped discovery and only
    // know about Peer 1 (the assumed host) — after the bidirectional exchange they
    // have each other's full peer entries.
    // wasNew prevents infinite ping-pong: the second exchange finds wasNew=false → stops.
    if (wasNew) {
        const uint8_t myId = m_service->GetLocalPeerId();

        // Reply to the new peer so they add us (existing bidirectional ack).
        Networking::Packets::PeerAnnounce reply{};
        reply.header.type     = Networking::Packets::PacketType::PeerAnnounce;
        reply.header.senderId = myId;
        reply.peerID          = myId;
        m_service->Send(peerID, &reply, sizeof(reply));
        GE_LOG_INFO("NetworkBridge: sent PeerAnnounce reply to new peer "
                    + std::to_string(peerID));

        // Relay the new peer's PeerAnnounce to every OTHER registered peer via unicast.
        //
        // Why this is needed:
        //   PeerAnnounce is broadcast to the subnet, but a peer whose game port
        //   is inbound-blocked (e.g. Machine A port 54001) cannot receive the
        //   broadcast.  However, it CAN receive from peers it has already
        //   established a stateful firewall rule with (e.g. Machine B via discovery).
        //   By forwarding PeerAnnounce{N} through Machine B → Machine A, Machine A
        //   registers Peer N and sends a reply outbound (always allowed), which
        //   creates the stateful rule for Peer N → Machine A — completing full sync.
        //
        // Loop terminates: forwarded packets arrive at already-connected peers with
        // wasNew=false → no further relay → 1 hop maximum per new peer per notifier.
        {
            Networking::Packets::PeerAnnounce forward{};
            forward.header.type     = Networking::Packets::PacketType::PeerAnnounce;
            forward.header.senderId = peerID;  // looks like it originated from the new peer
            forward.peerID          = peerID;
            // peerAddr carries the new peer's actual IP so receivers don't overwrite
            // their peer table with this relay node's IP (the packet's source address).
            forward.peerAddr        = senderAddr;

            for (uint8_t p = 1U; p <= Networking::NetworkService::MAX_PEERS; ++p) {
                if (p == peerID || p == myId) { continue; }  // skip new peer and self
                if (m_service->HasPeer(p)) {
                    m_service->Send(p, &forward, sizeof(forward));
                    GE_LOG_INFO("NetworkBridge: relayed PeerAnnounce{" + std::to_string(peerID)
                                + "} to Peer " + std::to_string(p)
                                + " (assists inbound-blocked peers)");
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------
// BeginAutoConnect — LAN peer discovery jthread
// ---------------------------------------------------------------------------

void NetworkBridge::BeginAutoConnect(const std::string& hostIP)
{
    // If a previous discovery is still running, let it finish first
    if (m_discoveryThread.joinable()) {
        m_autoConnectState.store(AutoConnectState::Idle);
        m_discoveryThread.request_stop();
        m_discoveryThread.join();
    }

    m_autoConnectState.store(AutoConnectState::Discovering);
    m_autoConnectStatus = "Discovering...";

    m_discoveryThread = std::jthread([this, hostIP](std::stop_token stopToken) {
        static constexpr uint16_t BASE_PORT = 54000U;
        static constexpr int      WAIT_MS   = 1500;

        // Shut down any existing game socket BEFORE probing.
        // This prevents our own socket from answering the broadcast and falsely
        // occupying our current slot in the discovery responses.
        if (m_service->IsConnected()) {
            m_service->Shutdown();
        }

        // Random jitter 0–300 ms to reduce simultaneous-click collisions
        {
            std::mt19937 rng(static_cast<uint32_t>(
                std::chrono::steady_clock::now().time_since_epoch().count()));
            std::uniform_int_distribution<int> dist(0, 300);
            std::this_thread::sleep_for(std::chrono::milliseconds(dist(rng)));
        }
        if (stopToken.stop_requested()) { return; }

        // Capture scene path for this discovery session (set by OnLoad before UI is accessible)
        std::string localScene;
        {
            std::lock_guard<std::mutex> lock(m_scenePathMutex);
            localScene = m_currentScenePath;
        }

        // --- Open temporary socket ---
        WSADATA wsa{};
        WSAStartup(MAKEWORD(2, 2), &wsa);

        const SOCKET tempSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (tempSock == INVALID_SOCKET) {
            m_autoConnectStatus = "Failed: could not create socket";
            m_autoConnectState.store(AutoConnectState::Failed);
            WSACleanup();
            return;
        }

        // Enable broadcast and bind to discovery reply port
        const int yes = 1;
        setsockopt(tempSock, SOL_SOCKET, SO_BROADCAST,
                   reinterpret_cast<const char*>(&yes), static_cast<int>(sizeof(yes)));

        sockaddr_in local{};
        local.sin_family      = AF_INET;
        local.sin_addr.s_addr = INADDR_ANY;
        local.sin_port        = 0;  // OS assigns a free ephemeral port — no same-machine conflicts
        if (bind(tempSock, reinterpret_cast<sockaddr*>(&local), sizeof(local)) == SOCKET_ERROR) {
            m_autoConnectStatus = "Failed: could not bind discovery socket";
            m_autoConnectState.store(AutoConnectState::Failed);
            closesocket(tempSock);
            WSACleanup();
            return;
        }

        // Log the OS-assigned ephemeral port — useful for firewall debugging
        {
            sockaddr_in boundAddr{};
            int boundLen = static_cast<int>(sizeof(boundAddr));
            if (getsockname(tempSock, reinterpret_cast<sockaddr*>(&boundAddr), &boundLen) == 0) {
                logDiscovery("Discovery temp socket open on port "
                             + std::to_string(ntohs(boundAddr.sin_port)));
            }
        }

        u_long nbMode = 1;
        ioctlsocket(tempSock, FIONBIO, &nbMode);

        // --- Broadcast DiscoveryHello to all four game ports ---
        Networking::Packets::DiscoveryHello hello{};
        hello.header.senderId = 0U;  // not yet assigned
        {
            const std::size_t len = std::min(localScene.size(), sizeof(hello.scenePath) - 1U);
            std::memcpy(hello.scenePath, localScene.c_str(), len);
            hello.scenePath[len] = '\0';
        }

        // --- Send DiscoveryHello (unicast to host IP if given, broadcast otherwise) ---
        sockaddr_in dest{};
        dest.sin_family = AF_INET;

        if (!hostIP.empty()) {
            if (inet_pton(AF_INET, hostIP.c_str(), &dest.sin_addr) != 1) {
                m_autoConnectStatus = "Failed: invalid host IP '" + hostIP + "'";
                m_autoConnectState.store(AutoConnectState::Failed);
                closesocket(tempSock);
                WSACleanup();
                return;
            }
            logDiscovery("-> DiscoveryHello unicast to " + hostIP);
        } else {
            dest.sin_addr.s_addr = INADDR_BROADCAST;
            logDiscovery("-> DiscoveryHello broadcast (host mode)");
        }

        // Helper: send DiscoveryHello to all four game ports, log any OS send errors.
        const auto sendHello = [&](const char* label) {
            for (int p = 0; p < 4; ++p) {
                dest.sin_port = htons(static_cast<uint16_t>(BASE_PORT + p));
                const int sent = sendto(tempSock,
                                        reinterpret_cast<const char*>(&hello),
                                        static_cast<int>(sizeof(hello)), 0,
                                        reinterpret_cast<sockaddr*>(&dest),
                                        static_cast<int>(sizeof(dest)));
                if (sent == SOCKET_ERROR) {
                    logDiscovery(std::string("   sendto port ")
                                 + std::to_string(BASE_PORT + p)
                                 + " FAILED (WSA " + std::to_string(WSAGetLastError()) + ")"
                                 + " [" + label + "]");
                }
            }
        };

        // Initial send, then two retries at 1/3 and 2/3 of the collection window.
        // This compensates for UDP packet loss AND for the timing race where the
        // host's game socket is briefly offline during its own discovery phase.
        sendHello("initial");

        // --- Collect DiscoveryResponse packets for WAIT_MS milliseconds ---
        // key = peerID (1–4), value = sender IP (network byte order)
        std::map<uint8_t, uint32_t> discovered;

        const auto windowStart = std::chrono::steady_clock::now();
        const auto deadline    = windowStart + std::chrono::milliseconds(WAIT_MS);
        const auto retryAt1    = windowStart + std::chrono::milliseconds(WAIT_MS / 3);
        const auto retryAt2    = windowStart + std::chrono::milliseconds(2 * WAIT_MS / 3);
        bool sentRetry1 = false, sentRetry2 = false;

        while (std::chrono::steady_clock::now() < deadline && !stopToken.stop_requested()) {

            // Retry sends within the wait window to handle packet loss and timing races
            const auto now = std::chrono::steady_clock::now();
            if (!sentRetry1 && now >= retryAt1) {
                sentRetry1 = true;
                sendHello("retry 1/2");
                logDiscovery("-> DiscoveryHello retry 1/2 (500 ms)");
            }
            if (!sentRetry2 && now >= retryAt2) {
                sentRetry2 = true;
                sendHello("retry 2/2");
                logDiscovery("-> DiscoveryHello retry 2/2 (1000 ms)");
            }

            fd_set readSet;
            FD_ZERO(&readSet);
            FD_SET(tempSock, &readSet);
            timeval tv{ 0, 50'000 };  // 50 ms poll interval
            const int ready = select(0, &readSet, nullptr, nullptr, &tv);
            if (ready <= 0) { continue; }

            char buf[256]{};
            sockaddr_in from{};
            int fromLen = static_cast<int>(sizeof(from));
            const int n = recvfrom(tempSock, buf, static_cast<int>(sizeof(buf)),
                                   0, reinterpret_cast<sockaddr*>(&from), &fromLen);
            if (n < static_cast<int>(sizeof(Networking::Packets::DiscoveryResponse))) {
                continue;
            }

            Networking::Packets::DiscoveryResponse resp{};
            std::memcpy(&resp, buf, sizeof(resp));
            if (resp.header.type != Networking::Packets::PacketType::DiscoveryResponse) {
                continue;
            }
            if (resp.peerID < 1U || resp.peerID > 4U) { continue; }

            // Scene filter: ignore peers on a different scene
            resp.scenePath[sizeof(resp.scenePath) - 1] = '\0';
            if (!localScene.empty() && resp.scenePath[0] != '\0'
                && std::string(resp.scenePath) != localScene) {
                logDiscovery("<- DiscoveryResponse from Peer " + std::to_string(resp.peerID)
                             + " REJECTED — scene mismatch");
                continue;
            }

            // Log the received response — for relayed entries show the actual peer IP
            // (stored in peerAddr), not the packet sender's IP (always the host).
            {
                char fromIp[INET_ADDRSTRLEN]{};
                inet_ntop(AF_INET, &from.sin_addr, fromIp, sizeof(fromIp));
                const bool isRelay = (resp.peerAddr != 0U);
                char actualIp[INET_ADDRSTRLEN]{};
                if (isRelay) {
                    inet_ntop(AF_INET, &resp.peerAddr, actualIp, sizeof(actualIp));
                } else {
                    std::memcpy(actualIp, fromIp, sizeof(actualIp));
                }
                logDiscovery("<- DiscoveryResponse: Peer " + std::to_string(resp.peerID)
                             + " at " + std::string(actualIp)
                             + (isRelay
                                ? " [relayed via " + std::string(fromIp) + "]"
                                : " [direct]"));
            }

            // Prefer relayed IP if the host provided one; otherwise use the packet sender's IP.
            discovered[resp.peerID] = (resp.peerAddr != 0U) ? resp.peerAddr : from.sin_addr.s_addr;
        }

        closesocket(tempSock);
        WSACleanup();

        if (stopToken.stop_requested()) { return; }

        // ── "Assume Host" fallback ────────────────────────────────────────────
        // When the user provided a hostIP but received no DiscoveryResponse
        // (common when the host's Windows Firewall blocks inbound unicast on
        // port 54000 while still allowing cross-machine broadcast), inject the
        // host as Peer 1 so we take slot 2 rather than slot 1 (which would
        // create a peer-ID conflict). The raw-broadcast PeerAnnounce below then
        // notifies the host of our existence via broadcast (which the IT firewall
        // fix permits), completing the connection from the host's side.
        bool assumedHost = false;
        if (!hostIP.empty() && discovered.empty()) {
            uint32_t assumedAddr = 0U;
            if (inet_pton(AF_INET, hostIP.c_str(), &assumedAddr) == 1) {
                discovered[1] = assumedAddr;
                assumedHost   = true;
                logDiscovery("!! No response after 3 attempts — assume-host fallback for "
                             + hostIP + " (firewall or socket timing race)");
            }
        } else if (discovered.empty() && hostIP.empty()) {
            logDiscovery("No peers found via broadcast — connecting as host (Peer 1)");
        }

        // --- Determine lowest free slot ---
        uint8_t slot = 0U;
        for (uint8_t s = 1U; s <= 4U; ++s) {
            if (discovered.find(s) == discovered.end()) { slot = s; break; }
        }

        if (slot == 0U) {
            m_autoConnectStatus = "Failed: no free slot (4/4 peers occupied)";
            m_autoConnectState.store(AutoConnectState::Failed);
            return;
        }

        // --- Initialise game socket ---
        // Fall back through higher slots if the preferred port is already bound by
        // another local instance running on a different scene.
        uint16_t gamePort = 0U;
        while (slot <= 4U) {
            gamePort = static_cast<uint16_t>(BASE_PORT + slot - 1U);
            if (m_service->Init(gamePort)) { break; }
            GE_LOG_INFO("NetworkBridge: port " + std::to_string(gamePort)
                        + " in use, trying next slot");
            ++slot;
            // Skip slots already occupied by discovered same-scene peers
            while (slot <= 4U && discovered.count(slot)) { ++slot; }
        }
        if (slot > 4U) {
            m_autoConnectStatus = "Failed: all ports 54000-54003 in use";
            m_autoConnectState.store(AutoConnectState::Failed);
            return;
        }
        m_service->SetLocalPeerId(slot);
        m_service->EnableBroadcast();

        // --- Register discovered peers ---
        std::string connectedStr;
        for (const auto& [peerID, addr] : discovered) {
            char ipBuf[INET_ADDRSTRLEN]{};
            inet_ntop(AF_INET, &addr, ipBuf, sizeof(ipBuf));
            const uint16_t peerPort = static_cast<uint16_t>(BASE_PORT + peerID - 1U);
            m_service->AddPeer(peerID, ipBuf, peerPort);
            RegisterScenePeer(peerID);  // scene was verified during DiscoveryResponse filtering
            if (!connectedStr.empty()) { connectedStr += ", "; }
            connectedStr += "P" + std::to_string(peerID) + "(" + ipBuf + ")";
        }

        // --- UDP hole punch: Heartbeat from game socket to each discovered peer ---
        // Windows Firewall is stateful for UDP: sending outbound from our game socket
        // (A:54001 → B:54000) creates a temporary firewall rule allowing the reverse
        // (B:54000 → A:54001) for ~30–120 s. This is how inbound StateUpdate packets
        // from peers can reach a machine whose game port has no explicit inbound rule.
        // Without this, the first BroadcastOwnedStates (up to 16 ms away) would do the
        // same thing, but this immediate Heartbeat ensures the pinhole is open BEFORE
        // any incoming PeerAnnounce reply or StateUpdate packet arrives.
        if (!discovered.empty()) {
            Networking::Packets::Heartbeat hb{};
            hb.header.type     = Networking::Packets::PacketType::Heartbeat;
            hb.header.senderId = slot;
            hb.header.sequence = m_outSequence++;
            for (const auto& [peerID, addr] : discovered) {
                m_service->Send(peerID, &hb, sizeof(hb));
            }
            logDiscovery("-> Hole-punch Heartbeat to " + std::to_string(discovered.size())
                         + " peer(s) — opens stateful firewall pinholes");
        }

        // --- Announce ourselves: unicast to discovered peers + raw subnet broadcast ---
        // Unicast via Broadcast() covers peers found normally via DiscoveryResponse.
        // The raw 255.255.255.255 broadcast covers the firewall-blocked case: the host
        // couldn't respond to our DiscoveryHello (unicast blocked inbound) but CAN
        // receive cross-machine broadcast (enabled by Warren's IT firewall fix).
        // This allows the host to learn our peer ID and port without us needing
        // a direct unicast response.
        {
            Networking::Packets::PeerAnnounce announce{};
            announce.header.type     = Networking::Packets::PacketType::PeerAnnounce;
            announce.header.senderId = slot;
            announce.peerID          = slot;

            // Unicast to any peers discovered normally via DiscoveryResponse
            if (!discovered.empty() && !assumedHost) {
                m_service->Broadcast(&announce, sizeof(announce));
            }

            // Raw subnet broadcast to all game ports so hosts with inbound-unicast-
            // blocked firewalls still receive our PeerAnnounce via broadcast.
            // INADDR_BROADCAST (0xFFFFFFFF) is byte-order-neutral.
            // SendRaw expects port already in network byte order.
            for (int p = 0; p < 4; ++p) {
                m_service->SendRaw(
                    INADDR_BROADCAST,
                    htons(static_cast<uint16_t>(BASE_PORT + p)),
                    &announce, sizeof(announce));
            }
        }

        // --- Finalise ---
        m_autoConnectStatus = "Peer " + std::to_string(slot) +
                              " | Port " + std::to_string(gamePort) +
                              (connectedStr.empty()
                                  ? " | No peers found yet"
                                  : (assumedHost
                                      ? " | Assumed host at " + hostIP
                                        + " (no response -- firewall?)"
                                      : " | Connected to: " + connectedStr));

        m_pendingPostConnectSync.store(true);
        m_autoConnectState.store(AutoConnectState::Done);

        logDiscovery("Connected as Peer " + std::to_string(slot)
                     + " on port " + std::to_string(gamePort)
                     + (assumedHost ? " [assume-host]" : " [full handshake]"));
        GE_LOG_INFO("NetworkBridge: auto-connect complete — " + m_autoConnectStatus);
    });
}

} // namespace GE
