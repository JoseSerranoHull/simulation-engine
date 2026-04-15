# Chapter 1 — Entity Component System: From Concept to Implementation

## 1.1 What Is ECS and Why Does It Exist?

Imagine a classic object-oriented game engine. You might have a class `GameObject` that holds position, health, a sprite, AI state, and physics data all together. As the game grows, you add more fields. Subclasses proliferate — `Enemy extends GameObject`, `FlyingEnemy extends Enemy`. Within a few months you have an inheritance tree that is impossible to reason about, and changing one base class breaks everything below it.

**ECS (Entity Component System)** solves this by flipping the design:

- **Entity** — just an ID (a number). It holds no data.
- **Component** — a plain data struct. It holds data but has no logic.
- **System** — a class that holds logic and operates on collections of components.

### Traditional OOP vs ECS

```mermaid
classDiagram
    class OOP_GameObject {
        +position: vec3
        +velocity: vec3
        +health: float
        +sprite: Texture*
        +aiState: AIState
        +update()
        +render()
        +takeDamage()
    }
    class OOP_Enemy {
        +aggression: float
        +attackPlayer()
    }
    class OOP_FlyingEnemy {
        +wingFlap: float
        +fly()
    }
    OOP_GameObject <|-- OOP_Enemy
    OOP_Enemy <|-- OOP_FlyingEnemy

    class ECS_Entity {
        +id: uint32
    }
    class ECS_Transform {
        +position: vec3
        +rotation: vec3
        +scale: vec3
    }
    class ECS_RigidBody {
        +velocity: vec3
        +mass: float
    }
    class ECS_FlockingComponent {
        +maxSpeed: float
        +groupId: uint8
    }
    class ECS_PhysicsSystem {
        +OnUpdate(dt)
    }
    class ECS_FlockingSystem {
        +OnUpdate(dt)
    }
    ECS_Entity ..> ECS_Transform : has component
    ECS_Entity ..> ECS_RigidBody : has component
    ECS_Entity ..> ECS_FlockingComponent : has component
    ECS_PhysicsSystem --> ECS_RigidBody : processes
    ECS_FlockingSystem --> ECS_FlockingComponent : processes
```

The key insight: **composition over inheritance**. A "flying enemy that has health and AI" is just an entity with `Transform`, `RigidBody`, `FlockingComponent`, and `HealthComponent` — no subclassing needed.

### Performance Benefit: Data-Oriented Design

OOP stores all data for one object together (**Array of Structs / AoS**). ECS stores all data of one type together (**Structure of Arrays / SoA**).

```
AoS (OOP): Memory looks like...
[Entity0: pos|vel|health|sprite][Entity1: pos|vel|health|sprite]...
 ↑ When iterating physics, we read pos+vel but skip health+sprite — cache wasted

SoA (ECS): Memory looks like...
[pos0|pos1|pos2|...] [vel0|vel1|vel2|...] [health0|health1|...]
 ↑ Physics iterates only pos+vel arrays — every cache line is useful
```

This difference can be 3–10× faster for large numbers of entities.

---

## 1.2 This Engine's ECS at a Glance

```mermaid
classDiagram
    class EntityManager {
        +CreateEntity() EntityID
        +DestroyEntity(id)
        +AddComponent~T~(id, component)
        +GetTIComponent~T~(id) T*
        +RegisterSystem(system)
        +UpdateCpuStages(dt)
        +UpdateGpuStages(dt, cb)
        -m_freeEntities: stack~EntityID~
        -m_componentArrays: vector~IComponentArray*~
        -m_allComponentIndices: vector~uint32~
        -m_systems: array~vector~IECSystem*~~
    }

    class ComponentArray_T {
        +Add(entityID, data) uint32
        +Remove(entityID)
        +Get(entityID) T*
        +Data() T*
        +GetCount() uint32
        -m_data: vector~T~
        -m_index: vector~uint32~
        -m_reverse: vector~uint32~
    }

    class IECSystem {
        <<abstract>>
        +GetStage() ESystemStage
        +OnUpdate(dt, cb)
    }

    class ICpuSystem {
        <<abstract>>
        +OnUpdate(dt)
    }

    class IGpuSystem {
        <<abstract>>
        +OnUpdate(dt, cb)
    }

    EntityManager "1" *-- "many" ComponentArray_T : owns
    EntityManager "1" *-- "many" IECSystem : manages
    IECSystem <|-- ICpuSystem
    IECSystem <|-- IGpuSystem
```

**Files involved:**
- `include/ecs/EntityManager.h` — central ECS hub
- `include/ecs/ComponentArray.h` — SoA storage per component type
- `include/ecs/IECSystem.h` — system interfaces + `ESystemStage` enum
- `include/ecs/Entity.h` — `Entity` struct (wraps `uint32_t`)
- `include/ecs/ComponentType.h` — auto-generates type IDs at compile time

---

## 1.3 Entities: Just Numbers

```cpp
// include/ecs/Entity.h
struct Entity {
    Entity();
    ~Entity();
    bool Initialize(uint32_t id);
    void Shutdown();
    uint32_t m_id;    // The entity IS this number
};

using EntityID = uint32_t;
```

Creating and destroying entities:

```cpp
// Somewhere in a Scenario::OnLoad():
GE::ECS::EntityManager* em = ServiceLocator::GetEntityManager();

EntityID sphereID = em->CreateEntity();    // Gets next available ID
EntityID planeID  = em->CreateEntity();    // Another ID

// Later, when the scenario unloads:
em->DestroyEntity(sphereID);               // ID pushed back onto free stack
em->ClearAllEntities();                    // Wipes everything
```

**Entity recycling:** `EntityManager` keeps a `std::stack<EntityID> m_freeEntities`. When you destroy entity 42, its ID goes onto the stack. The next `CreateEntity()` call pops it and reuses 42. This prevents ID overflow and avoids gaps.

---

## 1.4 Components: Pure Data Structs

Components are **plain data** — no virtual methods, no constructors with side effects, no logic. Here are representative examples from this engine:

```cpp
// include/components/Transform.h — every visible entity has this
namespace GE::Components {
struct Transform {
    glm::vec3 m_position { 0.0f, 0.0f, 0.0f };
    glm::vec3 m_rotation { 0.0f, 0.0f, 0.0f };  // Euler angles (degrees)
    glm::vec3 m_scale    { 1.0f, 1.0f, 1.0f };
    glm::mat4 m_worldMatrix { 1.0f };             // Computed by TransformSystem
    glm::mat4 m_localMatrix { 1.0f };
};
}

// include/components/PhysicsComponents.h — entities with physics
namespace GE::Components {
struct RigidBody {
    float     mass { 1.0f };
    float     inverseMass { 1.0f };    // Pre-computed 1/mass; 0 = infinite (static)
    glm::vec3 velocity { 0.0f };
    glm::vec3 angularVelocity { 0.0f };
    glm::vec3 forceAccum { 0.0f };     // Accumulated forces, cleared each tick
    glm::vec3 torqueAccum { 0.0f };
    float     restitution { 0.5f };    // Bounciness coefficient 0–1
    bool      useGravity { true };
    bool      isStatic { false };
};
}
```

### Component Type IDs (Auto-generated)

Each component type gets a unique integer ID at runtime, generated via a static atomic counter:

```cpp
// include/ecs/ComponentType.h
namespace GE::ECS::Internal {
    inline std::atomic<uint32_t> g_nextComponentID { 0U };
    inline uint32_t FetchNextComponentID() {
        return g_nextComponentID.fetch_add(1U, std::memory_order_relaxed);
    }
}

template <typename T>
class ComponentType {
public:
    static uint32_t ID() {
        // Static local: initialised exactly once, thread-safe by C++11 rules
        static const uint32_t id = Internal::FetchNextComponentID();
        return id;
    }
};

// Usage:
uint32_t transformID = ComponentType<Transform>::ID();  // e.g. returns 0
uint32_t rigidBodyID = ComponentType<RigidBody>::ID();  // e.g. returns 1
```

The first call to `ComponentType<T>::ID()` for any type T creates and locks in its ID for the process lifetime.

---

## 1.5 ComponentArray: Structure of Arrays in Practice

Each component type gets its own `ComponentArray<T>` — a contiguous block of data, tightly packed:

```cpp
// include/ecs/ComponentArray.h (simplified key members)
template <typename T>
class ComponentArray final : public IComponentArray {
    std::vector<T>        m_data;     // [0]=component for entity A, [1]=component for entity B...
    std::vector<uint32_t> m_index;    // m_index[slot] = entityID that owns this slot
    std::vector<uint32_t> m_reverse;  // m_reverse[entityID] = slot (UINT32_MAX if absent)
    uint32_t              m_size { 0U };
};
```

### How these three arrays work together

Say entities 0, 5, and 12 each have a `Transform` component:

```
m_data:    [Transform@0] [Transform@5] [Transform@12]
m_index:   [0]           [5]           [12]
m_reverse: [0] [MAX] [MAX] [MAX] [MAX] [1] [MAX]...[2]...
            ↑0         ↑1         ↑2         ↑5           ↑12
```

- **Add:** Append to `m_data` and `m_index`, update `m_reverse[entityID]`
- **Get:** `m_data[m_reverse[entityID]]` — one indirection, O(1)
- **Iterate all:** Loop `m_data[0..m_size)` — perfectly contiguous, maximally cache-friendly
- **Remove (swap-and-pop):** Overwrite the removed slot with the last slot, update mappings, pop the tail

### EntityManager's flat component index table

```cpp
// include/ecs/EntityManager.h
// Flat 2D: [typeID * m_maxEntities + entityID] → packed slot index (or UINT32_MAX)
std::vector<uint32_t> m_allComponentIndices;
```

This gives O(1) "does entity X have component of type Y?" checks without any hash maps.

---

## 1.6 Systems: Stage-Ordered Logic Runners

Systems are where the logic lives. Each system declares which **stage** it belongs to, so the engine can run them in a predictable order every frame.

### ESystemStage — Execution Order

```cpp
// include/ecs/IECSystem.h
enum class ESystemStage {
    EarlyUpdate  = 0,  // Pre-physics setup, collision broadphase prep
    Transform    = 1,  // Compute world matrices from pos/rot/scale
    Animation    = 2,  // Move animated objects along their waypoints
    Physics      = 3,  // Apply forces, integrate, resolve collisions
    SceneControl = 4,  // High-level scene switches
    DayNight     = 5,  // Environmental / lighting changes
    GameLogic    = 6,  // User scripts (ScriptSystem)
    Camera       = 7,  // Update camera matrices
    GUI          = 8,  // ImGui preparations
    Particle     = 9,  // Particle GPU command recording
    Render       = 10, // Record draw commands
    LateUpdate   = 11, // Final per-frame cleanup
    Count        = 12
};
```

Internally, `EntityManager` holds one bucket per stage:

```cpp
std::array<std::vector<IECSystem*>, static_cast<size_t>(ESystemStage::Count)> m_systems;
```

### ICpuSystem vs IGpuSystem

Not all systems are the same. The key distinction is whether a system needs to **record Vulkan GPU commands**:

```cpp
// include/ecs/IECSystem.h

// CPU-only system — safe to run from any thread, no GPU command buffer needed
struct ICpuSystem : IECSystem {
    // Engine calls this with cb=VK_NULL_HANDLE; the override ignores it
    void OnUpdate(float dt, VkCommandBuffer /*cb*/) final { OnUpdate(dt); }
    virtual void OnUpdate(float dt) = 0;
};

// GPU system — MUST run on the main thread with a live command buffer
struct IGpuSystem : IECSystem {
    // Override OnUpdate(float dt, VkCommandBuffer cb) directly
};
```

**Examples:**
| System | Type | Stage |
|--------|------|-------|
| `TransformSystem` | `ICpuSystem` | Transform |
| `PhysicsSystem` | `ICpuSystem` | Physics |
| `ClothSystem` | `ICpuSystem` | Physics |
| `FlockingSystem` | `ICpuSystem` | GameLogic |
| `AnimationSystem` | `ICpuSystem` | Animation |
| `ScriptSystem` | `ICpuSystem` | GameLogic |
| `ParticleEmitterSystem` | `IGpuSystem` | Particle |

### How stages are called

```cpp
// Physics thread calls this (CPU stages only — EarlyUpdate through Camera)
em->UpdateCpuStages(fixedDt);

// Main thread calls this (GPU stages — Particle through LateUpdate, with live VkCommandBuffer)
em->UpdateGpuStages(renderDt, commandBuffer);
```

```mermaid
sequenceDiagram
    participant PhysicsThread
    participant MainThread
    participant EntityManager

    PhysicsThread->>EntityManager: UpdateCpuStages(fixedDt)
    Note over EntityManager: Stages 0–7 (EarlyUpdate → Camera)<br/>ICpuSystem::OnUpdate(dt) called for each

    MainThread->>EntityManager: UpdateGpuStages(renderDt, cb)
    Note over EntityManager: Stages 8–11 (GUI → LateUpdate)<br/>IGpuSystem::OnUpdate(dt, cb) called for each
```

---

## 1.7 Adding, Getting, and Iterating Components

### Adding a component to an entity

```cpp
GE::ECS::EntityManager* em = ServiceLocator::GetEntityManager();

// Create entity
EntityID sphereID = em->CreateEntity();

// Add a Transform
GE::Components::Transform tr;
tr.m_position = glm::vec3(0.0f, 5.0f, 0.0f);
em->AddComponent<GE::Components::Transform>(sphereID, tr);

// Add a RigidBody
GE::Components::RigidBody rb;
rb.mass        = 2.5f;
rb.inverseMass = 1.0f / rb.mass;
rb.useGravity  = true;
em->AddComponent<GE::Components::RigidBody>(sphereID, rb);
```

### Retrieving a single component

```cpp
auto* tr = em->GetTIComponent<GE::Components::Transform>(sphereID);
if (tr != nullptr) {
    tr->m_position.y += 0.1f;  // Move sphere up
}
```

### Retrieving multiple components at once (structured binding)

```cpp
auto [tr, rb] = em->GetTIComponents<GE::Components::Transform,
                                    GE::Components::RigidBody>(sphereID);
// tr and rb are pointers; either may be nullptr if component is absent
if (tr && rb) {
    rb->forceAccum += glm::vec3(0.0f, -9.8f * rb->mass, 0.0f);  // gravity
}
```

### Iterating ALL components of a type (hot path in systems)

```cpp
// Inside PhysicsSystem::OnUpdate():
auto& transforms = em->GetCompArr<GE::Components::Transform>();
auto& rigidBodies = em->GetCompArr<GE::Components::RigidBody>();

const uint32_t count = rigidBodies.GetCount();
for (uint32_t i = 0; i < count; ++i) {
    GE::Components::RigidBody& rb = rigidBodies.Data()[i];
    EntityID eid                  = rigidBodies.Index()[i];

    auto* tr = em->GetTIComponent<GE::Components::Transform>(eid);
    if (!tr || rb.isStatic) continue;

    // Integrate position...
}
```

---

## 1.8 System Registration & Lifecycle

### Engine-global systems (registered once at startup)

```cpp
// source/core/EngineOrchestrator.cpp (constructor, simplified)
auto* transformSystem  = new GE::Systems::TransformSystem();
auto* particleSystem   = new GE::Systems::ParticleEmitterSystem();
em->RegisterSystem(transformSystem);
em->RegisterSystem(particleSystem);
// These persist for the entire engine lifetime
```

### Scenario-specific systems (registered per scene load)

```cpp
// source/scene/FlatBuffersScenario.cpp

void FlatBuffersScenario::OnLoad(GpuUploadContext& ctx) {
    auto* em = ServiceLocator::GetEntityManager();

    // Create and register systems that only exist while this scenario is active
    m_clothSystem    = new GE::Systems::ClothSystem(ctx);
    m_flockingSystem = new GE::Systems::FlockingSystem();
    m_physicsSystem  = new GE::Systems::PhysicsSystem(&m_interactionRegistry);
    m_animationSystem = new GE::Systems::AnimationSystem();
    m_spawnerSystem  = new GE::Systems::SpawnerSystem();

    em->RegisterSystem(m_clothSystem);
    em->RegisterSystem(m_flockingSystem);
    em->RegisterSystem(m_physicsSystem);
    em->RegisterSystem(m_animationSystem);
    em->RegisterSystem(m_spawnerSystem);

    // ... load entities from .bin file
}

void FlatBuffersScenario::OnUnload() {
    auto* em = ServiceLocator::GetEntityManager();

    // Unregister in reverse order (good practice, prevents dangling pointers)
    em->UnregisterSystem(m_spawnerSystem);
    em->UnregisterSystem(m_animationSystem);
    em->UnregisterSystem(m_physicsSystem);
    em->UnregisterSystem(m_flockingSystem);
    em->UnregisterSystem(m_clothSystem);

    // delete is handled by unique_ptr or explicit ownership
    em->ClearAllEntities();
}
```

---

## 1.9 Summary: How a Frame Flows Through the ECS

```mermaid
sequenceDiagram
    participant Orch as EngineOrchestrator
    participant PT as Physics Thread
    participant MT as Main Thread
    participant EM as EntityManager

    PT->>EM: UpdateCpuStages(fixedDt)
    EM->>EM: Stage::EarlyUpdate → all ICpuSystems
    EM->>EM: Stage::Transform → TransformSystem
    EM->>EM: Stage::Animation → AnimationSystem
    EM->>EM: Stage::Physics → PhysicsSystem, ClothSystem
    EM->>EM: Stage::GameLogic → FlockingSystem, ScriptSystem
    EM->>EM: Stage::Camera → CameraSystem
    PT->>Orch: snapshot transforms to back SimulationState buffer
    PT->>Orch: atomic flip frontSimIdx

    MT->>EM: UpdateGpuStages(renderDt, cmdBuffer)
    EM->>EM: Stage::Particle → ParticleEmitterSystem (records GPU compute)
    EM->>EM: Stage::Render → records draw calls
    MT->>Orch: vkQueueSubmit(cmdBuffer) → present frame
```

The complete separation between CPU stages (physics thread) and GPU stages (main thread) is what makes the engine thread-safe without needing locks on the ECS itself.

---

*Next: [Chapter 2 — Threading & Concurrency](02_Threading_and_Concurrency.md)*
