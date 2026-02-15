#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "Ray.h"
#include "Sphere.h"
#include "Plane.h"
#include "ServiceLocator.h"
#include "PhysicsComponents.h"
#include "Transform.h"

namespace GE::Physics {
    /**
     * @class Physics
     * @brief The primary gateway for physics world queries.
     * Integrates geometric primitives with ECS component storage.
     */
    class Physics {
    public:
        // --- 1. Basic Primitive Raycasts ---

        /** * @brief Ray vs. Sphere Intersection math.
         */
        static bool Raycast(const Ray& ray, const Sphere& sphere, RaycastHit& outHit, float maxDistance = 1000.0f) {
            glm::vec3 L = sphere.GetCenter() - ray.GetOrigin();
            float tca = glm::dot(L, ray.GetDirection());
            if (tca < 0) return false;

            float d2 = glm::dot(L, L) - tca * tca;
            float r2 = sphere.GetRadius() * sphere.GetRadius();
            if (d2 > r2) return false;

            float thc = sqrt(r2 - d2);
            float t0 = tca - thc;
            float t1 = tca + thc;

            if (t0 < 0) t0 = t1;
            if (t0 < 0 || t0 > maxDistance) return false;

            outHit.hit = true;
            outHit.distance = t0;
            outHit.point = ray.GetPoint(t0);
            outHit.normal = glm::normalize(outHit.point - sphere.GetCenter());
            return true;
        }

        /** * @brief Ray vs. Plane Intersection math.
         */
        static bool Raycast(const Ray& ray, const Plane& plane, RaycastHit& outHit, float maxDistance = 1000.0f) {
            float denom = glm::dot(plane.GetNormal(), ray.GetDirection());

            if (std::abs(denom) > 1e-6f) {
                float t = glm::dot(plane.GetPoint() - ray.GetOrigin(), plane.GetNormal()) / denom;
                if (t >= 0 && t <= maxDistance) {
                    outHit.hit = true;
                    outHit.distance = t;
                    outHit.point = ray.GetPoint(t);
                    outHit.normal = plane.GetNormal();
                    return true;
                }
            }
            return false;
        }

        // --- 2. Advanced World Queries (ECS Integrated) ---

        /** * @brief Casts a ray against all sphere colliders in the Scene.
         * @return Number of hits found.
         */
        static int RaycastAll(const Ray& ray, std::vector<RaycastHit>& outHits, float maxDistance = 1000.0f) {
            auto* em = ServiceLocator::GetEntityManager();
            auto& sphereArray = em->GetCompArr<GE::Components::SphereCollider>();

            int hitCount = 0;
            for (uint32_t i = 0; i < sphereArray.GetCount(); ++i) {
                uint32_t entityID = sphereArray.Index()[i];
                auto& col = sphereArray.Data()[i];
                auto* trans = em->GetTIComponent<GE::Scene::Components::Transform>(entityID);

                if (trans) {
                    Sphere s(trans->m_position, col.radius);
                    RaycastHit hit;
                    if (Raycast(ray, s, hit, maxDistance)) {
                        hitCount++;
                        outHits.push_back(hit);
                    }
                }
            }
            return hitCount;
        }

        /** * @brief Find all sphere colliders overlapping a point, storing IDs in a buffer.
         * Fulfills the "NonAlloc" requirement for memory efficiency.
         * @return Number of colliders found and stored in the buffer.
         */
        static int OverlapSphereNonAlloc(const glm::vec3& center, float radius, uint32_t* resultsBuffer, int bufferSize) {
            auto* em = ServiceLocator::GetEntityManager();
            auto& sphereArray = em->GetCompArr<GE::Components::SphereCollider>();

            int foundCount = 0;
            for (uint32_t i = 0; i < sphereArray.GetCount() && foundCount < bufferSize; ++i) {
                uint32_t entityID = sphereArray.Index()[i];
                auto& col = sphereArray.Data()[i];
                auto* trans = em->GetTIComponent<GE::Scene::Components::Transform>(entityID);

                if (trans) {
                    float distanceSq = glm::distance2(center, trans->m_position);
                    float combinedRadius = radius + col.radius;

                    if (distanceSq <= (combinedRadius * combinedRadius)) {
                        resultsBuffer[foundCount++] = entityID;
                    }
                }
            }
            return foundCount;
        }
    };
}