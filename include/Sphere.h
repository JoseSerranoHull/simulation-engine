#pragma once
#include "Collider.h"

namespace GE::Physics {
    class Sphere : public Collider {
    public:
        Sphere(const glm::vec3& center = glm::vec3(0.0f), float radius = 1.0f)
            : m_center(center), m_radius(radius) {
        }

        bool IsInside(const glm::vec3& point) const override {
            return glm::distance(point, m_center) <= m_radius;
        }

        bool Intersects(const Line& line) const override {
            // Shortest distance from center to infinite line
            float dist = line.DistanceToPoint(m_center);
            return dist <= m_radius;
        }

        const glm::vec3& GetCenter() const { return m_center; }
        float GetRadius() const { return m_radius; }

    private:
        glm::vec3 m_center;
        float m_radius;
    };
}