#include "systems/PhysicsSystem.h"
#include "core/ServiceLocator.h"
#include "components/PhysicsComponents.h"
#include "components/AnimationComponents.h"
#include "physics/Sphere.h"
#include "physics/Plane.h"
#include "components/Transform.h"

/* parasoft-begin-suppress ALL */
#include <glm/gtc/matrix_transform.hpp>  // translate, scale
#include <algorithm>                     // std::min, std::max
/* parasoft-end-suppress ALL */

namespace GE::Systems {

    static constexpr glm::vec3 GRAVITY{ 0.0f, -9.81f, 0.0f };

    // ---- Angular dynamics helpers (PhysicsObject workshop pattern) ----------

    /**
     * @brief Builds the 3x3 skew-symmetric cross-product matrix for vector ω.
     * SkewSymmetric(ω) · v  ==  cross(ω, v).
     * Used to integrate orientation: dR/dt = Skew(ω) · R.
     */
    static glm::mat3 SkewSymmetric(const glm::vec3& w) {
        // GLM mat3 constructor fills column-by-column.
        return glm::mat3(
             0.0f,  w.z, -w.y,   // col 0
            -w.z,  0.0f,  w.x,   // col 1
             w.y,  -w.x, 0.0f    // col 2
        );
    }

    /**
     * @brief Re-orthogonalises a rotation matrix using Gram-Schmidt to prevent
     * floating-point drift accumulating over many integration steps.
     * Mirrors PhysicsObject::Orthogonalise().
     */
    static void Orthogonalise(glm::mat3& R) {
        const glm::vec3 x = glm::normalize(R[0]);
        const glm::vec3 y = glm::normalize(R[1] - glm::dot(R[1], x) * x);
        R[0] = x;
        R[1] = y;
        R[2] = glm::cross(x, y);
    }

    void PhysicsSystem::ApplyForceAtPoint(GE::Components::RigidBody& rb,
                                           const glm::vec3& force,
                                           const glm::vec3& pointRelCoM)
    {
        rb.forceAccum  += force;
        rb.torqueAccum += glm::cross(pointRelCoM, force);  // τ = r × F
    }

    // =========================================================================
    // SyncWorldToLocal — convert m_worldPosition → m_localPosition after corrections
    // =========================================================================

    void PhysicsSystem::SyncWorldToLocal(GE::Components::Transform& trans,
                                          GE::ECS::EntityManager* em)
    {
        if (trans.m_parentEntityID == UINT32_MAX) {
            trans.m_localPosition = trans.m_worldPosition;
        } else {
            auto* p = em->TryGetTIComponent<GE::Components::Transform>(trans.m_parentEntityID);
            if (p) {
                trans.m_localPosition = glm::vec3(
                    glm::inverse(p->m_worldMatrix) * glm::vec4(trans.m_worldPosition, 1.0f));
            } else {
                trans.m_localPosition = trans.m_worldPosition;
            }
        }
        // Patch the translation column of localMatrix so TransformSystem pass 2 computes
        // the correct worldMatrix. Column 3 of a TRS matrix is always pure translation —
        // the rotation/scale columns [0..2] are unaffected by this write.
        // Without this, collision corrections to m_worldPosition are lost when
        // TransformSystem overwrites m_worldPosition from the stale localMatrix in pass 3.
        trans.m_localMatrix[3] = glm::vec4(trans.m_localPosition, 1.0f);
    }

    void PhysicsSystem::OnUpdate(float dt) {
        Integrate(dt);
        for (int iter = 0; iter < m_solverIterations; ++iter) {
            ResolveCollisions();
        }
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
        m_lastDt = dt;
        auto* em = ServiceLocator::GetEntityManager();
        auto& rbArray = em->GetCompArr<GE::Components::RigidBody>();

        for (uint32_t i = 0; i < rbArray.GetCount(); ++i) {
            auto  id    = rbArray.Index()[i];
            auto& rb    = rbArray.Data()[i];
            auto* trans = em->TryGetTIComponent<GE::Components::Transform>(id);

            if (!trans || rb.isStatic) continue;

            // --- 1. Force Accumulation ---
            // Fulfills Q3: accumulate gravity as a force (F = m * g), then derive
            // acceleration via a = F * (1/m) = F * inverseMass.
            if (m_gravityEnabled && rb.useGravity) {
                rb.forceAccum += GRAVITY * rb.mass;
            }

            const glm::vec3 accel = rb.forceAccum * rb.inverseMass;

            // --- 2. Integration (method-dependent) — operates on world-space position ---
            switch (m_integrationMethod) {

            case IntegrationMethod::Euler:
                // Explicit (Forward) Euler: position uses OLD velocity.
                // Tends to gain energy; useful as an educational comparison.
                trans->m_worldPosition += rb.velocity * dt;
                rb.velocity            += accel * dt;
                break;

            case IntegrationMethod::SemiImplicit:
                // Symplectic Euler: velocity updated FIRST, then used for position.
                // Energy-conserving for conservative forces — preferred default.
                rb.velocity            += accel * dt;
                trans->m_worldPosition += rb.velocity * dt;
                break;

            case IntegrationMethod::RK4:
                // 4th-order Runge-Kutta. Matches the analytic formula
                // s = ut + 0.5*a*t² exactly for constant acceleration.
                IntegrateRK4(trans->m_worldPosition, rb.velocity, accel, dt);
                break;
            }

            // Linear damping — prevents slow energy build-up in spring chains.
            rb.velocity *= glm::pow(rb.linearDamping, dt);

            // --- 3. Clear linear force accumulator ---
            rb.forceAccum = glm::vec3(0.0f);

            // --- 4. Angular integration (PhysicsObject::Integrate() pattern) ---
            // Re-inject constant per-frame torque (Lab 6: drives spin-up demos via ini).
            rb.torqueAccum += rb.constantTorque;

            // Q5: refresh world-space inverse inertia tensor each frame.
            // I_world⁻¹ = R · I_body⁻¹ · R^T
            // Correct for non-isotropic bodies (cylinders, cuboids); identity for spheres.
            rb.invInertiaTensorWorld = rb.orientation * rb.invInertiaTensor * glm::transpose(rb.orientation);

            // α = I_world⁻¹ · τ_world  (Q3/Q5: object-space inertia correctly mapped to world)
            const glm::vec3 angularAccel = rb.invInertiaTensorWorld * rb.torqueAccum;
            rb.angularVelocity += angularAccel * dt;

            // dR/dt = Skew(ω) · R  →  R_new = R + dt · Skew(ω) · R
            if (glm::dot(rb.angularVelocity, rb.angularVelocity) > 1e-12f) {
                rb.orientation = rb.orientation + dt * SkewSymmetric(rb.angularVelocity) * rb.orientation;
                Orthogonalise(rb.orientation);  // prevent floating-point drift
            }

            // Clear torque accumulator for next frame
            rb.torqueAccum = glm::vec3(0.0f);

            // --- 4b. Angular displacement (Lab 5 Q1) ---
            // Rotates from current orientation by a finite target angle then stops.
            // angularDisplacementVec encodes axis * totalAngle (radians).
            {
                const float dispTotal = glm::length(rb.angularDisplacementVec);
                if (dispTotal > 1e-6f && rb.angularDisplacementApplied < dispTotal) {
                    const float remaining = dispTotal - rb.angularDisplacementApplied;
                    const float step      = glm::min(rb.angularDisplacementSpeed * dt, remaining);
                    const glm::vec3 axis  = rb.angularDisplacementVec / dispTotal;
                    const glm::mat3 R     = glm::mat3(glm::rotate(glm::mat4(1.0f), step, axis));
                    rb.orientation = R * rb.orientation;
                    Orthogonalise(rb.orientation);
                    rb.angularDisplacementApplied += step;
                }
            }

            // --- 5. Write-back: world position → local, then build local matrix ---
            // Bypasses TransformSystem's Euler-angle reconstruction so that the
            // orientation matrix (not Euler angles) drives the render transform.
            // Setting state = Clean prevents TransformSystem pass 1 from overwriting it.
            SyncWorldToLocal(*trans, em);
            const glm::mat4 tMat = glm::translate(glm::mat4(1.0f), trans->m_localPosition);
            const glm::mat4 rMat = glm::mat4(rb.orientation);
            const glm::mat4 sMat = glm::scale(glm::mat4(1.0f), trans->m_localScale);
            trans->m_localMatrix = tMat * rMat * sMat;
            trans->m_state = GE::Components::Transform::TransformState::Clean;
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
        // Clear collision event sets — ScriptSystem reads these after this call.
        m_currentContacts.clear();
        m_currentContactInfos.clear();
        m_currentTriggers.clear();

        auto* em = ServiceLocator::GetEntityManager();
        auto& sphereArray = em->GetCompArr<GE::Components::SphereCollider>();
        auto& planeArray  = em->GetCompArr<GE::Components::PlaneCollider>();
        auto& boxArray    = em->GetCompArr<GE::Components::BoxCollider>();

        // Returns true if testPoint is within the bounded region of pCol.
        // Always true when sizeX or sizeZ is 0 (plane is infinite).
        const auto isInsidePlaneBounds = [](
            const GE::Components::PlaneCollider& pCol,
            const glm::vec3& testPoint,
            const GE::Components::Transform* pTrans
        ) -> bool {
            if (pCol.sizeX <= 0.0f || pCol.sizeZ <= 0.0f) return true;
            if (pTrans == nullptr) return true;
            const glm::vec3 n = glm::normalize(pCol.normal);
            const glm::vec3 worldUp = (glm::abs(glm::dot(glm::vec3(0,1,0), n)) > 0.999f)
                ? glm::vec3(1,0,0) : glm::vec3(0,1,0);
            const glm::vec3 right   = glm::normalize(glm::cross(worldUp, n));
            const glm::vec3 forward = glm::normalize(glm::cross(n, right));
            const glm::vec3 toPoint = testPoint - glm::vec3(pTrans->m_worldMatrix[3]);
            return (glm::abs(glm::dot(toPoint, right))   <= pCol.sizeX * 0.5f &&
                    glm::abs(glm::dot(toPoint, forward)) <= pCol.sizeZ * 0.5f);
        };

        // -----------------------------------------------------------------
        // Pass A: Sphere-Plane collisions
        // -----------------------------------------------------------------
        for (uint32_t sIdx = 0; sIdx < sphereArray.GetCount(); ++sIdx) {
            const auto  sID   = sphereArray.Index()[sIdx];
            auto&       sCol  = sphereArray.Data()[sIdx];
            auto* const sTrans = em->TryGetTIComponent<GE::Components::Transform>(sID);
            auto* const sRB   = em->TryGetTIComponent<GE::Components::RigidBody>(sID);

            if (!sTrans || (sRB && sRB->isStatic)) continue;

            for (uint32_t pIdx = 0; pIdx < planeArray.GetCount(); ++pIdx) {
                const auto& pCol = planeArray.Data()[pIdx];
                const auto  pID  = planeArray.Index()[pIdx];

                // Skip if sphere centre is outside the plane's bounded extent (sizeX/sizeZ == 0 → infinite).
                const auto* pTrBounds = em->TryGetTIComponent<GE::Components::Transform>(pID);
                if (!isInsidePlaneBounds(pCol, sTrans->m_worldPosition, pTrBounds)) continue;

                GE::Physics::Sphere sphere(sTrans->m_worldPosition, sCol.radius);
                GE::Physics::Plane  plane(pCol.normal * pCol.offset, pCol.normal);

                const float dist = plane.DistanceToPoint(sphere.GetCenter());

                if (dist < sphere.GetRadius()) {
                    if (sCol.isTrigger) {
                        m_currentTriggers.insert({ std::min(sID, pID), std::max(sID, pID) });
                        continue;
                    }

                    const float penetration = sphere.GetRadius() - dist;

                    // 1. Positional correction — push out of the plane
                    sTrans->m_worldPosition += plane.GetNormal() * penetration;
                    SyncWorldToLocal(*sTrans, em);

                    // 2. Velocity reflection with restitution (Q5: override if active)
                    if (sRB) {
                        float e = sRB->restitution;
                        if (m_restitutionOverride >= 0.0f) {
                            e = m_restitutionOverride;
                        } else if (m_registry) {
                            const auto* matS = em->TryGetTIComponent<GE::Components::PhysicsMaterialTag>(sID);
                            const auto* matP = em->TryGetTIComponent<GE::Components::PhysicsMaterialTag>(pID);
                            if (matS && matP) {
                                GE::Physics::MaterialInteractionRecord rec;
                                if (m_registry->Lookup(matS->name, matP->name, rec)) { e = rec.restitution; }
                            }
                        }

                        // Kinematic animated plane: compute its velocity from prevPosition
                        glm::vec3 planeVel{ 0.0f };
                        const auto* pTrans = em->TryGetTIComponent<GE::Components::Transform>(pID);
                        const auto* aoc   = em->TryGetTIComponent<GE::Components::AnimatedObjectComponent>(pID);
                        if (aoc && pTrans && m_lastDt > 1e-6f) {
                            planeVel = (pTrans->m_worldPosition - aoc->prevPosition) / m_lastDt;
                        }

                        const glm::vec3& pn = plane.GetNormal();
                        // Relative velocity of sphere w.r.t. plane surface
                        const glm::vec3 relVel = sRB->velocity - planeVel;
                        const float     vRelN  = glm::dot(relVel, pn);
                        // Only resolve if approaching
                        if (vRelN < 0.0f) {
                            sRB->velocity -= (1.0f + e) * vRelN * pn;
                        }

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
            auto* const aTrans = em->TryGetTIComponent<GE::Components::Transform>(aID);
            auto* const aRB    = em->TryGetTIComponent<GE::Components::RigidBody>(aID);

            if (!aTrans) continue;

            for (uint32_t bIdx = aIdx + 1; bIdx < sphereArray.GetCount(); ++bIdx) {
                const auto  bID    = sphereArray.Index()[bIdx];
                auto&       bCol   = sphereArray.Data()[bIdx];
                auto* const bTrans = em->TryGetTIComponent<GE::Components::Transform>(bID);
                auto* const bRB    = em->TryGetTIComponent<GE::Components::RigidBody>(bID);

                if (!bTrans) continue;
                // Skip pairs where both are static
                if (aRB && aRB->isStatic && bRB && bRB->isStatic) continue;

                const glm::vec3 diff      = aTrans->m_worldPosition - bTrans->m_worldPosition;
                const float     dist      = glm::length(diff);
                const float     radiusSum = aCol.radius + bCol.radius;

                if (dist >= radiusSum || dist < 1e-6f) continue;

                const glm::vec3 n           = diff / dist;             // Unit normal A←B
                const float     penetration = radiusSum - dist;

                // --- Trigger check ---
                // If either collider is a trigger, record the overlap and skip physics resolution.
                if (aCol.isTrigger || bCol.isTrigger) {
                    GE::Scripts::EntityPair pair{ std::min(aID, bID), std::max(aID, bID) };
                    m_currentTriggers.insert(pair);
                    continue;
                }

                // --- Solid contact: record for ScriptSystem collision callbacks ---
                {
                    GE::Scripts::EntityPair pair{ std::min(aID, bID), std::max(aID, bID) };
                    GE::Scripts::CollisionInfo info;
                    info.otherEntity  = bID;   // from A's perspective; ScriptSystem mirrors for B
                    info.contactPoint = aTrans->m_worldPosition + (-n) * aCol.radius;
                    info.normal       = n;
                    info.penetration  = penetration;
                    m_currentContacts.insert(pair);
                    m_currentContactInfos[pair] = info;
                }

                // Animated objects are kinematic: infinite effective mass, velocity from prevPosition
                const auto* aAOC = em->TryGetTIComponent<GE::Components::AnimatedObjectComponent>(aID);
                const auto* bAOC = em->TryGetTIComponent<GE::Components::AnimatedObjectComponent>(bID);
                const bool aIsAnimated = (aAOC != nullptr);
                const bool bIsAnimated = (bAOC != nullptr);

                const float invMassA = (aRB && !aRB->isStatic && !aIsAnimated) ? aRB->inverseMass : 0.0f;
                const float invMassB = (bRB && !bRB->isStatic && !bIsAnimated) ? bRB->inverseMass : 0.0f;
                const float totalInvMass = invMassA + invMassB;

                // 1. Positional correction — proportional to inverse mass
                if (totalInvMass > 0.0f) {
                    const glm::vec3 correction = (penetration / totalInvMass) * n;
                    if (aRB && !aRB->isStatic && !aIsAnimated) {
                        aTrans->m_worldPosition += correction * invMassA;
                        SyncWorldToLocal(*aTrans, em);
                    }
                    if (bRB && !bRB->isStatic && !bIsAnimated) {
                        bTrans->m_worldPosition -= correction * invMassB;
                        SyncWorldToLocal(*bTrans, em);
                    }
                }

                // 2. Impulse response — general formula:  j = -(1+e)*vRel / (1/mA + 1/mB)
                // Animated spheres contribute their kinematic velocity to the relative velocity calc
                // but do NOT receive an impulse (invMass is 0 for them).
                // Q5: restitutionOverride replaces per-body values when active.
                // Q4: force-based mode converts impulse J to force F=J/dt for next Integrate().
                if (totalInvMass > 0.0f) {
                    // Determine effective velocities for relative velocity calculation
                    glm::vec3 velA = aRB ? aRB->velocity : glm::vec3{ 0.0f };
                    glm::vec3 velB = bRB ? bRB->velocity : glm::vec3{ 0.0f };
                    if (aIsAnimated && m_lastDt > 1e-6f) {
                        velA = (aTrans->m_worldPosition - aAOC->prevPosition) / m_lastDt;
                    }
                    if (bIsAnimated && m_lastDt > 1e-6f) {
                        velB = (bTrans->m_worldPosition - bAOC->prevPosition) / m_lastDt;
                    }

                    const float vRel = glm::dot(velA - velB, n);
                    if (vRel < 0.0f) {  // Only resolve if objects are approaching
                        float e = glm::min(
                            aRB ? aRB->restitution : 0.6f,
                            bRB ? bRB->restitution : 0.6f);
                        if (m_restitutionOverride >= 0.0f) {
                            e = m_restitutionOverride;
                        } else if (m_registry) {
                            const auto* matA = em->TryGetTIComponent<GE::Components::PhysicsMaterialTag>(aID);
                            const auto* matB = em->TryGetTIComponent<GE::Components::PhysicsMaterialTag>(bID);
                            if (matA && matB) {
                                GE::Physics::MaterialInteractionRecord rec;
                                if (m_registry->Lookup(matA->name, matB->name, rec)) { e = rec.restitution; }
                            }
                        }
                        const float j = -(1.0f + e) * vRel / totalInvMass;

                        if (m_useForceBasedImpulse && m_lastDt > 1e-6f) {
                            // Q4: convert impulse to force F = J/dt; applied on the next Integrate() call.
                            if (aRB && !aRB->isStatic && !aIsAnimated) aRB->forceAccum += (j * invMassA / m_lastDt) * n;
                            if (bRB && !bRB->isStatic && !bIsAnimated) bRB->forceAccum -= (j * invMassB / m_lastDt) * n;
                        } else {
                            // Default: apply velocity change directly this frame.
                            if (aRB && !aRB->isStatic && !aIsAnimated) aRB->velocity += j * invMassA * n;
                            if (bRB && !bRB->isStatic && !bIsAnimated) bRB->velocity -= j * invMassB * n;
                        }
                    }
                }
            }
        }

        // -----------------------------------------------------------------
        // Pass C: Sphere-Box (AABB) collisions — enables animated cuboid
        // platforms to collide with and transfer momentum to spheres.
        // -----------------------------------------------------------------
        for (uint32_t sIdx = 0; sIdx < sphereArray.GetCount(); ++sIdx) {
            const auto  sID    = sphereArray.Index()[sIdx];
            auto&       sCol   = sphereArray.Data()[sIdx];
            auto* const sTrans = em->TryGetTIComponent<GE::Components::Transform>(sID);
            auto* const sRB    = em->TryGetTIComponent<GE::Components::RigidBody>(sID);

            if (!sTrans || (sRB && sRB->isStatic)) continue;

            for (uint32_t bIdx = 0; bIdx < boxArray.GetCount(); ++bIdx) {
                const auto  bID    = boxArray.Index()[bIdx];
                const auto& bCol   = boxArray.Data()[bIdx];
                auto* const bTrans = em->TryGetTIComponent<GE::Components::Transform>(bID);

                if (!bTrans || sID == bID) continue;

                // Build AABB from box centre (world position) and half-extents
                const glm::vec3 halfExt{ bCol.sizeX * 0.5f, bCol.sizeY * 0.5f, bCol.sizeZ * 0.5f };
                const glm::vec3& boxCenter = bTrans->m_worldPosition;

                // Closest point on AABB to sphere centre
                const glm::vec3 closest{
                    glm::clamp(sTrans->m_worldPosition.x, boxCenter.x - halfExt.x, boxCenter.x + halfExt.x),
                    glm::clamp(sTrans->m_worldPosition.y, boxCenter.y - halfExt.y, boxCenter.y + halfExt.y),
                    glm::clamp(sTrans->m_worldPosition.z, boxCenter.z - halfExt.z, boxCenter.z + halfExt.z)
                };

                const glm::vec3 diff = sTrans->m_worldPosition - closest;
                const float distSq = glm::dot(diff, diff);

                if (distSq >= sCol.radius * sCol.radius || distSq < 1e-12f) continue;

                const float dist = std::sqrt(distSq);
                const glm::vec3 normal = diff / dist;  // Points from box toward sphere
                const float penetration = sCol.radius - dist;

                // Trigger check
                if (sCol.isTrigger || bCol.isTrigger) {
                    GE::Scripts::EntityPair pair{ std::min(sID, bID), std::max(sID, bID) };
                    m_currentTriggers.insert(pair);
                    continue;
                }

                // Contact event
                {
                    GE::Scripts::EntityPair pair{ std::min(sID, bID), std::max(sID, bID) };
                    GE::Scripts::CollisionInfo info;
                    info.otherEntity  = bID;
                    info.contactPoint = closest;
                    info.normal       = normal;
                    info.penetration  = penetration;
                    m_currentContacts.insert(pair);
                    m_currentContactInfos[pair] = info;
                }

                // Positional correction — push sphere out
                sTrans->m_worldPosition += normal * penetration;
                SyncWorldToLocal(*sTrans, em);

                // Impulse response
                if (sRB) {
                    const auto* bRB  = em->TryGetTIComponent<GE::Components::RigidBody>(bID);
                    const auto* bAOC = em->TryGetTIComponent<GE::Components::AnimatedObjectComponent>(bID);

                    // Box is treated as infinite mass (static or animated)
                    glm::vec3 boxVel{ 0.0f };
                    if (bAOC && m_lastDt > 1e-6f) {
                        boxVel = (bTrans->m_worldPosition - bAOC->prevPosition) / m_lastDt;
                    }

                    float e = sRB->restitution;
                    if (m_restitutionOverride >= 0.0f) {
                        e = m_restitutionOverride;
                    } else if (m_registry) {
                        const auto* matS = em->TryGetTIComponent<GE::Components::PhysicsMaterialTag>(sID);
                        const auto* matB = em->TryGetTIComponent<GE::Components::PhysicsMaterialTag>(bID);
                        if (matS && matB) {
                            GE::Physics::MaterialInteractionRecord rec;
                            if (m_registry->Lookup(matS->name, matB->name, rec)) { e = rec.restitution; }
                        }
                    }

                    const glm::vec3 relVel = sRB->velocity - boxVel;
                    const float vRelN = glm::dot(relVel, normal);
                    if (vRelN < 0.0f) {
                        sRB->velocity -= (1.0f + e) * vRelN * normal;
                    }

                    if (glm::length(sRB->velocity) < 0.05f) {
                        sRB->velocity = glm::vec3(0.0f);
                    }
                }
            }
        }

        // -----------------------------------------------------------------
        // Pass D: Box-Plane collisions — enables cuboid shapes to rest on
        // floors and bounce off plane surfaces.
        // -----------------------------------------------------------------
        auto& cylArray     = em->GetCompArr<GE::Components::CylinderCollider>();
        auto& capsuleArray = em->GetCompArr<GE::Components::CapsuleCollider>();

        for (uint32_t bIdx = 0; bIdx < boxArray.GetCount(); ++bIdx) {
            const auto  bID    = boxArray.Index()[bIdx];
            const auto& bCol   = boxArray.Data()[bIdx];
            auto* const bTrans = em->TryGetTIComponent<GE::Components::Transform>(bID);
            auto* const bRB    = em->TryGetTIComponent<GE::Components::RigidBody>(bID);

            if (!bTrans || !bRB || bRB->isStatic) continue;

            const glm::vec3 halfExt{ bCol.sizeX * 0.5f, bCol.sizeY * 0.5f, bCol.sizeZ * 0.5f };

            for (uint32_t pIdx = 0; pIdx < planeArray.GetCount(); ++pIdx) {
                const auto& pCol = planeArray.Data()[pIdx];
                const auto  pID  = planeArray.Index()[pIdx];

                const auto* pTrBounds = em->TryGetTIComponent<GE::Components::Transform>(pID);
                if (!isInsidePlaneBounds(pCol, bTrans->m_worldPosition, pTrBounds)) continue;

                GE::Physics::Plane plane(pCol.normal * pCol.offset, pCol.normal);
                const glm::vec3& pn = plane.GetNormal();

                // Find the corner with minimum signed distance to the plane.
                // The support point in the -normal direction gives deepest penetration.
                const glm::vec3 support{
                    (pn.x >= 0.0f) ? -halfExt.x : halfExt.x,
                    (pn.y >= 0.0f) ? -halfExt.y : halfExt.y,
                    (pn.z >= 0.0f) ? -halfExt.z : halfExt.z
                };
                const glm::vec3 deepestPoint = bTrans->m_worldPosition + support;
                // Signed distance: negative means deepestPoint has crossed the plane.
                const float dist = glm::dot(pn, deepestPoint - plane.GetPoint());

                if (dist < 0.0f) {
                    if (bCol.isTrigger) {
                        m_currentTriggers.insert({ std::min(bID, pID), std::max(bID, pID) });
                        continue;
                    }

                    const float penetration = -dist;
                    bTrans->m_worldPosition += pn * penetration;
                    SyncWorldToLocal(*bTrans, em);

                    float e = bRB->restitution;
                    if (m_restitutionOverride >= 0.0f) {
                        e = m_restitutionOverride;
                    } else if (m_registry) {
                        const auto* matB = em->TryGetTIComponent<GE::Components::PhysicsMaterialTag>(bID);
                        const auto* matP = em->TryGetTIComponent<GE::Components::PhysicsMaterialTag>(pID);
                        if (matB && matP) {
                            GE::Physics::MaterialInteractionRecord rec;
                            if (m_registry->Lookup(matB->name, matP->name, rec)) { e = rec.restitution; }
                        }
                    }

                    glm::vec3 planeVel{ 0.0f };
                    const auto* pTrans = em->TryGetTIComponent<GE::Components::Transform>(pID);
                    const auto* aoc    = em->TryGetTIComponent<GE::Components::AnimatedObjectComponent>(pID);
                    if (aoc && pTrans && m_lastDt > 1e-6f) {
                        planeVel = (pTrans->m_worldPosition - aoc->prevPosition) / m_lastDt;
                    }

                    const glm::vec3 relVel = bRB->velocity - planeVel;
                    const float     vRelN  = glm::dot(relVel, pn);
                    if (vRelN < 0.0f) {
                        bRB->velocity -= (1.0f + e) * vRelN * pn;
                    }

                    if (glm::length(bRB->velocity) < 0.05f) {
                        bRB->velocity = glm::vec3(0.0f);
                    }
                }
            }
        }

        // -----------------------------------------------------------------
        // Pass E: Capsule-Plane collisions — capsule = line segment + radius.
        // Find closest point on the capsule axis to the plane, then treat as
        // sphere of capsule radius at that point.
        // -----------------------------------------------------------------
        for (uint32_t cIdx = 0; cIdx < capsuleArray.GetCount(); ++cIdx) {
            const auto  cID    = capsuleArray.Index()[cIdx];
            const auto& cCol   = capsuleArray.Data()[cIdx];
            auto* const cTrans = em->TryGetTIComponent<GE::Components::Transform>(cID);
            auto* const cRB    = em->TryGetTIComponent<GE::Components::RigidBody>(cID);

            if (!cTrans || !cRB || cRB->isStatic) continue;

            // Capsule axis endpoints (Y-aligned)
            const float halfH = cCol.height * 0.5f;
            const glm::vec3 top    = cTrans->m_worldPosition + glm::vec3(0.0f, +halfH, 0.0f);
            const glm::vec3 bottom = cTrans->m_worldPosition + glm::vec3(0.0f, -halfH, 0.0f);

            for (uint32_t pIdx = 0; pIdx < planeArray.GetCount(); ++pIdx) {
                const auto& pCol = planeArray.Data()[pIdx];
                const auto  pID  = planeArray.Index()[pIdx];

                const auto* pTrBounds = em->TryGetTIComponent<GE::Components::Transform>(pID);
                if (!isInsidePlaneBounds(pCol, cTrans->m_worldPosition, pTrBounds)) continue;

                GE::Physics::Plane plane(pCol.normal * pCol.offset, pCol.normal);
                const glm::vec3& pn = plane.GetNormal();

                // Find the capsule axis endpoint closest to (most penetrating into) the plane
                const float distTop    = plane.DistanceToPoint(top);
                const float distBottom = plane.DistanceToPoint(bottom);
                const float minDist    = glm::min(distTop, distBottom);

                if (minDist < cCol.radius) {
                    if (cCol.isTrigger) {
                        m_currentTriggers.insert({ std::min(cID, pID), std::max(cID, pID) });
                        continue;
                    }

                    const float penetration = cCol.radius - minDist;
                    cTrans->m_worldPosition += pn * penetration;
                    SyncWorldToLocal(*cTrans, em);

                    float e = cRB->restitution;
                    if (m_restitutionOverride >= 0.0f) {
                        e = m_restitutionOverride;
                    } else if (m_registry) {
                        const auto* matC = em->TryGetTIComponent<GE::Components::PhysicsMaterialTag>(cID);
                        const auto* matP = em->TryGetTIComponent<GE::Components::PhysicsMaterialTag>(pID);
                        if (matC && matP) {
                            GE::Physics::MaterialInteractionRecord rec;
                            if (m_registry->Lookup(matC->name, matP->name, rec)) { e = rec.restitution; }
                        }
                    }

                    glm::vec3 planeVel{ 0.0f };
                    const auto* pTrans = em->TryGetTIComponent<GE::Components::Transform>(pID);
                    const auto* aoc    = em->TryGetTIComponent<GE::Components::AnimatedObjectComponent>(pID);
                    if (aoc && pTrans && m_lastDt > 1e-6f) {
                        planeVel = (pTrans->m_worldPosition - aoc->prevPosition) / m_lastDt;
                    }

                    const glm::vec3 relVel = cRB->velocity - planeVel;
                    const float     vRelN  = glm::dot(relVel, pn);
                    if (vRelN < 0.0f) {
                        cRB->velocity -= (1.0f + e) * vRelN * pn;
                    }

                    if (glm::length(cRB->velocity) < 0.05f) {
                        cRB->velocity = glm::vec3(0.0f);
                    }
                }
            }
        }

        // -----------------------------------------------------------------
        // Pass F: Cylinder-Plane collisions — approximated as capsule
        // (line segment axis + radius). Uses the same approach as Pass E.
        // -----------------------------------------------------------------
        for (uint32_t cIdx = 0; cIdx < cylArray.GetCount(); ++cIdx) {
            const auto  cID    = cylArray.Index()[cIdx];
            const auto& cCol   = cylArray.Data()[cIdx];
            auto* const cTrans = em->TryGetTIComponent<GE::Components::Transform>(cID);
            auto* const cRB    = em->TryGetTIComponent<GE::Components::RigidBody>(cID);

            if (!cTrans || !cRB || cRB->isStatic) continue;

            const float halfH = cCol.height * 0.5f;
            const glm::vec3 top    = cTrans->m_worldPosition + glm::vec3(0.0f, +halfH, 0.0f);
            const glm::vec3 bottom = cTrans->m_worldPosition + glm::vec3(0.0f, -halfH, 0.0f);

            for (uint32_t pIdx = 0; pIdx < planeArray.GetCount(); ++pIdx) {
                const auto& pCol = planeArray.Data()[pIdx];
                const auto  pID  = planeArray.Index()[pIdx];

                const auto* pTrBounds = em->TryGetTIComponent<GE::Components::Transform>(pID);
                if (!isInsidePlaneBounds(pCol, cTrans->m_worldPosition, pTrBounds)) continue;

                GE::Physics::Plane plane(pCol.normal * pCol.offset, pCol.normal);
                const glm::vec3& pn = plane.GetNormal();

                const float distTop    = plane.DistanceToPoint(top);
                const float distBottom = plane.DistanceToPoint(bottom);
                const float minDist    = glm::min(distTop, distBottom);

                if (minDist < cCol.radius) {
                    if (cCol.isTrigger) {
                        m_currentTriggers.insert({ std::min(cID, pID), std::max(cID, pID) });
                        continue;
                    }

                    const float penetration = cCol.radius - minDist;
                    cTrans->m_worldPosition += pn * penetration;
                    SyncWorldToLocal(*cTrans, em);

                    float e = cRB->restitution;
                    if (m_restitutionOverride >= 0.0f) {
                        e = m_restitutionOverride;
                    } else if (m_registry) {
                        const auto* matC = em->TryGetTIComponent<GE::Components::PhysicsMaterialTag>(cID);
                        const auto* matP = em->TryGetTIComponent<GE::Components::PhysicsMaterialTag>(pID);
                        if (matC && matP) {
                            GE::Physics::MaterialInteractionRecord rec;
                            if (m_registry->Lookup(matC->name, matP->name, rec)) { e = rec.restitution; }
                        }
                    }

                    glm::vec3 planeVel{ 0.0f };
                    const auto* pTrans = em->TryGetTIComponent<GE::Components::Transform>(pID);
                    const auto* aoc    = em->TryGetTIComponent<GE::Components::AnimatedObjectComponent>(pID);
                    if (aoc && pTrans && m_lastDt > 1e-6f) {
                        planeVel = (pTrans->m_worldPosition - aoc->prevPosition) / m_lastDt;
                    }

                    const glm::vec3 relVel = cRB->velocity - planeVel;
                    const float     vRelN  = glm::dot(relVel, pn);
                    if (vRelN < 0.0f) {
                        cRB->velocity -= (1.0f + e) * vRelN * pn;
                    }

                    if (glm::length(cRB->velocity) < 0.05f) {
                        cRB->velocity = glm::vec3(0.0f);
                    }
                }
            }
        }

        // -----------------------------------------------------------------
        // Pass G: Capsule-Box collisions.
        // Find the closest point on the capsule axis segment to the box
        // centre, then run a sphere-box test at that point with capsule radius.
        // Box is treated as infinite mass (same convention as Pass C).
        // -----------------------------------------------------------------
        for (uint32_t cIdx = 0; cIdx < capsuleArray.GetCount(); ++cIdx) {
            const auto  cID    = capsuleArray.Index()[cIdx];
            const auto& cCol   = capsuleArray.Data()[cIdx];
            auto* const cTrans = em->TryGetTIComponent<GE::Components::Transform>(cID);
            auto* const cRB    = em->TryGetTIComponent<GE::Components::RigidBody>(cID);

            if (!cTrans || !cRB || cRB->isStatic) continue;

            const float halfH    = cCol.height * 0.5f;
            const glm::vec3 capBot = cTrans->m_worldPosition - glm::vec3(0.0f, halfH, 0.0f);
            const glm::vec3 axis   = glm::vec3(0.0f, cCol.height, 0.0f); // capTop - capBot
            const float axisLenSq  = cCol.height * cCol.height;

            for (uint32_t bIdx = 0; bIdx < boxArray.GetCount(); ++bIdx) {
                const auto  bID    = boxArray.Index()[bIdx];
                const auto& bCol   = boxArray.Data()[bIdx];
                auto* const bTrans = em->TryGetTIComponent<GE::Components::Transform>(bID);

                if (!bTrans || cID == bID) continue;

                const glm::vec3 halfExt{ bCol.sizeX * 0.5f, bCol.sizeY * 0.5f, bCol.sizeZ * 0.5f };
                const glm::vec3& boxCenter = bTrans->m_worldPosition;

                // Closest point on capsule segment to box centre
                const float t = glm::clamp(glm::dot(boxCenter - capBot, axis) / axisLenSq, 0.0f, 1.0f);
                const glm::vec3 closestOnAxis = capBot + t * axis;

                // Closest point on AABB to that segment point
                const glm::vec3 closest{
                    glm::clamp(closestOnAxis.x, boxCenter.x - halfExt.x, boxCenter.x + halfExt.x),
                    glm::clamp(closestOnAxis.y, boxCenter.y - halfExt.y, boxCenter.y + halfExt.y),
                    glm::clamp(closestOnAxis.z, boxCenter.z - halfExt.z, boxCenter.z + halfExt.z)
                };

                const glm::vec3 diff   = closestOnAxis - closest;
                const float     distSq = glm::dot(diff, diff);

                if (distSq >= cCol.radius * cCol.radius || distSq < 1e-12f) continue;

                if (cCol.isTrigger || bCol.isTrigger) {
                    m_currentTriggers.insert({ std::min(cID, bID), std::max(cID, bID) });
                    continue;
                }

                const float     dist        = std::sqrt(distSq);
                const glm::vec3 normal      = diff / dist;   // box → capsule
                const float     penetration = cCol.radius - dist;

                cTrans->m_worldPosition += normal * penetration;
                SyncWorldToLocal(*cTrans, em);

                float e = cRB->restitution;
                if (m_restitutionOverride >= 0.0f) {
                    e = m_restitutionOverride;
                } else if (m_registry) {
                    const auto* matC = em->TryGetTIComponent<GE::Components::PhysicsMaterialTag>(cID);
                    const auto* matB = em->TryGetTIComponent<GE::Components::PhysicsMaterialTag>(bID);
                    if (matC && matB) {
                        GE::Physics::MaterialInteractionRecord rec;
                        if (m_registry->Lookup(matC->name, matB->name, rec)) { e = rec.restitution; }
                    }
                }

                const auto* bAOC = em->TryGetTIComponent<GE::Components::AnimatedObjectComponent>(bID);
                glm::vec3 boxVel{ 0.0f };
                if (bAOC && m_lastDt > 1e-6f) {
                    boxVel = (bTrans->m_worldPosition - bAOC->prevPosition) / m_lastDt;
                }

                const glm::vec3 relVel = cRB->velocity - boxVel;
                const float     vRelN  = glm::dot(relVel, normal);
                if (vRelN < 0.0f) {
                    cRB->velocity -= (1.0f + e) * vRelN * normal;
                }

                if (glm::length(cRB->velocity) < 0.05f) {
                    cRB->velocity = glm::vec3(0.0f);
                }
            }
        }

        // -----------------------------------------------------------------
        // Pass H: Sphere-Capsule collisions.
        // Find the closest point on the capsule axis to the sphere centre,
        // then treat as sphere-sphere with the capsule radius.
        // Both bodies receive impulses (general two-body, like Pass B).
        // -----------------------------------------------------------------
        for (uint32_t sIdx = 0; sIdx < sphereArray.GetCount(); ++sIdx) {
            const auto  sID    = sphereArray.Index()[sIdx];
            auto&       sCol   = sphereArray.Data()[sIdx];
            auto* const sTrans = em->TryGetTIComponent<GE::Components::Transform>(sID);
            auto* const sRB    = em->TryGetTIComponent<GE::Components::RigidBody>(sID);

            if (!sTrans || (sRB && sRB->isStatic)) continue;

            for (uint32_t cIdx = 0; cIdx < capsuleArray.GetCount(); ++cIdx) {
                const auto  cID    = capsuleArray.Index()[cIdx];
                const auto& cCol   = capsuleArray.Data()[cIdx];
                auto* const cTrans = em->TryGetTIComponent<GE::Components::Transform>(cID);
                auto* const cRB    = em->TryGetTIComponent<GE::Components::RigidBody>(cID);

                if (!cTrans || sID == cID) continue;

                const float halfH      = cCol.height * 0.5f;
                const glm::vec3 capBot = cTrans->m_worldPosition - glm::vec3(0.0f, halfH, 0.0f);
                const glm::vec3 axis   = glm::vec3(0.0f, cCol.height, 0.0f);
                const float axisLenSq  = cCol.height * cCol.height;

                // Closest point on capsule axis to sphere centre
                const float t = glm::clamp(glm::dot(sTrans->m_worldPosition - capBot, axis) / axisLenSq, 0.0f, 1.0f);
                const glm::vec3 closestOnAxis = capBot + t * axis;

                const glm::vec3 diff      = sTrans->m_worldPosition - closestOnAxis;
                const float     distSq    = glm::dot(diff, diff);
                const float     sumRadius = sCol.radius + cCol.radius;

                if (distSq >= sumRadius * sumRadius || distSq < 1e-12f) continue;

                if (sCol.isTrigger || cCol.isTrigger) {
                    m_currentTriggers.insert({ std::min(sID, cID), std::max(sID, cID) });
                    continue;
                }

                const float     dist        = std::sqrt(distSq);
                const glm::vec3 normal      = diff / dist;   // capsule → sphere
                const float     penetration = sumRadius - dist;

                const float invMassS     = (sRB && !sRB->isStatic) ? sRB->inverseMass : 0.0f;
                const float invMassC     = (cRB && !cRB->isStatic) ? cRB->inverseMass : 0.0f;
                const float totalInvMass = invMassS + invMassC;

                if (totalInvMass > 0.0f) {
                    const glm::vec3 correction = (penetration / totalInvMass) * normal;
                    if (sRB && !sRB->isStatic) {
                        sTrans->m_worldPosition += correction * invMassS;
                        SyncWorldToLocal(*sTrans, em);
                    }
                    if (cRB && !cRB->isStatic) {
                        cTrans->m_worldPosition -= correction * invMassC;
                        SyncWorldToLocal(*cTrans, em);
                    }
                }

                if (totalInvMass > 0.0f) {
                    float e = 0.6f;
                    if (m_restitutionOverride >= 0.0f) {
                        e = m_restitutionOverride;
                    } else if (m_registry) {
                        const auto* matS = em->TryGetTIComponent<GE::Components::PhysicsMaterialTag>(sID);
                        const auto* matC = em->TryGetTIComponent<GE::Components::PhysicsMaterialTag>(cID);
                        if (matS && matC) {
                            GE::Physics::MaterialInteractionRecord rec;
                            if (m_registry->Lookup(matS->name, matC->name, rec)) { e = rec.restitution; }
                        }
                    }

                    const glm::vec3 velS = sRB ? sRB->velocity : glm::vec3{ 0.0f };
                    const glm::vec3 velC = cRB ? cRB->velocity : glm::vec3{ 0.0f };
                    const float     vRel = glm::dot(velS - velC, normal);
                    if (vRel < 0.0f) {
                        const float j = -(1.0f + e) * vRel / totalInvMass;
                        if (sRB && !sRB->isStatic) sRB->velocity += j * invMassS * normal;
                        if (cRB && !cRB->isStatic) cRB->velocity -= j * invMassC * normal;
                    }
                }
            }
        }

        // -----------------------------------------------------------------
        // Pass I: Box-Box (AABB) collisions — SAT on the three world axes.
        // Both bodies may be dynamic; static boxes act as infinite mass.
        // -----------------------------------------------------------------
        for (uint32_t aIdx = 0; aIdx < boxArray.GetCount(); ++aIdx) {
            const auto  aID    = boxArray.Index()[aIdx];
            const auto& aCol   = boxArray.Data()[aIdx];
            auto* const aTrans = em->TryGetTIComponent<GE::Components::Transform>(aID);
            auto* const aRB    = em->TryGetTIComponent<GE::Components::RigidBody>(aID);

            if (!aTrans) continue;
            const bool aStatic = (!aRB || aRB->isStatic);

            const glm::vec3 hA{ aCol.sizeX * 0.5f, aCol.sizeY * 0.5f, aCol.sizeZ * 0.5f };

            for (uint32_t bIdx = aIdx + 1; bIdx < boxArray.GetCount(); ++bIdx) {
                const auto  bID    = boxArray.Index()[bIdx];
                const auto& bCol   = boxArray.Data()[bIdx];
                auto* const bTrans = em->TryGetTIComponent<GE::Components::Transform>(bID);
                auto* const bRB    = em->TryGetTIComponent<GE::Components::RigidBody>(bID);

                if (!bTrans) continue;
                const bool bStatic = (!bRB || bRB->isStatic);
                if (aStatic && bStatic) continue;

                const glm::vec3 hB{ bCol.sizeX * 0.5f, bCol.sizeY * 0.5f, bCol.sizeZ * 0.5f };
                const glm::vec3 delta = bTrans->m_worldPosition - aTrans->m_worldPosition;

                const float ox = hA.x + hB.x - std::abs(delta.x);
                const float oy = hA.y + hB.y - std::abs(delta.y);
                const float oz = hA.z + hB.z - std::abs(delta.z);

                if (ox <= 0.0f || oy <= 0.0f || oz <= 0.0f) continue;

                if (aCol.isTrigger || bCol.isTrigger) {
                    m_currentTriggers.insert({ std::min(aID, bID), std::max(aID, bID) });
                    continue;
                }

                // Minimum overlap axis
                glm::vec3 normal;
                float penetration;
                if (ox <= oy && ox <= oz) {
                    penetration = ox;
                    normal = glm::vec3(delta.x < 0.0f ? -1.0f : 1.0f, 0.0f, 0.0f);
                } else if (oy <= ox && oy <= oz) {
                    penetration = oy;
                    normal = glm::vec3(0.0f, delta.y < 0.0f ? -1.0f : 1.0f, 0.0f);
                } else {
                    penetration = oz;
                    normal = glm::vec3(0.0f, 0.0f, delta.z < 0.0f ? -1.0f : 1.0f);
                }

                const float invMassA     = aStatic ? 0.0f : aRB->inverseMass;
                const float invMassB     = bStatic ? 0.0f : bRB->inverseMass;
                const float totalInvMass = invMassA + invMassB;
                if (totalInvMass <= 0.0f) continue;

                // Positional correction
                const glm::vec3 correction = (penetration / totalInvMass) * normal;
                if (!aStatic) {
                    aTrans->m_worldPosition -= correction * invMassA;
                    SyncWorldToLocal(*aTrans, em);
                }
                if (!bStatic) {
                    bTrans->m_worldPosition += correction * invMassB;
                    SyncWorldToLocal(*bTrans, em);
                }

                // Impulse response
                float e = 0.3f;
                if (m_restitutionOverride >= 0.0f) {
                    e = m_restitutionOverride;
                } else if (m_registry) {
                    const auto* matA = em->TryGetTIComponent<GE::Components::PhysicsMaterialTag>(aID);
                    const auto* matB = em->TryGetTIComponent<GE::Components::PhysicsMaterialTag>(bID);
                    if (matA && matB) {
                        GE::Physics::MaterialInteractionRecord rec;
                        if (m_registry->Lookup(matA->name, matB->name, rec)) { e = rec.restitution; }
                    }
                }

                const glm::vec3 velA = aRB ? aRB->velocity : glm::vec3{ 0.0f };
                const glm::vec3 velB = bRB ? bRB->velocity : glm::vec3{ 0.0f };
                const float     vRel = glm::dot(velB - velA, normal);
                if (vRel < 0.0f) {
                    const float j = -(1.0f + e) * vRel / totalInvMass;
                    if (!aStatic) aRB->velocity -= j * invMassA * normal;
                    if (!bStatic) bRB->velocity += j * invMassB * normal;
                }
            }
        }

        // -----------------------------------------------------------------
        // Pass J: Cylinder-Box collisions.
        // Same approximation as Capsule-Box (Pass G): find the closest point
        // on the cylinder axis segment to the box centre, then run a
        // sphere-box test at that point using the cylinder radius.
        // -----------------------------------------------------------------
        for (uint32_t cIdx = 0; cIdx < cylArray.GetCount(); ++cIdx) {
            const auto  cID    = cylArray.Index()[cIdx];
            const auto& cCol   = cylArray.Data()[cIdx];
            auto* const cTrans = em->TryGetTIComponent<GE::Components::Transform>(cID);
            auto* const cRB    = em->TryGetTIComponent<GE::Components::RigidBody>(cID);

            if (!cTrans || !cRB || cRB->isStatic) continue;

            const float halfH     = cCol.height * 0.5f;
            const glm::vec3 cylBot = cTrans->m_worldPosition - glm::vec3(0.0f, halfH, 0.0f);
            const glm::vec3 axis   = glm::vec3(0.0f, cCol.height, 0.0f);
            const float axisLenSq  = cCol.height * cCol.height;

            for (uint32_t bIdx = 0; bIdx < boxArray.GetCount(); ++bIdx) {
                const auto  bID    = boxArray.Index()[bIdx];
                const auto& bCol   = boxArray.Data()[bIdx];
                auto* const bTrans = em->TryGetTIComponent<GE::Components::Transform>(bID);

                if (!bTrans || cID == bID) continue;

                const glm::vec3 halfExt{ bCol.sizeX * 0.5f, bCol.sizeY * 0.5f, bCol.sizeZ * 0.5f };
                const glm::vec3& boxCenter = bTrans->m_worldPosition;

                const float t = glm::clamp(glm::dot(boxCenter - cylBot, axis) / axisLenSq, 0.0f, 1.0f);
                const glm::vec3 closestOnAxis = cylBot + t * axis;

                const glm::vec3 closest{
                    glm::clamp(closestOnAxis.x, boxCenter.x - halfExt.x, boxCenter.x + halfExt.x),
                    glm::clamp(closestOnAxis.y, boxCenter.y - halfExt.y, boxCenter.y + halfExt.y),
                    glm::clamp(closestOnAxis.z, boxCenter.z - halfExt.z, boxCenter.z + halfExt.z)
                };

                const glm::vec3 diff   = closestOnAxis - closest;
                const float     distSq = glm::dot(diff, diff);

                if (distSq >= cCol.radius * cCol.radius || distSq < 1e-12f) continue;

                const float     dist        = std::sqrt(distSq);
                if (cCol.isTrigger || bCol.isTrigger) {
                    m_currentTriggers.insert({ std::min(cID, bID), std::max(cID, bID) });
                    continue;
                }

                const glm::vec3 normal      = diff / dist;
                const float     penetration = cCol.radius - dist;

                cTrans->m_worldPosition += normal * penetration;
                SyncWorldToLocal(*cTrans, em);

                float e = cRB->restitution;
                if (m_restitutionOverride >= 0.0f) {
                    e = m_restitutionOverride;
                } else if (m_registry) {
                    const auto* matC = em->TryGetTIComponent<GE::Components::PhysicsMaterialTag>(cID);
                    const auto* matB = em->TryGetTIComponent<GE::Components::PhysicsMaterialTag>(bID);
                    if (matC && matB) {
                        GE::Physics::MaterialInteractionRecord rec;
                        if (m_registry->Lookup(matC->name, matB->name, rec)) { e = rec.restitution; }
                    }
                }

                const auto* bAOC = em->TryGetTIComponent<GE::Components::AnimatedObjectComponent>(bID);
                glm::vec3 boxVel{ 0.0f };
                if (bAOC && m_lastDt > 1e-6f) {
                    boxVel = (bTrans->m_worldPosition - bAOC->prevPosition) / m_lastDt;
                }

                const glm::vec3 relVel = cRB->velocity - boxVel;
                const float     vRelN  = glm::dot(relVel, normal);
                if (vRelN < 0.0f) {
                    cRB->velocity -= (1.0f + e) * vRelN * normal;
                }

                if (glm::length(cRB->velocity) < 0.05f) {
                    cRB->velocity = glm::vec3(0.0f);
                }
            }
        }

        // -----------------------------------------------------------------
        // Pass K: Cylinder-Sphere collisions.
        // Find the closest point on the cylinder axis to the sphere centre,
        // then treat as sphere-sphere with the cylinder radius.
        // -----------------------------------------------------------------
        for (uint32_t cIdx = 0; cIdx < cylArray.GetCount(); ++cIdx) {
            const auto  cID    = cylArray.Index()[cIdx];
            const auto& cCol   = cylArray.Data()[cIdx];
            auto* const cTrans = em->TryGetTIComponent<GE::Components::Transform>(cID);
            auto* const cRB    = em->TryGetTIComponent<GE::Components::RigidBody>(cID);

            if (!cTrans || !cRB || cRB->isStatic) continue;

            const float halfH     = cCol.height * 0.5f;
            const glm::vec3 cylBot = cTrans->m_worldPosition - glm::vec3(0.0f, halfH, 0.0f);
            const glm::vec3 axis   = glm::vec3(0.0f, cCol.height, 0.0f);
            const float axisLenSq  = cCol.height * cCol.height;

            for (uint32_t sIdx = 0; sIdx < sphereArray.GetCount(); ++sIdx) {
                const auto  sID    = sphereArray.Index()[sIdx];
                const auto& sCol   = sphereArray.Data()[sIdx];
                auto* const sTrans = em->TryGetTIComponent<GE::Components::Transform>(sID);
                auto* const sRB    = em->TryGetTIComponent<GE::Components::RigidBody>(sID);

                if (!sTrans || cID == sID) continue;

                const float t = glm::clamp(glm::dot(sTrans->m_worldPosition - cylBot, axis) / axisLenSq, 0.0f, 1.0f);
                const glm::vec3 closestOnAxis = cylBot + t * axis;

                const glm::vec3 diff   = sTrans->m_worldPosition - closestOnAxis;
                const float     distSq = glm::dot(diff, diff);
                const float     sumR   = cCol.radius + sCol.radius;

                if (distSq >= sumR * sumR || distSq < 1e-12f) continue;

                if (cCol.isTrigger || sCol.isTrigger) {
                    m_currentTriggers.insert({ std::min(cID, sID), std::max(cID, sID) });
                    continue;
                }

                const float     dist        = std::sqrt(distSq);
                const glm::vec3 normal      = diff / dist;
                const float     penetration = sumR - dist;

                const bool sStatic = (!sRB || sRB->isStatic);
                const float invMassC     = cRB->inverseMass;
                const float invMassS     = sStatic ? 0.0f : sRB->inverseMass;
                const float totalInvMass = invMassC + invMassS;
                if (totalInvMass <= 0.0f) continue;

                const glm::vec3 correction = (penetration / totalInvMass) * normal;
                cTrans->m_worldPosition -= correction * invMassC;
                SyncWorldToLocal(*cTrans, em);
                if (!sStatic) {
                    sTrans->m_worldPosition += correction * invMassS;
                    SyncWorldToLocal(*sTrans, em);
                }

                float e = cRB->restitution;
                if (!sStatic) e = glm::min(e, sRB->restitution);
                if (m_restitutionOverride >= 0.0f) { e = m_restitutionOverride; }
                else if (m_registry) {
                    const auto* matC = em->TryGetTIComponent<GE::Components::PhysicsMaterialTag>(cID);
                    const auto* matS = em->TryGetTIComponent<GE::Components::PhysicsMaterialTag>(sID);
                    if (matC && matS) {
                        GE::Physics::MaterialInteractionRecord rec;
                        if (m_registry->Lookup(matC->name, matS->name, rec)) { e = rec.restitution; }
                    }
                }

                const glm::vec3 velS   = sStatic ? glm::vec3{0.0f} : sRB->velocity;
                const float     vRelN  = glm::dot(cRB->velocity - velS, normal);
                if (vRelN < 0.0f) {
                    const float j = -(1.0f + e) * vRelN / totalInvMass;
                    cRB->velocity -= j * invMassC * normal;
                    if (!sStatic) sRB->velocity += j * invMassS * normal;
                }
            }
        }

        // -----------------------------------------------------------------
        // Pass L: Cylinder-Cylinder collisions.
        // Find closest points on both axes, then treat as sphere-sphere
        // with the sum of radii.
        // -----------------------------------------------------------------
        for (uint32_t aIdx = 0; aIdx < cylArray.GetCount(); ++aIdx) {
            const auto  aID    = cylArray.Index()[aIdx];
            const auto& aCol   = cylArray.Data()[aIdx];
            auto* const aTrans = em->TryGetTIComponent<GE::Components::Transform>(aID);
            auto* const aRB    = em->TryGetTIComponent<GE::Components::RigidBody>(aID);

            if (!aTrans || !aRB || aRB->isStatic) continue;

            const glm::vec3 aBot  = aTrans->m_worldPosition - glm::vec3(0.0f, aCol.height * 0.5f, 0.0f);
            const glm::vec3 aAxis = glm::vec3(0.0f, aCol.height, 0.0f);
            const float aLenSq    = aCol.height * aCol.height;

            for (uint32_t bIdx = aIdx + 1; bIdx < cylArray.GetCount(); ++bIdx) {
                const auto  bID    = cylArray.Index()[bIdx];
                const auto& bCol   = cylArray.Data()[bIdx];
                auto* const bTrans = em->TryGetTIComponent<GE::Components::Transform>(bID);
                auto* const bRB    = em->TryGetTIComponent<GE::Components::RigidBody>(bID);

                if (!bTrans) continue;
                const bool bStatic = (!bRB || bRB->isStatic);

                const glm::vec3 bBot  = bTrans->m_worldPosition - glm::vec3(0.0f, bCol.height * 0.5f, 0.0f);
                const glm::vec3 bAxis = glm::vec3(0.0f, bCol.height, 0.0f);
                const float bLenSq    = bCol.height * bCol.height;

                // Closest point on A's axis to B's centre, and vice versa
                const float tA = glm::clamp(glm::dot(bTrans->m_worldPosition - aBot, aAxis) / aLenSq, 0.0f, 1.0f);
                const float tB = glm::clamp(glm::dot(aTrans->m_worldPosition - bBot, bAxis) / bLenSq, 0.0f, 1.0f);
                const glm::vec3 pA = aBot + tA * aAxis;
                const glm::vec3 pB = bBot + tB * bAxis;
                const glm::vec3 midpoint = (pA + pB) * 0.5f;

                // Use midpoint as representative contact; test with sumR
                const float tA2 = glm::clamp(glm::dot(midpoint - aBot, aAxis) / aLenSq, 0.0f, 1.0f);
                const float tB2 = glm::clamp(glm::dot(midpoint - bBot, bAxis) / bLenSq, 0.0f, 1.0f);
                const glm::vec3 closestA = aBot + tA2 * aAxis;
                const glm::vec3 closestB = bBot + tB2 * bAxis;

                const glm::vec3 diff   = closestA - closestB;
                const float     distSq = glm::dot(diff, diff);
                const float     sumR   = aCol.radius + bCol.radius;

                if (distSq >= sumR * sumR || distSq < 1e-12f) continue;

                if (aCol.isTrigger || bCol.isTrigger) {
                    m_currentTriggers.insert({ std::min(aID, bID), std::max(aID, bID) });
                    continue;
                }

                const float     dist        = std::sqrt(distSq);
                const glm::vec3 normal      = diff / dist;
                const float     penetration = sumR - dist;

                const float invMassA     = aRB->inverseMass;
                const float invMassB     = bStatic ? 0.0f : bRB->inverseMass;
                const float totalInvMass = invMassA + invMassB;
                if (totalInvMass <= 0.0f) continue;

                const glm::vec3 correction = (penetration / totalInvMass) * normal;
                aTrans->m_worldPosition += correction * invMassA;
                SyncWorldToLocal(*aTrans, em);
                if (!bStatic) {
                    bTrans->m_worldPosition -= correction * invMassB;
                    SyncWorldToLocal(*bTrans, em);
                }

                float e = aRB->restitution;
                if (!bStatic) e = glm::min(e, bRB->restitution);
                if (m_restitutionOverride >= 0.0f) { e = m_restitutionOverride; }
                else if (m_registry) {
                    const auto* matA = em->TryGetTIComponent<GE::Components::PhysicsMaterialTag>(aID);
                    const auto* matB = em->TryGetTIComponent<GE::Components::PhysicsMaterialTag>(bID);
                    if (matA && matB) {
                        GE::Physics::MaterialInteractionRecord rec;
                        if (m_registry->Lookup(matA->name, matB->name, rec)) { e = rec.restitution; }
                    }
                }

                const glm::vec3 velB  = bStatic ? glm::vec3{0.0f} : bRB->velocity;
                const float     vRelN = glm::dot(aRB->velocity - velB, normal);
                if (vRelN < 0.0f) {
                    const float j = -(1.0f + e) * vRelN / totalInvMass;
                    aRB->velocity += j * invMassA * normal;
                    if (!bStatic) bRB->velocity -= j * invMassB * normal;
                }
            }
        }
    }
}
