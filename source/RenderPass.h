#pragma once

/* parasoft-begin-suppress ALL */
#include "libs.h"
/* parasoft-end-suppress ALL */

#include "VulkanContext.h"

/**
 * @class RenderPass
 * @brief RAII wrapper for a VkRenderPass handle.
 * Ensures that the render pass is explicitly destroyed when this object goes out of scope,
 * preventing memory leaks in the Vulkan driver.
 */
class RenderPass final {
private:
    VulkanContext* context{ nullptr };
    VkRenderPass   renderPass{ VK_NULL_HANDLE };

public:
    /**
     * @brief Constructor: Takes ownership of an existing Render Pass handle.
     */
    RenderPass(VulkanContext* const ctx, const VkRenderPass pass);

    /** @brief Destructor: Releases the render pass handle via vkDestroyRenderPass. */
    ~RenderPass();

    // RAII Safety: Prevent accidental copying of the hardware handle.
    RenderPass(const RenderPass&) = delete;
    RenderPass& operator=(const RenderPass&) = delete;

    /** @brief Returns the raw Vulkan handle for use in command recording or framebuffers. */
    VkRenderPass getHandle() const { return renderPass; }
};