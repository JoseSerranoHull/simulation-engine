#pragma once
#include "physics/Collider.h"
#include "physics/Plane.h"
#include <cmath>

namespace GE::Physics {
    class Cylinder; // Forward declaration
    class Capsule;  // Forward declaration

    class Sphere : public Collider {
    public:
        Sphere(glm::vec3 center = glm::vec3(0.0f), float radius = 1.0f)
            : m_center(center), m_radius(radius) {
            SetPosition(center);
        }

        void SetCenter(const glm::vec3& c) { m_center = c; SetPosition(c); }
        void SetRadius(float r) { m_radius = r; }
        const glm::vec3& GetCenter() const { return m_center; }
        float GetRadius() const { return m_radius; }

        bool IsInside(const glm::vec3& point) const override {
            return glm::distance(point, m_center) <= m_radius;
        }

        /** @brief Sphere-Sphere collision check. */
        bool CollideWith(const Sphere& other) const {
            float dist2 = glm::distance2(other.m_center, m_center);
            float radiusSum = m_radius + other.m_radius;
            return dist2 <= (radiusSum * radiusSum);
        }

        /** @brief Checks intersection against infinite line. */
        bool Intersects(const Line& line) const override {
            return line.DistanceToPoint(m_center) <= m_radius;
        }

        /** @brief Checks intersection against finite segment. Used for fast-moving projectile collision. */
        bool IntersectsSegment(const Line& line) const {
            glm::vec3 ab = line.Direction();
            float abLen2 = glm::dot(ab, ab);
            if (abLen2 <= 1e-12f) return IsInside(line.GetPoint1());

            float t = glm::dot(m_center - line.GetPoint1(), ab) / abLen2;
            t = glm::clamp(t, 0.0f, 1.0f);
            glm::vec3 closest = line.GetPoint1() + t * ab;
            return glm::distance(m_center, closest) <= m_radius;
        }

        /** @brief Sphere-Plane intersection. Fulfills Lab 3 Q4 requirements. */
        bool Intersects(const Plane& plane) const {
            return plane.DistanceToPoint(m_center) <= m_radius;
        }

        // Robust tests against new shapes
        bool Intersects(const Cylinder& cyl) const;
        bool Intersects(const Capsule& cap) const;

    private:
        glm::vec3 m_center;
        float m_radius;
    };
}