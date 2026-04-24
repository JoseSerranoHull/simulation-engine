/* parasoft-begin-suppress ALL */
#include <cmath>
/* parasoft-end-suppress ALL */

#include "systems/ClothSystem.h"
#include "components/ClothComponent.h"
#include "components/PhysicsComponents.h"
#include "components/Transform.h"
#include "ecs/ComponentArray.h"
#include "core/ServiceLocator.h"
#include "core/Common.h"

/* parasoft-begin-suppress ALL */
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
/* parasoft-end-suppress ALL */

namespace GE::Systems {

ClothSystem::ClothSystem() {
    m_typeID = ECS::IECSystem::GetUniqueISystemTypeID<ClothSystem>();
    m_stage  = ECS::ESystemStage::Physics;
    m_state  = SystemState::Running;
}

static constexpr float GRAVITY    = -9.81f;
static constexpr float MIN_LENGTH = 1e-6f;

// Apply spring force between two particles (A and B), rest length = restLen.
// Adds F to A, subtracts F from B. Pinned particles receive no force.
static void applySpring(
    GE::Components::ClothParticle& pA,
    GE::Components::ClothParticle& pB,
    float restLen,
    float kSpring,
    float kDamp,
    float dt)
{
    const glm::vec3 d   = pA.position - pB.position;
    const float     len = glm::length(d);
    if (len < MIN_LENGTH) { return; }

    const glm::vec3 dir = d / len;

    // Velocities estimated from Verlet positions
    const glm::vec3 velA = (dt > 0.0f) ? (pA.position - pA.prevPosition) / dt : glm::vec3{ 0.0f };
    const glm::vec3 velB = (dt > 0.0f) ? (pB.position - pB.prevPosition) / dt : glm::vec3{ 0.0f };

    const float springMag = kSpring * (len - restLen);
    const float dampMag   = kDamp  * glm::dot(velA - velB, dir);
    const glm::vec3 F     = (springMag + dampMag) * dir;

    if (!pA.pinned) { pA.force -= F; }
    if (!pB.pinned) { pB.force += F; }
}

void ClothSystem::OnUpdate(float dt) {
    if (dt <= 0.0f) { return; }

    GE::ECS::EntityManager* em = ServiceLocator::GetEntityManager();
    if (em == nullptr) { return; }

    auto& clothArr   = em->GetCompArr<GE::Components::ClothComponent>();
    auto& sphereArr  = em->GetCompArr<GE::Components::SphereCollider>();
    auto& transformArr = em->GetCompArr<GE::Components::Transform>();

    const uint32_t clothCount = clothArr.GetCount();
    for (uint32_t ci = 0U; ci < clothCount; ++ci) {
        GE::Components::ClothComponent& cc = clothArr.Data()[ci];
        const int R = cc.rows;
        const int C = cc.cols;
        if (R <= 0 || C <= 0 || cc.particles.empty()) { continue; }

        // ---------------------------------------------------------------
        // 1. Accumulate external forces
        // ---------------------------------------------------------------
        for (auto& p : cc.particles) {
            if (p.pinned) { p.force = glm::vec3{ 0.0f }; continue; }
            // Burned particles receive gravity only (springs already gone, no wind drag)
            if (p.burned) {
                p.force = glm::vec3{ 0.0f, GRAVITY * cc.particleMass, 0.0f };
                continue;
            }
            p.force  = glm::vec3{ 0.0f,
                                  GRAVITY * cc.particleMass,
                                  0.0f };
            p.force += glm::vec3{ cc.windX * cc.particleMass,
                                  0.0f,
                                  cc.windZ * cc.particleMass };
        }

        // ---------------------------------------------------------------
        // 2-4. Springs (structural + shear + flexion) via explicit list.
        //      Supports tearing: springs with active==false are skipped permanently.
        // ---------------------------------------------------------------
        for (GE::Components::ClothSpring& s : cc.springs) {
            if (!s.active) { continue; }

            GE::Components::ClothParticle& pA = cc.particles[s.a];
            GE::Components::ClothParticle& pB = cc.particles[s.b];

            // Deactivate springs involving burned particles
            if (pA.burned || pB.burned) { s.active = false; continue; }

            applySpring(pA, pB, s.restLen, s.kSpring, cc.damping, dt);

            // Tearing check: if current length exceeds threshold * restLen, tear permanently
            const float currentLen = glm::length(pA.position - pB.position);
            if (currentLen > cc.tearThreshold * s.restLen) {
                s.active = false;
            }
        }

        // ---------------------------------------------------------------
        // 5. Verlet integration
        // ---------------------------------------------------------------
        const float dampFactor = 1.0f - cc.damping * dt;
        for (auto& p : cc.particles) {
            if (p.pinned) { continue; }  // burned particles fall freely under gravity
            const glm::vec3 acc    = p.force / cc.particleMass;
            const glm::vec3 newPos = p.position
                                   + (p.position - p.prevPosition) * dampFactor
                                   + acc * (dt * dt);
            p.prevPosition = p.position;
            p.position     = newPos;
            p.force        = glm::vec3{ 0.0f };
        }

        // ---------------------------------------------------------------
        // 6. Sphere-cloth collision
        // ---------------------------------------------------------------
        const uint32_t sphereCount = sphereArr.GetCount();
        for (uint32_t si = 0U; si < sphereCount; ++si) {
            const GE::Components::SphereCollider& sc = sphereArr.Data()[si];
            const GE::ECS::EntityID seid             = sphereArr.Index()[si];

            // Find the entity's Transform
            GE::Components::Transform* tr = em->TryGetTIComponent<GE::Components::Transform>(seid);
            if (tr == nullptr) { continue; }

            const glm::vec3 sphereCenter = tr->m_position;
            const float     sphereRadius = sc.radius;

            for (auto& p : cc.particles) {
                if (p.pinned) { continue; }
                const glm::vec3 diff = p.position - sphereCenter;
                const float     dist = glm::length(diff);
                if (dist < sphereRadius && dist > MIN_LENGTH) {
                    p.position = sphereCenter + glm::normalize(diff) * sphereRadius;
                }
            }
        }

        // ---------------------------------------------------------------
        // 7. Burning: heat particles within burnCenter radius, free when >= 1.0
        // ---------------------------------------------------------------
        if (cc.burnActive) {
            for (uint32_t pi = 0U; pi < static_cast<uint32_t>(cc.particles.size()); ++pi) {
                GE::Components::ClothParticle& p = cc.particles[pi];
                if (p.burned || p.pinned) { continue; }

                const float dist = glm::length(p.position - cc.burnCenter);
                if (dist < cc.burnRadius) {
                    p.heat += cc.burnRate * dt;
                }

                if (p.heat >= 1.0f && !p.burned) {
                    p.burned = true;
                    p.pinned = false; // ensure free fall
                    // Deactivate all springs connected to this particle
                    for (GE::Components::ClothSpring& s : cc.springs) {
                        if (s.a == static_cast<uint16_t>(pi) ||
                            s.b == static_cast<uint16_t>(pi)) {
                            s.active = false;
                        }
                    }
                }
            }
        }
    }
}

} // namespace GE::Systems
