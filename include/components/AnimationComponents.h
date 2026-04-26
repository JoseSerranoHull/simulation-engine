#pragma once

/* parasoft-begin-suppress ALL */
#include <vector>
#include <string>
#include <cstdint>
/* parasoft-end-suppress ALL */

#include <glm/glm.hpp>

// Forward declaration to avoid circular include with FBSceneContext.h
namespace GE::Scene::FB { struct PrefabTemplate; }

namespace GE::Components {

    enum class EasingType : uint8_t { LINEAR, SMOOTHSTEP };
    enum class PathMode   : uint8_t { STOP, LOOP, REVERSE };

    struct FBWaypoint {
        glm::vec3 position { 0.0f };
        glm::vec3 rotDeg   { 0.0f };  // yaw/pitch/roll in degrees
        float     time     { 0.0f };  // absolute arrival time (seconds)
    };

    struct AnimatedObjectComponent {
        std::vector<FBWaypoint> waypoints;
        float      totalDuration { 1.0f };
        EasingType easing        { EasingType::LINEAR };
        PathMode   pathMode      { PathMode::STOP };
        float      elapsed       { 0.0f };   // runtime time accumulator
        glm::vec3  prevPosition  { 0.0f };   // position before this frame; used by PhysicsSystem for kinematic velocity
        bool       reversed      { false };  // REVERSE mode: true when playing backwards
    };

    struct PhysicsMaterialTag {
        std::string name;
        float       density { 1.0f };
    };

    // ---------------------------------------------------------------------------
    // Spawner support
    // ---------------------------------------------------------------------------

    /** @brief How pre-spawned entities are positioned when activated. */
    enum class SpawnLocType : uint8_t { FIXED, RANDOM_BOX, RANDOM_SPHERE };

    /**
     * @struct SpawnerComponent
     * @brief ECS component holding all runtime state for one logical spawner.
     *        References a PrefabTemplate (built at scene load time); SpawnerSystem
     *        calls EntityFactory::InstantiatePrefab() to create real entities at runtime.
     */
    struct SpawnerComponent {
        // --- Timing ---
        float startTime          { 0.0f };
        float elapsed            { 0.0f };
        float timeSinceLastSpawn { 0.0f };
        bool  activated          { false };

        // --- Spawn type ---
        bool     isBurst   { true };
        uint32_t burstCount{ 1 };
        float    interval  { 1.0f };

        // --- Location ---
        SpawnLocType locationType { SpawnLocType::FIXED };
        glm::vec3    fixedPos     { 0.0f };
        glm::vec3    boxMin       { -1.0f };
        glm::vec3    boxMax       {  1.0f };
        glm::vec3    sphereCenter {  0.0f };
        float        sphereRadius { 1.0f };

        // --- Velocity ranges applied to each entity on spawn ---
        glm::vec3 linVelMin { 0.0f }, linVelMax { 0.0f };
        glm::vec3 angVelMin { 0.0f }, angVelMax { 0.0f };

        // --- Ownership (which peer fires this spawner) ---
        uint8_t ownerPeerId { 0 };

        // --- Prefab-based runtime spawning ---
        // Non-owning pointer into FlatBuffersScenario::m_prefabRegistry (valid until OnUnload)
        const GE::Scene::FB::PrefabTemplate* prefabTemplate { nullptr };
        uint32_t                             spawnedCount   { 0 };
        uint32_t                             maxCount       { 0 };
        // True when SpawnerOwnerType::SEQUENTIAL — color cycles across 4 peers per spawn.
        bool                                 isSequential   { false };
        // When true, the auto-spawn timer freezes; ForceSpawnOne (Fire button) still works.
        bool                                 paused         { false };
        // Entity IDs of entities spawned by this spawner (world-space root entities).
        // Populated at runtime by SpawnerSystem; cleared on Reset.
        // Used by DebugOverlay to show them as virtual children without m_parentEntityID.
        std::vector<uint32_t>                spawnedEntityIds;
    };

} // namespace GE::Components
