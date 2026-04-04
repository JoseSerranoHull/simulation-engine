#pragma once

/* parasoft-begin-suppress ALL */
#include <cstdint>
#include <array>
#include <chrono>
#include <mutex>
#include <optional>
#include <string>
/* parasoft-end-suppress ALL */

// NetworkBridge is the ONLY class permitted to include both networking and ECS headers.
#include "networking/NetworkService.h"
#include "networking/Packets.h"
#include "ecs/EntityManager.h"
#include "components/PhysicsComponents.h"
#include "components/Transform.h"

namespace GE {

    /**
     * @class NetworkBridge
     * @brief Sole mediator between GE::Networking:: and the ECS.
     *
     * Design rule: nothing in GE::Networking:: is allowed to include ECS or Vulkan
     * headers.  This class breaks the isolation intentionally and is the single
     * seam where packet bytes are translated to/from ECS component data.
     */
    class NetworkBridge {
    public:
        NetworkBridge(Networking::NetworkService* service,
                      GE::ECS::EntityManager*    entityManager)
            : m_service(service), m_entityManager(entityManager)
        {}

        // Non-copyable
        NetworkBridge(const NetworkBridge&)            = delete;
        NetworkBridge& operator=(const NetworkBridge&) = delete;

        // --- State Sync ---

        /**
         * @brief Packs a StateUpdate for every entity owned by the local peer
         *        and broadcasts it to all registered peers.
         *
         * Call site: physics thread, after each completed tick, throttled to
         * ~60 broadcasts/sec.
         */
        void BroadcastOwnedStates();

        /**
         * @brief Deserialises an incoming datagram and dispatches it to the
         *        appropriate handler (StateUpdate / SceneChange / SpawnObject).
         *
         * Call site: networking thread poll callback.
         */
        void ApplyReceivedState(uint8_t senderId, const uint8_t* data, std::size_t size);

        /**
         * @brief Packs and broadcasts a SceneChange packet (sent 3× for reliability).
         * Call site: main thread (ImGui Scene menu).
         */
        void BroadcastSceneChange(const std::string& path);

        /**
         * @brief Packs and broadcasts a SpawnObject packet.
         * Call site: SpawnerSystem (owning peer, after local activation).
         */
        void BroadcastSpawnObject(uint32_t entityId, uint8_t ownerPeerId, uint8_t shapeType,
                                  const glm::vec3& position, const glm::vec3& scale,
                                  const glm::vec3& linearVelocity, float mass);

        /**
         * @brief Returns and clears any pending network-triggered scene path.
         *        Thread-safe — call from the main thread each frame.
         */
        std::optional<std::string> PollPendingSceneChange();

        // --- Accessors used by EngineOrchestrator / ImGui ---

        Networking::NetworkService* GetService() const { return m_service; }

    private:
        Networking::NetworkService* m_service       { nullptr };
        GE::ECS::EntityManager*     m_entityManager  { nullptr };

        /// Per-sender last accepted sequence number (peers 1-4 → indices 0-3).
        std::array<uint16_t, Networking::NetworkService::MAX_PEERS> m_lastSeenSequence {};

        /// Sequence counter for packets we emit.
        uint16_t m_outSequence { 0 };

        /// Throttle: time-point of the last broadcast.
        std::chrono::steady_clock::time_point m_lastBroadcast {};

        /// Pending network-triggered scene change (networking thread writes, main thread reads).
        std::string m_pendingNetworkScene;
        std::mutex  m_pendingNetworkSceneMutex;

        // --- Per-type packet handlers (called by ApplyReceivedState) ---
        void handleStateUpdate(uint8_t senderId, const uint8_t* data, std::size_t size);
        void handleSceneChange(const uint8_t* data, std::size_t size);
        void handleSpawnObject(const uint8_t* data, std::size_t size);
    };

} // namespace GE
