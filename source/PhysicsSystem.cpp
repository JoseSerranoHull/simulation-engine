#include "../include/PhysicsSystem.h"
#include "../include/ServiceLocator.h"
#include "../include/PhysicsComponents.h"
#include "../include/Sphere.h"
#include "../include/Plane.h"
#include "../include/Transform.h"

namespace GE::Systems {

    void PhysicsSystem::OnUpdate(float dt, VkCommandBuffer cb) {
        // Step 1: Predict movement based on forces
        Integrate(dt);

        // Step 2: Correct movement based on geometric constraints
        ResolveCollisions();
    }

    void PhysicsSystem::Integrate(float dt) {
        auto* em = ServiceLocator::GetEntityManager();
        auto& rbArray = em->GetCompArr<GE::Components::RigidBody>();

        for (uint32_t i = 0; i < rbArray.GetCount(); ++i) {
            auto id = rbArray.Index()[i];
            auto& rb = rbArray.Data()[i];
            auto* trans = em->GetTIComponent<GE::Components::Transform>(id);

            if (trans && !rb.isStatic) {
                // Fulfills Q3: Accumulate Gravity Force
                if (rb.useGravity) {
                    // a = F/m. Assuming gravity force is mass * 9.81.
                    rb.velocity += glm::vec3(0, -9.81f, 0) * dt;
                }

                // Fulfills Q2: Choose Integration Method
                // This is 'Semi-Implicit Euler' because we use the UPDATED velocity 
                // to calculate the NEW position.
                trans->m_position += rb.velocity * dt;

                // Sync the internal collider position with the new ECS position
                // This prepares the collider for the ResolveCollisions step.
                trans->m_state = GE::Components::Transform::TransformState::Dirty;
            }
        }
    }

    void PhysicsSystem::ResolveCollisions() {
        auto* em = ServiceLocator::GetEntityManager();
        auto& sphereArray = em->GetCompArr<GE::Components::SphereCollider>();
        auto& planeArray = em->GetCompArr<GE::Components::PlaneCollider>();

        // Fulfills Q4: Detect Sphere-Plane Collisions
        for (uint32_t sIdx = 0; sIdx < sphereArray.GetCount(); ++sIdx) {
            auto sID = sphereArray.Index()[sIdx];
            auto& sCol = sphereArray.Data()[sIdx];
            auto* sTrans = em->GetTIComponent<GE::Components::Transform>(sID);
            auto* sRB = em->GetTIComponent<GE::Components::RigidBody>(sID);

            if (!sTrans) continue;

            for (uint32_t pIdx = 0; pIdx < planeArray.GetCount(); ++pIdx) {
                auto& pCol = planeArray.Data()[pIdx];

                // Setup geometric proxies using latest refactored classes
                GE::Physics::Sphere sphere(sTrans->m_position, sCol.radius);
                GE::Physics::Plane plane(pCol.normal * pCol.offset, pCol.normal);

                float dist = plane.DistanceToPoint(sphere.GetCenter());

                // Collision Detection Logic
                if (dist < sphere.GetRadius()) {

                    // 1. Static Resolution: Push the object out of the floor
                    float penetration = sphere.GetRadius() - dist;
                    sTrans->m_position += plane.GetNormal() * penetration;

                    // 2. Dynamic Response: Reflection
                    if (sRB) {
                        // Reflect velocity based on plane normal
                        // v' = v - 2(v . n)n
                        sRB->velocity = glm::reflect(sRB->velocity, plane.GetNormal());

                        // Apply Coefficient of Restitution (Elasticity)
                        sRB->velocity *= sRB->restitution;

                        // Friction hack: if velocity is very low after reflection, kill it to prevent jitter
                        if (glm::length(sRB->velocity) < 0.1f) {
                            sRB->velocity = glm::vec3(0.0f);
                        }
                    }
                }
            }
        }
    }
}