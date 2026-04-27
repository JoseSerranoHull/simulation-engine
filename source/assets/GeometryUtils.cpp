#include "assets/GeometryUtils.h"

/* parasoft-begin-suppress ALL */
#include <cmath>
#include <glm/glm.hpp>
/* parasoft-end-suppress ALL */

namespace GE::Assets {

/**
 * @brief Generates a truncated sphere dome for the glass enclosure.
 */
OBJLoader::MeshData GeometryUtils::generateSphere(const uint32_t segments, const float radius, const float cutoffY, const glm::vec3& color) {
    OBJLoader::MeshData data{};
    data.name = "Procedural_Sphere_Dome";

    const float fSegments = static_cast<float>(segments);

    // Step 1: Generate Vertices along the latitude and longitude rings
    for (uint32_t y = 0U; y <= segments; ++y) {
        const float phi = (GeometryUtils::PI * static_cast<float>(y)) / fSegments;
        const float yPos = static_cast<float>(std::cos(static_cast<double>(phi))) * radius;

        // Truncate the sphere if it falls below the specified cutoff height
        if (yPos < cutoffY) {
            continue;
        }

        for (uint32_t x = 0U; x <= segments; ++x) {
            const float theta = (GeometryUtils::TWO_PI * static_cast<float>(x)) / fSegments;
            const float xPos = static_cast<float>(std::sin(static_cast<double>(phi)) * std::cos(static_cast<double>(theta))) * radius;
            const float zPos = static_cast<float>(std::sin(static_cast<double>(phi)) * std::sin(static_cast<double>(theta))) * radius;

            data.vertices.push_back({
                glm::vec3(xPos, yPos, zPos),
                color,
                glm::vec2(static_cast<float>(x) / fSegments, static_cast<float>(y) / fSegments),
                glm::normalize(glm::vec3(xPos, yPos, zPos))
                });
        }
    }

    // Step 2: Generate Indices using a Counter-Clockwise (CCW) winding order
    const uint32_t actualRings = static_cast<uint32_t>(data.vertices.size()) / (segments + 1U);
    for (uint32_t y = 0U; y < (actualRings - 1U); ++y) {
        for (uint32_t x = 0U; x < segments; ++x) {
            const uint32_t first = (y * (segments + 1U)) + x;
            const uint32_t second = first + segments + 1U;

            // Fix: Swapped second and first+1 to achieve CCW winding on the outside
            data.indices.push_back(first);
            data.indices.push_back(first + 1U);
            data.indices.push_back(second);

            data.indices.push_back(second);
            data.indices.push_back(first + 1U);
            data.indices.push_back(second + 1U);
        }
    }
    return data;
}

/**
 * @brief Generates the interior sand terrain with a bowl-shaped underside.
 */
OBJLoader::MeshData GeometryUtils::generatePlug(const uint32_t segments, const float rimRadius, const float bowlRadius, const float depth, const glm::vec3& color) {
    OBJLoader::MeshData data{};
    data.name = "Procedural_Sand_Plug";

    const float fSegments = static_cast<float>(segments);
    const uint32_t numRings = segments / 4U;
    const float fNumRings = static_cast<float>(numRings);

    // Step 1: Generate the Top Dune Surface using sinusoidal displacement
    for (uint32_t r = 0U; r <= numRings; ++r) {
        const float currentRadius = (static_cast<float>(r) / fNumRings) * rimRadius;
        for (uint32_t s = 0U; s <= segments; ++s) {
            const float angle = (static_cast<float>(s) / fSegments) * GeometryUtils::TWO_PI;
            const float x = static_cast<float>(std::cos(static_cast<double>(angle))) * currentRadius;
            const float z = static_cast<float>(std::sin(static_cast<double>(angle))) * currentRadius;

            // Apply height displacement based on dune frequencies
            const float edgeWeight = GeometryUtils::FLOAT_ONE - (currentRadius / rimRadius);
            const float h = static_cast<float>((std::sin(static_cast<double>(x * GeometryUtils::DUNE_FREQ_X)) * static_cast<double>(GeometryUtils::DUNE_AMP_X)) +
                (std::cos(static_cast<double>(z * GeometryUtils::DUNE_FREQ_Z)) * static_cast<double>(GeometryUtils::DUNE_AMP_Z))) * edgeWeight;

            const glm::vec2 uv = glm::vec2(GeometryUtils::FLOAT_HALF + (x / (rimRadius * 2.0f)), GeometryUtils::FLOAT_HALF + (z / (rimRadius * 2.0f))) * GeometryUtils::SAND_TILING;
            data.vertices.push_back({ glm::vec3(x, h, z), color, uv, glm::vec3(0.0f, 1.0f, 0.0f) });
        }
    }

    // Step 2: Index the dune surface rings
    for (uint32_t r = 0U; r < numRings; ++r) {
        const uint32_t startCur = r * (segments + 1U);
        const uint32_t startNext = (r + 1U) * (segments + 1U);
        for (uint32_t s = 0U; s < segments; ++s) {
            const uint32_t curInn = startCur + s;
            const uint32_t nxtInn = startCur + s + 1U;
            const uint32_t curOut = startNext + s;
            const uint32_t nxtOut = startNext + s + 1U;

            data.indices.push_back(curInn);
            data.indices.push_back(nxtInn);
            data.indices.push_back(nxtOut);

            data.indices.push_back(curInn);
            data.indices.push_back(nxtOut);
            data.indices.push_back(curOut);
        }
    }

    // Step 3: Generate the Under-Side Bowl geometry
    const uint32_t bowlStartIdx = static_cast<uint32_t>(data.vertices.size());
    const uint32_t bowlLatSegments = 8U;
    const float fBowlLat = static_cast<float>(bowlLatSegments);

    for (uint32_t y = 1U; y <= bowlLatSegments; ++y) {
        const float phi = GeometryUtils::HALF_PI + (GeometryUtils::HALF_PI * (static_cast<float>(y) / fBowlLat));
        for (uint32_t x = 0U; x <= segments; ++x) {
            const float theta = GeometryUtils::TWO_PI * (static_cast<float>(x) / fSegments);
            const float xPos = static_cast<float>(std::sin(static_cast<double>(phi)) * std::cos(static_cast<double>(theta))) * bowlRadius;
            const float yPos = static_cast<float>(std::cos(static_cast<double>(phi))) * depth;
            const float zPos = static_cast<float>(std::sin(static_cast<double>(phi)) * std::sin(static_cast<double>(theta))) * bowlRadius;

            data.vertices.push_back({
                glm::vec3(xPos, yPos, zPos),
                color,
                glm::vec2(static_cast<float>(x) / fSegments, static_cast<float>(y) / fBowlLat),
                glm::normalize(glm::vec3(xPos, yPos, zPos))
                });
        }
    }

    // Step 4: Stitch the Seam between the top rim and the bowl
    const uint32_t rimStartIdx = numRings * (segments + 1U);
    for (uint32_t s = 0U; s < segments; ++s) {
        const uint32_t topRimCur = rimStartIdx + s;
        const uint32_t topRimNxt = rimStartIdx + s + 1U;
        const uint32_t bowlRimCur = bowlStartIdx + s;
        const uint32_t bowlRimNxt = bowlStartIdx + s + 1U;

        data.indices.push_back(topRimCur);
        data.indices.push_back(bowlRimCur);
        data.indices.push_back(topRimNxt);

        data.indices.push_back(bowlRimCur);
        data.indices.push_back(bowlRimNxt);
        data.indices.push_back(topRimNxt);
    }

    // Step 5: Index the remaining bowl latitudinal segments
    for (uint32_t y = 0U; y < (bowlLatSegments - 1U); ++y) {
        for (uint32_t x = 0U; x < segments; ++x) {
            const uint32_t first = bowlStartIdx + (y * (segments + 1U)) + x;
            const uint32_t second = first + segments + 1U;

            data.indices.push_back(first);
            data.indices.push_back(second);
            data.indices.push_back(first + 1U);

            data.indices.push_back(second);
            data.indices.push_back(second + 1U);
            data.indices.push_back(first + 1U);
        }
    }
    return data;
}

/**
 * @brief Generates a tapered cylinder for the decorative base.
 */
OBJLoader::MeshData GeometryUtils::generateCylinder(const uint32_t segments, const float bottomRadius, const float topRadius, const float height, const glm::vec3& color, bool generateTopCap, bool generateBottomCap) {
    OBJLoader::MeshData data{};
    data.name = "Procedural_Rattan_Base";

    const float fSegments = static_cast<float>(segments);

    // Step 1: Generate Wall Vertices with UV tiling for the rattan texture
    for (uint32_t i = 0U; i <= segments; ++i) {
        const float angle = (static_cast<float>(i) / fSegments) * GeometryUtils::TWO_PI;
        const float u = (static_cast<float>(i) / fSegments) * GeometryUtils::RATTAN_REPEAT_H;

        const float xT = static_cast<float>(std::cos(static_cast<double>(angle))) * topRadius;
        const float zT = static_cast<float>(std::sin(static_cast<double>(angle))) * topRadius;
        const float xB = static_cast<float>(std::cos(static_cast<double>(angle))) * bottomRadius;
        const float zB = static_cast<float>(std::sin(static_cast<double>(angle))) * bottomRadius;

        // Normals slightly angled for the tapered look
        const glm::vec3 norm = glm::normalize(glm::vec3(std::cos(static_cast<double>(angle)), 0.2, std::sin(static_cast<double>(angle))));

        data.vertices.push_back({ glm::vec3(xT,  height * 0.5f, zT), color, glm::vec2(u, GeometryUtils::RATTAN_REPEAT_V), norm });
        data.vertices.push_back({ glm::vec3(xB, -height * 0.5f, zB), color, glm::vec2(u, GeometryUtils::FLOAT_ZERO), norm });
    }

    // Step 2: Index the side walls (strips)
    for (uint32_t i = 0U; i < segments; ++i) {
        const uint32_t t_cur = i * 2U;
        const uint32_t b_cur = (i * 2U) + 1U;
        const uint32_t t_nxt = (i + 1U) * 2U;
        const uint32_t b_nxt = ((i + 1U) * 2U) + 1U;

        data.indices.push_back(t_cur);
        data.indices.push_back(t_nxt);
        data.indices.push_back(b_cur);

        data.indices.push_back(t_nxt);
        data.indices.push_back(b_nxt);
        data.indices.push_back(b_cur);
    }

    if (generateBottomCap) {
        // Step 3: Generate and Index the Bottom Cap
        const uint32_t botCenterIdx = static_cast<uint32_t>(data.vertices.size());
        data.vertices.push_back({ glm::vec3(0.0f, -height * 0.5f, 0.0f), color, glm::vec2(GeometryUtils::FLOAT_HALF, GeometryUtils::FLOAT_HALF), glm::vec3(0.0f, -1.0f, 0.0f) });

        for (uint32_t i = 0U; i <= segments; ++i) {
            const float angle = (static_cast<float>(i) / fSegments) * GeometryUtils::TWO_PI;
            data.vertices.push_back({ glm::vec3(std::cos(static_cast<double>(angle)) * static_cast<double>(bottomRadius), static_cast<double>(-height * 0.5f), std::sin(static_cast<double>(angle)) * static_cast<double>(bottomRadius)), color, glm::vec2(GeometryUtils::FLOAT_ZERO, GeometryUtils::FLOAT_ZERO), glm::vec3(0.0f, -1.0f, 0.0f) });

            if (i > 0U) {
                data.indices.push_back(botCenterIdx);
                data.indices.push_back(botCenterIdx + i);
                data.indices.push_back(botCenterIdx + i + 1U);
            }
        }
    }

    if (generateTopCap) {
        // Step 4: Generate and Index the Top Cap
        // Reversed winding vs bottom cap so the cross product gives normal +Y (front-facing from above).
        const uint32_t topCenterIdx = static_cast<uint32_t>(data.vertices.size());
        data.vertices.push_back({ glm::vec3(0.0f, height * 0.5f, 0.0f), color, glm::vec2(GeometryUtils::FLOAT_HALF, GeometryUtils::FLOAT_HALF), glm::vec3(0.0f, 1.0f, 0.0f) });

        for (uint32_t i = 0U; i <= segments; ++i) {
            const float angle = (static_cast<float>(i) / fSegments) * GeometryUtils::TWO_PI;
            data.vertices.push_back({ glm::vec3(std::cos(static_cast<double>(angle)) * static_cast<double>(topRadius), static_cast<double>(height * 0.5f), std::sin(static_cast<double>(angle)) * static_cast<double>(topRadius)), color, glm::vec2(GeometryUtils::FLOAT_ZERO, GeometryUtils::FLOAT_ZERO), glm::vec3(0.0f, 1.0f, 0.0f) });

            if (i > 0U) {
                data.indices.push_back(topCenterIdx);
                data.indices.push_back(topCenterIdx + i + 1U);  // reversed order → normal +Y
                data.indices.push_back(topCenterIdx + i);
            }
        }
    }
    return data;
}

/**
 * @brief Generates a simple flat plane for the snow globe base.
 */
OBJLoader::MeshData GeometryUtils::generatePlane(float width, float depth) {
    OBJLoader::MeshData data;
    float w2 = width * 0.5f; float d2 = depth * 0.5f;

    // Vertex positions (assuming Y-up world)
    data.vertices = {
        {{-w2, 0.0f, -d2}, {0,1,0}, {0,0}}, // 0: Top-Left
        {{ w2, 0.0f, -d2}, {0,1,0}, {1,0}}, // 1: Top-Right
        {{ w2, 0.0f,  d2}, {0,1,0}, {1,1}}, // 2: Bottom-Right
        {{-w2, 0.0f,  d2}, {0,1,0}, {0,1}}  // 3: Bottom-Left
    };

    // Fix: Changed winding to Counter-Clockwise (0 -> 3 -> 2 and 0 -> 2 -> 1)
    data.indices = { 0, 3, 2, 2, 1, 0 };
    return data;
}

/**
 * @brief Generates a capsule mesh (cylinder with two hemispherical caps).
 * Matches the local Vertex structure (position, color, texcoord, normal).
 */
OBJLoader::MeshData GeometryUtils::generateCapsule(float radius, float height, int segments, int stacks) {
    OBJLoader::MeshData mesh;
    const float halfHeight = height * 0.5f;

    // --- 1. Vertex Generation ---
    // We iterate through the vertical stacks (hemisphere 1 + cylinder + hemisphere 2)
    for (int stack = 0; stack <= 2 * stacks; ++stack) {
        // Vertical angle from -90 to +90 degrees
        float phi = -glm::half_pi<float>() + glm::pi<float>() * (float)stack / (float)(2 * stacks);

        float cosPhi = std::cos(phi);
        float sinPhi = std::sin(phi);

        for (int slice = 0; slice <= segments; ++slice) {
            // Horizontal angle
            float theta = 2.0f * glm::pi<float>() * (float)slice / (float)segments;
            float cosTheta = std::cos(theta);
            float sinTheta = std::sin(theta);

            Vertex v;

            // 1. Position Logic: Offset the y-coordinate based on which cap we are in
            v.position.x = radius * cosPhi * cosTheta;
            v.position.z = radius * cosPhi * sinTheta;

            // If in the bottom hemisphere, shift down; if in top, shift up.
            float yShift = (stack <= stacks) ? -halfHeight : halfHeight;
            v.position.y = (radius * sinPhi) + yShift;

            // 2. Normal Logic: Normalized vector from the center of the respective cap/cylinder
            v.normal = glm::normalize(glm::vec3(
                v.position.x,
                v.position.y - yShift,
                v.position.z
            ));

            // 3. Color Logic: Default to White
            v.color = glm::vec3(1.0f);

            // 4. TexCoord Logic: Map slice to X and stack to Y
            v.texcoord.x = (float)slice / (float)segments;
            v.texcoord.y = (float)stack / (float)(2 * stacks);

            mesh.vertices.push_back(v);
        }
    }

    // --- 2. Index Generation (CCW Winding) ---
    for (int stack = 0; stack < 2 * stacks; ++stack) {
        for (int slice = 0; slice < segments; ++slice) {
            uint32_t first = stack * (segments + 1) + slice;
            uint32_t second = first + (segments + 1);

            // Triangle 1
            mesh.indices.push_back(first);
            mesh.indices.push_back(second);
            mesh.indices.push_back(first + 1);

            // Triangle 2
            mesh.indices.push_back(second);
            mesh.indices.push_back(second + 1);
            mesh.indices.push_back(first + 1);
        }
    }

    return mesh;
}

OBJLoader::MeshData GeometryUtils::generateBox(float sizeX, float sizeY, float sizeZ, const glm::vec3& color)
{
    OBJLoader::MeshData data{};
    data.name = "Procedural_Box";

    const float hw = sizeX * FLOAT_HALF;
    const float hh = sizeY * FLOAT_HALF;
    const float hd = sizeZ * FLOAT_HALF;

    // Per-face vertex layout: CCW winding when viewed from outside.
    // Each face: 4 vertices, 6 indices (2 triangles: [0,1,2] and [0,2,3]).
    // UVs: v0=(0,1), v1=(1,1), v2=(1,0), v3=(0,0).

    struct FaceDef {
        glm::vec3 v[4];
        glm::vec3 normal;
    };

    const FaceDef faces[6] = {
        // +X face
        { { {+hw,-hh,+hd}, {+hw,-hh,-hd}, {+hw,+hh,-hd}, {+hw,+hh,+hd} }, {+1.0f, 0.0f, 0.0f} },
        // -X face
        { { {-hw,-hh,-hd}, {-hw,-hh,+hd}, {-hw,+hh,+hd}, {-hw,+hh,-hd} }, {-1.0f, 0.0f, 0.0f} },
        // +Y face (winding corrected: CCW from above → geometric normal +Y)
        { { {-hw,+hh,+hd}, {+hw,+hh,+hd}, {+hw,+hh,-hd}, {-hw,+hh,-hd} }, {0.0f, +1.0f, 0.0f} },
        // -Y face (winding corrected: CCW from below → geometric normal -Y)
        { { {-hw,-hh,-hd}, {+hw,-hh,-hd}, {+hw,-hh,+hd}, {-hw,-hh,+hd} }, {0.0f, -1.0f, 0.0f} },
        // +Z face
        { { {-hw,-hh,+hd}, {+hw,-hh,+hd}, {+hw,+hh,+hd}, {-hw,+hh,+hd} }, {0.0f, 0.0f, +1.0f} },
        // -Z face
        { { {+hw,-hh,-hd}, {-hw,-hh,-hd}, {-hw,+hh,-hd}, {+hw,+hh,-hd} }, {0.0f, 0.0f, -1.0f} },
    };

    const glm::vec2 uvs[4] = { {0.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f} };

    for (const auto& face : faces) {
        const uint32_t base = static_cast<uint32_t>(data.vertices.size());
        for (uint32_t i = 0U; i < 4U; ++i) {
            data.vertices.push_back({ face.v[i], color, uvs[i], face.normal });
        }
        data.indices.push_back(base + 0U);
        data.indices.push_back(base + 1U);
        data.indices.push_back(base + 2U);
        data.indices.push_back(base + 0U);
        data.indices.push_back(base + 2U);
        data.indices.push_back(base + 3U);
    }

    return data;
}

OBJLoader::MeshData GeometryUtils::generateWireBox(const glm::vec3& color)
{
    OBJLoader::MeshData data{};
    data.name = "Procedural_WireBox";

    // Unit box: half-extents = 1.0 on all axes. Caller scales via model matrix.
    // 8 corners, 12 edges as LINE_LIST (24 indices).
    const glm::vec3 corners[8] = {
        {-1.0f, -1.0f, -1.0f}, // 0
        {+1.0f, -1.0f, -1.0f}, // 1
        {+1.0f, +1.0f, -1.0f}, // 2
        {-1.0f, +1.0f, -1.0f}, // 3
        {-1.0f, -1.0f, +1.0f}, // 4
        {+1.0f, -1.0f, +1.0f}, // 5
        {+1.0f, +1.0f, +1.0f}, // 6
        {-1.0f, +1.0f, +1.0f}, // 7
    };

    for (const auto& c : corners) {
        data.vertices.push_back({ c, color, glm::vec2(FLOAT_ZERO), glm::vec3(FLOAT_ZERO) });
    }

    // Bottom ring (-Z face, -Y)
    data.indices.push_back(0U); data.indices.push_back(1U);
    data.indices.push_back(1U); data.indices.push_back(5U);
    data.indices.push_back(5U); data.indices.push_back(4U);
    data.indices.push_back(4U); data.indices.push_back(0U);
    // Top ring (+Y)
    data.indices.push_back(3U); data.indices.push_back(2U);
    data.indices.push_back(2U); data.indices.push_back(6U);
    data.indices.push_back(6U); data.indices.push_back(7U);
    data.indices.push_back(7U); data.indices.push_back(3U);
    // Vertical edges
    data.indices.push_back(0U); data.indices.push_back(3U);
    data.indices.push_back(1U); data.indices.push_back(2U);
    data.indices.push_back(5U); data.indices.push_back(6U);
    data.indices.push_back(4U); data.indices.push_back(7U);

    return data;
}

} // namespace GE::Assets