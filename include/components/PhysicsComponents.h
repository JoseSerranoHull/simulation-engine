#pragma once
#include <glm/glm.hpp>

namespace GE::Components {

    // --- 3D Physics ---
    struct RigidBody {
        // ---- Linear dynamics ------------------------------------------------
        glm::vec3 velocity { 0.0f };
        glm::vec3 acceleration { 0.0f };
        glm::vec3 forceAccum { 0.0f };  // Accumulated forces; reset after each integration step
        float mass { 1.0f };
        float inverseMass { 1.0f };  // Cached 1/mass; 0 for static bodies (infinite mass)
        float restitution { 0.6f };
        bool isStatic { false };
        bool useGravity { true };
        // Velocity damping multiplier applied each frame: velocity *= pow(linearDamping, dt).
        // Near-transparent for normal physics (default 0.999) but stabilises spring oscillation.
        float linearDamping { 0.999f };

        // ---- Angular dynamics (PhysicsObject workshop pattern) ---------------
        // Orientation represented as a 3x3 rotation matrix (columns = local axes).
        // Integrated each tick via  dR/dt = Skew(ω) · R  then re-orthogonalised.
        glm::mat3 orientation { glm::mat3(1.0f) };

        // Angular velocity ω in world space (rad/s).
        glm::vec3 angularVelocity { 0.0f };

        // Accumulated torque τ for the current frame (N·m, world space).
        // Cleared to zero after each Integrate() call.
        glm::vec3 torqueAccum { 0.0f };

        // Inverse inertia tensor I⁻¹.
        // For a uniform solid sphere: I = (2/5)·m·r²·Identity
        //  I⁻¹ = (5/(2·m·r²))·Identity
        // Computed at load time by SceneLoader when a SphereCollider is present.
        // Static bodies keep the identity (angular dynamics are skipped for them).
        glm::mat3 invInertiaTensor{ glm::mat3(1.0f) };

        // Angular displacement (Lab 5 Q1):
        // Encodes a target finite rotation as axis * totalAngle (radians).
        // The body rotates at angularDisplacementSpeed (rad/s) until
        // angularDisplacementApplied reaches the total angle, then stops.
        // Zero vector = disabled (no displacement applied).
        glm::vec3 angularDisplacementVec { 0.0f };   // axis * totalAngle (rad)
        float angularDisplacementSpeed { 1.5708f };    // rad/s (default π/2 ≈ 90°/s)
        float  angularDisplacementApplied { 0.0f };   // accumulated radians so far

        // Lab 6: persistent per-frame torque (N·m, world space).
        // Re-injected into torqueAccum every Integrate() step before clearing.
        // Drives spin-up demos via .ini without needing per-frame scripting.
        // Zero vector = disabled.
        glm::vec3 constantTorque { 0.0f };

        // Lab 6 Q5: world-space inverse inertia tensor, refreshed each frame.
        // Caches  R · I_body⁻¹ · R^T  so torque can be applied directly in world space.
        // Correct for non-isotropic bodies (cylinders, cuboids) unlike the body-space tensor.
        glm::mat3 invInertiaTensorWorld{ glm::mat3(1.0f) };
    };

    // --- 3D Colliders (Lab 6) ---

    struct CylinderCollider {
        float radius { 1.0f };
        float height { 2.0f };
        bool isTrigger { false };  ///< If true, skip impulse resolution; detect overlap only (OnTrigger* events)
        bool isContainer { false };  ///< If true, collision resolves from inside (Pass N in PhysicsSystem)
    };

    struct BoxCollider {
        // Full extents (not half-extents): width × height × depth along local X, Y, Z.
        float sizeX { 1.0f };
        float sizeY { 1.0f };
        float sizeZ { 1.0f };
        bool isTrigger { false };   // < If true, skip impulse resolution; detect overlap only (OnTrigger* events)
        bool isContainer { false }; // < If true, collision resolves from inside (Pass G in PhysicsSystem)
    };

    struct SphereCollider {
        float radius { 1.0f };
        bool isTrigger{ false }; //< If true, skip impulse resolution; detect overlap only (OnTrigger* events)
    };

    struct PlaneCollider {
        glm::vec3 normal{ 0.0f, 1.0f, 0.0f };
        float offset{ 0.0f };
        // Bounded extent in world units (full width × depth). 0 = infinite plane.
        float sizeX{ 0.0f };
        float sizeZ{ 0.0f };
    };

    struct CapsuleCollider {
        float radius { 0.5f };
        float height { 1.0f };  // cylindrical body height (not total capsule height)
        bool isTrigger { false };
        bool isContainer { false }; // < If true, collision resolves from inside (Pass O in PhysicsSystem)
    };

    enum class OwnerType : uint8_t { ONE = 0, TWO = 1, THREE = 2, FOUR = 3, NONE = 255 };
    struct OwnerComponent {
        OwnerType owner { OwnerType::NONE };
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