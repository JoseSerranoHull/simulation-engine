#include "../include/RenderPass.h"

/**
 * @brief Initializes the wrapper. Note: This class borrows the device pointer
 * from the context to perform destruction later.
 */
RenderPass::RenderPass(const VkRenderPass pass)
    : renderPass(pass)
{
}

/**
 * @brief Destructor: Performs the mandatory Vulkan cleanup.
 * Validation layers will flag an error if a Render Pass is destroyed while
 * commands using it are still in flight; ensure SyncManager has fenced the frame.
 */
RenderPass::~RenderPass() {
    VulkanContext* context = ServiceLocator::GetContext();

    // Parasoft Fix: Explicit safety checks for borrowed context and hardware handles
    if ((context != nullptr) && (context->device != VK_NULL_HANDLE)) {
        if (renderPass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(context->device, renderPass, nullptr);

            // Explicitly nullify to prevent use-after-free in complex teardowns
            renderPass = VK_NULL_HANDLE;
        }
    }
    context = nullptr;
}