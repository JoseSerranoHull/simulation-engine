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
#include <algorithm>
#include <chrono>
#include <map>
#include <random>
#include <glm/gtc/quaternion.hpp>
/* parasoft-end-suppress ALL */

namespace GE {

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
            handlePeerAnnounce(pa.peerID, senderAddr);
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

    // Drop out-of-order / duplicate packets
    const uint8_t peerIdx = senderId - 1U;
    if (peerIdx >= Networking::NetworkService::MAX_PEERS) { return; }

    // Drop packets from peers not registered for the current scene
    if (!((m_acceptedPeerMask.load(std::memory_order_relaxed) >> peerIdx) & 1U)) { return; }

    if (pkt.header.sequence <= m_lastSeenSequence[peerIdx]) { return; }
    m_lastSeenSequence[peerIdx] = pkt.header.sequence;

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
        const uint8_t peerIdx = pkt.header.senderId - 1U;
        if (peerIdx >= Networking::NetworkService::MAX_PEERS) { return; }
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
        m_service->Broadcast(&pkt, sizeof(pkt));
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
        const uint8_t peerIdx = pkt.header.senderId - 1U;
        if (peerIdx >= Networking::NetworkService::MAX_PEERS) { return; }
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
    if ((m_service == nullptr) || !m_service->IsConnected()) { return; }
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
            GE_LOG_INFO("NetworkBridge: ignoring DiscoveryHello — scene mismatch ("
                        + incomingScene + ")");
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

    GE_LOG_INFO("NetworkBridge: sent DiscoveryResponse (peer " + std::to_string(myId) + ")");
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
    m_service->AddPeer(peerID, ipBuf, peerPort);
    RegisterScenePeer(peerID);  // arrived via scene-matched discovery chain

    GE_LOG_INFO("NetworkBridge: peer " + std::to_string(peerID) +
                " announced itself from " + ipBuf);
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
        static constexpr uint16_t BASE_PORT      = 54000U;
        static constexpr uint16_t DISCOVERY_PORT = 54998U;
        static constexpr int      WAIT_MS        = 1500;

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
        local.sin_port        = htons(DISCOVERY_PORT);
        if (bind(tempSock, reinterpret_cast<sockaddr*>(&local), sizeof(local)) == SOCKET_ERROR) {
            m_autoConnectStatus = "Failed: port 54998 already in use";
            m_autoConnectState.store(AutoConnectState::Failed);
            closesocket(tempSock);
            WSACleanup();
            return;
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
            GE_LOG_INFO("NetworkBridge: unicasting DiscoveryHello to " + hostIP);
        } else {
            dest.sin_addr.s_addr = INADDR_BROADCAST;
            GE_LOG_INFO("NetworkBridge: broadcasting DiscoveryHello");
        }

        for (int p = 0; p < 4; ++p) {
            dest.sin_port = htons(static_cast<uint16_t>(BASE_PORT + p));
            sendto(tempSock,
                   reinterpret_cast<const char*>(&hello),
                   static_cast<int>(sizeof(hello)), 0,
                   reinterpret_cast<sockaddr*>(&dest),
                   static_cast<int>(sizeof(dest)));
        }

        // --- Collect DiscoveryResponse packets for WAIT_MS milliseconds ---
        // key = peerID (1–4), value = sender IP (network byte order)
        std::map<uint8_t, uint32_t> discovered;

        const auto deadline = std::chrono::steady_clock::now() +
                              std::chrono::milliseconds(WAIT_MS);

        while (std::chrono::steady_clock::now() < deadline && !stopToken.stop_requested()) {
            fd_set readSet;
            FD_ZERO(&readSet);
            FD_SET(tempSock, &readSet);
            timeval tv{ 0, 50'000 };  // 50 ms poll interval
            const int ready = select(0, &readSet, nullptr, nullptr, &tv);
            if (ready <= 0) { continue; }

            char buf[256]{};  // must hold DiscoveryResponse (136 bytes)
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
                GE_LOG_INFO("NetworkBridge: skipping DiscoveryResponse from peer "
                            + std::to_string(resp.peerID) + " — scene mismatch ("
                            + std::string(resp.scenePath) + ")");
                continue;
            }

            discovered[resp.peerID] = from.sin_addr.s_addr;
        }

        closesocket(tempSock);
        WSACleanup();

        if (stopToken.stop_requested()) { return; }

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

        // --- Announce ourselves to all discovered peers so they can add us back ---
        // This fixes one-way discovery: when we found existing peers but they didn't
        // know about us (because we weren't running when they connected).
        if (!discovered.empty()) {
            Networking::Packets::PeerAnnounce announce{};
            announce.header.type     = Networking::Packets::PacketType::PeerAnnounce;
            announce.header.senderId = slot;
            announce.peerID          = slot;
            m_service->Broadcast(&announce, sizeof(announce));
        }

        // --- Finalise ---
        m_autoConnectStatus = "Peer " + std::to_string(slot) +
                              " | Port " + std::to_string(gamePort) +
                              (connectedStr.empty()
                                  ? " | No peers found yet"
                                  : " | Connected to: " + connectedStr);

        m_pendingPostConnectSync.store(true);
        m_autoConnectState.store(AutoConnectState::Done);

        GE_LOG_INFO("NetworkBridge: auto-connect complete — " + m_autoConnectStatus);
    });
}

} // namespace GE
