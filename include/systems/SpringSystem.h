#pragma once
#include "ecs/IECSystem.h"
#include "ecs/EntityManager.h"
#include <glm/glm.hpp>
#include <vector>

namespace GE::Systems {

/**
 * @brief Lab 7 — Hooke's law spring force generator.
 *
 * Each SpringData connects two endpoints (entity or world-space anchor).
 * OnUpdate() accumulates F = -k·x - b·v into RigidBody::forceAccum before
 * PhysicsSystem::OnUpdate() integrates it.
 *
 * Springs are stored at the system level (not as ECS components) because cloth
 * nodes need up to 8 springs each and ECS allows only one component per type.
 */
class SpringSystem : public ECS::ICpuSystem {
public:
    // Sentinel: entityA/entityB == WORLD_ANCHOR means use worldAnchorA/B instead.
    static constexpr GE::ECS::EntityID WORLD_ANCHOR = GE::ECS::INVALID_ENTITY_ID;

    struct SpringData {
        GE::ECS::EntityID entityA { WORLD_ANCHOR };
        GE::ECS::EntityID entityB { WORLD_ANCHOR };
        glm::vec3 worldAnchorA { 0.0f };    // used when entityA == WORLD_ANCHOR
        glm::vec3 worldAnchorB { 0.0f };    // used when entityB == WORLD_ANCHOR
        float restLength { 1.0f };
        float springConstant { 10.0f };
        float dampingCoeff { 0.0f };
    };

    SpringSystem() = default;
    ~SpringSystem() override = default;

    void OnUpdate(float dt) override;
    void Shutdown() override { m_springs.clear(); }

    void AddSpring(const SpringData& s) { m_springs.push_back(s); }
    void ClearSprings() { m_springs.clear(); }

    const std::vector<SpringData>& GetSprings() const { return m_springs; }
          std::vector<SpringData>& GetSpringsMutable() { return m_springs; }

private:
    std::vector<SpringData> m_springs;
};

} // namespace GE::Systems
