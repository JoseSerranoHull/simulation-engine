#pragma once
#include "Collider.h"

namespace GE::Physics {
    class Plane : public Collider {
    public:
        Plane(const glm::vec3& point = glm::vec3(0.0f), const glm::vec3& normal = glm::vec3(0.0f, 1.0f, 0.0f))
            : m_point(point), m_normal(glm::normalize(normal)) {
        }

        bool IsInside(const glm::vec3& point) const override {
            return glm::dot(m_normal, point - m_point) <= 0.0f;
        }

        bool Intersects(const Line& line) const override {
            float denom = glm::dot(m_normal, line.Direction());
            return std::abs(denom) > 1e-6f; // If not parallel, it intersects at some point
        }

        float DistanceToPoint(const glm::vec3& point) const {
            return glm::dot(m_normal, point - m_point);
        }

        const glm::vec3& GetNormal() const { return m_normal; }
        const glm::vec3& GetPoint() const { return m_point; }

    private:
        glm::vec3 m_point;
        glm::vec3 m_normal;
    };
}