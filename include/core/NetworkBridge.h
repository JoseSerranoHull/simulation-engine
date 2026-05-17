#pragma once

/* parasoft-begin-suppress ALL */
#include <atomic>
#include <cstdint>
#include <array>
#include <chrono>
#include <deque>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
/* parasoft-end-suppress ALL */

// NetworkBridge is the ONLY class permitted to include both networking and ECS headers.
#include "networking/NetworkService.h"
#include "networking/Packets.h"
#include "ecs/EntityManager.h"
#include "components/AnimationComponent.h"
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
        NetworkBridge(Networking::NetworkService* service, GE::ECS::EntityManager* entityManager)
            : m_service(service), m_entityManager(entityManager)
        {}

        // Non-copyable
        NetworkBridge(const NetworkBridge&) = delete;
        NetworkBridge& operator=(const NetworkBridge&) = delete;

        // --- State Sync ---

        /**
         * @brief Packs a StateUpdate for every entity owned by the local peer
         * and broadcasts it to all registered peers.
         *
         * Call site: physics thread, after each completed tick, throttled to
         * ~60 broadcasts/sec.
         */
        void BroadcastOwnedStates();

        /**
         * @brief Deserialises an incoming datagram and dispatches it to the
         * appropriate handler (StateUpdate / SceneChange / SpawnObject / Discovery).
         *
         * Call site: networking thread poll callback.
         * senderAddr / senderPort are in network byte order (from recvfrom).
         */
        void ApplyReceivedState(uint8_t senderId, const uint8_t* data, std::size_t size, uint32_t senderAddr = 0, uint16_t senderPort = 0);

        /**
         * @brief Packs and broadcasts a SceneChange packet (sent 3× for reliability).
         * Call site: main thread (ImGui Scene menu).
         */
        void BroadcastSceneChange(const std::string& path);

        /**
         * @brief Broadcasts a PeerLeave packet (sent 3× for reliability) so every
         * connected peer frees this slot before the local socket shuts down.
         * Call site: disconnectNetwork(), BEFORE NetworkService::Shutdown().
         */
        void BroadcastPeerLeave();

        /**
         * @brief Broadcasts the current elapsed/reversed state for every
         * AnimatedObjectComponent so remote peers can snap their local
         * animation timers to match. Call once after connecting.
         */
        void BroadcastAnimationStates();

        /**
         * @brief Packs and broadcasts a SpawnObject packet.
         * Call site: SpawnerSystem (owning peer, after local activation).
         */
        void BroadcastSpawnObject(uint32_t entityId,
            uint8_t ownerPeerId,
            uint8_t shapeType,
            const glm::vec3& position,
            const glm::vec3& scale,
            const glm::vec3& linearVelocity,
            float mass
        );

        /**
         * @brief Returns and clears any pending network-triggered scene path.
         *        Thread-safe — call from the main thread each frame.
         */
        std::optional<std::string> PollPendingSceneChange();

        /**
         * @brief Applies dead-reckoned positions to all tracked remote entities.
         *        Smoothly blends from the last rendered position toward the predicted
         *        authoritative position over BLEND_DURATION seconds.
         *
         * Call site: physics thread, immediately after BroadcastOwnedStates().
         * Lock ordering: physics thread acquires simMutex first, then remoteStatesMutex here.
         */
        void UpdateRemoteEntities(float dt);

        /**
         * @brief Clears all tracked remote entity states.
         *        Call on scene change to prevent stale IDs from a previous scene.
         */
        void ClearRemoteStates();

        /**
         * @brief Registers the current scene path used for discovery filtering.
         *        Call from FlatBuffersScenario::OnLoad (with path) and OnUnload (with "").
         *        Thread-safe — networking thread reads this during discovery.
         */
        void SetCurrentScene(const std::string& path);

        /**
         * @brief Marks a peer as scene-compatible so its state packets are accepted.
         *        Call alongside every AddPeer (auto-connect, manual connect, PeerAnnounce).
         *        Cleared by ClearRemoteStates() on scene change / disconnect.
         */
        void RegisterScenePeer(uint8_t peerId);

        // --- Auto-connect (LAN peer discovery) ---

        enum class AutoConnectState { Idle, Discovering, Done, Failed };

        /**
         * @brief Starts LAN peer discovery in a background jthread.
         * Broadcasts DiscoveryHello to fixed game ports, waits 1.5 s for responses,
         * picks the lowest free slot (1–4), initialises the game socket, and
         * registers all discovered peers. Safe to call from ImGui (returns immediately).
         */
        void BeginAutoConnect(const std::string& hostIP = "");

        AutoConnectState GetAutoConnectState() const { return m_autoConnectState.load(); }
        const std::string& GetAutoConnectStatus() const { return m_autoConnectStatus; }

        void ResetAutoConnect() {
            if (m_discoveryThread.joinable()) {
                m_discoveryThread.request_stop();
                m_discoveryThread.join();
            }
            m_autoConnectState.store(AutoConnectState::Idle);
            m_autoConnectStatus = "Idle";
            m_pendingPostConnectSync.store(false);
        }

        /**
         * @brief Returns true (and clears the flag) once on the first call after
         * auto-connect completes. Use from the physics/update thread to fire
         * BroadcastAnimationStates() and other post-connect ECS syncs.
         */
        bool ConsumePostConnectSync() {
            bool expected = true;
            return m_pendingPostConnectSync.compare_exchange_strong(expected, false);
        }

        // --- Accessors used by EngineOrchestrator / ImGui / Scripts ---

        Networking::NetworkService* GetService() const { return m_service; }

        uint8_t GetLocalPeerId() const {
            return (m_service != nullptr) ? m_service->GetLocalPeerId() : 0U;
        }

        std::size_t GetRemoteStateCount() const {
            std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(m_remoteStatesMutex));
            return m_remoteStates.size();
        }

        // --- Network Diagnostics ---

        /// A timestamped discovery event logged on every significant handshake step.
        /// Displayed in the ImGui Network → Diagnostics panel and echoed to the terminal.
        struct DiscoveryEvent {
            std::string timestamp; // < "HH:MM:SS"
            std::string text; // < human-readable description of the event
        };

        /// Thread-safe snapshot of the last MAX_DISCOVERY_LOG events. Safe to call from ImGui.
        std::vector<DiscoveryEvent> GetDiscoveryLogSnapshot() const;

        /// Clears the discovery log. Call from disconnectNetwork() between sessions.
        void ClearDiscoveryLog();

        /// Wall-clock milliseconds when the last StateUpdate arrived from peerId (1–4).
        /// Returns 0 if no StateUpdate has been received yet from that peer.
        uint64_t GetPeerLastPacketMs(uint8_t peerId) const;

    private:
        // Dead reckoning state for each tracked remote entity.
        // Lock ordering:
        // physics thread acquires simMutex FIRST, then remoteStatesMutex.
        // networking thread acquires remoteStatesMutex ONLY (never simMutex).
        struct RemoteEntityState {
            glm::vec3 authPosition { 0.0f };  // last received authoritative position
            glm::vec3 authVelocity { 0.0f };  // last received authoritative velocity
            glm::vec3 renderPosition { 0.0f };  // current interpolated position applied to entity
            double authTimeSec { 0.0 };   // wall-clock time when authPosition was received
            float blendTimer { 0.0f };  // seconds remaining in blend (0 = pure dead reckoning)
            static constexpr float BLEND_DURATION = 0.12f; // 120 ms blend window
        };

        std::unordered_map<uint32_t, RemoteEntityState> m_remoteStates;
        std::mutex m_remoteStatesMutex;

        Networking::NetworkService* m_service { nullptr };
        GE::ECS::EntityManager* m_entityManager { nullptr };

        /// Per-sender last accepted sequence number (peers 1-4 → indices 0-3).
        std::array<uint16_t, Networking::NetworkService::MAX_PEERS> m_lastSeenSequence {};

        /// Sequence counter for packets we emit.
        uint16_t m_outSequence { 0 };

        /// Current scene path — used to filter discovery to same-scene peers only.
        std::string m_currentScenePath;
        std::mutex  m_scenePathMutex;

        /// Bitmask of scene-matched peers whose state packets should be applied.
        /// Bit (peerId-1) is set by RegisterScenePeer; cleared by ClearRemoteStates.
        std::atomic<uint8_t> m_acceptedPeerMask { 0 };

        /// Throttle: time-point of the last broadcast.
        std::chrono::steady_clock::time_point m_lastBroadcast {};

        /// Pending network-triggered scene change (networking thread writes, main thread reads).
        std::string m_pendingNetworkScene;
        std::mutex  m_pendingNetworkSceneMutex;

        // Auto-connect state
        std::atomic<AutoConnectState> m_autoConnectState { AutoConnectState::Idle };
        std::string m_autoConnectStatus { "Idle" };
        std::jthread m_discoveryThread;
        std::atomic<bool> m_pendingPostConnectSync { false };

        // --- Network diagnostics (private storage) ---

        /// Wall-clock ms of last received StateUpdate per peer (networking thread writes, main reads).
        std::array<std::atomic<uint64_t>, Networking::NetworkService::MAX_PEERS> m_peerLastPacketMs {};

        /// Discovery event ring buffer (mutex-protected; written by jthread + networking thread).
        mutable std::mutex m_discoveryLogMutex;
        std::deque<DiscoveryEvent> m_discoveryLog;
        static constexpr std::size_t MAX_DISCOVERY_LOG { 14U };

        /// Appends a discovery event with the current wall-clock timestamp.
        /// Also emits GE_LOG_INFO so the same text appears in the terminal.
        void logDiscovery(const std::string& text);

        // --- Per-type packet handlers (called by ApplyReceivedState) ---
        void handleStateUpdate (uint8_t senderId, const uint8_t* data, std::size_t size);
        void handleSceneChange (const uint8_t* data, std::size_t size);
        void handleSpawnObject (const uint8_t* data, std::size_t size);
        void handleAnimationSync (const uint8_t* data, std::size_t size);
        void handleDiscoveryHello (uint32_t senderAddr, uint16_t senderPort, const uint8_t* data, std::size_t size);
        void handlePeerAnnounce (uint8_t peerID, uint32_t senderAddr);
        void handlePeerLeave (uint8_t peerID);
    };

} // namespace GE
