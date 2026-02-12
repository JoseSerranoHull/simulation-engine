#include "../include/Image.h"

/**
 * @brief Constructor: Allocates GPU memory and initializes a Vulkan Image and View.
 * Orchestrates the creation of hardware handles via the utility layer.
 */
Image::Image(const uint32_t width, const uint32_t height,
    const VkSampleCountFlagBits samples, const VkFormat fmt,
    const VkImageTiling tiling, const VkImageUsageFlags usage,
    const VkMemoryPropertyFlags properties, const VkImageAspectFlags aspect)
    : format(fmt)
{
	VulkanContext* context = ServiceLocator::GetContext();

    // Step 1: Create the hardware Image handle and allocate backing Device Memory.
    // Mip levels are fixed at 1U for generic offscreen attachments.
    VulkanUtils::createImage(context->device, context->physicalDevice, width, height,
        1U, samples, format, tiling, usage, properties, image, imageMemory);

    // Step 2: Generate the Image View required for framebuffer and descriptor usage.
    imageView = VulkanUtils::createImageView(context->device, image, format, aspect, 1U);
}

/**
 * @brief Destructor: Safely releases GPU resources in reverse order of allocation.
 */
Image::~Image() {
    VulkanContext* context = ServiceLocator::GetContext();

    if ((context != nullptr) && (context->device != VK_NULL_HANDLE)) {

        // Step 1: Destroy the Image View first to avoid dangling references.
        if (imageView != VK_NULL_HANDLE) {
            vkDestroyImageView(context->device, imageView, nullptr);
            imageView = VK_NULL_HANDLE;
        }

        // Step 2: Release the raw Image handle from the hardware device.
        if (image != VK_NULL_HANDLE) {
            vkDestroyImage(context->device, image, nullptr);
            image = VK_NULL_HANDLE;
        }

        // Step 3: Free the actual VRAM memory associated with the resource.
        if (imageMemory != VK_NULL_HANDLE) {
            vkFreeMemory(context->device, imageMemory, nullptr);
            imageMemory = VK_NULL_HANDLE;
        }
    }
}