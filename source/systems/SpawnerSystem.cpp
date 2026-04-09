#include "systems/SpawnerSystem.h"
#include "core/ServiceLocator.h"
#include "core/NetworkBridge.h"
#include "networking/NetworkService.h"
#include "ecs/EntityManager.h"
#include "components/Transform.h"
#include "components/PhysicsComponents.h"

/* parasoft-begin-suppress ALL */
#include <glm/glm.hpp>
/* parasoft-end-suppress ALL */

namespace GE::Systems {

    SpawnerSystem::SpawnerSystem()
        : m_rng(std::random_device{}())
    {
        m_typeID = GE::ECS::IECSystem::GetUniqueISystemTypeID<SpawnerSystem>();
        m_stage  = GE::ECS::ESystemStage::GameLogic;
        m_state  = SystemState::Running;
    }

    ERROR_CODE SpawnerSystem::Shutdown() {
        m_state = SystemState::ShuttingDown;
        return ERROR_CODE::OK;
    }

    // -------------------------------------------------------------------------
    // Helpers
    // -------------------------------------------------------------------------

    glm::vec3 SpawnerSystem::randomInRange(const glm::vec3& lo, const glm::vec3& hi) {
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        return {
            lo.x + dist(m_rng) * (hi.x - lo.x),
            lo.y + dist(m_rng) * (hi.y - lo.y),
            lo.z + dist(m_rng) * (hi.z - lo.z)
        };
    }

    glm::vec3 SpawnerSystem::pickLocation(const GE::Components::SpawnerComponent& sc) {
        using LT = GE::Components::SpawnLocType;
        switch (sc.locationType) {
        case LT::FIXED:
            return sc.fixedPos;

        case LT::RANDOM_BOX:
            return randomInRange(sc.boxMin, sc.boxMax);

        case LT::RANDOM_SPHERE: {
            // Uniform sampling inside a sphere via rejection-free method
            std::uniform_real_distribution<float> dist(0.0f, 1.0f);
            const float u     = dist(m_rng);
            const float v     = dist(m_rng);
            const float theta = 2.0f * glm::pi<float>() * u;
            const float phi   = std::acos(1.0f - 2.0f * v);
            const float r     = sc.sphereRadius * std::cbrt(dist(m_rng));
            return sc.sphereCenter + glm::vec3{
                r * std::sin(phi) * std::cos(theta),
                r * std::cos(phi),
                r * std::sin(phi) * std::sin(theta)
            };
        }
        }
        return sc.fixedPos;
    }

    // -------------------------------------------------------------------------
    // Per-entity activation helper (file-local)
    // -------------------------------------------------------------------------

    static void activateEntity(GE::ECS::EntityManager* em,
                               GE::Components::SpawnerComponent& sc,
                               const glm::vec3& position,
                               const glm::vec3& linVel,
                               const glm::vec3& angVelDeg)
    {
        const GE::ECS::EntityID id = sc.pendingIds.front();
        sc.pendingIds.pop_front();

        auto* tr = em->TryGetTIComponent<GE::Components::Transform>(id);
        auto* rb = em->TryGetTIComponent<GE::Components::RigidBody>(id);
        if (!tr || !rb) return;

        tr->m_position = position;
        rb->isStatic   = false;
        rb->useGravity = true;
        rb->velocity           = linVel;
        rb->angularVelocity    = glm::radians(angVelDeg);
    }

    // -------------------------------------------------------------------------
    // ForceSpawnOne — manual trigger from ImGui
    // -------------------------------------------------------------------------

    void SpawnerSystem::ForceSpawnOne(GE::Components::SpawnerComponent& sc) {
        if (sc.pendingIds.empty()) return;

        auto* em = ServiceLocator::GetEntityManager();
        if (!em) return;

        const glm::vec3 pos    = pickLocation(sc);
        const glm::vec3 linVel = randomInRange(sc.linVelMin, sc.linVelMax);
        const glm::vec3 angVel = randomInRange(sc.angVelMin, sc.angVelMax);
        activateEntity(em, sc, pos, linVel, angVel);
    }

    // -------------------------------------------------------------------------
    // OnUpdate
    // -------------------------------------------------------------------------

    void SpawnerSystem::OnUpdate(float dt) {
        auto* em  = ServiceLocator::GetEntityManager();
        if (!em) return;
        auto& arr = em->GetCompArr<GE::Components::SpawnerComponent>();

        // Resolve networking state once per frame (avoids repeated ServiceLocator calls)
        GE::NetworkBridge* bridge   = ServiceLocator::GetNetworkBridge();
        auto* netSvc = (bridge != nullptr) ? bridge->GetService() : nullptr;
        const bool  netActive       = (netSvc != nullptr) && netSvc->IsConnected();
        const uint8_t localPeerId   = netActive ? netSvc->GetLocalPeerId() : 0U;

        for (uint32_t i = 0; i < arr.GetCount(); ++i) {
            auto& sc = arr.Data()[i];
            sc.elapsed += dt;

            if (sc.elapsed < sc.startTime || sc.pendingIds.empty()) continue;

            // Ownership gate: when networking is active, only the owning peer fires this spawner.
            if (netActive && sc.ownerPeerId != 0U && sc.ownerPeerId != localPeerId) { continue; }

            // Helper: activate one entity and broadcast its spawn to remote peers.
            auto doActivate = [&]() {
                if (sc.pendingIds.empty()) return;

                // Peek ID before activateEntity pops it (needed for broadcast)
                const GE::ECS::EntityID id = sc.pendingIds.front();

                const glm::vec3 pos    = pickLocation(sc);
                const glm::vec3 linVel = randomInRange(sc.linVelMin, sc.linVelMax);
                const glm::vec3 angVel = randomInRange(sc.angVelMin, sc.angVelMax);
                activateEntity(em, sc, pos, linVel, angVel);

                // Broadcast to remote peers so they activate the same entity
                if (netActive && bridge != nullptr) {
                    auto* rb = em->TryGetTIComponent<GE::Components::RigidBody>(id);
                    const float mass = (rb != nullptr && rb->inverseMass > 0.0f)
                                       ? (1.0f / rb->inverseMass) : 0.0f;
                    bridge->BroadcastSpawnObject(id, sc.ownerPeerId, 0U, pos, glm::vec3{1.0f}, linVel, mass);
                }
            };

            if (sc.isBurst) {
                if (sc.activated) continue;
                sc.activated = true;
                const uint32_t toActivate = glm::min(
                    static_cast<uint32_t>(sc.pendingIds.size()), sc.burstCount);
                for (uint32_t k = 0; k < toActivate; ++k) { doActivate(); }
            } else {
                if (!sc.activated) { sc.activated = true; }
                sc.timeSinceLastSpawn += dt;
                while (sc.timeSinceLastSpawn >= sc.interval && !sc.pendingIds.empty()) {
                    sc.timeSinceLastSpawn -= sc.interval;
                    doActivate();
                }
            }
        }
    }

} // namespace GE::Systems
