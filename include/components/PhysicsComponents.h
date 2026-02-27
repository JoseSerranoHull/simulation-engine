#pragma once
#include <glm/glm.hpp>

namespace GE::Components {

    // --- 3D Physics ---
    struct RigidBody {
        // ---- Linear dynamics ------------------------------------------------
        glm::vec3 velocity    { 0.0f };
        glm::vec3 acceleration{ 0.0f };
        glm::vec3 forceAccum  { 0.0f };  // Accumulated forces; reset after each integration step
        float     mass        { 1.0f };
        float     inverseMass { 1.0f };  // Cached 1/mass; 0 for static bodies (infinite mass)
        float     restitution { 0.6f };
        bool      isStatic    { false };
        bool      useGravity  { true };

        // ---- Angular dynamics (PhysicsObject workshop pattern) ---------------
        // Orientation represented as a 3x3 rotation matrix (columns = local axes).
        // Integrated each tick via  dR/dt = Skew(ω) · R  then re-orthogonalised.
        glm::mat3 orientation     { glm::mat3(1.0f) };

        // Angular velocity ω in world space (rad/s).
        glm::vec3 angularVelocity { 0.0f };

        // Accumulated torque τ for the current frame (N·m, world space).
        // Cleared to zero after each Integrate() call.
        glm::vec3 torqueAccum     { 0.0f };

        // Inverse inertia tensor I⁻¹.
        // For a uniform solid sphere: I = (2/5)·m·r²·Identity
        //                            I⁻¹ = (5/(2·m·r²))·Identity
        // Computed at load time by SceneLoader when a SphereCollider is present.
        // Static bodies keep the identity (angular dynamics are skipped for them).
        glm::mat3 invInertiaTensor{ glm::mat3(1.0f) };
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