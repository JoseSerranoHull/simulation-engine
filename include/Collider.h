#pragma once
#include <glm/glm.hpp>
#include "Line.h"

namespace GE::Physics {
    /**
     * @class Collider
     * @brief Base interface for all geometric collision volumes.
     * Incorporates material properties and positional state for Simulation Lab 3.
     */
    class Collider {
    public:
        virtual ~Collider() = default;

        /** @brief Point-in-volume test. */
        virtual bool IsInside(const glm::vec3& point) const = 0;

        /** @brief Line-intersection test. Treated as a segment test for movement. */
        virtual bool Intersects(const Line& line) const = 0;

        // --- State Management ---

        void SetPosition(const glm::vec3& pos) { m_position = pos; }
        const glm::vec3& GetPosition() const { return m_position; }

        /** @brief Sets coefficient of restitution (0.0 = no bounce, 1.0 = perfect bounce). */
        void SetElasticity(float e) { m_elasticity = glm::clamp(e, 0.0f, 1.0f); }
        float GetElasticity() const { return m_elasticity; }

    protected:
        glm::vec3 m_position = glm::vec3(0.0f);
        float m_elasticity = 1.0f; /**< Fulfills Lab 3 Bonus Requirement for reflections. */
    };
}