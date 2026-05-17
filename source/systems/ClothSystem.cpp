/* parasoft-begin-suppress ALL */
#include <cmath>
/* parasoft-end-suppress ALL */

#include "systems/ClothSystem.h"
#include "components/ClothComponent.h"
#include "components/PhysicsComponents.h"
#include "components/Components.h"   // MeshRenderer / SubMesh for setIndexCount after rebuild
#include "components/Transform.h"
#include "ecs/ComponentArray.h"
#include "core/ServiceLocator.h"
#include "core/Common.h"
#include "assets/Vertex.h"           // GE::Assets::Vertex — layout of the mapped vertex buffer
#include "assets/Mesh.h"             // Mesh::setIndexCount()

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

static constexpr float GRAVITY           = -9.81f;
static constexpr float MIN_LENGTH        = 1e-6f;
static constexpr float CLOTH_RESTITUTION = 0.20f;   // fraction of normal velocity reflected by cloth
static constexpr float CLOTH_FRICTION    = 0.70f;   // fraction of tangential velocity retained
static constexpr float JAKOBSEN_REF_K   = 200.0f;   // kSpring value mapping to stiffness = 1.0

// Jakobsen positional distance constraint.
// Directly moves A and B so that |A.pos - B.pos| approaches effectiveRestLen.
// stiffness in [0,1]: 1.0 = full correction per iteration.
// shrinkScale reduces rest length with heat, causing wrinkling (Dayong 2011).
static void satisfyDistance(
    GE::Components::ClothParticle& A,
    GE::Components::ClothParticle& B,
    float restLen,
    float stiffness,
    float shrinkScale)
{
    const float avgHeat       = 0.5f * (A.heat + B.heat);
    const float effectiveRest = restLen * glm::max(1.0f - avgHeat * shrinkScale, 0.1f);

    const glm::vec3 delta = B.position - A.position;
    const float     len   = glm::length(delta);
    if (len < MIN_LENGTH) { return; }

    const glm::vec3 move = delta * (0.5f * stiffness * (len - effectiveRest) / len);
    if (!A.pinned && !A.burned) { A.position += move; }
    if (!B.pinned && !B.burned) { B.position -= move; }
}

// Hash-based value noise: maps (scalar, time) pair to [0,1].
// Closest point on line segment [A,B] to point P, clamped to the segment.
static glm::vec3 closestPointOnSegment(
    const glm::vec3& A, const glm::vec3& B, const glm::vec3& P)
{
    const glm::vec3 AB    = B - A;
    const float     denom = glm::dot(AB, AB);
    if (denom < 1e-12f) { return A; }
    return A + AB * glm::clamp(glm::dot(P - A, AB) / denom, 0.0f, 1.0f);
}

// Geometry rebuild — called from the physics thread when cc.rebuildPending is set.
// ALL particle and spring mutation is done here (physics thread only) so the render
// thread's vertex refresh loop never races with a particles.clear() call.
static void performGeometryRebuild(
    GE::Components::ClothComponent& cc,
    GE::ECS::EntityID               eid,
    GE::ECS::EntityManager*         em)
{
    const int R = cc.rebuildRows;
    const int C = cc.rebuildCols;
    if (R < 2 || C < 2 || R * C > 65535) { return; }

    const GE::Components::Transform* tr =
        em->TryGetTIComponent<GE::Components::Transform>(eid);
    const glm::vec3 origin = (tr != nullptr) ? tr->m_worldPosition : glm::vec3{ 0.0f };

    cc.rows     = R;
    cc.cols     = C;
    cc.cellSize = cc.rebuildCellSize;

    // Reinitialise particles
    cc.particles.clear();
    cc.particles.reserve(static_cast<std::size_t>(R * C));
    for (int r = 0; r < R; ++r) {
        for (int c = 0; c < C; ++c) {
            GE::Components::ClothParticle p;
            p.position     = origin + glm::vec3(static_cast<float>(c) * cc.cellSize,
                                                0.0f,
                                                static_cast<float>(r) * cc.cellSize);
            p.prevPosition = p.position;
            p.pinned       = (r == 0);
            cc.particles.push_back(p);
        }
    }

    // Rebuild spring list — structural / shear / flexion
    cc.springs.clear();
    const float restStruct = cc.cellSize;
    const float restShear  = cc.cellSize * 1.41421356f;
    const float restFlex   = cc.cellSize * 2.0f;

    for (int r = 0; r < R; ++r) {
        for (int c = 0; c < C; ++c) {
            if (c + 1 < C)
                cc.springs.push_back({ static_cast<uint16_t>(r*C+c),
                                       static_cast<uint16_t>(r*C+c+1),
                                       restStruct, cc.springK, true });
            if (r + 1 < R)
                cc.springs.push_back({ static_cast<uint16_t>(r*C+c),
                                       static_cast<uint16_t>((r+1)*C+c),
                                       restStruct, cc.springK, true });
        }
    }
    for (int r = 0; r < R-1; ++r) {
        for (int c = 0; c < C-1; ++c) {
            cc.springs.push_back({ static_cast<uint16_t>(r*C+c),
                                   static_cast<uint16_t>((r+1)*C+c+1),
                                   restShear, cc.shearK, true, 0.0f,
                                   GE::Components::SpringType::Shear });
            cc.springs.push_back({ static_cast<uint16_t>(r*C+c+1),
                                   static_cast<uint16_t>((r+1)*C+c),
                                   restShear, cc.shearK, true, 0.0f,
                                   GE::Components::SpringType::Shear });
        }
    }
    for (int r = 0; r < R; ++r)
        for (int c = 0; c < C-2; ++c)
            cc.springs.push_back({ static_cast<uint16_t>(r*C+c),
                                   static_cast<uint16_t>(r*C+c+2),
                                   restFlex, cc.flexionK, true, 0.0f,
                                   GE::Components::SpringType::Flexion });
    for (int r = 0; r < R-2; ++r)
        for (int c = 0; c < C; ++c)
            cc.springs.push_back({ static_cast<uint16_t>(r*C+c),
                                   static_cast<uint16_t>((r+2)*C+c),
                                   restFlex, cc.flexionK, true, 0.0f,
                                   GE::Components::SpringType::Flexion });

    // Reset burn sources — one default inactive source at cloth centre/bottom
    cc.burnSources.clear();
    {
        GE::Components::BurnSource src;
        src.center = {
            origin.x + static_cast<float>(C - 1) * 0.5f * cc.cellSize,
            origin.y - static_cast<float>(R - 1) * cc.cellSize,
            origin.z + static_cast<float>(R - 1) * 0.5f * cc.cellSize
        };
        src.radius = 0.8f;
        src.active = false;
        cc.burnSources.push_back(src);
    }

    // Update render counts (indexOffset fixed at load time — never changes)
    cc.vertexCount = static_cast<uint32_t>(R * C);
    cc.indexCount  = static_cast<uint32_t>((R - 1) * (C - 1) * 6);

    // Write initial vertex data (local space) into the persistently-mapped buffer
    if (cc.mappedVertices != nullptr) {
        auto* verts = static_cast<GE::Assets::Vertex*>(cc.mappedVertices);
        const glm::vec3 coldColor = cc.useTextureMode ? glm::vec3{ 1.0f } : cc.color;
        for (int r = 0; r < R; ++r) {
            for (int c = 0; c < C; ++c) {
                const int idx = r * C + c;
                GE::Assets::Vertex& v = verts[idx];
                v.position = cc.particles[idx].position - origin;
                v.color    = coldColor;
                v.texcoord = glm::vec2{ static_cast<float>(c) / static_cast<float>(C - 1),
                                        static_cast<float>(r) / static_cast<float>(R - 1) };
                v.normal   = glm::vec3{ 0.0f, 1.0f, 0.0f };
                v.tangent  = glm::vec3{ 1.0f, 0.0f, 0.0f };
            }
        }
    }

    // Sync Mesh::indexCount (Mesh captures it by value at load time)
    auto* mr = em->TryGetTIComponent<GE::Components::MeshRenderer>(eid);
    if (mr != nullptr && !mr->subMeshes.empty() && mr->subMeshes[0].m_mesh != nullptr) {
        mr->subMeshes[0].m_mesh->setIndexCount(cc.indexCount);
    }
}

// Hash-based value noise — cheap substitute for Perlin, no external dependency.
static float hashNoise(float x, float t) {
    const float v = std::sin(x * 12.9898f + t * 78.233f) * 43758.5453f;
    return v - std::floor(v);
}

// Per-triangle aerodynamic wind (panel method).
//
// For each triangle:
//   - Compute the surface normal and area from edge cross product.
//   - Estimate the triangle's velocity from Verlet position history.
//   - Project relative wind velocity onto the normal (dot product).
//   - Force = Cd * rho * area * (relVel · n) * n  (signed — both faces react).
//   - Sinusoidal gust scales the base wind; per-centroid hash noise adds spatial turbulence.
//   - Force distributed equally to the triangle's three particles.
//
// Replaces the old uniform-force approach (windX/Z * mass per particle), which was
// equivalent to a body force independent of cloth orientation and surface area.
static void applyAerodynamicWind(
    GE::Components::ClothComponent& cc,
    float dt,
    float simTime)
{
    if (!cc.windEnabled) { return; }

    static constexpr float AIR_DENSITY = 1.225f;   // kg/m³ at sea level

    // Sinusoidal gust: scales the entire wind vector ±gustAmplitude
    const float gustScale =
        1.0f + cc.gustAmplitude *
        std::sin(2.0f * glm::pi<float>() * cc.gustFrequency * simTime);
    const glm::vec3 baseWind{ cc.windX * gustScale, 0.0f, cc.windZ * gustScale };
    const float baseWindMag = glm::length(baseWind);

    const int C = cc.cols;

    for (int r = 0; r < cc.rows - 1; ++r) {
        for (int c = 0; c < C - 1; ++c) {
            const int i00 = r * C + c;
            const int i10 = (r + 1) * C + c;
            const int i01 = r * C + (c + 1);
            const int i11 = (r + 1) * C + (c + 1);

            // Apply aerodynamic force to one triangle.
            auto applyTri = [&](int ia, int ib, int ic) {
                GE::Components::ClothParticle& pa = cc.particles[ia];
                GE::Components::ClothParticle& pb = cc.particles[ib];
                GE::Components::ClothParticle& pc = cc.particles[ic];

                // Triangle normal — magnitude equals twice the triangle area.
                const glm::vec3 e1          = pb.position - pa.position;
                const glm::vec3 e2          = pc.position - pa.position;
                const glm::vec3 normalScaled = glm::cross(e1, e2);
                const float     area2       = glm::length(normalScaled);
                if (area2 < 1e-6f) { return; }
                const glm::vec3 normal = normalScaled / area2;

                // Spatial turbulence: per-centroid hash noise perturbs wind direction.
                const glm::vec3 centroid = (pa.position + pb.position + pc.position) / 3.0f;
                const float     nx = (hashNoise(centroid.x, simTime * 1.1f) - 0.5f)
                                     * cc.gustAmplitude * baseWindMag * 0.4f;
                const float     nz = (hashNoise(centroid.z, simTime * 0.9f) - 0.5f)
                                     * cc.gustAmplitude * baseWindMag * 0.4f;
                const glm::vec3 effectiveWind = baseWind + glm::vec3{ nx, 0.0f, nz };

                // Average triangle velocity estimated from Verlet position difference.
                const glm::vec3 triVel = (dt > 0.0f)
                    ? ((pa.position - pa.prevPosition) +
                       (pb.position - pb.prevPosition) +
                       (pc.position - pc.prevPosition)) / (3.0f * dt)
                    : glm::vec3{ 0.0f };

                // Signed normal component — both faces react; signed drag correctly
                // opposes cloth motion into the wind and aids motion away from it.
                const float     normalComp = glm::dot(effectiveWind - triVel, normal);
                const glm::vec3 F = cc.dragCoeff * AIR_DENSITY
                                  * (area2 * 0.5f) * normalComp * normal;

                // Distribute force equally to all three vertices.
                const glm::vec3 Fp = F / 3.0f;
                if (!pa.pinned && !pa.burned) { pa.force += Fp; }
                if (!pb.pinned && !pb.burned) { pb.force += Fp; }
                if (!pc.pinned && !pc.burned) { pc.force += Fp; }
            };

            // Two triangles per quad — same winding order as the GPU index buffer.
            applyTri(i00, i10, i01);   // top-left, bottom-left, top-right
            applyTri(i01, i10, i11);   // top-right, bottom-left, bottom-right
        }
    }
}

void ClothSystem::OnUpdate(float dt) {
    if (dt <= 0.0f) { return; }

    m_simTime += dt;   // drives sinusoidal gust oscillation

    GE::ECS::EntityManager* em = ServiceLocator::GetEntityManager();
    if (em == nullptr) { return; }

    auto& clothArr   = em->GetCompArr<GE::Components::ClothComponent>();
    auto& sphereArr  = em->GetCompArr<GE::Components::SphereCollider>();
    auto& planeArr   = em->GetCompArr<GE::Components::PlaneCollider>();
    auto& boxArr     = em->GetCompArr<GE::Components::BoxCollider>();
    auto& capsuleArr = em->GetCompArr<GE::Components::CapsuleCollider>();

    const uint32_t clothCount = clothArr.GetCount();
    for (uint32_t ci = 0U; ci < clothCount; ++ci) {
        GE::Components::ClothComponent& cc = clothArr.Data()[ci];

        // Geometry rebuild requested by render thread (ImGui).
        // Processed here (physics thread) so particles.clear() never races with iteration.
        if (cc.rebuildPending) {
            cc.rebuildPending = false;
            performGeometryRebuild(cc, clothArr.Index()[ci], em);
        }

        const int R = cc.rows;
        const int C = cc.cols;
        if (R <= 0 || C <= 0 || cc.particles.empty()) { continue; }

        // -------------------------------------------------------------------
        // 1a. External forces: gravity (per-particle).
        //     Spring forces are handled as positional constraints in step 3.
        //     Wind forces are handled per-triangle in step 1b.
        // -------------------------------------------------------------------
        for (auto& p : cc.particles) {
            if (p.pinned) { p.force = glm::vec3{ 0.0f }; continue; }
            // Gravity only — burned particles lose all spring and wind interaction.
            p.force = glm::vec3{ 0.0f, GRAVITY * cc.particleMass, 0.0f };
        }

        // -------------------------------------------------------------------
        // 1b. Aerodynamic wind: per-triangle panel-method drag.
        //     Replaces the old uniform (windX/Z * mass) body-force approach.
        //     Force scales with surface area and the angle between the wind
        //     and the triangle normal — cloth parallel to wind gets no force.
        // -------------------------------------------------------------------
        applyAerodynamicWind(cc, dt, m_simTime);

        // -------------------------------------------------------------------
        // 2. Verlet integration — predicts new positions from external forces.
        //    Spring stiffness is no longer coupled to dt here, eliminating the
        //    k*dt^2 < 2m stability ceiling of the old force-accumulation approach.
        // -------------------------------------------------------------------
        const float dampFactor = 1.0f - cc.damping * dt;
        for (auto& p : cc.particles) {
            if (p.pinned) { continue; }
            const glm::vec3 acc    = p.force / cc.particleMass;
            const glm::vec3 newPos = p.position
                                   + (p.position - p.prevPosition) * dampFactor
                                   + acc * (dt * dt);
            p.prevPosition = p.position;
            p.position     = newPos;
            p.force        = glm::vec3{ 0.0f };
        }

        // -------------------------------------------------------------------
        // 3. Jakobsen constraint relaxation.
        //    Each spring's kSpring is normalised to stiffness in [0,1] against
        //    JAKOBSEN_REF_K (200). Structural (200) → 1.0, shear (100) → 0.5,
        //    flexion (50) → 0.25. Multiple iterations improve convergence.
        //    Shrink scale reduces effectiveRestLen with heat → wrinkling.
        // -------------------------------------------------------------------
        const int iters = glm::clamp(cc.constraintIters, 1, 8);
        for (int iter = 0; iter < iters; ++iter) {
            for (GE::Components::ClothSpring& s : cc.springs) {
                if (!s.active) { continue; }
                GE::Components::ClothParticle& pA = cc.particles[s.a];
                GE::Components::ClothParticle& pB = cc.particles[s.b];
                if (pA.burned || pB.burned) { s.active = false; continue; }
                const float stiffness = glm::clamp(s.kSpring / JAKOBSEN_REF_K, 0.0f, 1.0f);
                satisfyDistance(pA, pB, s.restLen, stiffness, cc.shrinkScale);
            }
        }

        // -------------------------------------------------------------------
        // 4. Tearing: deactivate springs exceeding the effective stretch threshold.
        //    effectiveThreshold = tearThreshold - stressAccum, floored at 0.5 to
        //    prevent zero-length tears.
        //    On tear: stress transfer lowers neighbours' threshold (crack propagation);
        //    jitter displaces particles at the edge to create a ragged visual.
        // -------------------------------------------------------------------
        for (GE::Components::ClothSpring& s : cc.springs) {
            if (!s.active) { continue; }
            const float curLen = glm::length(
                cc.particles[s.a].position - cc.particles[s.b].position);

            const float effectiveThreshold =
                glm::max(cc.tearThreshold - s.stressAccum, 0.5f);

            if (curLen <= effectiveThreshold * s.restLen) { continue; }

            s.active = false;

            // Stress transfer: distribute excess tension to springs sharing either endpoint.
            if (cc.stressTransferRate > 0.0f) {
                const float stretchRatio  = curLen / glm::max(s.restLen, MIN_LENGTH);
                const float excessStress  =
                    (stretchRatio - cc.tearThreshold) / glm::max(cc.tearThreshold, 0.1f);
                if (excessStress > 0.0f) {
                    const float delta = excessStress * cc.stressTransferRate;
                    for (GE::Components::ClothSpring& nb : cc.springs) {
                        if (!nb.active) { continue; }
                        if (nb.a == s.a || nb.b == s.a || nb.a == s.b || nb.b == s.b) {
                            nb.stressAccum += delta;
                        }
                    }
                }
            }

            // Tear edge jitter: small deterministic displacement creates a ragged edge.
            if (cc.tearRoughness > 0.0f) {
                auto applyJitter = [&](uint16_t idx) {
                    GE::Components::ClothParticle& p = cc.particles[idx];
                    if (p.pinned || p.burned) { return; }
                    const float fi = static_cast<float>(idx) * 7.13f
                                   + static_cast<float>(s.b)  * 3.71f;
                    const glm::vec3 j{
                        (hashNoise(fi,        0.0f) - 0.5f) * cc.tearRoughness,
                        (hashNoise(fi + 1.0f, 0.0f) - 0.5f) * cc.tearRoughness * 0.3f,
                        (hashNoise(fi + 2.0f, 0.0f) - 0.5f) * cc.tearRoughness
                    };
                    p.position     += j;
                    p.prevPosition += j;  // preserve implicit Verlet velocity
                };
                applyJitter(s.a);
                applyJitter(s.b);
            }
        }

        // -------------------------------------------------------------------
        // 5. Sphere-cloth collision: push particles outside each sphere.
        //    Accumulates normals across all colliding particles and applies
        //    a reaction impulse to the sphere's RigidBody.
        // -------------------------------------------------------------------
        const uint32_t sphereCount = sphereArr.GetCount();
        for (uint32_t si = 0U; si < sphereCount; ++si) {
            const GE::Components::SphereCollider& sc = sphereArr.Data()[si];
            const GE::ECS::EntityID seid             = sphereArr.Index()[si];

            GE::Components::Transform* tr =
                em->TryGetTIComponent<GE::Components::Transform>(seid);
            if (tr == nullptr) { continue; }

            const glm::vec3 sphereCenter = tr->m_worldPosition;
            const float     sphereRadius = sc.radius;

            int       collisionCount = 0;
            glm::vec3 totalNormal{ 0.0f };

            for (auto& p : cc.particles) {
                if (p.pinned) { continue; }
                const glm::vec3 diff = p.position - sphereCenter;
                const float     dist = glm::length(diff);
                if (dist < sphereRadius && dist > MIN_LENGTH) {
                    const glm::vec3 normal = diff / dist;
                    p.position = sphereCenter + normal * sphereRadius;
                    totalNormal += normal;
                    ++collisionCount;
                }
            }

            if (collisionCount > 0 && glm::dot(totalNormal, totalNormal) > MIN_LENGTH) {
                auto* rb = em->TryGetTIComponent<GE::Components::RigidBody>(seid);
                if (rb != nullptr && !rb->isStatic) {
                    const glm::vec3 avgNormal = glm::normalize(totalNormal);
                    const float     vAlongN   = glm::dot(rb->velocity, avgNormal);
                    if (vAlongN < 0.0f) {
                        const glm::vec3 vNorm = vAlongN * avgNormal;
                        const glm::vec3 vTang = rb->velocity - vNorm;
                        rb->velocity = (-CLOTH_RESTITUTION * vNorm) + (CLOTH_FRICTION * vTang);
                    }
                }
            }
        }

        // -------------------------------------------------------------------
        // 5b. Plane-cloth collision.
        //     signedDist = dot(normal, pos) - offset  (positive = above plane).
        //     When negative the particle has penetrated; push it back to the surface
        //     and kill the normal component of its implicit Verlet velocity (no bounce),
        //     retaining CLOTH_FRICTION of the tangential component.
        // -------------------------------------------------------------------
        {
            const uint32_t planeCount = planeArr.GetCount();
            for (uint32_t pi = 0U; pi < planeCount; ++pi) {
                const GE::Components::PlaneCollider& pc = planeArr.Data()[pi];

                for (auto& p : cc.particles) {
                    if (p.pinned) { continue; }
                    const float sd = glm::dot(p.position, pc.normal) - pc.offset;
                    if (sd >= 0.0f) { continue; }

                    // Push particle back onto the plane surface.
                    p.position -= sd * pc.normal;   // sd is negative, so this adds |sd|*n

                    // Verlet friction: remove normal velocity, damp tangential.
                    const glm::vec3 vel   = p.position - p.prevPosition;
                    const glm::vec3 vNorm = glm::dot(vel, pc.normal) * pc.normal;
                    const glm::vec3 vTang = vel - vNorm;
                    p.prevPosition = p.position - vTang * CLOTH_FRICTION;
                }
            }
        }

        // -------------------------------------------------------------------
        // 5c. Capsule-cloth collision.
        //     Model: sphere swept along the capsule spine (base → tip in local Y).
        //     For each particle, find the closest point on the spine segment and
        //     push the particle to the capsule surface — identical logic to sphere.
        // -------------------------------------------------------------------
        {
            const uint32_t capCount = capsuleArr.GetCount();
            for (uint32_t ki = 0U; ki < capCount; ++ki) {
                const GE::Components::CapsuleCollider& cap = capsuleArr.Data()[ki];
                const GE::ECS::EntityID ceid               = capsuleArr.Index()[ki];

                const GE::Components::Transform* tr =
                    em->TryGetTIComponent<GE::Components::Transform>(ceid);
                if (tr == nullptr) { continue; }

                // Spine endpoints in local space (cylindrical body only; hemispheres sit beyond).
                const glm::vec3 localBase{ 0.0f, -cap.height * 0.5f, 0.0f };
                const glm::vec3 localTip { 0.0f,  cap.height * 0.5f, 0.0f };
                const glm::vec3 worldBase =
                    glm::vec3(tr->m_worldMatrix * glm::vec4(localBase, 1.0f));
                const glm::vec3 worldTip  =
                    glm::vec3(tr->m_worldMatrix * glm::vec4(localTip,  1.0f));

                for (auto& p : cc.particles) {
                    if (p.pinned) { continue; }
                    const glm::vec3 closest = closestPointOnSegment(worldBase, worldTip, p.position);
                    const glm::vec3 diff    = p.position - closest;
                    const float     dist    = glm::length(diff);
                    if (dist < cap.radius && dist > MIN_LENGTH) {
                        p.position = closest + (diff / dist) * cap.radius;
                    }
                }
            }
        }

        // -------------------------------------------------------------------
        // 5d. Box/OBB-cloth collision.
        //     Particle is transformed into the box's local space (handles rotation).
        //     Outside particles: no collision.
        //     Inside particles: eject along the axis of minimum penetration depth.
        // -------------------------------------------------------------------
        {
            const uint32_t boxCount = boxArr.GetCount();
            for (uint32_t bi = 0U; bi < boxCount; ++bi) {
                const GE::Components::BoxCollider& box = boxArr.Data()[bi];
                const GE::ECS::EntityID beid           = boxArr.Index()[bi];

                const GE::Components::Transform* tr =
                    em->TryGetTIComponent<GE::Components::Transform>(beid);
                if (tr == nullptr) { continue; }

                // Half-extents in local space (BoxCollider stores full extents).
                const glm::vec3 half{ box.sizeX * 0.5f,
                                      box.sizeY * 0.5f,
                                      box.sizeZ * 0.5f };
                const glm::mat4 invWorld = glm::inverse(tr->m_worldMatrix);

                for (auto& p : cc.particles) {
                    if (p.pinned) { continue; }

                    // Particle in box local space.
                    const glm::vec3 lp =
                        glm::vec3(invWorld * glm::vec4(p.position, 1.0f));

                    // Inside test: all components within half-extents.
                    if (!(glm::abs(lp.x) < half.x &&
                          glm::abs(lp.y) < half.y &&
                          glm::abs(lp.z) < half.z)) { continue; }

                    // Minimum penetration axis ejection.
                    const glm::vec3 pen = half - glm::abs(lp);
                    int   axis  = (pen.x < pen.y) ? 0 : 1;
                    if (pen.z < pen[axis]) { axis = 2; }

                    glm::vec3 corrLocal = lp;
                    corrLocal[axis]     = (lp[axis] >= 0.0f ? 1.0f : -1.0f) * half[axis];

                    p.position = glm::vec3(tr->m_worldMatrix * glm::vec4(corrLocal, 1.0f));
                }
            }
        }

        // -------------------------------------------------------------------
        // 6. Burning: accumulate heat for particles within each active ignition source.
        //    Burn-state transitions are deferred to step 7 so heat diffusion
        //    can spread the ignition front before particles are freed.
        // -------------------------------------------------------------------
        for (const GE::Components::BurnSource& src : cc.burnSources) {
            if (!src.active) { continue; }
            for (auto& p : cc.particles) {
                if (p.burned || p.pinned) { continue; }
                if (glm::length(p.position - src.center) < src.radius) {
                    p.heat += cc.burnRate * dt;
                }
            }
        }

        // -------------------------------------------------------------------
        // 7. Heat diffusion along active spring graph (Laplacian, Dayong 2011).
        //    flux = conductivity * (heat_B - heat_A) * dt per spring.
        //    After applying deltas, particles that reach heat >= 1.0 are freed.
        //    Vertex curl (Dayong 2011): a one-time hash-noise position warp is
        //    applied at the exact tick a particle burns, simulating charring.
        // -------------------------------------------------------------------
        const auto N = static_cast<uint32_t>(cc.particles.size());

        // Shared burn-state transition: called for each particle that just reached heat=1.
        auto burnParticle = [&](uint32_t pi) {
            GE::Components::ClothParticle& p = cc.particles[pi];
            p.burned = true;
            p.pinned = false;

            // Vertex curl: one-time warp applied at the burn-transition tick.
            if (!p.wasCurled && cc.curlAmount > 0.0f) {
                p.wasCurled = true;
                const float fi = static_cast<float>(pi) * 7.13f;
                const glm::vec3 curl{
                    (hashNoise(fi,        m_simTime) - 0.5f) * cc.curlAmount,
                    (hashNoise(fi + 1.0f, m_simTime) - 0.5f) * cc.curlAmount * 0.3f,
                    (hashNoise(fi + 2.0f, m_simTime) - 0.5f) * cc.curlAmount
                };
                p.position    += curl;
                p.prevPosition = p.position; // zero out Verlet velocity at curl point
            }

            // Sever all springs connected to this particle.
            for (GE::Components::ClothSpring& s : cc.springs) {
                if (s.a == static_cast<uint16_t>(pi) ||
                    s.b == static_cast<uint16_t>(pi)) {
                    s.active = false;
                }
            }
        };

        if (cc.heatConductivity > 0.0f) {
            m_heatDelta.assign(N, 0.0f);

            for (const GE::Components::ClothSpring& s : cc.springs) {
                if (!s.active) { continue; }
                const GE::Components::ClothParticle& pA = cc.particles[s.a];
                const GE::Components::ClothParticle& pB = cc.particles[s.b];
                if (pA.burned || pB.burned) { continue; }
                const float flux = cc.heatConductivity * (pB.heat - pA.heat) * dt;
                m_heatDelta[s.a] += flux;
                m_heatDelta[s.b] -= flux;
            }

            for (uint32_t pi = 0U; pi < N; ++pi) {
                GE::Components::ClothParticle& p = cc.particles[pi];
                if (p.burned || p.pinned) { continue; }
                p.heat = glm::clamp(p.heat + m_heatDelta[pi], 0.0f, 1.0f);
                if (p.heat >= 1.0f) { burnParticle(pi); }
            }
        } else {
            // Diffusion disabled: still need burn-state transitions from direct ignition.
            for (uint32_t pi = 0U; pi < N; ++pi) {
                GE::Components::ClothParticle& p = cc.particles[pi];
                if (p.burned || p.pinned || p.heat < 1.0f) { continue; }
                burnParticle(pi);
            }
        }
    }
}

} // namespace GE::Systems
