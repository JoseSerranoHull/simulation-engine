#pragma once
#include <glm/glm.hpp>
#include <cstdint>

namespace GE::Components {

struct FlockingComponent {
    // Neighbourhood radii (world units)
    float separationRadius { 1.5f };
    float alignmentRadius { 3.0f };
    float cohesionRadius { 5.0f };

    // Force weights for Weighted Truncated Sum
    float wSeparation { 2.0f };
    float wAlignment { 1.0f };
    float wCohesion { 1.0f };
    float wAvoidance { 3.0f };  // obstacle collision avoidance

    // Speed / force limits
    float maxSpeed { 6.0f };
    float maxForce { 15.0f };

    // Group tag — agents only flock with same group (0 = any)
    uint8_t groupId { 0 };
};

} // namespace GE::Components
