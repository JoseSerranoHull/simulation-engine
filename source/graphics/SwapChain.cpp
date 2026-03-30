#include "graphics/SwapChain.h"

namespace GE::Graphics {

/**
 * @brief Constructor: Initializes the container.
 * Note: Hardware handle allocation is typically performed by the GpuResourceManager.
 */
SwapChain::SwapChain()
    : swapChain(VK_NULL_HANDLE),
    swapChainImageFormat(VK_FORMAT_UNDEFINED),
    swapChainExtent{ 0U, 0U }
{
}

/**
 * @brief Destructor: Ensures that all hardware handles are released before the object is destroyed.
 */
SwapChain::~SwapChain() {
    try {
        cleanup();
    }
    catch (...) {
        // We catch all exceptions (the "..." syntax) to prevent std::terminate().
        // In a destructor, "swallowing" an exception is the standard safety 
        // procedure because we are already in the process of shutting down.
    }
}

/**
 * @brief Explicitly destroys all swapchain-related resources in the correct order.
 * Order: Framebuffers -> GpuImage Views -> Swapchain KHR.
 */
void SwapChain::cleanup() {
    VulkanContext* context = ServiceLocator::GetContext();

    // Parasoft Safety: Verify context and device validity before calling Vulkan API
    if ((context != nullptr) && (context->device != VK_NULL_HANDLE)) {

        // 1. Destroy Framebuffers
        // These are the top-level containers for the render targets.
        for (const VkFramebuffer framebuffer : swapChainFramebuffers) {
            if (framebuffer != VK_NULL_HANDLE) {
                vkDestroyFramebuffer(context->device, framebuffer, nullptr);
            }
        }
        swapChainFramebuffers.clear();

        // 2. Destroy GpuImage Views
        // These are the "windows" into the raw swapchain images.
        for (const VkImageView imageView : swapChainImageViews) {
            if (imageView != VK_NULL_HANDLE) {
                vkDestroyImageView(context->device, imageView, nullptr);
            }
        }
        swapChainImageViews.clear();

        // 3. Destroy the Swapchain KHR
        // This releases the link to the OS windowing system.
        if (swapChain != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(context->device, swapChain, nullptr);
            swapChain = VK_NULL_HANDLE;
        }

        /** * @note raw VkImage handles retrieved from the swapchain do NOT need to be destroyed;
         * they are owned and cleaned up by the VkSwapchainKHR itself.
         */
        swapChainImages.clear();
    }
}

/**
 * @brief Transfers ownership of swapchain image handles to this object.
 */
void SwapChain::initImages(const std::vector<VkImage>& images) {
    // Clear old data to prevent stale handles
    swapChainImages.clear();
    swapChainImages = images;
}

/**
 * @brief Adds a managed ImageView handle to the internal collection.
 */
void SwapChain::addImageView(VkImageView view) {
    if (view != VK_NULL_HANDLE) {
        swapChainImageViews.push_back(view);
    }
}

/**
 * @brief Adds a managed Framebuffer handle to the internal collection.
 */
void SwapChain::addFramebuffer(VkFramebuffer buffer) {
    if (buffer != VK_NULL_HANDLE) {
        swapChainFramebuffers.push_back(buffer);
    }
}

} // namespace GE::Graphics