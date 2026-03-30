#pragma once

/* parasoft-begin-suppress ALL */
#include <vector>
#include <string>
#include "core/libs.h"
/* parasoft-end-suppress ALL */

#include "graphics/VulkanUtils.h"
#include "particles/Particle.h"
#include "graphics/ShaderModule.h"
#include "core/Common.h"
#include "graphics/VulkanContext.h"
#include "core/ServiceLocator.h"

/**
 * @class GpuParticleBackend
 * @brief Manages GPU-based particle simulation (Compute) and rendering (Graphics).
 * * This system utilizes a Storage Buffer (SSBO) to store particle state, allowing
 * the Compute shader to update physics while the Graphics shader reads them for
 * instantiation and rendering.
 */
class GpuParticleBackend final {
public:
    // --- Lifecycle ---

    /**
     * @brief Full constructor for the Particle System.
     * Orchestrates the creation of compute simulation and graphics rendering pipelines.
     */
    explicit GpuParticleBackend(
        const VkRenderPass renderPass,
        const VkDescriptorSetLayout inGlobalSetLayout,
        const std::string& compPath,
        const std::string& vertPath,
        const std::string& fragPath,
        const glm::vec3& spawnPos,
        const uint32_t maxParticles,
        const VkSampleCountFlagBits inMsaa
    );

    /** @brief Destructor: Releases all compute and graphics GPU resources. */
    ~GpuParticleBackend();

    // RAII: Prevent duplication of GPU handles and mapped memory to maintain ownership.
    GpuParticleBackend(const GpuParticleBackend&) = delete;
    GpuParticleBackend& operator=(const GpuParticleBackend&) = delete;

    // --- Core Execution ---

    /**
     * @brief Dispatches the Compute shader to simulate particle movement.
     * Implementation must be recorded outside of an active RenderPass.
     */
    void update(const VkCommandBuffer commandBuffer, const float deltaTime, const bool spawnEnabled,
        const float totalTime, const glm::vec3& lightColor = glm::vec3(1.0f),
        const glm::vec3& emitterPos = glm::vec3(0.0f));

    /**
    * @brief Records drawing commands for the particles into the graphics stream.
    */
    void draw(const VkCommandBuffer commandBuffer, const VkDescriptorSet globalDescriptorSet) const;

    /**
     * @brief Retrieves dynamic light data from the simulated particles for UBO injection.
     */
    std::vector<SparkLight> getLightData() const;

private:
    /**
     * @struct ParticleUBO
     * @brief Internal UBO for Compute Shader parameters.
     * Aligned to 16-byte boundaries to satisfy GLSL std140 layout requirements.
     */
    struct ParticleUBO {
        float deltaTime;
        float spawnEnabled;
        float totalTime;
        float padding;      /**< Explicit alignment padding. */

        glm::vec3 lightColor;
        float padding2;     /**< Explicit alignment padding. */

        glm::vec3 emitterPos;
        float padding3;     /**< Explicit alignment padding. */
    };

    // --- Named Constants ---
    static constexpr uint32_t COMPUTE_WORKGROUP_SIZE = 256U;
    static constexpr uint32_t BINDING_UBO = 0U;
    static constexpr uint32_t BINDING_STORAGE = 1U;
    static constexpr uint32_t DESCRIPTOR_COUNT_ONE = 1U;
    static constexpr uint32_t SET_INDEX_GLOBAL = 0U;

    // --- Core Dependencies ---
    VkDescriptorSetLayout globalSetLayout;

    // --- Configuration ---
    uint32_t particleCount;
    VkSampleCountFlagBits msaaSamples;
    glm::vec3 lastEmitterPos;

    // --- GPU Storage Resources ---
    VkBuffer storageBuffer;
    VkDeviceMemory storageBufferMemory;

    // --- Uniform Resources ---
    VkBuffer uniformBuffer;
    VkDeviceMemory uniformBufferMemory;
    void* uniformBufferMapped;

    // --- Compute GraphicsPipeline State ---
    VkDescriptorSetLayout computeSetLayout;
    VkPipelineLayout computePipelineLayout;
    VkPipeline computePipeline;
    VkDescriptorPool computeDescriptorPool;
    VkDescriptorSet computeDescriptorSet;

    // --- Graphics GraphicsPipeline State ---
    VkPipelineLayout graphicsPipelineLayout;
    VkPipeline graphicsPipeline;

    // --- Internal Initialization Helpers ---
    void createBuffers(const glm::vec3& spawnPos);
    void createComputeDescriptors();
    void createComputePipeline(const std::string& path);
    void createGraphicsPipeline(const VkRenderPass renderPass, const std::string& vPath, const std::string& fPath);
};