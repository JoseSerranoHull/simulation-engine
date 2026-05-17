#pragma once

/* parasoft-begin-suppress ALL */
#include <vector>
#include <cstdint>
/* parasoft-end-suppress ALL */

#include <glm/glm.hpp>

// Forward declaration to avoid circular include with FBSceneContext.h
namespace GE::Scene::FB { struct PrefabTemplate; }

namespace GE::Components {

    /** @brief How pre-spawned entities are positioned when activated. */
    enum class SpawnLocType : uint8_t { FIXED, RANDOM_BOX, RANDOM_SPHERE };

    /**
     * @struct SpawnerComponent
     * @brief ECS component holding all runtime state for one logical spawner.
     * References a PrefabTemplate (built at scene load time); SpawnerSystem
     * calls EntityFactory::InstantiatePrefab() to create real entities at runtime.
     */
    struct SpawnerComponent {
        // --- Timing ---
        float startTime { 0.0f };
        float elapsed { 0.0f };
        float timeSinceLastSpawn { 0.0f };
        bool activated { false };

        // --- Spawn type ---
        bool isBurst { true };
        uint32_t burstCount { 1 };
        float interval { 1.0f };

        // --- Location ---
        SpawnLocType locationType { SpawnLocType::FIXED };
        glm::vec3 fixedPos { 0.0f };
        glm::vec3 boxMin { -1.0f };
        glm::vec3 boxMax {  1.0f };
        glm::vec3 sphereCenter {  0.0f };
        float sphereRadius { 1.0f };

        // --- Velocity ranges applied to each entity on spawn ---
        glm::vec3 linVelMin { 0.0f }, linVelMax { 0.0f };
        glm::vec3 angVelMin { 0.0f }, angVelMax { 0.0f };

        // --- Ownership (which peer fires this spawner) ---
        uint8_t ownerPeerId { 0 };

        // --- Prefab-based runtime spawning ---
        // Non-owning pointer into FlatBuffersScenario::m_prefabRegistry (valid until OnUnload)
        const GE::Scene::FB::PrefabTemplate* prefabTemplate { nullptr };
        uint32_t spawnedCount { 0 };
        uint32_t maxCount { 0 };
        // True when SpawnerOwnerType::SEQUENTIAL — ownership AND color cycle across 4 peers per spawn (1→2→3→4→1).
        bool isSequential { false };
        bool paused { false };
        // Entity IDs of entities spawned by this spawner (world-space root entities).
        // Populated at runtime by SpawnerSystem; cleared on Reset.
        std::vector<uint32_t> spawnedEntityIds;

        // Size-variant prefabs for spawners generated from radius_range (no prefab_ref).
        // SpawnerSystem picks a random entry each spawn; empty = use prefabTemplate only.
        std::vector<const GE::Scene::FB::PrefabTemplate*> prefabVariants;
    };

} // namespace GE::Components
