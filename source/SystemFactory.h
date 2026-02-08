#pragma once

/* parasoft-begin-suppress ALL */
#include <memory>
#include <string>
#include <glm/glm.hpp>
/* parasoft-end-suppress ALL */

#include "VulkanContext.h"
#include "VulkanEngine.h"
#include "PostProcessor.h"
#include "ParticleSystem.h"
#include "PointLight.h"
#include "ClimateManager.h"

/**
 * @class SystemFactory
 * @brief Provides a centralized interface for instantiating high-level engine systems.
 * This factory encapsulates the complex initialization logic required for post-processing,
 * compute-based particles, and environmental lighting.
 */
class SystemFactory final {
public:
    // --- Factory Methods ---

    /**
     * @brief Instantiates the HDR Post-Processing stack.
     */
    static std::unique_ptr<PostProcessor>  createPostProcessingSystem(VulkanEngine* const eng);

    /** @brief Creates the compute-driven Dust particle system. */
    static std::unique_ptr<ParticleSystem> createDustSystem(const VkRenderPass rp, const VkSampleCountFlagBits msaa);

    /** @brief Creates the Fire particle system (Additively blended). */
    static std::unique_ptr<ParticleSystem> createFireSystem(const VkRenderPass rp, const VkSampleCountFlagBits msaa);

    /** @brief Creates the Smoke particle system (Alpha blended). */
    static std::unique_ptr<ParticleSystem> createSmokeSystem(const VkRenderPass rp, const VkSampleCountFlagBits msaa);

    /** @brief Creates the Rain particle system with velocity-aligned stretching. */
    static std::unique_ptr<ParticleSystem> createRainSystem(const VkRenderPass rp, const VkSampleCountFlagBits msaa);

    /** @brief Creates the Snow particle system with oscillating horizontal drift. */
    static std::unique_ptr<ParticleSystem> createSnowSystem(const VkRenderPass rp, const VkSampleCountFlagBits msaa);

    /**
     * @brief Instantiates a dynamic Point Light.
     */
    static std::unique_ptr<PointLight>     createLightSystem(const glm::vec3& pos, const glm::vec3& color, const float intensity);

    /** @brief Creates the logic manager for day/night cycles and weather state. */
    static std::unique_ptr<ClimateManager> createClimateSystem();

private:
    // Static utility class: Constructor and Destructor are private to prevent instantiation.
    SystemFactory() = default;
    ~SystemFactory() = default;
};