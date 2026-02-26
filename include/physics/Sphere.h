#pragma once
#include "physics/Collider.h"
#include "physics/Plane.h"
#include "physics/Cylinder.h"
#include "physics/Capsule.h"
#include <cmath>

namespace GE::Physics {

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

        /**
         * @brief Sphere-Cylinder intersection (finite capped cylinder).
         * Tests the lateral body and both flat end caps.
         * Ported from PhysicsLibrary/Sphere.h.
         */
        bool Intersects(const Cylinder& cyl) const {
            const glm::vec3 A  = cyl.GetStart();
            const glm::vec3 B  = cyl.GetEnd();
            const float     R  = cyl.GetRadius();
            const glm::vec3 AB = B - A;
            const float     L2 = glm::dot(AB, AB);
            const float     eps = 1e-12f;

            // Degenerate cylinder → treat as sphere at A with radius R
            if (L2 <= eps) {
                const glm::vec3 d = m_center - A;
                const float rSum  = m_radius + R;
                return glm::dot(d, d) <= rSum * rSum;
            }

            const float L    = std::sqrt(L2);
            const glm::vec3 u = AB / L;               // unit axis
            const glm::vec3 AC = m_center - A;
            const float proj   = glm::dot(AC, u);
            const float t      = proj / L;             // normalised [0,1] if within body

            if (t >= 0.0f && t <= 1.0f) {
                // Projection lands on the lateral body
                const float radialLen = glm::length(m_center - (A + proj * u));
                return radialLen <= (R + m_radius);
            }
            else if (t < 0.0f) {
                // Closest to start cap at A
                const glm::vec3 radialVec = AC - proj * u;
                const float     radialLen = glm::length(radialVec);
                if (radialLen <= R) return std::fabs(proj) <= m_radius;
                const float rim = radialLen - R;
                return std::sqrt(rim * rim + proj * proj) <= m_radius;
            }
            else {
                // Closest to end cap at B
                const glm::vec3 BC    = m_center - B;
                const float     projB = glm::dot(BC, u);
                const glm::vec3 radialVec = BC - projB * u;
                const float     radialLen = glm::length(radialVec);
                if (radialLen <= R) return std::fabs(projB) <= m_radius;
                const float rim = radialLen - R;
                return std::sqrt(rim * rim + projB * projB) <= m_radius;
            }
        }

        /**
         * @brief Sphere-Capsule intersection (segment with rounded ends).
         * Finds closest point on the capsule axis segment and compares combined radii.
         * Ported from PhysicsLibrary/Sphere.h.
         */
        bool Intersects(const Capsule& cap) const {
            const glm::vec3 A    = cap.GetStart();
            const glm::vec3 B    = cap.GetEnd();
            const float     R    = cap.GetRadius();
            const glm::vec3 AB   = B - A;
            const float     ab2  = glm::dot(AB, AB);
            const float     eps  = 1e-12f;

            // Degenerate capsule → sphere-sphere test
            if (ab2 <= eps) {
                const glm::vec3 d = m_center - A;
                const float rSum  = m_radius + R;
                return glm::dot(d, d) <= rSum * rSum;
            }

            // Closest point on segment to sphere centre (clamped to [0,1])
            const float t       = glm::clamp(glm::dot(m_center - A, AB) / ab2, 0.0f, 1.0f);
            const glm::vec3 closest = A + t * AB;
            const glm::vec3 diff    = m_center - closest;
            const float rSum        = m_radius + R;
            return glm::dot(diff, diff) <= rSum * rSum;
        }

    private:
        glm::vec3 m_center;
        float m_radius;
    };
}