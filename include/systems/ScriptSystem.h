#pragma once

#include "ecs/IECSystem.h"
#include "ecs/EntityManager.h"
#include "scripts/CollisionInfo.h"

/* parasoft-begin-suppress ALL */
#include <unordered_set>
/* parasoft-end-suppress ALL */

// Forward declarations
namespace GE::Systems { class PhysicsSystem; }
namespace GE::Scripts { class GameScriptComponent; }

namespace GE::Systems {

    /**
     * @class ScriptSystem
     * @brief ECS system that drives the GameScriptComponent lifecycle.
     * Runs at ESystemStage::GameLogic (stage 6), after PhysicsSystem (stage 3),
     * so collision data from the current frame is ready when events are dispatched.
     *
     * ## Per-frame execution order
     *
     *   1. Awake loop — fires Awake() the first time m_active becomes true for an entity. Start() is fired immediately after on the same frame.
     *   2. FixedUpdate — accumulator-driven; 0–MAX_FIXED_STEPS calls per frame at m_fixedTimestep.
     *   3. Update loop — Update(dt) for every active script.
     *   4. LateUpdate — LateUpdate(dt) for every active script.
     *   5. Collision dispatch — set-diff against previous-frame contact/trigger sets → OnCollisionEnter/Exit, OnTriggerEnter/Exit.
     *   6. OnDestroy loop — fires OnDestroy() for entities removed from the scene this frame.
     *
     * ## Registration
     * Instantiate with a PhysicsSystem* pointer (may be nullptr if no physics in scene)
     * and register in GenericScenario::OnLoad after PhysicsSystem:
     * @code
     *   auto* ss = new ScriptSystem(m_physicsSystem);
     *   m_scriptSystem = ss;
     *   em->RegisterSystem(ss);
     * @endcode
     */
    class ScriptSystem : public ECS::ICpuSystem {
    public:
        /** Fixed timestep in seconds. ~60 Hz by default. Can be set per-scenario via ImGui. */
        float m_fixedTimestep { 0.01667f };

        explicit ScriptSystem(PhysicsSystem* physicsSystem = nullptr);
        ~ScriptSystem() override = default;

        void OnUpdate(float dt) override;

        void Shutdown() override { m_state = SystemState::ShuttingDown; }

    private:
        PhysicsSystem* m_physicsSystem;   ///< Non-owning pointer; read for collision event data.

        // --- Fixed-timestep accumulator ---
        float m_accumulator { 0.0f };
        static constexpr int MAX_FIXED_STEPS = 8;   ///< Spiral-of-death guard

        // --- Awake / Destroy tracking ---
        std::unordered_set<GE::ECS::EntityID> m_awakenedEntities;

        // --- Collision event tracking (Enter / Exit via set-diff) ---
        // Uses shared GE::Scripts::ContactSet type (EntityPair + PairHash from CollisionInfo.h).
        GE::Scripts::ContactSet m_prevContacts;   ///< Solid contacts from last frame
        GE::Scripts::ContactSet m_prevTriggers;   ///< Trigger overlaps from last frame

        // --- Private helpers ---
        void RunFixedUpdate(float dt);
        void DispatchCollisionEvents();

        void FireCollisionEnter(GE::ECS::EntityID self, GE::ECS::EntityID other,
                                const GE::Scripts::CollisionInfo& info);
        void FireCollisionExit (GE::ECS::EntityID self, GE::ECS::EntityID other,
                                const GE::Scripts::CollisionInfo& info);
        void FireTriggerEnter  (GE::ECS::EntityID self, GE::ECS::EntityID other);
        void FireTriggerExit   (GE::ECS::EntityID self, GE::ECS::EntityID other);

        /** Tries to get the script from a ScriptComponent on the given entity. Returns nullptr if absent. */
        GE::Scripts::GameScriptComponent* GetScript(GE::ECS::EntityID id) const;
    };

} // namespace GE::Systems
