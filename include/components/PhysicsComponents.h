#pragma once
#include <glm/glm.hpp>

namespace GE::Components {

    // --- 3D Physics ---
    struct RigidBody {
        glm::vec3 velocity{ 0.0f };
        glm::vec3 acceleration{ 0.0f };
        float mass{ 1.0f };
        float restitution{ 0.6f };
        bool isStatic{ false };
        bool useGravity{ true };
    };

    struct SphereCollider {
        float radius{ 1.0f };
    };

    struct PlaneCollider {
        glm::vec3 normal{ 0.0f, 1.0f, 0.0f };
        float offset{ 0.0f };
    };

    // --- 2D Physics (Lab Requirement) ---
    struct RigidBody2D {
        glm::vec2 velocity{ 0.0f };
        glm::vec2 acceleration{ 0.0f };
        float mass{ 1.0f };
        float bounciness{ 0.5f };
        bool isStatic{ false };
    };

    struct CircleCollider2D {
        float radius{ 1.0f };
        glm::vec2 offset{ 0.0f }; // Local offset from the transform position
    };

    struct BoxCollider2D {
        glm::vec2 size{ 1.0f, 1.0f }; // Half-extents
        glm::vec2 offset{ 0.0f };
    };
}