#include "systems/PhysicsSystem.h"
#include "core/ServiceLocator.h"
#include "components/PhysicsComponents.h"
#include "physics/Sphere.h"
#include "physics/Plane.h"
#include "components/Transform.h"

namespace GE::Systems {

    static constexpr glm::vec3 GRAVITY{ 0.0f, -9.81f, 0.0f };

    void PhysicsSystem::OnUpdate(float dt) {
        Integrate(dt);
        ResolveCollisions();
    }

    // =========================================================================
    // SECTION 1: INTEGRATION
    // =========================================================================

    /**
     * @brief Accumulates forces, then integrates velocity and position using the
     * selected method. Fulfills Lab 3 Q2 (multiple methods) and Q3 (gravity via
     * force accumulator with inverseMass).
     */
    void PhysicsSystem::Integrate(float dt) {
        auto* em = ServiceLocator::GetEntityManager();
        auto& rbArray = em->GetCompArr<GE::Components::RigidBody>();

        for (uint32_t i = 0; i < rbArray.GetCount(); ++i) {
            auto  id    = rbArray.Index()[i];
            auto& rb    = rbArray.Data()[i];
            auto* trans = em->GetTIComponent<GE::Components::Transform>(id);

            if (!trans || rb.isStatic) continue;

            // --- 1. Force Accumulation ---
            // Fulfills Q3: accumulate gravity as a force (F = m * g), then derive
            // acceleration via a = F * (1/m) = F * inverseMass.
            if (rb.useGravity) {
                rb.forceAccum += GRAVITY * rb.mass;
            }

            const glm::vec3 accel = rb.forceAccum * rb.inverseMass;

            // --- 2. Integration (method-dependent) ---
            switch (m_integrationMethod) {

            case IntegrationMethod::Euler:
                // Explicit (Forward) Euler: position uses OLD velocity.
                // Tends to gain energy; useful as an educational comparison.
                trans->m_position += rb.velocity * dt;
                rb.velocity       += accel * dt;
                break;

            case IntegrationMethod::SemiImplicit:
                // Symplectic Euler: velocity updated FIRST, then used for position.
                // Energy-conserving for conservative forces — preferred default.
                rb.velocity       += accel * dt;
                trans->m_position += rb.velocity * dt;
                break;

            case IntegrationMethod::RK4:
                // 4th-order Runge-Kutta. Matches the analytic formula
                // s = ut + 0.5*a*t² exactly for constant acceleration.
                IntegrateRK4(trans->m_position, rb.velocity, accel, dt);
                break;
            }

            // --- 3. Clear accumulator for next frame ---
            rb.forceAccum = glm::vec3(0.0f);

            trans->m_state = GE::Components::Transform::TransformState::Dirty;
        }
    }

    /**
     * @brief 4th-order Runge-Kutta integrator for coupled (position, velocity) ODE.
     * With constant acceleration the weighted-slope cancels to the analytic result,
     * satisfying the s = ut + 0.5*a*t² test cases required by Lab 3 Q2.
     */
    void PhysicsSystem::IntegrateRK4(glm::vec3& pos, glm::vec3& vel,
                                      const glm::vec3& accel, float dt)
    {
        // k_n = (dPos, dVel) evaluated at each sub-step
        // For constant accel: dVel is always accel; dPos is the velocity at that sub-step.
        const glm::vec3 k1p = vel;
        const glm::vec3 k1v = accel;

        const glm::vec3 k2p = vel + k1v * (dt * 0.5f);
        const glm::vec3 k2v = accel;

        const glm::vec3 k3p = vel + k2v * (dt * 0.5f);
        const glm::vec3 k3v = accel;

        const glm::vec3 k4p = vel + k3v * dt;
        const glm::vec3 k4v = accel;

        pos += (dt / 6.0f) * (k1p + 2.0f * k2p + 2.0f * k3p + k4p);
        vel += (dt / 6.0f) * (k1v + 2.0f * k2v + 2.0f * k3v + k4v);
    }

    // =========================================================================
    // SECTION 2: COLLISION RESOLUTION
    // =========================================================================

    /**
     * @brief Detects and resolves sphere-plane and sphere-sphere collisions.
     * Fulfills Lab 3 Q4. Sphere-sphere uses the general impulse formula,
     * which handles same-mass, different-mass, and one-static cases uniformly.
     */
    void PhysicsSystem::ResolveCollisions() {
        auto* em = ServiceLocator::GetEntityManager();
        auto& sphereArray = em->GetCompArr<GE::Components::SphereCollider>();
        auto& planeArray  = em->GetCompArr<GE::Components::PlaneCollider>();

        // -----------------------------------------------------------------
        // Pass A: Sphere-Plane collisions
        // -----------------------------------------------------------------
        for (uint32_t sIdx = 0; sIdx < sphereArray.GetCount(); ++sIdx) {
            const auto  sID   = sphereArray.Index()[sIdx];
            auto&       sCol  = sphereArray.Data()[sIdx];
            auto* const sTrans = em->GetTIComponent<GE::Components::Transform>(sID);
            auto* const sRB   = em->GetTIComponent<GE::Components::RigidBody>(sID);

            if (!sTrans || (sRB && sRB->isStatic)) continue;

            for (uint32_t pIdx = 0; pIdx < planeArray.GetCount(); ++pIdx) {
                const auto& pCol = planeArray.Data()[pIdx];

                GE::Physics::Sphere sphere(sTrans->m_position, sCol.radius);
                GE::Physics::Plane  plane(pCol.normal * pCol.offset, pCol.normal);

                const float dist = plane.DistanceToPoint(sphere.GetCenter());

                if (dist < sphere.GetRadius()) {
                    const float penetration = sphere.GetRadius() - dist;

                    // 1. Positional correction — push out of the plane
                    sTrans->m_position += plane.GetNormal() * penetration;

                    // 2. Velocity reflection with restitution
                    if (sRB) {
                        sRB->velocity = glm::reflect(sRB->velocity, plane.GetNormal());
                        sRB->velocity *= sRB->restitution;

                        // Kill micro-velocities to prevent jitter at rest
                        if (glm::length(sRB->velocity) < 0.05f) {
                            sRB->velocity = glm::vec3(0.0f);
                        }
                    }
                }
            }
        }

        // -----------------------------------------------------------------
        // Pass B: Sphere-Sphere collisions (general impulse, handles all mass cases)
        // -----------------------------------------------------------------
        for (uint32_t aIdx = 0; aIdx < sphereArray.GetCount(); ++aIdx) {
            const auto  aID    = sphereArray.Index()[aIdx];
            auto&       aCol   = sphereArray.Data()[aIdx];
            auto* const aTrans = em->GetTIComponent<GE::Components::Transform>(aID);
            auto* const aRB    = em->GetTIComponent<GE::Components::RigidBody>(aID);

            if (!aTrans) continue;

            for (uint32_t bIdx = aIdx + 1; bIdx < sphereArray.GetCount(); ++bIdx) {
                const auto  bID    = sphereArray.Index()[bIdx];
                auto&       bCol   = sphereArray.Data()[bIdx];
                auto* const bTrans = em->GetTIComponent<GE::Components::Transform>(bID);
                auto* const bRB    = em->GetTIComponent<GE::Components::RigidBody>(bID);

                if (!bTrans) continue;
                // Skip pairs where both are static
                if (aRB && aRB->isStatic && bRB && bRB->isStatic) continue;

                const glm::vec3 diff      = aTrans->m_position - bTrans->m_position;
                const float     dist      = glm::length(diff);
                const float     radiusSum = aCol.radius + bCol.radius;

                if (dist >= radiusSum || dist < 1e-6f) continue;

                const glm::vec3 n           = diff / dist;             // Unit normal A←B
                const float     penetration = radiusSum - dist;

                const float invMassA = (aRB && !aRB->isStatic) ? aRB->inverseMass : 0.0f;
                const float invMassB = (bRB && !bRB->isStatic) ? bRB->inverseMass : 0.0f;
                const float totalInvMass = invMassA + invMassB;

                // 1. Positional correction — proportional to inverse mass
                if (totalInvMass > 0.0f) {
                    const glm::vec3 correction = (penetration / totalInvMass) * n;
                    if (aRB && !aRB->isStatic) aTrans->m_position += correction * invMassA;
                    if (bRB && !bRB->isStatic) bTrans->m_position -= correction * invMassB;
                }

                // 2. Impulse response — general formula:  j = -(1+e)*vRel / (1/mA + 1/mB)
                if (aRB && bRB && totalInvMass > 0.0f) {
                    const float vRel = glm::dot(aRB->velocity - bRB->velocity, n);
                    if (vRel < 0.0f) {  // Only resolve if objects are approaching
                        const float e = glm::min(aRB->restitution, bRB->restitution);
                        const float j = -(1.0f + e) * vRel / totalInvMass;

                        if (!aRB->isStatic) aRB->velocity += j * invMassA * n;
                        if (!bRB->isStatic) bRB->velocity -= j * invMassB * n;
                    }
                }
            }
        }
    }
}
