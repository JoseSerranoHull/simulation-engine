#pragma once

/* parasoft-begin-suppress ALL */
#include "../include/libs.h"
#include <vector>
#include <memory>
/* parasoft-end-suppress ALL */

#include "../include/VulkanUtils.h"
#include "../include/Cubemap.h"
#include "../include/ShaderModule.h"
#include "../include/VulkanContext.h"
#include "../include/Common.h"
#include "../include/ServiceLocator.h"

/**
 * @class Skybox
 * @brief Manages the rendering of a cubic environmental background.
 * Uses a unit cube and a cubemap texture to provide a 360-degree background that
 * remains centered on the camera.
 */
class Skybox final {
public:
    static constexpr uint32_t VERT_FLOATS_PER_POS = 3U;

    /**
     * @brief Constructor: Initializes skybox geometry and graphics pipeline.
     */
    explicit Skybox(
        const VkRenderPass renderPass,
        Cubemap* const inTexture,
        const VkSampleCountFlagBits msaaSamples
    );

    /** @brief Destructor: Releases GPU buffers, descriptors, and pipelines. */
    ~Skybox();

    // RAII: Unique ownership of skybox-specific GPU resources
    Skybox(const Skybox&) = delete;
    Skybox& operator=(const Skybox&) = delete;

    /**
     * @brief Records the drawing commands for the skybox.
     * Must be called within an active RenderPass.
     */
    void draw(const VkCommandBuffer commandBuffer, const VkDescriptorSet globalDescriptorSet) const;

    /** * @brief Updates the environmental cubemap using 6 face paths.
     * Rebuilds the GPU image and updates descriptors.
     */
    void loadTextures(const std::vector<std::string>& facePaths);

private:
    // --- Dependencies ---
    std::unique_ptr<Cubemap> cubemapTexture{ nullptr };

    // --- GPU Resources ---
    mutable VkBuffer vertexBuffer{ VK_NULL_HANDLE };
    mutable VkDeviceMemory vertexMemory{ VK_NULL_HANDLE };

    mutable VkDescriptorSetLayout descriptorSetLayout{ VK_NULL_HANDLE };
    mutable VkDescriptorPool descriptorPool{ VK_NULL_HANDLE };
    mutable VkDescriptorSet descriptorSet{ VK_NULL_HANDLE };

    mutable VkPipeline pipeline{ VK_NULL_HANDLE };
    mutable VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };

    VkSampleCountFlagBits msaaSamples{ VK_SAMPLE_COUNT_1_BIT };

    /** * @brief Unit Cube Geometry (36 vertices).
     * Only positions are required as they double as texture coordinates for the cubemap.
     */
    const std::vector<float> vertices = {
        -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f
    };

    // --- Lifecycle Helpers ---
    void createVertexBuffer() const;
    void createDescriptorResources() const;
    void createPipeline(const VkRenderPass renderPass) const;
};