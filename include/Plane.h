#pragma once
#include "Collider.h"

namespace GE::Physics {
    class Plane : public Collider {
    public:
        Plane(const glm::vec3& point = glm::vec3(0.0f), const glm::vec3& normal = glm::vec3(0.0f, 1.0f, 0.0f))
            : m_point(point), m_normal(glm::normalize(normal)) {
            SetPosition(point);
        }

        void SetPoint(const glm::vec3& p) { m_point = p; SetPosition(p); }
        void SetNormal(const glm::vec3& n) { m_normal = glm::normalize(n); }

        const glm::vec3& GetNormal() const { return m_normal; }
        const glm::vec3& GetPoint() const { return m_point; }

        bool IsInside(const glm::vec3& point) const override {
            return glm::dot(m_normal, point - m_point) <= 0.0f;
        }

        bool Intersects(const Line& line) const override {
            float denom = glm::dot(m_normal, line.Direction());
            return std::abs(denom) > 1e-6f;
        }

        /** @brief Returns unsigned distance to the plane. Useful for ground collision thresholds. */
        float DistanceToPoint(const glm::vec3& point) const {
            return std::abs(glm::dot(m_normal, point - m_point));
        }

    private:
        glm::vec3 m_point;
        glm::vec3 m_normal;
    };
}