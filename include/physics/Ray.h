#pragma once
#include <glm/glm.hpp>

namespace GE::Physics {
    /**
     * @class Ray
     * @brief Foundation for semi-infinite line queries.
     */
    class Ray {
    public:
        Ray(const glm::vec3& origin = glm::vec3(0.0f),
            const glm::vec3& direction = glm::vec3(0.0f, 0.0f, 1.0f))
            : m_origin(origin), m_direction(glm::normalize(direction)) {
        }

        const glm::vec3& GetOrigin() const { return m_origin; }
        const glm::vec3& GetDirection() const { return m_direction; }

        /** @brief Returns a point along the ray at distance 't'. */
        glm::vec3 GetPoint(float t) const { return m_origin + m_direction * t; }

    private:
        glm::vec3 m_origin;
        glm::vec3 m_direction;
    };

    /**
     * @struct RaycastHit
     * @brief Stores information about a ray intersection event.
     */
    struct RaycastHit {
        bool hit = false;
        float distance = 0.0f;
        glm::vec3 point = glm::vec3(0.0f);
        glm::vec3 normal = glm::vec3(0.0f);
        // EntityID colliderID = ...; // Link to ECS (Phase 2)
    };
}