#pragma once

/**
 * @file DemoPlayerScript.h
 * @brief Example GameScriptComponent — demonstrates the full scripting lifecycle.
 *
 * HOW TO USE
 * ----------
 * 1. In your scenario's OnLoad(), after creating the entity and adding physics components:
 *
 *      GE::ECS::EntityID playerID = em->CreateEntity();
 *      em->AddComponent<GE::Components::Transform>(playerID, {});
 *      em->AddComponent<GE::Components::Tag>(playerID, GE::Components::Tag{"Player"});
 *      em->AddComponent<GE::Components::SphereCollider>(playerID, GE::Components::SphereCollider{0.5f});
 *      em->AddComponent<GE::Components::RigidBody>(playerID, GE::Components::RigidBody{});
 *      em->AddComponent<GE::Components::ScriptComponent>(
 *          playerID,
 *          GE::Components::ScriptComponent{ std::make_shared<DemoPlayerScript>() }
 *      );
 *
 * 2. Run the engine — use arrow keys to move the sphere. The inspector shows live variables.
 *
 * LIFECYCLE DEMONSTRATION
 * -----------------------
 *  Awake()            : runs once on the first frame the script is active
 *  Start()            : runs on the same first active frame, immediately after Awake
 *  Update()           : runs every frame — handles keyboard input
 *  FixedUpdate()      : runs at fixed ~60 Hz — applies constant downward nudge to show fixed step
 *  LateUpdate()       : runs after all Updates — clamps position to a boundary box
 *  OnCollisionEnter() : logs which entity was hit
 *  OnTriggerEnter()   : logs entry into a trigger volume
 */

#include "scripts/GameScriptComponent.h"
#include "components/ScriptComponent.h"   // for attachment via AddComponent<ScriptComponent>
#include "core/Logger.h"

/* parasoft-begin-suppress ALL */
#include <GLFW/glfw3.h>
#include <string>
/* parasoft-end-suppress ALL */

class DemoPlayerScript : public GE::Scripts::GameScriptComponent {
public:
    // -----------------------------------------------------------------------
    // Public fields — visible and editable in the ImGui Scripts inspector
    // -----------------------------------------------------------------------
    float     moveSpeed    { 5.0f };        ///< Horizontal movement speed (units/s)
    float     jumpImpulse  { 4.0f };        ///< Upward velocity on jump
    float     boundarySize { 8.0f };        ///< Half-extent of the arena clamp (LateUpdate demo)
    bool      invertX      { false };       ///< Flip left/right controls
    glm::vec3 startPosition{ 0.0f };        ///< Read-only: recorded at Start(), shown in inspector
    int       collisionCount{ 0 };          ///< Running count of Enter events this session

    const char* GetScriptName() const override { return "DemoPlayerScript"; }

    // -----------------------------------------------------------------------
    // Inspector
    // -----------------------------------------------------------------------
    void OnDrawInspector() override {
        GameScriptComponent::OnDrawInspector();      // draws m_active checkbox
        DrawField("Move Speed",    moveSpeed);
        DrawField("Jump Impulse",  jumpImpulse);
        DrawField("Boundary Size", boundarySize);
        DrawField("Invert X",      invertX);
        DrawField("Start Position", startPosition);
        DrawField("Collision Count", collisionCount);
    }

    // -----------------------------------------------------------------------
    // Lifecycle
    // -----------------------------------------------------------------------

    void Awake() override {
        // Runs once when the script first becomes active.
        // Safe to cache component data here; entityID is already set.
        GE_LOG_INFO("[DemoPlayerScript] Awake on entity " + std::to_string(GetEntityID()));
    }

    void Start() override {
        // Runs once on the same first active frame, immediately after Awake.
        // Record the spawn position for display in the inspector.
        if (auto* t = GetTransform()) {
            startPosition = t->m_localPosition;
        }
        GE_LOG_INFO("[DemoPlayerScript] Start — spawn at ("
            + std::to_string(startPosition.x) + ", "
            + std::to_string(startPosition.y) + ", "
            + std::to_string(startPosition.z) + ")");
    }

    void Update(float dt) override {
        auto* t   = GetTransform();
        auto* in  = GetInput();
        if (!t || !in) return;

        const float dir = invertX ? -1.0f : 1.0f;

        // Horizontal movement (arrow keys / WASD)
        if (in->IsKeyDown(GLFW_KEY_RIGHT) || in->IsKeyDown(GLFW_KEY_D))
            t->m_localPosition.x += dir * moveSpeed * dt;

        if (in->IsKeyDown(GLFW_KEY_LEFT) || in->IsKeyDown(GLFW_KEY_A))
            t->m_localPosition.x -= dir * moveSpeed * dt;

        if (in->IsKeyDown(GLFW_KEY_UP) || in->IsKeyDown(GLFW_KEY_W))
            t->m_localPosition.z -= moveSpeed * dt;

        if (in->IsKeyDown(GLFW_KEY_DOWN) || in->IsKeyDown(GLFW_KEY_S))
            t->m_localPosition.z += moveSpeed * dt;

        // Jump — apply upward velocity via RigidBody if present
        if (in->IsKeyDown(GLFW_KEY_SPACE)) {
            auto* rb = GetEntityManager()->TryGetTIComponent<GE::Components::RigidBody>(GetEntityID());
            if (rb && glm::abs(rb->velocity.y) < 0.1f) {   // simple grounded check
                rb->velocity.y = jumpImpulse;
            }
        }

        // Mark transform dirty so TransformSystem picks up the direct position change
        t->m_state = GE::Components::Transform::TransformState::Dirty;
    }

    void FixedUpdate(float fixedDt) override {
        // Called at a fixed ~60 Hz rate regardless of frame rate.
        // Good for deterministic physics nudges or network-synced logic.
        // Demo: apply a tiny downward nudge to the RigidBody forceAccum each fixed step.
        auto* rb = GetEntityManager()->TryGetTIComponent<GE::Components::RigidBody>(GetEntityID());
        if (rb && !rb->isStatic) {
            // Extra drag to make the demo ball feel snappier
            rb->velocity.x *= (1.0f - 2.0f * fixedDt);
            rb->velocity.z *= (1.0f - 2.0f * fixedDt);
        }
    }

    void LateUpdate(float /*dt*/) override {
        // Called after ALL Update()s in the frame.
        // Demo: clamp the entity inside a boundary box so it can't escape the arena.
        auto* t = GetTransform();
        if (!t) return;

        t->m_localPosition.x = glm::clamp(t->m_localPosition.x, -boundarySize, boundarySize);
        t->m_localPosition.z = glm::clamp(t->m_localPosition.z, -boundarySize, boundarySize);
    }

    void OnCollisionEnter(const GE::Scripts::CollisionInfo& info) override {
        ++collisionCount;
        GE_LOG_INFO("[DemoPlayerScript] Collision #" + std::to_string(collisionCount)
            + " with entity " + std::to_string(info.otherEntity)
            + "  normal=(" + std::to_string(info.normal.x) + ","
                           + std::to_string(info.normal.y) + ","
                           + std::to_string(info.normal.z) + ")"
            + "  pen=" + std::to_string(info.penetration));
    }

    void OnCollisionExit(const GE::Scripts::CollisionInfo& info) override {
        GE_LOG_INFO("[DemoPlayerScript] Collision ended with entity "
            + std::to_string(info.otherEntity));
    }

    void OnTriggerEnter(GE::ECS::EntityID other) override {
        GE_LOG_INFO("[DemoPlayerScript] Entered trigger zone of entity "
            + std::to_string(other));
    }

    void OnTriggerExit(GE::ECS::EntityID other) override {
        GE_LOG_INFO("[DemoPlayerScript] Exited trigger zone of entity "
            + std::to_string(other));
    }

    void OnDestroy() override {
        GE_LOG_INFO("[DemoPlayerScript] Entity " + std::to_string(GetEntityID())
            + " destroyed after " + std::to_string(collisionCount) + " collision(s).");
    }
};
