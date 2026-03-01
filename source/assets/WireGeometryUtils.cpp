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

} // namespace GE::Assets
