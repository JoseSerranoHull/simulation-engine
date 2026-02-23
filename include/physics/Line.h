#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>

class Line {
public:
    Line(const glm::vec3& p1 = glm::vec3(0.0f), const glm::vec3& p2 = glm::vec3(1.0f, 0.0f, 0.0f))
        : m_point1(p1), m_point2(p2) {
    }

    const glm::vec3& GetPoint1() const { return m_point1; }
    const glm::vec3& GetPoint2() const { return m_point2; }

    // Direction from point1 to point2 (not normalized)
    glm::vec3 Direction() const { return m_point2 - m_point1; }

    /** @brief Returns a unit-length direction vector. Handles degenerate lines. */
    glm::vec3 Normalize() const {
        glm::vec3 dir = Direction();
        float len2 = glm::length2(dir);
        const float eps = 1e-12f;
        if (len2 <= eps) return glm::vec3(0.0f);
        return glm::normalize(dir);
    }

    void SetPoints(const glm::vec3& p1, const glm::vec3& p2) {
        m_point1 = p1;
        m_point2 = p2;
    }

    /** @brief Computes the shortest distance from an arbitrary point to this infinite line. */
    float DistanceToPoint(const glm::vec3& point) const {
        glm::vec3 dir = Direction();
        float len2 = glm::length2(dir);
        const float eps = 1e-12f;
        if (len2 <= eps) return glm::length(point - m_point1);

        glm::vec3 unitDir = glm::normalize(dir);
        glm::vec3 v = point - m_point1;
        float proj = glm::dot(v, unitDir);
        glm::vec3 adjacent = unitDir * proj;
        glm::vec3 perp = v - adjacent;
        return glm::length(perp);
    }

private:
    glm::vec3 m_point1 = glm::vec3(0.0f);
    glm::vec3 m_point2 = glm::vec3(1.0f, 0.0f, 0.0f);
};