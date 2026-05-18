#include "systems/SpawnerSystem.h"
#include "scene/EntityFactory.h"
#include "scene/fb/FBSceneContext.h"
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
    m_stage = GE::ECS::ESystemStage::GameLogic;
    m_state = SystemState::Running;
}

void SpawnerSystem::Shutdown() {
    m_state = SystemState::ShuttingDown;
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
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        const float u = dist(m_rng);
        const float v = dist(m_rng);
        const float theta = 2.0f * glm::pi<float>() * u;
        const float phi = std::acos(1.0f - 2.0f * v);
        const float r = sc.sphereRadius * std::cbrt(dist(m_rng));
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
// ForceSpawnOne — manual trigger from ImGui Spawners tab
// -------------------------------------------------------------------------

void SpawnerSystem::ForceSpawnOne(GE::Components::SpawnerComponent& sc) {
    if (sc.spawnedCount >= sc.maxCount) { return; }
    if (sc.prefabTemplate == nullptr && sc.prefabVariants.empty()) { return; }

    auto* em = ServiceLocator::GetEntityManager();
    if (!em) { return; }

    const glm::vec3 pos = pickLocation(sc);
    const glm::vec3 linVel = randomInRange(sc.linVelMin, sc.linVelMax);
    const glm::vec3 angVel = randomInRange(sc.angVelMin, sc.angVelMax);

    const uint8_t colorIdx = sc.isSequential
        ? static_cast<uint8_t>(sc.spawnedCount % 4)
        : (sc.ownerPeerId > 0U ? sc.ownerPeerId - 1U : 0U);

    const uint8_t effectiveOwner = sc.isSequential
        ? static_cast<uint8_t>(colorIdx + 1U)
        : sc.ownerPeerId;

    // Pick random size variant if available, else fall back to canonical prefabTemplate
    const GE::Scene::FB::PrefabTemplate* tmpl = sc.prefabTemplate;
    if (!sc.prefabVariants.empty()) {
        std::uniform_int_distribution<std::size_t> vd(0, sc.prefabVariants.size() - 1);
        const auto* candidate = sc.prefabVariants[vd(m_rng)];
        if (candidate != nullptr) { tmpl = candidate; }
    }
    if (tmpl == nullptr) { return; }

    const GE::ECS::EntityID id = GE::Scene::EntityFactory::InstantiatePrefab(
        *tmpl, pos, glm::vec3{0.0f}, linVel, angVel,
        effectiveOwner, colorIdx, em);

    if (id != UINT32_MAX) {
        ++sc.spawnedCount;
        sc.spawnedEntityIds.push_back(id);
    }
}

// -------------------------------------------------------------------------
// OnUpdate
// -------------------------------------------------------------------------

void SpawnerSystem::OnUpdate(float dt) {
    auto* em = ServiceLocator::GetEntityManager();
    if (!em) { return; }
    auto& arr = em->GetCompArr<GE::Components::SpawnerComponent>();

    GE::NetworkBridge* bridge = ServiceLocator::GetNetworkBridge();
    auto* netSvc = (bridge != nullptr) ? bridge->GetService() : nullptr;
    const bool netActive = (netSvc != nullptr) && netSvc->IsConnected();
    const uint8_t localPeerId = netActive ? netSvc->GetLocalPeerId() : 0U;

    for (uint32_t i = 0; i < arr.GetCount(); ++i) {
        auto& sc = arr.Data()[i];

        // Only advance the timer when not paused
        if (!sc.paused) { sc.elapsed += dt; }

        if (sc.paused) { continue; }
        if (sc.elapsed < sc.startTime) { continue; }
        if (sc.spawnedCount >= sc.maxCount) { continue; }
        if (sc.prefabTemplate == nullptr && sc.prefabVariants.empty()) { continue; }

        // Ownership gate: when networked, only the owning peer fires
        if (netActive && sc.ownerPeerId != 0U && sc.ownerPeerId != localPeerId) { continue; }

        auto doSpawn = [&]() {
            if (sc.spawnedCount >= sc.maxCount) { return; }

            const glm::vec3 pos = pickLocation(sc);
            const glm::vec3 linVel = randomInRange(sc.linVelMin, sc.linVelMax);
            const glm::vec3 angVel = randomInRange(sc.angVelMin, sc.angVelMax);

            const uint8_t colorIdx = sc.isSequential
                ? static_cast<uint8_t>(sc.spawnedCount % 4)
                : (sc.ownerPeerId > 0U ? sc.ownerPeerId - 1U : 0U);

            const uint8_t effectiveOwner = sc.isSequential
                ? static_cast<uint8_t>(colorIdx + 1U)
                : sc.ownerPeerId;

            // Pick random size variant if available, else fall back to canonical prefabTemplate
            const GE::Scene::FB::PrefabTemplate* tmpl = sc.prefabTemplate;
            if (!sc.prefabVariants.empty()) {
                std::uniform_int_distribution<std::size_t> vd(0, sc.prefabVariants.size() - 1);
                const auto* candidate = sc.prefabVariants[vd(m_rng)];
                if (candidate != nullptr) { tmpl = candidate; }
            }
            if (tmpl == nullptr) { return; }

            const GE::ECS::EntityID id = GE::Scene::EntityFactory::InstantiatePrefab(
                *tmpl, pos, glm::vec3{0.0f}, linVel, angVel,
                effectiveOwner, colorIdx, em);

            if (id == UINT32_MAX) { return; }
            ++sc.spawnedCount;
            sc.spawnedEntityIds.push_back(id);

            // Broadcast to remote peers
            if (netActive && bridge != nullptr) {
                auto* rb = em->TryGetTIComponent<GE::Components::RigidBody>(id);
                const float mass = (rb != nullptr && rb->inverseMass > 0.0f)
                                   ? (1.0f / rb->inverseMass) : 0.0f;
                bridge->BroadcastSpawnObject(id, effectiveOwner, 0U, pos, glm::vec3{1.0f}, linVel, mass);
            }
        };

        if (sc.isBurst) {
            if (sc.activated) { continue; }
            sc.activated = true;
            const uint32_t toSpawn = glm::min(
                sc.burstCount, sc.maxCount - sc.spawnedCount);
            for (uint32_t k = 0; k < toSpawn; ++k) { doSpawn(); }
        } else {
            if (!sc.activated) { sc.activated = true; }
            sc.timeSinceLastSpawn += dt;
            while (sc.timeSinceLastSpawn >= sc.interval && sc.spawnedCount < sc.maxCount) {
                sc.timeSinceLastSpawn -= sc.interval;
                doSpawn();
            }
        }
    }

} // namespace GE::Systems

} // namespace GE::Systems
