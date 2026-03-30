#pragma once
#include "physics/Collider.h"

namespace GE::Physics {
    class Capsule : public Collider {
    public:
        Capsule(const glm::vec3& start = glm::vec3(0.0f), const glm::vec3& end = glm::vec3(1.0f, 0.0f, 0.0f), float radius = 1.0f)
            : m_start(start), m_end(end), m_radius(radius) {
            SetPosition((m_start + m_end) * 0.5f);
        }

        const glm::vec3& GetStart()  const { return m_start; }
        const glm::vec3& GetEnd()    const { return m_end; }
        float            GetRadius() const { return m_radius; }

        bool IsInside(const glm::vec3& point) const override {
            glm::vec3 ab = m_end - m_start;
            float abLen2 = glm::dot(ab, ab);
            if (abLen2 <= 1e-12f) return glm::distance(point, m_start) <= m_radius;

            float t = glm::clamp(glm::dot(point - m_start, ab) / abLen2, 0.0f, 1.0f);
            return glm::distance(point, m_start + t * ab) <= m_radius;
        }

        bool Intersects(const Line& line) const override {
            float dist2 = SegmentSegmentDistanceSq(m_start, m_end, line.GetPoint1(), line.GetPoint2());
            return dist2 <= (m_radius * m_radius);
        }

    private:
        static float SegmentSegmentDistanceSq(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& q1, const glm::vec3& q2) {
            // Implementation of Segment-Segment distance math
            const float EPS = 1e-12f;
            glm::vec3 d1 = p2 - p1, d2 = q2 - q1, r = p1 - q1;
            float a = glm::dot(d1, d1), e = glm::dot(d2, d2), f = glm::dot(d2, r);
            if (a <= EPS && e <= EPS) return glm::dot(p1 - q1, p1 - q1);
            if (a <= EPS) return glm::dot(p1 - (q1 + d2 * glm::clamp(f / e, 0.0f, 1.0f)), p1 - (q1 + d2 * glm::clamp(f / e, 0.0f, 1.0f)));
            float b = glm::dot(d1, d2), c = glm::dot(d1, r), denom = a * e - b * b;
            float s = (denom != 0.0f) ? glm::clamp((b * f - c * e) / denom, 0.0f, 1.0f) : 0.0f;
            float t = glm::clamp((b * s + f) / e, 0.0f, 1.0f);
            return glm::distance2(p1 + d1 * s, q1 + d2 * t);
        }

        glm::vec3 m_start, m_end;
        float m_radius;
    };
}