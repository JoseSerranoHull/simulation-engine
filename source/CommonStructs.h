#pragma once

/* parasoft-begin-suppress ALL */
#include "libs.h"
/* parasoft-end-suppress ALL */

/**
 * @namespace EngineConstants
 * @brief Central repository for global constants to eliminate magic numbers.
 * * This namespace organizes fixed parameters for rendering, memory management,
 * and environmental logic to satisfy Parasoft audit requirements.
 */
namespace EngineConstants {
    // --- Global Simulation & Rendering ---
    static constexpr uint32_t SHADOW_MAP_RES = 2048U;       /**< Resolution for the depth-pass texture. */
    static constexpr uint32_t PARTICLE_POOL_SIZE = 20000U;  /**< Maximum number of active particles. */
    static constexpr uint32_t MAX_SPARK_LIGHTS = 4U;        /**< Max dynamic emitters from particle system. */

    // --- Logic Sanitization ---
    static constexpr int32_t SHADER_FALSE = 0;             /**< Integer-based boolean for shaders. */
    static constexpr int32_t SHADER_TRUE = 1;              /**< Integer-based boolean for shaders. */

    // --- Universal Audit Constants ---
    static constexpr uint32_t INDEX_ZERO = 0U;
    static constexpr uint32_t INDEX_ONE = 1U;
    static constexpr uint32_t INDEX_TWO = 2U;
    static constexpr uint32_t INDEX_THREE = 3U;
    static constexpr uint32_t OFFSET_ZERO = 0U;
    static constexpr uint32_t OFFSET_ONE = 1U;
    static constexpr uint32_t COUNT_ONE = 1U;
    static constexpr VkDeviceSize SZ_ZERO = 0ULL;

    // --- Memory Pool Sizes ---
    // Rule: Explicit U suffixes used for unsigned arithmetic to satisfy MISRA.
    static constexpr uint32_t VRAM_POOL_SIZE = 256U * 1024U * 1024U; /**< Total budget for custom allocator (256MB). */

    // --- Descriptor Set Bindings ---
    static constexpr uint32_t BINDING_UBO = 0U;            /**< Binding for Global UBO (Set 0). */
    static constexpr uint32_t BINDING_SHADOW_SAMPLER = 1U; /**< Binding for Shadow Depth Sampler. */

    // --- Environmental & Orbital Parameters ---
    static const glm::vec3 COLOR_DAY{ 1.0f, 1.0f, 1.0f };
    static const glm::vec3 COLOR_SUNSET{ 1.0f, 0.4f, 0.2f };
    static const glm::vec3 COLOR_NIGHT{ 0.05f, 0.05f, 0.15f };
    static constexpr float SUN_X_OFFSET = 1.5f;
    static constexpr float NIGHT_INTENSITY = 0.5f;
}

/**
 * @struct SparkLight
 * @brief Light data for procedural spark particles, aligned for GPU consumption.
 * * Uses alignas(16) to comply with std140/std430 layout rules for GLSL arrays.
 */
struct SparkLight {
    alignas(16) glm::vec3 position{ 0.0f, 0.0f, 0.0f };
    alignas(16) glm::vec3 color{ 0.0f, 0.0f, 0.0f };
};

/**
 * @struct UniformBufferObject
 * @brief Global Uniform Buffer Object mapping to Set 0, Binding 0.
 * * Encapsulates view-projection matrices, scene-wide lighting data,
 * and dynamic light arrays for the fragment shader.
 */
struct UniformBufferObject {
    // 1. Transformation Matrices
    alignas(16) glm::mat4 view{ 1.0f };
    alignas(16) glm::mat4 proj{ 1.0f };
    alignas(16) glm::mat4 lightSpaceMatrix{ 1.0f };

    // 2. Global Scene Vectors
    alignas(16) glm::vec3 lightPos{ 0.0f, 0.0f, 0.0f };
    alignas(16) glm::vec3 viewPos{ 0.0f, 0.0f, 0.0f };
    alignas(16) glm::vec3 lightColor{ 1.0f, 1.0f, 1.0f };

    // 3. Simulation Logic Flags and Scalars
    int32_t useGouraud{ EngineConstants::SHADER_FALSE };
    float time{ 0.0f };

    // 4. Dynamic Light Data Array
    alignas(16) SparkLight sparks[EngineConstants::MAX_SPARK_LIGHTS]{};
};