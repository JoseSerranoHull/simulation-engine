#pragma once

/* parasoft-begin-suppress ALL */
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#include <array>
#include <map>
#include <string>
/* parasoft-end-suppress ALL */

// Engine Includes
#include "Mesh.h"
#include "Model.h"
#include "Skybox.h"
#include "ParticleSystem.h"
#include "PostProcessor.h"
#include "Pipeline.h"
#include "VulkanContext.h"

/**
 * @class Renderer
 * @brief Orchestrates the recording of command buffers for the multi-pass rendering pipeline.
 * Manages the sequence of Shadow Mapping, Opaque Forward Rendering, Scene Copying (Refraction),
 * Transparency, and GPU Particle Dispatches.
 */
class Renderer final {
public:
    // --- Functional Constants ---
    static constexpr uint32_t PIPELINE_IDX_SHADOW = 6U;
    static constexpr uint32_t VIEWPORT_COUNT_ONE = 1U;
    static constexpr uint32_t SCISSOR_COUNT_ONE = 1U;
    static constexpr float    DEPTH_CLEAR_VAL = 1.0f;

    /** @brief Constructor: Links the renderer to the global Vulkan context. */
    explicit Renderer() {}

    /** @brief Destructor: Standard default as this class does not own heavy GPU handles. */
    ~Renderer() = default;

    // RAII safety: Prevent copying of the global frame orchestrator to maintain state integrity.
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    /**
     * @brief Orchestrates the full frame recording sequence.
     * Transitions from depth pre-passes to the final post-processed output.
     */
    void recordFrame(
        const VkCommandBuffer cb,
        const VkExtent2D& extent,
        const std::map<std::string, std::unique_ptr<Model>>& models,
        const std::vector<std::unique_ptr<Model>>& ownedModels,
        const std::vector<Mesh*>& opaqueMeshes,
        const std::vector<Mesh*>& transparentMeshes,
        const Skybox* const skybox,                // FIX: Added const for MISRA
        const ParticleSystem* const dustSystem,
        const ParticleSystem* const fireSystem,
        const ParticleSystem* const smokeSystem,
        const ParticleSystem* const rainSystem,
        const ParticleSystem* const snowSystem,
        const PostProcessor* const postProcessor,
        const VkDescriptorSet globalDescriptorSet,
        const VkRenderPass shadowPass,
        const VkFramebuffer shadowFramebuffer,
        const std::vector<Pipeline*>& pipelines,
        const bool enableDust,
        const bool enableFire,
        const bool enableSmoke,
        const bool enableRain,
        const bool enableSnow
    ) const;

private:
	// --- Private Pass-Specific Recorders ---

    /** @brief Records Compute dispatches and Graphics draw calls for all particle systems.
     * Updated to const ParticleSystem* to satisfy MISRA2004.16_7.
     */
    void recordParticlePass(
        const VkCommandBuffer cb,
        const ParticleSystem* const dust,
        const ParticleSystem* const fire,
        const ParticleSystem* const smoke,
        const ParticleSystem* const rain,
        const ParticleSystem* const snow,
        const bool dustEnabled,
        const bool fireEnabled,
        const bool smokeEnabled,
        const bool rainEnabled,
        const bool snowEnabled,
        const VkDescriptorSet globalSet
    ) const;

    /** @brief Records the depth-only pass for shadow map generation. */
    void recordShadowPass(
        const VkCommandBuffer cb,
        const VkRenderPass renderPass,
        const VkFramebuffer framebuffer,
        const std::map<std::string, std::unique_ptr<Model>>& models,
        const std::vector<std::unique_ptr<Model>>& ownedModels,
        const Pipeline* const shadowPipeline,
        const VkDescriptorSet globalSet
    ) const; // Added const

    /** @brief Records the main forward rendering pass for opaque geometry. */
    void recordOpaquePass(
        const VkCommandBuffer cb,
        const VkExtent2D& extent,
        const std::vector<Mesh*>& opaque,
        const Skybox* const skybox,
        const PostProcessor* const postProcessor,
        const VkDescriptorSet globalSet
    ) const;

    /** @brief Records the alpha-blended pass for glass and environmental effects. */
    void recordTransparentPass(
        const VkCommandBuffer cb,
        const VkExtent2D& extent,
        const std::vector<Mesh*>& transparent,
        const ParticleSystem* const dust,
        const ParticleSystem* const fire,
        const ParticleSystem* const smoke,
        const ParticleSystem* const rain,
        const ParticleSystem* const snow,
        const PostProcessor* const postProcessor,
        const VkDescriptorSet globalSet,
        const bool dustEnabled,
        const bool fireEnabled,
        const bool smokeEnabled,
        const bool rainEnabled,
        const bool snowEnabled
    ) const;
};