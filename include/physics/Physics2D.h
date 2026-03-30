#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "core/ServiceLocator.h"
#include "components/PhysicsComponents.h"
#include "components/Transform.h"

namespace GE::Physics {

    struct RaycastHit2D {
        bool hit = false;
        float distance = 0.0f;
        glm::vec2 point = glm::vec2(0.0f);
        glm::vec2 normal = glm::vec2(0.0f);
        uint32_t entityID = 0xFFFFFFFF;
    };

    class Physics2D {
    public:
        /** @brief Ray vs Circle intersection logic. */
        static bool Raycast(const glm::vec2& origin, const glm::vec2& direction,
            float radius, const glm::vec2& center, RaycastHit2D& outHit) {
            glm::vec2 L = center - origin;
            float tca = glm::dot(L, direction);
            if (tca < 0) return false;

            float d2 = glm::dot(L, L) - tca * tca;
            float r2 = radius * radius;
            if (d2 > r2) return false;

            float thc = sqrt(r2 - d2);
            float t0 = tca - thc;
            outHit.hit = true;
            outHit.distance = t0;
            outHit.point = origin + direction * t0;
            outHit.normal = glm::normalize(outHit.point - center);
            return true;
        }

        /** @brief OverlapCircleNonAlloc. */
        static int OverlapCircleNonAlloc(const glm::vec2& center, float radius,
            uint32_t* resultsBuffer, int bufferSize) {
            auto* em = ServiceLocator::GetEntityManager();
            auto& circleArray = em->GetCompArr<GE::Components::CircleCollider2D>();

            int count = 0;
            for (uint32_t i = 0; i < circleArray.GetCount() && count < bufferSize; ++i) {
                uint32_t id = circleArray.Index()[i];
                auto& col = circleArray.Data()[i];
                auto* trans = em->GetTIComponent<GE::Components::Transform>(id);

                if (trans) {
                    glm::vec2 worldPos = glm::vec2(trans->m_position.x, trans->m_position.y) + col.offset;
                    float dist = glm::distance(center, worldPos);
                    if (dist <= (radius + col.radius)) {
                        resultsBuffer[count++] = id;
                    }
                }
            }
            return count;
        }
    };
}