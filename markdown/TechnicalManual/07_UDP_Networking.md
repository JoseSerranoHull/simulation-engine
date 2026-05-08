# Chapter 7 — UDP Peer-to-Peer Networking

## 7.1 Why UDP Instead of TCP?

For physics state synchronisation in a real-time multiplayer simulation, UDP has exactly the right properties:

| Property | TCP | UDP | Why UDP Wins |
|---|---|---|---|
| **Delivery** | Guaranteed | Best-effort | An old physics position is useless — drop it |
| **Order** | Maintained | Out-of-order possible | We track sequence numbers ourselves |
| **Latency** | Higher (retransmit delays) | Lower | No waiting for lost packets |
| **Overhead** | Higher (connection state) | Lower | We add only a 4-byte header |

**The key insight:** If we send position updates at 60 Hz and a packet is lost, we do not need the old position — we can predict where the entity is using its last known velocity (**dead reckoning**). Retransmitting a stale physics update would be worse than ignoring it.

---

## 7.2 Architecture: Three-Layer Design

One of the most important design constraints in this engine is that **networking code must not depend on ECS or Vulkan**. This keeps the networking layer portable, testable, and easy to reason about. The design splits into three layers:

```
┌─────────────────────────────────────────────┐
│           ECS Layer                         │
│  EntityManager · Transform · RigidBody      │
│  (include/ecs/, include/components/)        │
└───────────────────┬─────────────────────────┘
                    │ ← Only NetworkBridge crosses this boundary
┌───────────────────▼─────────────────────────┐
│           Bridge Layer                      │
│  NetworkBridge   (include/core/)            │
│  · BroadcastOwnedStates()                  │
│  · UpdateRemoteEntities(dt)                 │
│  · ApplyReceivedState()                     │
│  · BeginAutoConnect(hostIP)                 │
└───────────────────┬─────────────────────────┘
                    │
┌───────────────────▼─────────────────────────┐
│           Network Layer                     │
│  NetworkService  (include/networking/)      │
│  · Init(port) · Poll(cb) · Broadcast(data) │
│                                             │
│  Packets.h — all packet struct definitions  │
└─────────────────────────────────────────────┘
```

**The rule:** `NetworkService.h` and `Packets.h` must **never** include `EntityManager.h`, `Transform.h`, or any Vulkan header. `NetworkBridge` is the **only** class permitted to include both.

---

## 7.3 Packet Definitions (`include/networking/Packets.h`)

Every datagram starts with a 4-byte `Header` followed by a packet-type-specific payload. All structs are `#pragma pack(1)` so there is no compiler padding — what you see is exactly what goes over the wire.

```cpp
enum class PacketType : uint8_t {
    Heartbeat         = 0,  // Keep-alive, no payload (4 bytes total)
    StateUpdate       = 1,  // Physics state — sent ~60×/sec per owned entity
    SceneChange       = 2,  // Tell peers to load a new scene
    SpawnObject       = 3,  // Activate a pooled entity on all peers
    AnimationSync     = 4,  // Sync animated object timer state
    DiscoveryHello    = 5,  // Auto-connect probe from a new peer
    DiscoveryResponse = 6,  // Reply from an already-connected peer
    PeerAnnounce      = 7,  // Broadcast after auto-connect: "I am Peer N"
};

// ── Header — 4 bytes ───────────────────────────────────────────────────────
struct Header {
    PacketType type     { PacketType::Heartbeat };
    uint8_t    senderId { 0 };    // Peer ID of the sender (1–4)
    uint16_t   sequence { 0 };   // Monotonically increasing; drop if ≤ last seen
};
static_assert(sizeof(Header) == 4);

// ── StateUpdate — 60 bytes ─────────────────────────────────────────────────
// Sent once per physics tick per owned entity, throttled to ~60 packets/sec.
struct StateUpdate {
    Header    header          {};
    uint32_t  entityId        { 0 };
    glm::vec3 position        { 0.0f };
    float     orientation[4]  { 0.0f, 0.0f, 0.0f, 1.0f }; // quaternion x,y,z,w
    // Note: stored as float[4] rather than glm::quat to avoid including
    // glm/gtc/quaternion.hpp in a pure networking header.
    glm::vec3 linearVelocity  { 0.0f };
    glm::vec3 angularVelocity { 0.0f };
};

// ── SceneChange — 132 bytes ────────────────────────────────────────────────
// Sent 3× for reliability (UDP has no retransmit). Receiver queues the path
// for the main thread to pick up safely via PollPendingSceneChange().
struct SceneChange {
    Header header      {};
    char   scenePath[128] {};   // Relative path to the .bin file
};

// ── SpawnObject — activates a pre-pooled entity on remote peers ────────────
struct SpawnObject {
    Header    header          {};
    uint32_t  entityId        { 0 };
    uint8_t   ownerPeerId     { 0 };
    uint8_t   shapeType       { 0 };   // 0=sphere, 1=box, 2=capsule
    uint8_t   _pad[2]         {};
    glm::vec3 position        { 0.0f };
    glm::vec3 scale           { 1.0f };
    glm::vec3 linearVelocity  { 0.0f };
    float     mass            { 1.0f };
};

// ── AnimationSync — syncs animated platform timers after connecting ─────────
struct AnimationSync {
    Header   header   {};
    uint32_t entityId { 0 };
    float    elapsed  { 0.0f };  // Current time along the waypoint path
    uint8_t  reversed { 0 };     // 0=forward, 1=reversed
    uint8_t  _pad[3]  {};
};

// ── Discovery packets (auto-connect handshake) ─────────────────────────────
// DiscoveryHello: sent by a new peer probing for existing peers.
struct DiscoveryHello {
    Header header { PacketType::DiscoveryHello };
    char   scenePath[128] {};  // The scene the new peer is currently on
};

// DiscoveryResponse: reply from an already-connected peer.
struct DiscoveryResponse {
    Header  header { PacketType::DiscoveryResponse };
    uint8_t peerID { 0 };      // 1–4: which slot this peer currently occupies
    uint8_t _pad[3]{};
    char    scenePath[128] {}; // The scene this peer is on (for scene filtering)
};

// PeerAnnounce: broadcast after auto-connect so existing peers can add us.
struct PeerAnnounce {
    Header  header { PacketType::PeerAnnounce };
    uint8_t peerID { 0 };      // Slot just claimed (1–4)
    uint8_t _pad[3]{};
};
```

---

## 7.4 NetworkService — The Raw UDP Layer

`NetworkService` (`include/networking/NetworkService.h`) wraps Winsock2. It knows nothing about the ECS — it just sends bytes to addresses.

```cpp
namespace GE::Networking {

class NetworkService {
public:
    static constexpr uint8_t MAX_PEERS = 4;

    // Callback type for received packets.
    // senderAddr/senderPort are in network byte order (from recvfrom).
    using ReceiveCallback = std::function<void(
        uint8_t senderId, const uint8_t* data, std::size_t size,
        uint32_t senderAddr, uint16_t senderPort)>;

    bool    Init(uint16_t localPort);  // Bind a non-blocking UDP socket
    void    Shutdown();                // Close socket + clear peer table

    void    AddPeer(uint8_t peerId, const std::string& ip, uint16_t port);
    void    Send(uint8_t peerId, const void* data, std::size_t size);
    void    Broadcast(const void* data, std::size_t size); // Send to all peers

    void    Poll(const ReceiveCallback& cb); // Non-blocking drain of recv queue
    void    SendRaw(uint32_t addr, uint16_t port,
                    const void* data, std::size_t size); // One-off unicast
    bool    EnableBroadcast();         // Sets SO_BROADCAST on the socket
    std::string GetLocalIPString() const; // Returns "192.168.x.x"

    bool    IsConnected()    const { return m_initialised; }
    uint8_t GetLocalPeerId() const { return m_localPeerId; }
    void    SetLocalPeerId(uint8_t id) { m_localPeerId = id; }

private:
    // SOCKET stored as uintptr_t to keep <winsock2.h> out of this header.
    // Every file including NetworkService.h would otherwise transitively
    // pull in winsock2.h, which clashes with <windows.h> unless ordered correctly.
    uintptr_t m_socket { ~static_cast<uintptr_t>(0) };  // INVALID_SOCKET sentinel
    bool      m_initialised { false };
    uint8_t   m_localPeerId { 1 };

    struct PeerEntry {
        bool     active { false };
        uint32_t addr   { 0 };  // sin_addr.s_addr, network byte order
        uint16_t port   { 0 };  // sin_port, network byte order
    };
    std::array<PeerEntry, MAX_PEERS> m_peers {};
};

} // namespace GE::Networking
```

### Key implementation detail — `Shutdown()` clears the peer table

When `Shutdown()` is called (e.g., when a player changes scene), it not only closes the socket but also zeros all `m_peers` entries:

```cpp
void NetworkService::Shutdown() {
    if (m_initialised) {
        closesocket(static_cast<SOCKET>(m_socket));
        m_socket      = INVALID_SOCK;
        m_initialised = false;
        WSACleanup();
        for (auto& peer : m_peers) { peer = PeerEntry{}; }  // ← Clear peer table
    }
}
```

**Why this matters:** Without this, a player who switches scenes and reconnects would still have the old peer table. `BroadcastOwnedStates()` would keep sending state updates to the old peers — leaking physics data to a completely different scene session. Clearing on shutdown ensures each connection starts from a clean slate.

### `Poll()` — non-blocking receive loop

```cpp
void NetworkService::Poll(const ReceiveCallback& cb) {
    static char buf[2048];
    sockaddr_in from{};
    int fromLen = sizeof(from);

    for (;;) {
        const int n = recvfrom(static_cast<SOCKET>(m_socket),
                               buf, sizeof(buf), 0,
                               reinterpret_cast<sockaddr*>(&from), &fromLen);

        if (n == SOCKET_ERROR) {
            if (WSAGetLastError() == WSAEWOULDBLOCK) break; // No more data
            if (WSAGetLastError() == WSAECONNRESET)  continue; // ICMP "port unreachable"
            break;
        }

        if (n < static_cast<int>(sizeof(Packets::Header))) continue; // Too small

        Packets::Header hdr{};
        std::memcpy(&hdr, buf, sizeof(hdr));

        // Pass raw bytes + sender address to the bridge callback
        cb(hdr.senderId,
           reinterpret_cast<const uint8_t*>(buf), static_cast<std::size_t>(n),
           from.sin_addr.s_addr, from.sin_port);
    }
}
```

The networking jthread calls `Poll()` in a tight loop with a 1 ms sleep, pinned to CPU cores 2–3 via `SetThreadAffinityMask`.

---

## 7.5 NetworkBridge — The ECS–Network Seam

`NetworkBridge` (`include/core/NetworkBridge.h`) is the **only** class permitted to include both ECS headers and networking headers. It translates between the two worlds.

### Sending — `BroadcastOwnedStates()`

Called on the physics thread after every physics tick, throttled to ~60 packets/sec. Only entities owned by the local peer are broadcast:

```cpp
void NetworkBridge::BroadcastOwnedStates() {
    if (!m_service->IsConnected()) return;

    // Throttle: don't send more often than once every 1/60 s
    const auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration<float>(now - m_lastBroadcast).count()
        < (1.0f / 60.0f)) return;
    m_lastBroadcast = now;

    const uint8_t localId = m_service->GetLocalPeerId();
    // OwnerType::ONE == 0, TWO == 1, ... maps 1-indexed peer ID to enum
    const auto localOwner = static_cast<GE::Components::OwnerType>(localId - 1U);

    auto& ownerArr = em->GetCompArr<GE::Components::OwnerComponent>();

    for (uint32_t i = 0; i < ownerArr.GetCount(); ++i) {
        if (ownerArr.Data()[i].owner != localOwner) continue; // Skip remote entities

        const uint32_t eid = ownerArr.Index()[i];
        auto* tr = em->TryGetTIComponent<Transform>(eid);
        auto* rb = em->TryGetTIComponent<RigidBody>(eid);
        if (!tr || !rb) continue;

        Packets::StateUpdate pkt{};
        pkt.header.type     = PacketType::StateUpdate;
        pkt.header.senderId = localId;
        pkt.header.sequence = m_outSequence++;
        pkt.entityId        = eid;
        pkt.position        = tr->m_worldPosition;
        pkt.linearVelocity  = rb->velocity;
        pkt.angularVelocity = rb->angularVelocity;

        // Decompose world matrix to quaternion, store as x,y,z,w floats
        const glm::quat q = glm::quat_cast(glm::mat3(tr->m_worldMatrix));
        pkt.orientation[0] = q.x; pkt.orientation[1] = q.y;
        pkt.orientation[2] = q.z; pkt.orientation[3] = q.w;

        m_service->Broadcast(&pkt, sizeof(pkt));
    }
}
```

### Receiving — `ApplyReceivedState()`

Called by the networking thread's poll callback when a packet arrives. Dispatches to a handler based on `PacketType`:

```cpp
void NetworkBridge::ApplyReceivedState(uint8_t senderId,
                                       const uint8_t* data, std::size_t size,
                                       uint32_t senderAddr, uint16_t senderPort)
{
    if (size < sizeof(Packets::Header)) return;
    Packets::Header hdr{};
    std::memcpy(&hdr, data, sizeof(hdr));

    switch (hdr.type) {
    case PacketType::StateUpdate:
        handleStateUpdate(senderId, data, size);    break;
    case PacketType::SceneChange:
        handleSceneChange(data, size);              break;
    case PacketType::SpawnObject:
        handleSpawnObject(data, size);              break;
    case PacketType::AnimationSync:
        handleAnimationSync(data, size);            break;
    case PacketType::DiscoveryHello:
        handleDiscoveryHello(senderAddr, senderPort, data, size); break;
    case PacketType::PeerAnnounce:
        handlePeerAnnounce(pkt.peerID, senderAddr); break;
    default: break;
    }
}
```

### Scene-Matched Peer Filtering — The Accepted Peer Mask

Once connected, the engine must ensure it only applies state from peers that are **on the same scene**. A peer that switches to a different scene should stop affecting your simulation immediately.

This is solved with an atomic bitmask: `m_acceptedPeerMask` (1 bit per peer slot, peer `N` → bit `N-1`).

```cpp
// In NetworkBridge (private):
std::atomic<uint8_t> m_acceptedPeerMask { 0 };  // cleared on scene change

// Set when a peer is registered through scene-matched auto-connect or manual connect:
void NetworkBridge::RegisterScenePeer(uint8_t peerId) {
    m_acceptedPeerMask.fetch_or(1U << (peerId - 1U));
}

// Cleared when the scene changes (via ClearRemoteStates()):
void NetworkBridge::ClearRemoteStates() {
    std::lock_guard lock(m_remoteStatesMutex);
    m_remoteStates.clear();
    m_acceptedPeerMask.store(0);  // ← All peers become untrusted after a scene change
}
```

Every state-applying handler checks this mask first:

```cpp
void NetworkBridge::handleStateUpdate(uint8_t senderId, ...) {
    // Explicit range check before subtraction (senderId=0 would wrap uint8_t)
    if (senderId < 1U || senderId > MAX_PEERS) return;
    const uint8_t peerIdx = senderId - 1U;

    // Drop packets from peers not registered for the current scene
    if (!((m_acceptedPeerMask.load() >> peerIdx) & 1U)) return;

    // ...rest of handler (sequence check, dead reckoning update)
}
```

The same mask check guards `handleSpawnObject` and `handleAnimationSync`.

### Scene Path Registration — `SetCurrentScene()`

`NetworkBridge` needs to know the current scene path for two reasons:
1. To include it in `DiscoveryHello` / `DiscoveryResponse` packets (so peers can reject mismatched scenes)
2. To filter out `DiscoveryHello` from peers on different scenes

```cpp
// Called from FlatBuffersScenario::OnLoad() after loading the scene binary
void NetworkBridge::SetCurrentScene(const std::string& path) {
    std::lock_guard lock(m_scenePathMutex);
    m_currentScenePath = path;
}
// Called from FlatBuffersScenario::OnUnload() before teardown
// -> called with "" to clear
```

---

## 7.6 Dead Reckoning + Blend Correction

Between received packets (arriving at ~60 Hz), remote entities must still move smoothly. The engine uses **dead reckoning** — predicting position from the last known velocity:

```
predictedPos = lastAuthPosition + lastAuthVelocity × (time since last packet)
```

However, predictions drift. When a new authoritative packet arrives, snapping to the corrected position causes a visible teleport. Instead, the engine blends from the current rendered position toward the prediction over **120 milliseconds**:

```cpp
// Dead-reckoning state per remote entity (NetworkBridge private struct):
struct RemoteEntityState {
    glm::vec3 authPosition  { 0.0f };  // Last received authoritative position
    glm::vec3 authVelocity  { 0.0f };  // Last received authoritative velocity
    glm::vec3 renderPosition{ 0.0f };  // Current interpolated position on screen
    double    authTimeSec   { 0.0  };  // Wall-clock time of last packet
    float     blendTimer    { 0.0f };  // Seconds left in the blend window
    static constexpr float BLEND_DURATION = 0.12f;  // 120 ms blend
};
```

```cpp
// NetworkBridge::UpdateRemoteEntities() — runs on physics thread each tick
void NetworkBridge::UpdateRemoteEntities(float dt) {
    std::lock_guard lock(m_remoteStatesMutex);

    for (auto& [entityId, rs] : m_remoteStates) {
        auto* tr = em->TryGetTIComponent<Transform>(entityId);
        auto* rb = em->TryGetTIComponent<RigidBody>(entityId);
        if (!tr || !rb) continue;

        // Guard: if no packet received in 2 s, peer has likely disconnected
        const float dtSincePacket = static_cast<float>(
            std::chrono::duration<double>(now - rs.authTimeSec).count());
        if (dtSincePacket > 2.0f) continue;

        // Dead-reckoning prediction
        const glm::vec3 predictedPos = rs.authPosition + rs.authVelocity * dtSincePacket;

        if (rs.blendTimer > 0.0f) {
            // Active blend: lerp from current position toward the prediction
            const float alpha = glm::clamp(dt / rs.blendTimer, 0.0f, 1.0f);
            rs.renderPosition = glm::mix(tr->m_worldPosition, predictedPos, alpha);
            rs.blendTimer    -= dt;
        } else {
            rs.renderPosition = predictedPos;  // Pure dead reckoning
        }

        tr->m_localPosition  = rs.renderPosition;
        tr->m_worldPosition  = rs.renderPosition;
        tr->m_worldMatrix[3] = glm::vec4(rs.renderPosition, 1.0f);
        rb->velocity         = rs.authVelocity;
    }
}
```

**Timeline walkthrough:**
```
t=0.000s  Packet arrives: authPos=P1, authVel=V1 → blendTimer=0.12
t=0.016s  predict=P1+V1×0.016; renderPos=lerp(current, predict, 0.016/0.12)
t=0.032s  predict=P1+V1×0.032; renderPos=lerp(current, predict, 0.032/0.104)
  ...     (smooth lerp for 120 ms)
t=0.120s  blendTimer=0; renderPos=P1+V1×0.120   (pure dead reckoning)
t=0.133s  blendTimer=0; renderPos=P1+V1×0.133
  ...
t=0.200s  New packet: authPos=P2, authVel=V2 → blendTimer=0.12 (reset)
```

---

## 7.7 Scene Change Synchronisation

When a player changes scene, all connected peers are notified to switch too. The packet is sent **3 times** to compensate for potential UDP loss:

```cpp
void NetworkBridge::BroadcastSceneChange(const std::string& path) {
    if (!m_service->IsConnected()) return;

    Packets::SceneChange pkt{};
    pkt.header.type     = PacketType::SceneChange;
    pkt.header.senderId = m_service->GetLocalPeerId();
    pkt.header.sequence = m_outSequence++;

    const std::size_t len = std::min(path.size(), sizeof(pkt.scenePath) - 1U);
    std::memcpy(pkt.scenePath, path.c_str(), len);

    for (int i = 0; i < 3; ++i) {          // Sent 3× for reliability
        m_service->Broadcast(&pkt, sizeof(pkt));
    }
}
```

On the receiving end, the networking thread writes the path to a thread-safe pending queue:

```cpp
void NetworkBridge::handleSceneChange(const uint8_t* data, std::size_t size) {
    Packets::SceneChange pkt{};
    std::memcpy(&pkt, data, sizeof(pkt));
    pkt.scenePath[127] = '\0';   // Safety null-terminate

    std::lock_guard lock(m_pendingNetworkSceneMutex);
    m_pendingNetworkScene = std::string(pkt.scenePath);
}
```

The main thread polls this once per frame and triggers the scene change safely after `vkDeviceWaitIdle`:

```cpp
// EngineOrchestrator::drawFrame()
if (auto pending = m_networkBridge->PollPendingSceneChange()) {
    requestScenarioChange(*pending);   // GPU-safe deferred switch
}
```

---

## 7.8 Spawn Synchronisation

Spawners pre-create a pool of entities at scene load time (they exist but are invisible at `y = −1000`). When a spawner fires, it activates the entity locally and tells all peers to do the same via `SpawnObject`:

```cpp
// Called from SpawnerSystem — only the spawner's owner peer does this
void NetworkBridge::BroadcastSpawnObject(uint32_t entityId, uint8_t ownerPeerId,
                                          uint8_t shapeType,
                                          const glm::vec3& pos,
                                          const glm::vec3& scale,
                                          const glm::vec3& vel, float mass)
{
    Packets::SpawnObject pkt{};
    pkt.header.type     = PacketType::SpawnObject;
    pkt.header.senderId = m_service->GetLocalPeerId();
    pkt.header.sequence = m_outSequence++;
    pkt.entityId        = entityId;
    pkt.ownerPeerId     = ownerPeerId;
    pkt.shapeType       = shapeType;
    pkt.position        = pos;
    pkt.scale           = scale;
    pkt.linearVelocity  = vel;
    pkt.mass            = mass;
    m_service->Broadcast(&pkt, sizeof(pkt));
}
```

Remote peers receive this and move the matching pool entity (same `entityId`) to the broadcast position, enabling gravity, and setting its initial velocity.

---

## 7.9 Auto-Connect Peer Negotiation

### The Problem with Manual Configuration

The manual connect approach requires each user to:
1. Look up another machine's IP address (run `ipconfig`)
2. Type it into the engine's Network menu
3. Agree on peer IDs (1–4) without any coordination mechanism
4. Both click "Connect" at the right time

In a classroom setting this breaks down — two students both pick peer ID 1, neither realises it, and physics state flickers silently. Auto-connect eliminates all of this.

### How Auto-Connect Works — Two Modes

`NetworkBridge::BeginAutoConnect(const std::string& hostIP)` runs a discovery handshake in a background `std::jthread`. The `hostIP` parameter determines the send strategy:

| Mode | `hostIP` value | What happens |
|---|---|---|
| **Host / First peer** | `""` (empty) | Broadcasts `DiscoveryHello` to `255.255.255.255` on ports 54000–54003. Works reliably on the same machine or a LAN with UDP broadcast enabled. |
| **Joiner** | `"192.168.x.x"` | Unicasts `DiscoveryHello` directly to the host's IP on ports 54000–54003. Deterministic and works on any network. |

### The Full Handshake

```
Step 1 — Joiner sends DiscoveryHello (with its current scene path)
    New peer ──► unicast/broadcast ──► 192.168.1.10:54000 (existing Peer 1)
                                    ──► 192.168.1.10:54001
                                    ──► 192.168.1.10:54002
                                    ──► 192.168.1.10:54003

Step 2 — Existing peer responds (scene-filtered!)
    Peer 1 receives DiscoveryHello:
      · Checks: does incoming scenePath == my scenePath?
      · If NO  → silently ignore (different scene session, no connection)
      · If YES → send DiscoveryResponse { peerID=1, scenePath="…01_multiplayer…" }
                 to the sender's temp socket (port 54998)

Step 3 — Joiner picks lowest free slot
    Joiner collects responses for 1.5 s:
      · discovered = { 1: "192.168.1.10" }
      · Tries slot 1 (taken) → tries slot 2 → binds port 54001 successfully
      · Sets localPeerId = 2

Step 4 — Joiner announces itself
    Peer 2 ──► PeerAnnounce { peerID=2 } ──► broadcast to all game ports
    Peer 1 receives it → handlePeerAnnounce() → AddPeer(2, "192.168.x.x", 54001)
                       → RegisterScenePeer(2)  ← now Peer 1 accepts Peer 2's state
    Both sides now trust each other: m_acceptedPeerMask has the right bit set.
```

### Scene Filtering During Discovery

`DiscoveryHello` and `DiscoveryResponse` both carry a `scenePath[128]` field. The responder checks:

```cpp
void NetworkBridge::handleDiscoveryHello(uint32_t senderAddr, uint16_t senderPort,
                                          const uint8_t* data, std::size_t size)
{
    // Parse incoming scene path
    Packets::DiscoveryHello hello{};
    std::memcpy(&hello, data, sizeof(hello));
    const std::string incomingScene(hello.scenePath);

    // Scene filter: if both sides have a scene set and they differ → reject
    {
        std::lock_guard lock(m_scenePathMutex);
        if (!incomingScene.empty() && !m_currentScenePath.empty()
            && incomingScene != m_currentScenePath) {
            return;  // Different scene — don't respond
        }
    }

    // Build response with our own scene path
    Packets::DiscoveryResponse resp{};
    resp.header.type = PacketType::DiscoveryResponse;
    resp.peerID      = m_service->GetLocalPeerId();
    { /* copy m_currentScenePath into resp.scenePath */ }

    m_service->SendRaw(senderAddr, senderPort, &resp, sizeof(resp));
}
```

The joiner applies the same filter when collecting responses — it ignores any `DiscoveryResponse` whose `scenePath` doesn't match its own. This means **two peers on different scenes will never accidentally join each other's session**.

### Port Fallback

On the same machine (common during development), two instances may both want port 54000. When `Init(gamePort)` fails with `WSAEADDRINUSE`, the engine automatically tries the next slot:

```cpp
uint16_t gamePort = 0U;
while (slot <= 4U) {
    gamePort = static_cast<uint16_t>(BASE_PORT + slot - 1U);
    if (m_service->Init(gamePort)) break;         // Success
    ++slot;                                        // Port taken — try next
    while (slot <= 4U && discovered.count(slot)) ++slot; // Skip known peers
}
```

On separate physical machines this is never needed — each machine has its own port namespace.

### Auto-Connect Code Skeleton

```cpp
void NetworkBridge::BeginAutoConnect(const std::string& hostIP) {
    m_autoConnectState.store(AutoConnectState::Discovering);

    m_discoveryThread = std::jthread([this, hostIP](std::stop_token st) {
        // 1. Shut down game socket so it doesn't answer its own broadcast
        m_service->Shutdown();

        // 2. Random jitter 0–300 ms (reduces simultaneous-click slot collisions)
        std::this_thread::sleep_for(std::chrono::milliseconds(rng(0, 300)));

        // 3. Capture local scene path for this discovery session
        std::string localScene;
        { std::lock_guard lock(m_scenePathMutex); localScene = m_currentScenePath; }

        // 4. Open temporary socket on port 54998 (receives DiscoveryResponses)
        SOCKET tempSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        setsockopt(tempSock, SOL_SOCKET, SO_BROADCAST, ...);
        bind(tempSock, INADDR_ANY:54998, ...);

        // 5. Build hello packet with scene path
        Packets::DiscoveryHello hello{};
        std::memcpy(hello.scenePath, localScene.c_str(), ...);

        // 6. Send — unicast to host IP, or broadcast if no host IP given
        sockaddr_in dest{};
        if (!hostIP.empty()) {
            inet_pton(AF_INET, hostIP.c_str(), &dest.sin_addr);
        } else {
            dest.sin_addr.s_addr = INADDR_BROADCAST;
        }
        for (int p = 0; p < 4; ++p) {
            dest.sin_port = htons(BASE_PORT + p);
            sendto(tempSock, &hello, sizeof(hello), ...);
        }

        // 7. Collect DiscoveryResponses for 1.5 s, filter by scene
        std::map<uint8_t, uint32_t> discovered;
        // ... receive loop, check scenePath matches localScene ...

        closesocket(tempSock);

        // 8. Pick lowest free slot, bind game socket, register discovered peers
        // 9. Broadcast PeerAnnounce so existing peers can add us back
        m_autoConnectState.store(AutoConnectState::Done);
    });
}
```

---

## 7.10 The Network Menu (ImGui)

`FlatBuffersScenario::OnGUI()` draws the Network menu. It has two independent sections:

### Auto Connect Section

```
Host IP: [________________]  [Auto Connect]
                              ↑ or [Disconnect] when connected (red)
Status: Peer 1 | Port 54000 | No peers found yet     ← green when Done
  My IP: 192.168.1.10   Port: 54000   Peer ID: 1
  Scene: Multiplayer Arena
```

- **Host IP field** (empty = host/broadcaster, filled = joiner/unicaster)
- When auto-connected, the field and button are replaced by a red **Disconnect** button
- The status line changes colour: yellow=discovering, green=done, red=failed
- The indented block shows the local IP that other peers should type into their Host IP field

### Manual Configuration Section

```
── Manual Configuration ──
Local Peer  [-] 1 [+]  ● (Red)    Port: [7000]

Remote Peers (up to 3)
  Peer A   ID: [-] 2 [+]   IP: [127.0.0.1      ]   Port: [7001] [OK]
  Peer B   ID: [-] 3 [+]   IP: [                ]   Port: [7002]
  Peer C   ID: [-] 4 [+]   IP: [                ]   Port: [7003]

[Connect]   Connected (peer 1)
  My IP: 192.168.1.10   Port: 7000   Peer ID: 1
  Scene: Multiplayer Arena
    Peer 2  127.0.0.1 : 7001
```

Manual connect is for cases where you already know the exact IPs and ports (cross-machine, verified in the lab). Auto-connect and manual connect are mutually exclusive — each disables the other's controls while active.

### Dead Reckoning Status

At the bottom of the Network menu:
```
Dead Reckoning
Tracked remote entities: 3
```

This shows how many remote entities the engine is currently interpolating. Useful for debugging — if this is 0 while connected, state packets aren't arriving.

---

## 7.11 Thread Model and Affinity

The engine uses three threads with CPU affinity pinned per the assessment spec:

```
Core 1  (mask 0x01) — Main thread
    GLFW poll → Vulkan render → ImGui → drawFrame()
    Reads front SimulationState snapshot (double-buffered)

Core 4  (mask 0x08) — Physics jthread
    Fixed-step accumulator loop
    BroadcastOwnedStates() + UpdateRemoteEntities() per tick
    Writes back SimulationState buffer

Cores 2–3  (mask 0x06) — Networking jthread
    NetworkService::Poll() in tight loop (1 ms sleep)
    NetworkBridge::ApplyReceivedState() per packet
    Writes only to m_remoteStates (mutex-guarded) and m_pendingNetworkScene
```

The **double-buffered `SimulationState`** decouples physics and render rates — the physics thread writes to the back buffer while the render thread reads from the front buffer. A mutex swap at the end of each physics step moves the new state to the front.

```cpp
// set in EngineOrchestrator::run():
SetThreadAffinityMask(GetCurrentThread(), 0x01);                         // Core 1 — main
SetThreadAffinityMask(physicsThread.native_handle(),    0x08);           // Core 4 — physics
SetThreadAffinityMask(networkingThread.native_handle(), 0x06);           // Cores 2–3 — net
```

---

## 7.12 Connection Lifecycle Summary

```
1. Scene loaded (OnLoad)
   → SetCurrentScene(m_configPath)      ← registers scene path for discovery filter

2. User clicks Auto Connect (host, empty Host IP)
   → BeginAutoConnect("")
   → broadcasts DiscoveryHello, finds no peers
   → binds port 54000, localPeerId = 1
   → shows "My IP: 192.168.x.x" in Network menu

3. Second machine clicks Auto Connect (types host's IP)
   → BeginAutoConnect("192.168.1.10")
   → unicasts DiscoveryHello to 192.168.1.10:54000–54003
   → host responds with DiscoveryResponse { peerID=1 }
   → joiner takes slot 2, binds port 54001
   → broadcasts PeerAnnounce { peerID=2 }
   → host's handlePeerAnnounce() adds Peer 2, calls RegisterScenePeer(2)
   → Both: m_acceptedPeerMask has peer's bit set → state packets accepted

4. Physics loop (each tick)
   → BroadcastOwnedStates() — sends owned entity state to all peers
   → UpdateRemoteEntities(dt) — dead reckons + blends remote entities

5. Scene change
   → OnUnload() calls disconnectNetwork()
       → Shutdown() closes socket + clears m_peers
       → ClearRemoteStates() clears m_remoteStates + zeroes m_acceptedPeerMask
       → SetCurrentScene("") clears scene path
   → Must reconnect after loading new scene
```

---

*Next: [Chapter 8 — Gameplay Scripting & Services](08_Scripting_and_Services.md)*

---

## 7.13 Networking Debugging Checklist

| Symptom | Most Likely Cause | Diagnostic Step | Fix |
|---------|------------------|----------------|-----|
| Peer never receives any packets | Windows Firewall blocking UDP | Test with `netstat -an` — is your port listed as LISTEN? | Add an inbound UDP rule in Windows Firewall for ports 54000–54003 |
| `WSAEADDRINUSE` on connect | Port already in use from a previous session | Check `winerror` from `bind()` return code | Call `NetworkService::Shutdown()` + `Init()` to release and re-bind; the auto-connect path also implements port fallback |
| Stale entity positions after reconnect | Old `m_remoteStates` not cleared | Log which states are in the map before and after reconnect | Verify `ClearRemoteStates()` is called in `NetworkBridge::disconnectNetwork()` |
| Crash on disconnect (null pointer in handleStateUpdate) | `GetTIComponent` asserts when entity does not exist | Check stack trace — is it in `handleStateUpdate`? | `handleStateUpdate` must use `TryGetTIComponent` (returns nullptr) not `GetTIComponent` (fatal assert) |
| Remote entity jumps instead of blending | Blend duration too short or `blendTimer` not reset on new packet | Log `blendTimer` when receiving `StateUpdate` | Ensure `blendTimer = BLEND_DURATION` is set each time a `StateUpdate` is applied |
| Dead reckoning overshoots on high latency | `authVelocity` too large (fast-moving entity) | Log `predictedPos - authPos` after 200 ms | Increase `BLEND_DURATION` from 120 ms to 200 ms on high-latency networks |
| All peers get wrong physics Hz | `physicsHz` slider not synced over network | By design — physics Hz is local only | If consistency is required, broadcast a custom config packet with Hz value |
| Scene mismatch (peer on different scene receives state) | `m_acceptedPeerMask` includes peers from a different scene | Log `m_acceptedPeerMask` after discovery | Verify `SetCurrentScene(path)` is called in `OnLoad` and `OnUnload`; discovery filters by scene path |
| Spawned objects appear at world origin on remote | `SpawnObject` packet sent before entity is positioned | Log `entity.position` at the moment of broadcast | Apply position before broadcasting spawn packet |
| Animation timer drifts out of sync | `BroadcastAnimationStates()` not called after connect | Log calls to `BroadcastAnimationStates` | Ensure `m_pendingPostConnectSync = true` is set during discovery and flushed in the next bridge tick |
| Same-machine testing: second instance fails to bind | Port collision on localhost | Intentional design — two instances of same scene share a port | Test same-scene multiplayer on two separate machines; or modify ports to be instance-unique |
