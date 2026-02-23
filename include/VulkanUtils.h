#pragma once

/* parasoft-begin-suppress ALL */
#include "libs.h"
#include <vector>
#include <string>
/* parasoft-end-suppress ALL */

#include "../include/Common.h"

namespace GE::Graphics {

/**
 * @class VulkanUtils
 * @brief Static utility library for Vulkan resource management.
 * * This class provides deduplicated helper functions for creating buffers,
 * transitioning image layouts, and managing pipeline state creation.
 * Marked 'final' to satisfy CODSTA-MCPP.23.
 */
class VulkanUtils final {
public:
    // --- Named Constants ---
    static constexpr uint32_t SHIFT_ONE = 1U;
    static constexpr float DEFAULT_ANISOTROPY = 16.0f;
    static constexpr float LOD_ZERO = 0.0f;
    static constexpr int32_t EXTENT_DEPTH_ONE = 1;
    static constexpr int32_t DIVISOR_TWO = 2;

    // --- Core GPU & Command Helpers ---

    /** @brief Queries the hardware for a memory type that satisfies the filter and properties. */
    static uint32_t findMemoryType(const VkPhysicalDevice physicalDevice, const uint32_t typeFilter, const VkMemoryPropertyFlags properties);

    /** @brief Inline helper for buffer requirements (No loops, stays in header). */
    static VkMemoryRequirements getBufferMemoryRequirements(const VkDevice device, const VkBuffer buffer) {
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, buffer, &memRequirements);
        return memRequirements;
    }

    /** @brief Allocates and begins a primary command buffer for one-time GPU transfers. */
    static VkCommandBuffer beginSingleTimeCommands(const VkDevice device, const VkCommandPool commandPool);

    /** @brief Submits, synchronizes, and frees a one-time command buffer. */
    static void endSingleTimeCommands(const VkDevice device, const VkCommandPool commandPool, const VkQueue graphicsQueue, const VkCommandBuffer commandBuffer);

    // --- Buffer Management ---

    /** @brief Creates a Vulkan buffer and allocates its backing device memory. */
    static void createBuffer(const VkDevice device, const VkPhysicalDevice physicalDevice, const VkDeviceSize size,
        const VkBufferUsageFlags usage, const VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

    /** @brief Executes a GPU-side copy between two buffer resources. */
    static void copyBuffer(const VkDevice device, const VkCommandPool commandPool, const VkQueue graphicsQueue,
        const VkBuffer srcBuffer, const VkBuffer dstBuffer, const VkDeviceSize size);

    // --- GpuImage & Texture Management ---

    /** @brief Creates a Vulkan image and allocates its backing device memory. */
    static void createImage(const VkDevice device, const VkPhysicalDevice physicalDevice, const uint32_t width, const uint32_t height,
        const uint32_t mipLevels, const VkSampleCountFlagBits numSamples, const VkFormat format, const VkImageTiling tiling,
        const VkImageUsageFlags usage, const VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory,
        const uint32_t arrayLayers = 1U, const VkImageCreateFlags flags = 0U);

    /** @brief Generates a view for an image, supporting 2D, Cubemaps, and Mipmap chains. */
    static VkImageView createImageView(const VkDevice device, const VkImage image, const VkFormat format, const VkImageAspectFlags aspectFlags,
        const uint32_t mipLevels, const VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D, const uint32_t layerCount = 1U);

    /** @brief Records a pipeline barrier to change the layout (access pattern) of an image. */
    static void transitionImageLayout(const VkDevice device, const VkCommandPool commandPool, const VkQueue graphicsQueue,
        const VkImage image, const VkFormat format, const VkImageLayout oldLayout, const VkImageLayout newLayout,
        const uint32_t mipLevels, const uint32_t layerCount = 1U);

    /** @brief Executes a copy from a CPU-visible buffer to a GPU-optimal image. */
    static void copyBufferToImage(
        const VkDevice device,
        const VkCommandPool commandPool,
        const VkQueue graphicsQueue,
        const VkBuffer buffer,
        const VkImage image,
        const uint32_t width,
        const uint32_t height,
        const uint32_t layerCount = 1U
    );

    /** @brief Creates a sampler object for texture filtering and mipmap sampling. */
    static void createTextureSampler(const VkDevice device, VkSampler& sampler, const uint32_t mipLevels,
        const VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT);

    /** @brief Procedurally generates a mipmap chain for a texture using blit commands. */
    static void generateMipmaps(const VkDevice device, const VkPhysicalDevice physicalDevice, const VkCommandPool commandPool,
        const VkQueue graphicsQueue, const VkImage image, const VkFormat imageFormat, const int32_t texWidth, const int32_t texHeight,
        const uint32_t mipLevels, const uint32_t layerCount = 1U);

    // --- GraphicsPipeline & Barrier Synchronization ---

    /** @brief Records a high-level image memory barrier for synchronization. */
    static void recordImageBarrier(VkCommandBuffer cb, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout,
        VkAccessFlags srcAccess, VkAccessFlags dstAccess, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, uint32_t mipLevels = 1U);

    /** @brief Creates a minimal render pass used specifically for shadow depth maps. */
    static VkRenderPass createDepthRenderPass(const VkDevice device, const VkFormat depthFormat);

    /** @brief Helper to initialize the complex Graphics GraphicsPipeline creation structure. */
    static VkGraphicsPipelineCreateInfo preparePipelineCreateInfo(
        const VkPipelineShaderStageCreateInfo* shaderStages,
        const VkPipelineVertexInputStateCreateInfo* vertexInput,
        const VkPipelineInputAssemblyStateCreateInfo* inputAssembly,
        const VkPipelineViewportStateCreateInfo* viewport,
        const VkPipelineRasterizationStateCreateInfo* rasterizer,
        const VkPipelineMultisampleStateCreateInfo* multisampling,
        const VkPipelineDepthStencilStateCreateInfo* depthStencil,
        const VkPipelineColorBlendStateCreateInfo* colorBlending,
        const VkPipelineDynamicStateCreateInfo* dynamicState,
        const VkPipelineLayout layout,
        const VkRenderPass renderPass
    );

    // --- GraphicsPipeline State Preparation Helpers ---
    static VkPipelineDynamicStateCreateInfo prepareDynamicState(const std::vector<VkDynamicState>& states);
    static VkPipelineRasterizationStateCreateInfo prepareRasterizer(VkCullModeFlags cullMode);
    static VkPipelineMultisampleStateCreateInfo prepareMultisampling(VkSampleCountFlagBits samples);
    static VkPipelineDepthStencilStateCreateInfo prepareDepthStencil(VkBool32 depthWrite);
};

} // namespace GE::Graphics