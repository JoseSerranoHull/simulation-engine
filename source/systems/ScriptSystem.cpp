#include "systems/ScriptSystem.h"
#include "systems/PhysicsSystem.h"
#include "components/ScriptComponent.h"
#include "core/ServiceLocator.h"

/* parasoft-begin-suppress ALL */
#include <algorithm>
/* parasoft-end-suppress ALL */

namespace GE::Systems {

    ScriptSystem::ScriptSystem(PhysicsSystem* physicsSystem)
        : m_physicsSystem(physicsSystem)
    {
        m_typeID = ECS::IECSystem::GetUniqueISystemTypeID<ScriptSystem>();
        m_stage  = ECS::ESystemStage::GameLogic;
        m_state  = SystemState::Running;
    }

    void ScriptSystem::OnUpdate(float dt) {
        auto* em = ServiceLocator::GetEntityManager();
        auto& scriptArr = em->GetCompArr<GE::Components::ScriptComponent>();

        // -----------------------------------------------------------------------
        // 1. Awake / Start loop — fires on the first frame m_active becomes true.
        // -----------------------------------------------------------------------
        for (uint32_t i = 0; i < scriptArr.GetCount(); ++i) {
            const GE::ECS::EntityID id = scriptArr.Index()[i];
            auto& sc = scriptArr.Data()[i];
            if (!sc.script || !sc.script->m_active) continue;

            if (m_awakenedEntities.find(id) == m_awakenedEntities.end()) {
                sc.script->SetEntityID(id);
                sc.script->Awake();
                sc.script->Start();
                sc.script->m_started = true;
                m_awakenedEntities.insert(id);
            }
        }

        // -----------------------------------------------------------------------
        // 2. FixedUpdate — accumulator-driven, capped at MAX_FIXED_STEPS per frame.
        // -----------------------------------------------------------------------
        RunFixedUpdate(dt);

        // -----------------------------------------------------------------------
        // 3. Update loop
        // -----------------------------------------------------------------------
        for (uint32_t i = 0; i < scriptArr.GetCount(); ++i) {
            const GE::ECS::EntityID id = scriptArr.Index()[i];
            auto& sc = scriptArr.Data()[i];
            if (!sc.script || !sc.script->m_active) continue;
            sc.script->Update(dt);
        }

        // -----------------------------------------------------------------------
        // 4. LateUpdate loop
        // -----------------------------------------------------------------------
        for (uint32_t i = 0; i < scriptArr.GetCount(); ++i) {
            auto& sc = scriptArr.Data()[i];
            if (!sc.script || !sc.script->m_active) continue;
            sc.script->LateUpdate(dt);
        }

        // -----------------------------------------------------------------------
        // 5. Collision event dispatch (set-diff against previous frame).
        // -----------------------------------------------------------------------
        DispatchCollisionEvents();

        // -----------------------------------------------------------------------
        // 6. OnDestroy — detect entities removed since last frame.
        //    Any ID in m_awakenedEntities that no longer has a ScriptComponent fires OnDestroy.
        // -----------------------------------------------------------------------
        std::vector<GE::ECS::EntityID> toErase;
        for (const GE::ECS::EntityID id : m_awakenedEntities) {
            if (!em->HasComponent<GE::Components::ScriptComponent>(id)) {
                auto* script = GetScript(id);
                if (script) script->OnDestroy();
                toErase.push_back(id);
            }
        }
        for (const GE::ECS::EntityID id : toErase) {
            m_awakenedEntities.erase(id);
        }
    }

    // -------------------------------------------------------------------------
    // Fixed-timestep accumulator
    // -------------------------------------------------------------------------
    void ScriptSystem::RunFixedUpdate(float dt) {
        m_accumulator += dt;
        int steps = 0;
        auto* em = ServiceLocator::GetEntityManager();
        auto& scriptArr = em->GetCompArr<GE::Components::ScriptComponent>();

        while (m_accumulator >= m_fixedTimestep && steps < MAX_FIXED_STEPS) {
            for (uint32_t i = 0; i < scriptArr.GetCount(); ++i) {
                auto& sc = scriptArr.Data()[i];
                if (!sc.script || !sc.script->m_active) continue;
                sc.script->FixedUpdate(m_fixedTimestep);
            }
            m_accumulator -= m_fixedTimestep;
            ++steps;
        }
    }

    // -------------------------------------------------------------------------
    // Collision / trigger event dispatch
    // -------------------------------------------------------------------------
    void ScriptSystem::DispatchCollisionEvents() {
        if (!m_physicsSystem) return;

        const auto& currentContacts = m_physicsSystem->m_currentContacts;
        const auto& currentInfos    = m_physicsSystem->m_currentContactInfos;
        const auto& currentTriggers = m_physicsSystem->m_currentTriggers;

        // --- Solid collision Enter (in current, NOT in previous) ---
        for (const auto& pair : currentContacts) {
            if (m_prevContacts.find(pair) == m_prevContacts.end()) {
                // Look up contact info (stored from A's perspective)
                auto it = currentInfos.find(pair);
                if (it != currentInfos.end()) {
                    // A's perspective
                    GE::Scripts::CollisionInfo infoA = it->second;  // otherEntity = B
                    FireCollisionEnter(pair.first, pair.second, infoA);

                    // B's perspective (mirror normal)
                    GE::Scripts::CollisionInfo infoB;
                    infoB.otherEntity  = pair.first;
                    infoB.contactPoint = it->second.contactPoint;
                    infoB.normal       = -it->second.normal;
                    infoB.penetration  = it->second.penetration;
                    FireCollisionEnter(pair.second, pair.first, infoB);
                } else {
                    FireCollisionEnter(pair.first,  pair.second, {pair.second});
                    FireCollisionEnter(pair.second, pair.first,  {pair.first});
                }
            }
        }

        // --- Solid collision Exit (in previous, NOT in current) ---
        for (const auto& pair : m_prevContacts) {
            if (currentContacts.find(pair) == currentContacts.end()) {
                GE::Scripts::CollisionInfo exitInfoA{ pair.second };
                GE::Scripts::CollisionInfo exitInfoB{ pair.first };
                FireCollisionExit(pair.first,  pair.second, exitInfoA);
                FireCollisionExit(pair.second, pair.first,  exitInfoB);
            }
        }

        // --- Trigger Enter ---
        for (const auto& pair : currentTriggers) {
            if (m_prevTriggers.find(pair) == m_prevTriggers.end()) {
                FireTriggerEnter(pair.first,  pair.second);
                FireTriggerEnter(pair.second, pair.first);
            }
        }

        // --- Trigger Exit ---
        for (const auto& pair : m_prevTriggers) {
            if (currentTriggers.find(pair) == currentTriggers.end()) {
                FireTriggerExit(pair.first,  pair.second);
                FireTriggerExit(pair.second, pair.first);
            }
        }

        // --- Swap sets for next frame ---
        m_prevContacts = currentContacts;
        m_prevTriggers = currentTriggers;
    }

    // -------------------------------------------------------------------------
    // Fire helpers — look up script on the given entity and call the callback.
    // -------------------------------------------------------------------------
    GE::Scripts::GameScriptComponent* ScriptSystem::GetScript(GE::ECS::EntityID id) const {
        auto* em = ServiceLocator::GetEntityManager();
        auto* sc = em->TryGetTIComponent<GE::Components::ScriptComponent>(id);
        if (sc && sc->script) return sc->script.get();
        return nullptr;
    }

    void ScriptSystem::FireCollisionEnter(GE::ECS::EntityID self,
                                           GE::ECS::EntityID /*other*/,
                                           const GE::Scripts::CollisionInfo& info) {
        auto* script = GetScript(self);
        if (script && script->m_active) script->OnCollisionEnter(info);
    }

    void ScriptSystem::FireCollisionExit(GE::ECS::EntityID self,
                                          GE::ECS::EntityID /*other*/,
                                          const GE::Scripts::CollisionInfo& info) {
        auto* script = GetScript(self);
        if (script && script->m_active) script->OnCollisionExit(info);
    }

    void ScriptSystem::FireTriggerEnter(GE::ECS::EntityID self, GE::ECS::EntityID other) {
        auto* script = GetScript(self);
        if (script && script->m_active) script->OnTriggerEnter(other);
    }

    void ScriptSystem::FireTriggerExit(GE::ECS::EntityID self, GE::ECS::EntityID other) {
        auto* script = GetScript(self);
        if (script && script->m_active) script->OnTriggerExit(other);
    }

} // namespace GE::Systems
