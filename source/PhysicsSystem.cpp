#include "../include/PhysicsSystem.h"
#include "../include/ServiceLocator.h"
#include "../include/PhysicsComponents.h"
#include "../include/Sphere.h"
#include "../include/Plane.h"
#include "../include/Transform.h"

namespace GE::Systems {

    void PhysicsSystem::OnUpdate(float dt) {
        // High-level orchestration
        Integrate(dt);
        ResolveCollisions();
    }

    void PhysicsSystem::Integrate(float dt) {
        auto* em = ServiceLocator::GetEntityManager();

        // Fulfills Requirement: Simulation of motion
        auto& rbArray = em->GetCompArr<GE::Components::RigidBody>();
        for (uint32_t i = 0; i < rbArray.GetCount(); ++i) {
            auto id = rbArray.Index()[i];
            auto& rb = rbArray.Data()[i];
            auto* trans = em->GetTIComponent<GE::Scene::Components::Transform>(id);

            if (trans && !rb.isStatic) {
                // Apply Gravity if enabled
                if (rb.useGravity) {
                    rb.velocity += glm::vec3(0, -9.81f, 0) * dt;
                }

                // Euler Integration
                trans->m_position += rb.velocity * dt;

                // Mark transform as dirty for the Renderer
                trans->m_state = GE::Scene::Components::Transform::TransformState::Dirty;
            }
        }
    }

    void PhysicsSystem::ResolveCollisions() {
        auto* em = ServiceLocator::GetEntityManager();
        auto& sphereArray = em->GetCompArr<GE::Components::SphereCollider>();
        auto& planeArray = em->GetCompArr<GE::Components::PlaneCollider>();

        // Check independent objects against the floor
        for (uint32_t sIdx = 0; sIdx < sphereArray.GetCount(); ++sIdx) {
            auto sID = sphereArray.Index()[sIdx];
            auto& sCol = sphereArray.Data()[sIdx];
            auto* sTrans = em->GetTIComponent<GE::Scene::Components::Transform>(sID);
            auto* sRB = em->GetTIComponent<GE::Components::RigidBody>(sID);

            if (!sTrans) continue;

            for (uint32_t pIdx = 0; pIdx < planeArray.GetCount(); ++pIdx) {
                auto& pCol = planeArray.Data()[pIdx];

                // Fulfills Requirement: Use primitive shapes (Sphere/Plane)
                GE::Physics::Sphere sphere(sTrans->m_position, sCol.radius);
                GE::Physics::Plane plane(pCol.normal * pCol.offset, pCol.normal);

                // Check distance to plane
                float dist = plane.DistanceToPoint(sphere.GetCenter());

                if (dist < sphere.GetRadius()) {
                    // 1. Position Correction (Static Resolution)
                    float penetration = sphere.GetRadius() - dist;
                    sTrans->m_position += plane.GetNormal() * penetration;

                    // 2. Velocity Reflection (Dynamic Response)
                    if (sRB) {
                        // Reflect velocity based on plane normal and restitution
                        sRB->velocity = glm::reflect(sRB->velocity, plane.GetNormal()) * sRB->restitution;
                    }
                }
            }
        }
    }
}
