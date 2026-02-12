/* parasoft-begin-suppress ALL */
#include "../include/SyncManager.h"
#include <iostream>
/* parasoft-end-suppress ALL */

/**
 * @brief Constructor: Links the manager to the centralized Vulkan context.
 */
SyncManager::SyncManager() {
    // Note: The actual allocation of buffers and primitives is handled by 
    // VulkanResourceManager to satisfy hardware dependencies during init.
}

/**
 * @brief Destructor: Ensures all GPU synchronization primitives are safely destroyed.
 */
SyncManager::~SyncManager() {
    try {
        VulkanContext* context = ServiceLocator::GetContext();
        if ((context != nullptr) && (context->device != VK_NULL_HANDLE)) {

            // 1. Cleanup per-frame semaphores and fences
            const size_t syncCount = imageAvailableSemaphores.size();
            for (size_t i = 0U; i < syncCount; ++i) {
                if (imageAvailableSemaphores[i] != VK_NULL_HANDLE) {
                    vkDestroySemaphore(context->device, imageAvailableSemaphores[i], nullptr);
                }
                if (inFlightFences[i] != VK_NULL_HANDLE) {
                    vkDestroyFence(context->device, inFlightFences[i], nullptr);
                }
            }

            // 2. Cleanup per-swapchain-image semaphores
            for (const VkSemaphore semaphore : renderFinishedSemaphores) {
                if (semaphore != VK_NULL_HANDLE) {
                    vkDestroySemaphore(context->device, semaphore, nullptr);
                }
            }

            // 3. Command Buffer Note: Handled by Pool destruction.
            // We wrap the print in the try-block to catch exceptions from operator<<
            std::cout << "Engine: SyncManager Cleaned Up." << std::endl;
        }
    }
    catch (...) {
        // Exception "swallowed" to ensure exception safety during destruction.
    }
}

/**
 * @brief Initializes synchronization primitives.
 */
void SyncManager::init(uint32_t maxFrames, uint32_t imageCount) {
    commandBuffers.resize(maxFrames);
    imageAvailableSemaphores.resize(maxFrames);
    inFlightFences.resize(maxFrames);
    renderFinishedSemaphores.resize(imageCount);

    const VkSemaphoreCreateInfo semInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0U };

    VkFenceCreateInfo fenceInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VulkanContext* context = ServiceLocator::GetContext();

    for (uint32_t i = 0U; i < maxFrames; ++i) {
        // Accessing ctx->device is allowed because ctx is a pointer to a constant VulkanContext.
        static_cast<void>(vkCreateSemaphore(context->device, &semInfo, nullptr, &imageAvailableSemaphores[i]));
        static_cast<void>(vkCreateFence(context->device, &fenceInfo, nullptr, &inFlightFences[i]));
    }

    for (uint32_t i = 0U; i < imageCount; ++i) {
        static_cast<void>(vkCreateSemaphore(context->device, &semInfo, nullptr, &renderFinishedSemaphores[i]));
    }
}

/**
 * @brief Allocates command buffers.
 */
void SyncManager::allocateCommandBuffers(VkCommandPool pool, uint32_t count) {
    commandBuffers.resize(static_cast<size_t>(count));

    VkCommandBufferAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    allocInfo.commandPool = pool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = count;

    VulkanContext* context = ServiceLocator::GetContext();

    if (vkAllocateCommandBuffers(context->device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("SyncManager: Failed to allocate hardware command buffers!");
    }
}