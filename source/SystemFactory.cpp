#include "SystemFactory.h"

/**
 * @brief Constructs the PostProcessor by extracting swapchain metadata from the engine.
 */
std::unique_ptr<PostProcessor> SystemFactory::createPostProcessingSystem(VulkanEngine* const eng) {
    // Hidden knowledge: Resolution and Format requirements for the PostProcessor logic
    return std::make_unique<PostProcessor>(
        eng->getSwapChainExtent().width,
        eng->getSwapChainExtent().height,
        eng->getSwapChainFormat(),
        eng->getFinalRenderPass(),
        eng->getMsaaSamples()
    );
}

/**
 * @brief Creates the compute-driven Dust system.
 * Spawns in the center of the scene to provide environmental ambiance.
 */
std::unique_ptr<ParticleSystem> SystemFactory::createDustSystem(const VkRenderPass rp, const VkSampleCountFlagBits msaa) {
    // Hidden knowledge: Specific shader paths for the Dust simulation
    VulkanContext* context = ServiceLocator::GetContext();
    return std::make_unique<ParticleSystem>(
        rp, context->globalSetLayout,
        "./shaders/dust_comp.spv", "./shaders/dust_vert.spv", "./shaders/dust_frag.spv",
        glm::vec3(0.0f, 1.2f, 0.0f), 1000U, msaa
    );
}

/**
 * @brief Creates the Fire system.
 * Positioned specifically at the camp-fire location in the desert scene.
 */
std::unique_ptr<ParticleSystem> SystemFactory::createFireSystem(const VkRenderPass rp, const VkSampleCountFlagBits msaa) {
    VulkanContext* context = ServiceLocator::GetContext();
    return std::make_unique<ParticleSystem>(
        rp, context->globalSetLayout,
        "./shaders/fire_comp.spv", "./shaders/fire_vert.spv", "./shaders/fire_frag.spv",
        glm::vec3(-0.8f, -0.15f, -0.5f), 500U, msaa
    );
}

/**
 * @brief Creates the Smoke system.
 * Shares the fire origin but uses a lower particle count for alpha-blending performance.
 */
std::unique_ptr<ParticleSystem> SystemFactory::createSmokeSystem(const VkRenderPass rp, const VkSampleCountFlagBits msaa) {
    VulkanContext* context = ServiceLocator::GetContext();
	return std::make_unique<ParticleSystem>(
		rp, context->globalSetLayout,
        "./shaders/smoke_comp.spv", "./shaders/smoke_vert.spv", "./shaders/smoke_frag.spv",
        glm::vec3(-0.8f, -0.15f, -0.5f),
        250U, // Verified count for compute shader dispatch parity
        msaa
    );
}

/**
 * @brief Creates the Rain system.
 * Spawns at the apex of the glass dome for gravity-based simulation.
 */
std::unique_ptr<ParticleSystem> SystemFactory::createRainSystem(const VkRenderPass rp, const VkSampleCountFlagBits msaa) {
    VulkanContext* context = ServiceLocator::GetContext();
	return std::make_unique<ParticleSystem>(
        rp, context->globalSetLayout,
        "./shaders/rain_comp.spv", "./shaders/rain_vert.spv", "./shaders/rain_frag.spv",
        glm::vec3(0.0f, 1.8f, 0.0f), 5000U, msaa
    );
}

/**
 * @brief Creates the Snow system.
 * Spawns at the apex; uses a 3000U count to balance visibility and GPU overhead.
 */
std::unique_ptr<ParticleSystem> SystemFactory::createSnowSystem(const VkRenderPass rp, const VkSampleCountFlagBits msaa) {
    VulkanContext* context = ServiceLocator::GetContext();
    return std::make_unique<ParticleSystem>(
        rp, context->globalSetLayout,
        "./shaders/snow_comp.spv", "./shaders/snow_vert.spv", "./shaders/snow_frag.spv",
        glm::vec3(0.0f, 1.8f, 0.0f), 3000U, msaa
    );
}

/**
 * @brief Factory for Point Lights.
 */
std::unique_ptr<PointLight> SystemFactory::createLightSystem(const glm::vec3& pos, const glm::vec3& color, const float intensity) {
    return std::make_unique<PointLight>(pos, color, intensity);
}

/**
 * @brief Factory for the Climate logic controller.
 */
std::unique_ptr<ClimateManager> SystemFactory::createClimateSystem() {
    return std::make_unique<ClimateManager>();
}