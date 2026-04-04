#pragma once

/* parasoft-begin-suppress ALL */
#include <deque>
#include <vector>
#include <string>
#include <cstdint>
/* parasoft-end-suppress ALL */

#include <glm/glm.hpp>
#include "ecs/Entity.h"

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
     *        The entity pool (pendingIds) is pre-created at scene load time by
     *        FBSceneAdapter::adaptSpawners() so that no GPU uploads happen during gameplay.
     */
    struct SpawnerComponent {
        // --- Timing ---
        float startTime          { 0.0f };
        float elapsed            { 0.0f };
        float timeSinceLastSpawn { 0.0f };
        bool  activated          { false };  // burst: fire-once guard; repeating: marks first tick past startTime

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

        // --- Velocity ranges applied to each entity on activation ---
        glm::vec3 linVelMin { 0.0f }, linVelMax { 0.0f };
        glm::vec3 angVelMin { 0.0f }, angVelMax { 0.0f };

        // --- Ownership (which peer fires this spawner) ---
        /// Peer ID (1-4) that owns this spawner.  0 = unowned (all peers run it).
        uint8_t ownerPeerId { 0 };

        // --- Pre-created entity pool (populated at load time) ---
        std::deque<GE::ECS::EntityID> pendingIds;
    };

} // namespace GE::Components
