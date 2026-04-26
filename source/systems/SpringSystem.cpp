#include "systems/SpringSystem.h"
#include "core/ServiceLocator.h"
#include "components/PhysicsComponents.h"
#include "components/Transform.h"

/* parasoft-begin-suppress ALL */
#include <glm/glm.hpp>
/* parasoft-end-suppress ALL */

namespace GE::Systems {

void SpringSystem::OnUpdate(float dt) {
    auto* em = ServiceLocator::GetEntityManager();
    if (!em) return;

    for (const auto& s : m_springs) {
        // --- Resolve positions and velocities for both endpoints ---
        glm::vec3 posA{}, posB{}, velA{}, velB{};
        GE::Components::RigidBody* rbA = nullptr;
        GE::Components::RigidBody* rbB = nullptr;

        if (s.entityA == WORLD_ANCHOR) {
            posA = s.worldAnchorA;
        } else {
            auto* t = em->TryGetTIComponent<GE::Components::Transform>(s.entityA);
            if (!t) continue;
            posA = t->m_worldPosition;
            rbA  = em->TryGetTIComponent<GE::Components::RigidBody>(s.entityA);
            if (rbA) velA = rbA->velocity;
        }

        if (s.entityB == WORLD_ANCHOR) {
            posB = s.worldAnchorB;
        } else {
            auto* t = em->TryGetTIComponent<GE::Components::Transform>(s.entityB);
            if (!t) continue;
            posB = t->m_worldPosition;
            rbB  = em->TryGetTIComponent<GE::Components::RigidBody>(s.entityB);
            if (rbB) velB = rbB->velocity;
        }

        // --- Hooke's law: F = -k(L - L_rest) - b * v_rel_along_spring ---
        const glm::vec3 delta = posA - posB;
        const float     L     = glm::length(delta);
        if (L < 1e-6f) continue;

        const glm::vec3 dir = delta / L;  // unit vector from B → A

        float forceMag = -s.springConstant * (L - s.restLength);
        if (s.dampingCoeff > 0.0f)
            forceMag -= s.dampingCoeff * glm::dot(velA - velB, dir);

        // force on A; Newton's 3rd law gives -force on B
        const glm::vec3 force = forceMag * dir;

        if (rbA && !rbA->isStatic) rbA->forceAccum += force;
        if (rbB && !rbB->isStatic) rbB->forceAccum -= force;
    }
}

} // namespace GE::Systems
