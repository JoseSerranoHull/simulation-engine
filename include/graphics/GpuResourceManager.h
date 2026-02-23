#pragma once

/* parasoft-begin-suppress ALL */
#include "core/libs.h"
#include <vector>
#include <memory>
/* parasoft-end-suppress ALL */

#include "graphics/VulkanUtils.h"
#include "graphics/VulkanDevice.h"
#include "graphics/VulkanContext.h"
#include "graphics/FrameSyncManager.h"
#include "assets/AssetManager.h"
#include "graphics/PostProcessBackend.h"

namespace GE::Graphics {

/**
 * @class GpuResourceManager
 * @brief Orchestrates the lifecycle of global GPU resources and descriptor management.
 * * This manager handles the allocation of global uniform buffers, the creation of
 * the shadow mapping sub-system, and the synchronization primitives required for
 * frame-overlapping (Double/Triple buffering).
 */
class GpuResourceManager final {
public:
    // --- Lifecycle ---

    /** @brief Constructor: Links the manager to the centralized Vulkan hardware context. */
    explicit GpuResourceManager();

    /** @brief Destructor: Triggers the full cleanup of allocated GPU handles. */
    ~GpuResourceManager();

    // RAII: Prevent duplication to ensure explicit ownership of GPU memory and handles.
    GpuResourceManager(const GpuResourceManager&) = delete;
    GpuResourceManager& operator=(const GpuResourceManager&) = delete;

    // --- Core Resource API ---

    /** @brief Initializes the resource manager and child synchronization systems. */
    void init(const VulkanDevice* const engine, const uint32_t maxFrames);

    /** @brief Defines the Descriptor Set Layouts for global UBO access. */
    void createLayouts() const;

    /** @brief Reserves memory pools for command recording and descriptor allocation. */
    void createPools(const VulkanDevice* const engine);

    /** @brief Initializes the textures and render passes required for Shadow Mapping. */
    void createShadowResources(const VulkanDevice* const engine);

    /** @brief Allocates and maps memory for the per-frame Uniform Buffer Objects (UBO). */
    void createUniformBuffers(const uint32_t imageCount);

    /** @brief Links allocated UBOs and shadow maps to the GPU Descriptor Sets. */
    void updateDescriptorSets(const VulkanDevice* const engine, const PostProcessBackend* const postProcessor);

    /** @brief Safely releases all managed Vulkan handles and mapped memory. */
    void cleanup();

    // --- Accessors (Const-Safe) ---

    /** @brief Returns the manager responsible for CPU-GPU fence synchronization. */
    FrameSyncManager* getSyncManager() const { return syncManager.get(); }

    /** @brief Returns the pool used for allocating global descriptor sets. */
    VkDescriptorPool getDescriptorPool() const { return descriptorPool; }

    /** @brief Returns the Descriptor Set (Set 0) for a specific frame index. */
    VkDescriptorSet getDescriptorSet(uint32_t index) const { return descriptorSets[index]; }

    /** @brief Returns the CPU-mapped pointer for writing UBO data for a specific frame. */
    void* getMappedBuffer(uint32_t index) const { return uniformBuffersMapped[index]; }

    /** @brief Returns the render pass used for depth-only shadow recording. */
    VkRenderPass getShadowRenderPass() const { return shadowRenderPass; }

    /** @brief Returns the framebuffer target for shadow map generation. */
    VkFramebuffer getShadowFramebuffer() const { return shadowFramebuffer; }

private:
    // --- Context & Synchronization ---
    std::unique_ptr<FrameSyncManager> syncManager;

    // --- Global Pools ---
    VkDescriptorPool descriptorPool;
    VkCommandPool transferCommandPool;

    // --- Descriptor State ---
    std::vector<VkDescriptorSet> descriptorSets;

    // --- Uniform Buffer Resources (Per-Frame) ---
    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;

    // --- Shadow Map Resources ---
    VkImage shadowImage;
    VkDeviceMemory shadowImageMemory;
    VkImageView shadowImageView;
    VkSampler shadowSampler;
    VkRenderPass shadowRenderPass;
    VkFramebuffer shadowFramebuffer;
};

} // namespace GE::Graphics