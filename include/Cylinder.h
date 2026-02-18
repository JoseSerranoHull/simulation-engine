#pragma once
#include "Collider.h"

namespace GE::Physics {
    class Cylinder : public Collider {
    public:
        Cylinder(const glm::vec3& start = glm::vec3(0.0f), const glm::vec3& end = glm::vec3(1.0f, 0.0f, 0.0f), float radius = 1.0f)
            : m_start(start), m_end(end), m_radius(radius) {
            SetPosition((m_start + m_end) * 0.5f);
        }

        const glm::vec3& GetStart() const { return m_start; }
        const glm::vec3& GetEnd() const { return m_end; }
        float GetRadius() const { return m_radius; }

        bool IsInside(const glm::vec3& point) const override {
            glm::vec3 ab = m_end - m_start;
            float abLen2 = glm::dot(ab, ab);
            if (abLen2 <= 1e-12f) return glm::distance(point, m_start) <= m_radius;

            float t = glm::clamp(glm::dot(point - m_start, ab) / abLen2, 0.0f, 1.0f);
            return glm::distance(point, m_start + t * ab) <= m_radius;
        }

        bool Intersects(const Line& line) const override {
            if (line.DistanceToPoint(m_start) <= m_radius) return true;
            if (line.DistanceToPoint(m_end) <= m_radius) return true;
            return false;
        }

    private:
        glm::vec3 m_start, m_end;
        float m_radius;
    };
}