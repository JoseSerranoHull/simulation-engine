#pragma once

/* parasoft-begin-suppress ALL */
#include <vector>
#include <glm/glm.hpp>
/* parasoft-end-suppress ALL */

#include "assets/OBJLoader.h"

/**
 * @class GeometryUtils
 * @brief Static procedural geometry factory for generating engine-primitive meshes.
 * * Provides optimized algorithms for creating spherical, cylindrical, and
 * custom terrain-plug geometry used throughout the snow globe simulation.
 */
namespace GE::Assets {

class GeometryUtils final {
public:
    // --- Mathematical Constants ---
    static constexpr float PI = 3.14159265f;
    static constexpr float TWO_PI = 6.28318530f;
    static constexpr float HALF_PI = 1.57079632f;
    static constexpr float FLOAT_ONE = 1.0f;
    static constexpr float FLOAT_ZERO = 0.0f;
    static constexpr float FLOAT_HALF = 0.5f;

    // --- Procedural Surface Parameters ---
    static constexpr float SAND_TILING = 8.0f;
    static constexpr float RATTAN_REPEAT_H = 12.0f;
    static constexpr float RATTAN_REPEAT_V = 4.0f;
    static constexpr float DUNE_FREQ_X = 3.0f;
    static constexpr float DUNE_FREQ_Z = 2.5f;
    static constexpr float DUNE_AMP_X = 0.08f;
    static constexpr float DUNE_AMP_Z = 0.05f;

    // --- Core Generation Interface ---

    /**
     * @brief Generates a UV-mapped sphere with an optional Y-axis cutoff.
     */
    static GE::Assets::OBJLoader::MeshData generateSphere(const uint32_t segments, const float radius, const float cutoffY, const glm::vec3& color = glm::vec3(1.0f));

    /**
     * @brief Generates a displaced circular mesh representing the internal sand terrain.
     */
    static GE::Assets::OBJLoader::MeshData generateSandPlug(const uint32_t segments, const float rimRadius, const float bowlRadius, const float depth, const glm::vec3& color = glm::vec3(0.85f, 0.75f, 0.5f));

    /**
     * @brief Generates a cylinder with independent top and bottom radii.
     */
    static GE::Assets::OBJLoader::MeshData generateCylinder(const uint32_t segments, const float bottomRadius, const float topRadius, const float height, const glm::vec3& color = glm::vec3(0.25f, 0.15f, 0.1f));

    /**
     * @brief Generates a simple plane mesh for the base of the snow globe.
	 */
    static GE::Assets::OBJLoader::MeshData generatePlane(float width, float depth);

    /**
     * @brief Generates a capsule mesh by combining hemispherical ends with a cylindrical body.
	 */
	static GE::Assets::OBJLoader::MeshData generateCapsule(float radius, float height, int segments, int stacks);

    // --- Wire / Collider Visualizer Geometry ---

    /**
     * @brief Generates three great-circle rings (XY, XZ, YZ) as LINE_LIST pairs.
     * Vertex color is baked in. Radius = 1.0; caller scales via model matrix.
     * @param segments Number of segments per ring (default 32).
     * @param color    RGB color baked into every vertex (default: #83d42b green).
     */
    static GE::Assets::OBJLoader::MeshData generateWireSphere(
        uint32_t segments = 32U,
        const glm::vec3& color = glm::vec3(0.514f, 0.831f, 0.169f)
    );

    /**
     * @brief Generates a border quad + cross center in the local XZ plane as LINE_LIST pairs.
     * Intended as a visual indicator for PlaneColliders; caller applies orientation via model matrix.
     * @param halfSize Half-extent of the quad in world units (default 5.0).
     * @param color    RGB color baked into every vertex (default: #83d42b green).
     */
    static GE::Assets::OBJLoader::MeshData generateWirePlane(
        float halfSize = 5.0f,
        const glm::vec3& color = glm::vec3(0.514f, 0.831f, 0.169f)
    );
};

} // namespace GE::Assets