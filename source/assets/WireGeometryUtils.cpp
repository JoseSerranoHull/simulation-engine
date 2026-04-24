#include "assets/GeometryUtils.h"

/* parasoft-begin-suppress ALL */
#include <cmath>
/* parasoft-end-suppress ALL */

namespace GE::Assets {

/**
 * @brief Generates three great-circle rings (XY, XZ, YZ) for a unit sphere wireframe.
 * Each ring has `segments` vertices; indices are LINE_LIST pairs: (0,1),(1,2),...,(N-1,0).
 * Vertex color is baked in. Caller scales via model matrix to match collider radius.
 */
OBJLoader::MeshData GeometryUtils::generateWireSphere(
    const uint32_t segments,
    const glm::vec3& color)
{
    OBJLoader::MeshData data;
    data.name = "wire_sphere";

    // Three great circles: XY plane (c=0), XZ plane (c=1), YZ plane (c=2)
    for (uint32_t c = 0U; c < 3U; ++c) {
        const uint32_t base = static_cast<uint32_t>(data.vertices.size());

        for (uint32_t i = 0U; i < segments; ++i) {
            const float angle = TWO_PI * static_cast<float>(i) / static_cast<float>(segments);
            const float cosA  = std::cos(angle);
            const float sinA  = std::sin(angle);

            GE::Assets::Vertex v{};
            v.color   = color;
            v.texcoord = glm::vec2(FLOAT_ZERO);

            if (c == 0U) {
                // XY plane
                v.position = glm::vec3(cosA, sinA, FLOAT_ZERO);
                v.normal   = glm::vec3(FLOAT_ZERO, FLOAT_ZERO, FLOAT_ONE);
            } else if (c == 1U) {
                // XZ plane
                v.position = glm::vec3(cosA, FLOAT_ZERO, sinA);
                v.normal   = glm::vec3(FLOAT_ZERO, FLOAT_ONE, FLOAT_ZERO);
            } else {
                // YZ plane
                v.position = glm::vec3(FLOAT_ZERO, cosA, sinA);
                v.normal   = glm::vec3(FLOAT_ONE, FLOAT_ZERO, FLOAT_ZERO);
            }

            data.vertices.push_back(v);
        }

        // LINE_LIST: each segment i connects vertex i to vertex (i+1) % segments
        for (uint32_t i = 0U; i < segments; ++i) {
            data.indices.push_back(base + i);
            data.indices.push_back(base + (i + 1U) % segments);
        }
    }

    return data;
}

/**
 * @brief Generates a border quad + cross center in the local XZ plane (y=0).
 * Border: 4 edges connecting the 4 corners.  Cross: 2 lines through the centre.
 * Caller applies orientation (align +Y to plane normal) and translation via model matrix.
 */
OBJLoader::MeshData GeometryUtils::generateWirePlane(
    const float halfSize,
    const glm::vec3& color)
{
    OBJLoader::MeshData data;
    data.name = "wire_plane";

    const glm::vec3 up{ FLOAT_ZERO, FLOAT_ONE, FLOAT_ZERO };

    // 4 corner vertices (indices 0-3)
    const glm::vec3 corners[4] = {
        glm::vec3(-halfSize, FLOAT_ZERO, -halfSize),  // 0: -X, -Z
        glm::vec3( halfSize, FLOAT_ZERO, -halfSize),  // 1: +X, -Z
        glm::vec3( halfSize, FLOAT_ZERO,  halfSize),  // 2: +X, +Z
        glm::vec3(-halfSize, FLOAT_ZERO,  halfSize),  // 3: -X, +Z
    };

    // 4 edge-midpoint vertices for the centre cross (indices 4-7)
    const glm::vec3 midpoints[4] = {
        glm::vec3(-halfSize, FLOAT_ZERO,   FLOAT_ZERO),  // 4: left  midpoint
        glm::vec3( halfSize, FLOAT_ZERO,   FLOAT_ZERO),  // 5: right midpoint
        glm::vec3( FLOAT_ZERO, FLOAT_ZERO, -halfSize),   // 6: front midpoint
        glm::vec3( FLOAT_ZERO, FLOAT_ZERO,  halfSize),   // 7: back  midpoint
    };

    for (const auto& pos : corners) {
        GE::Assets::Vertex v{};
        v.position = pos;
        v.color    = color;
        v.normal   = up;
        v.texcoord = glm::vec2(FLOAT_ZERO);
        data.vertices.push_back(v);
    }

    for (const auto& pos : midpoints) {
        GE::Assets::Vertex v{};
        v.position = pos;
        v.color    = color;
        v.normal   = up;
        v.texcoord = glm::vec2(FLOAT_ZERO);
        data.vertices.push_back(v);
    }

    // Border quad (LINE_LIST): edges 0-1, 1-2, 2-3, 3-0
    data.indices = { 0U, 1U,  1U, 2U,  2U, 3U,  3U, 0U };

    // Centre cross (LINE_LIST): left-to-right, front-to-back
    data.indices.insert(data.indices.end(), { 4U, 5U,  6U, 7U });

    return data;
}

/**
 * @brief Unit wire cylinder shell: two rings at y=±1, radius=1, plus 4 cardinal struts.
 * Scale at draw time: vec3(radius, halfH, radius) for cylinders; same for capsule body.
 */
OBJLoader::MeshData GeometryUtils::generateWireCylinder(
    const uint32_t segments,
    const glm::vec3& color)
{
    OBJLoader::MeshData data;
    data.name = "wire_cylinder";

    // Top ring at y = +1 (vertices 0 .. segments-1)
    for (uint32_t i = 0U; i < segments; ++i) {
        const float angle = TWO_PI * static_cast<float>(i) / static_cast<float>(segments);
        GE::Assets::Vertex v{};
        v.position = glm::vec3(std::cos(angle), FLOAT_ONE, std::sin(angle));
        v.color    = color;
        v.normal   = glm::vec3(FLOAT_ZERO, FLOAT_ONE, FLOAT_ZERO);
        v.texcoord = glm::vec2(FLOAT_ZERO);
        data.vertices.push_back(v);
    }
    for (uint32_t i = 0U; i < segments; ++i) {
        data.indices.push_back(i);
        data.indices.push_back((i + 1U) % segments);
    }

    // Bottom ring at y = -1 (vertices segments .. 2*segments-1)
    for (uint32_t i = 0U; i < segments; ++i) {
        const float angle = TWO_PI * static_cast<float>(i) / static_cast<float>(segments);
        GE::Assets::Vertex v{};
        v.position = glm::vec3(std::cos(angle), -FLOAT_ONE, std::sin(angle));
        v.color    = color;
        v.normal   = glm::vec3(FLOAT_ZERO, -FLOAT_ONE, FLOAT_ZERO);
        v.texcoord = glm::vec2(FLOAT_ZERO);
        data.vertices.push_back(v);
    }
    for (uint32_t i = 0U; i < segments; ++i) {
        data.indices.push_back(segments + i);
        data.indices.push_back(segments + (i + 1U) % segments);
    }

    // 4 cardinal vertical struts (top ring idx → bottom ring idx, same cardinal angle)
    const uint32_t step = segments / 4U;
    for (uint32_t k = 0U; k < 4U; ++k) {
        const uint32_t ci = k * step;
        data.indices.push_back(ci);              // top ring cardinal vertex
        data.indices.push_back(segments + ci);   // bottom ring cardinal vertex
    }

    return data;
}

/**
 * @brief Unit wire hemisphere: equatorial ring at y=0 (radius=1) plus two semicircular arcs
 * in the XY and ZY planes rising from y=0 to y=+1. All at unit scale.
 * For the LOWER cap: negate Y in the scale matrix (scale(r, -r, r)) so the arcs dip to y=-1.
 */
OBJLoader::MeshData GeometryUtils::generateWireHemisphere(
    const uint32_t segments,
    const glm::vec3& color)
{
    OBJLoader::MeshData data;
    data.name = "wire_hemisphere";

    // Equatorial ring at y = 0 (vertices 0 .. segments-1)
    for (uint32_t i = 0U; i < segments; ++i) {
        const float angle = TWO_PI * static_cast<float>(i) / static_cast<float>(segments);
        GE::Assets::Vertex v{};
        v.position = glm::vec3(std::cos(angle), FLOAT_ZERO, std::sin(angle));
        v.color    = color;
        v.normal   = glm::vec3(FLOAT_ZERO, FLOAT_ONE, FLOAT_ZERO);
        v.texcoord = glm::vec2(FLOAT_ZERO);
        data.vertices.push_back(v);
    }
    for (uint32_t i = 0U; i < segments; ++i) {
        data.indices.push_back(i);
        data.indices.push_back((i + 1U) % segments);
    }

    // XY-plane arc: half-circle from (+1,0,0) through (0,+1,0) to (-1,0,0)
    // angle [0..π]: x=cos, y=sin, z=0
    const uint32_t halfSegs = segments / 2U;
    uint32_t base = segments;

    for (uint32_t i = 0U; i <= halfSegs; ++i) {
        const float angle = PI * static_cast<float>(i) / static_cast<float>(halfSegs);
        GE::Assets::Vertex v{};
        v.position = glm::vec3(std::cos(angle), std::sin(angle), FLOAT_ZERO);
        v.color    = color;
        v.normal   = glm::vec3(FLOAT_ZERO, FLOAT_ZERO, FLOAT_ONE);
        v.texcoord = glm::vec2(FLOAT_ZERO);
        data.vertices.push_back(v);
    }
    for (uint32_t i = 0U; i < halfSegs; ++i) {
        data.indices.push_back(base + i);
        data.indices.push_back(base + i + 1U);
    }

    // ZY-plane arc: half-circle from (0,0,+1) through (0,+1,0) to (0,0,-1)
    // angle [0..π]: x=0, y=sin, z=cos
    base = segments + halfSegs + 1U;

    for (uint32_t i = 0U; i <= halfSegs; ++i) {
        const float angle = PI * static_cast<float>(i) / static_cast<float>(halfSegs);
        GE::Assets::Vertex v{};
        v.position = glm::vec3(FLOAT_ZERO, std::sin(angle), std::cos(angle));
        v.color    = color;
        v.normal   = glm::vec3(FLOAT_ONE, FLOAT_ZERO, FLOAT_ZERO);
        v.texcoord = glm::vec2(FLOAT_ZERO);
        data.vertices.push_back(v);
    }
    for (uint32_t i = 0U; i < halfSegs; ++i) {
        data.indices.push_back(base + i);
        data.indices.push_back(base + i + 1U);
    }

    return data;
}

} // namespace GE::Assets
