#include "core/NetworkBridge.h"
#include "core/Logger.h"

/* parasoft-begin-suppress ALL */
#include <cstring>
#include <algorithm>
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

        auto* tr = m_entityManager->GetTIComponent<GE::Components::Transform>(entityId);
        auto* rb = m_entityManager->GetTIComponent<GE::Components::RigidBody>(entityId);
        if ((tr == nullptr) || (rb == nullptr)) { continue; }

        Networking::Packets::StateUpdate pkt{};
        pkt.header.type     = Networking::Packets::PacketType::StateUpdate;
        pkt.header.senderId = localId;
        pkt.header.sequence = m_outSequence++;

        pkt.entityId        = entityId;
        pkt.position        = tr->m_position;
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
                                       std::size_t    size)
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
    default:
        break;  // Heartbeat and unknown types are silently ignored
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

    if (pkt.header.sequence <= m_lastSeenSequence[peerIdx]) { return; }
    m_lastSeenSequence[peerIdx] = pkt.header.sequence;

    auto* tr = m_entityManager->GetTIComponent<GE::Components::Transform>(pkt.entityId);
    auto* rb = m_entityManager->GetTIComponent<GE::Components::RigidBody>(pkt.entityId);

    if (tr != nullptr) {
        tr->m_position = pkt.position;
        const glm::quat q(pkt.orientation[3],   // w
                          pkt.orientation[0],   // x
                          pkt.orientation[1],   // y
                          pkt.orientation[2]);  // z
        const glm::mat3 rotMat = glm::mat3_cast(q);
        const glm::vec3 scale  = tr->m_scale;
        tr->m_worldMatrix = glm::mat4(
            glm::vec4(rotMat[0] * scale.x, 0.0f),
            glm::vec4(rotMat[1] * scale.y, 0.0f),
            glm::vec4(rotMat[2] * scale.z, 0.0f),
            glm::vec4(pkt.position,         1.0f)
        );
    }

    if (rb != nullptr) {
        rb->velocity        = pkt.linearVelocity;
        rb->angularVelocity = pkt.angularVelocity;
    }
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

    auto* tr = m_entityManager->TryGetTIComponent<GE::Components::Transform>(pkt.entityId);
    auto* rb = m_entityManager->TryGetTIComponent<GE::Components::RigidBody>(pkt.entityId);

    if ((tr == nullptr) || (rb == nullptr)) {
        GE_LOG_WARN("NetworkBridge: SPAWN_OBJECT for unknown entity "
                    + std::to_string(pkt.entityId) + " — ignored.");
        return;
    }

    // Activate the entity: move it into the world and enable physics
    tr->m_position = pkt.position;
    tr->m_scale    = pkt.scale;
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

} // namespace GE
