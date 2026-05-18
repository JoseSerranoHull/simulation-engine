#pragma once

/* parasoft-begin-suppress ALL */
#include <glm/glm.hpp>
#include <unordered_map>
#include <unordered_set>
#include <utility>
/* parasoft-end-suppress ALL */

#include "ecs/Entity.h"

namespace GE::Scripts {

    /**
     * @struct CollisionInfo
     * @brief Passed to OnCollisionEnter / OnCollisionExit callbacks.
     *
     * Kept minimal — only physics-relevant fields computed during ResolveCollisions().
     * For OnCollisionExit, contactPoint/normal/penetration are zeroed; only otherEntity is valid.
     */
    struct CollisionInfo {
        GE::ECS::EntityID otherEntity  { GE::ECS::INVALID_ENTITY_ID };
        glm::vec3 contactPoint { 0.0f };   ///< World-space midpoint of contact
        glm::vec3 normal { 0.0f };   ///< Points away from otherEntity
        float penetration { 0.0f };   ///< Overlap depth in world units
    };

    // -----------------------------------------------------------------------
    // Shared collision pair types used by both PhysicsSystem and ScriptSystem.
    // Entity pairs are stored in canonical order: first < second.
    // -----------------------------------------------------------------------

    /// A canonical (sorted) pair of entity IDs representing a collision or trigger contact.
    using EntityPair = std::pair<GE::ECS::EntityID, GE::ECS::EntityID>;

    struct PairHash {
        size_t operator()(const EntityPair& p) const noexcept {
            return std::hash<uint64_t>{}(
                (static_cast<uint64_t>(p.first) << 32) | static_cast<uint64_t>(p.second));
        }
    };

    using ContactSet  = std::unordered_set<EntityPair, PairHash>;
    using ContactInfoMap = std::unordered_map<EntityPair, CollisionInfo, PairHash>;

} // namespace GE::Scripts
