#pragma once

/* parasoft-begin-suppress ALL */
#include <vector>
#include <glm/glm.hpp>
/* parasoft-end-suppress ALL */

#include "OBJLoader.h"

/**
 * @class GeometryUtils
 * @brief Static procedural geometry factory for generating engine-primitive meshes.
 * * Provides optimized algorithms for creating spherical, cylindrical, and
 * custom terrain-plug geometry used throughout the snow globe simulation.
 */
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
    static OBJLoader::MeshData generateSphere(const uint32_t segments, const float radius, const float cutoffY, const glm::vec3& color = glm::vec3(1.0f));

    /**
     * @brief Generates a displaced circular mesh representing the internal sand terrain.
     */
    static OBJLoader::MeshData generateSandPlug(const uint32_t segments, const float rimRadius, const float bowlRadius, const float depth, const glm::vec3& color = glm::vec3(0.85f, 0.75f, 0.5f));

    /**
     * @brief Generates a cylinder with independent top and bottom radii.
     */
    static OBJLoader::MeshData generateCylinder(const uint32_t segments, const float bottomRadius, const float topRadius, const float height, const glm::vec3& color = glm::vec3(0.25f, 0.15f, 0.1f));
};