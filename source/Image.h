#pragma once

/* parasoft-begin-suppress ALL */
#include <vulkan/vulkan.h>
/* parasoft-end-suppress ALL */

#include "VulkanContext.h"
#include "VulkanUtils.h"
#include "ServiceLocator.h"

/**
 * @class Image
 * @brief A generic wrapper for Vulkan Image resources, including their memory and views.
 * * This class encapsulates the creation, memory allocation, and view generation
 * for offscreen attachments, MSAA resolve targets, and depth-stencil buffers.
 */
class Image final {
public:
    // --- Lifecycle ---

    /**
     * @brief Constructor: Allocates and initializes a GPU image with a corresponding view.
     */
    Image(const uint32_t width, const uint32_t height,
        const VkSampleCountFlagBits samples, const VkFormat fmt, const VkImageTiling tiling,
        const VkImageUsageFlags usage, const VkMemoryPropertyFlags properties,
        const VkImageAspectFlags aspect);

    /** @brief Destructor: Releases all image, memory, and view handles from the GPU. */
    ~Image();

    // RAII: Prevent copying to maintain strict, unique ownership of GPU memory handles.
    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;

    // --- Accessors ---

    /** @brief Returns the raw VkImage handle. */
    VkImage getHandle() const { return image; }

    /** @brief Returns the VkImageView used for descriptor binding or framebuffers. */
    VkImageView getView() const { return imageView; }

    /** @brief Returns the pixel format of this image. */
    VkFormat getFormat() const { return format; }

private:
    // --- Internal State & GPU Handles ---
    VkImage image;               /**< Raw hardware image handle. */
    VkDeviceMemory imageMemory;  /**< Dedicated memory allocation for this image. */
    VkImageView imageView;       /**< View handle for shader and framebuffer access. */
    VkFormat format;             /**< The pixel layout format for this image. */
};