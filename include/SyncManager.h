#pragma once

/* parasoft-begin-suppress ALL */
#include <vulkan/vulkan.h>
#include <vector>
#include <cstdint>
/* parasoft-end-suppress ALL */

#include "../include/VulkanContext.h"
#include "../include/ServiceLocator.h"

namespace GE::Graphics {

/**
 * @class SyncManager
 * @brief Orchestrates per-frame synchronization primitives (Semaphores, Fences)
 * and command buffer lifecycles to ensure safe GPU/CPU execution.
 */
class SyncManager final {
public:
    /**
     * @brief Define the maximum number of frames that can be processed concurrently.
     * Usually 2 (Double Buffering) or 3 (Triple Buffering).
     */
    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2U;

    // --- Lifecycle ---

    explicit SyncManager();
    ~SyncManager();

    // RAII: Prevent accidental copies of synchronization handles
    SyncManager(const SyncManager&) = delete;
    SyncManager& operator=(const SyncManager&) = delete;

    void init(uint32_t maxFrames, uint32_t imageCount);

    /** * @brief Allocates command buffers from the provided pool.
     * Satisfies Encapsulation by allowing the class to manage its own private vector.
     */
    void allocateCommandBuffers(VkCommandPool pool, uint32_t count);

    // --- Accessors ---

/** @brief Returns the Command Buffer for a specific frame in flight. */
    VkCommandBuffer getCommandBuffer(const uint32_t index) const {
        return commandBuffers.at(index);
    }

    /** @brief Returns the 'Image Available' semaphore for a specific frame. */
    VkSemaphore getImageAvailableSemaphore(const uint32_t index) const {
        return imageAvailableSemaphores.at(index);
    }

    /** @brief Returns the 'Render Finished' semaphore for a specific frame. */
    VkSemaphore getRenderFinishedSemaphore(const uint32_t index) const {
        return renderFinishedSemaphores.at(index);
    }

    /** @brief Returns the 'In Flight' fence for a specific frame. */
    VkFence getInFlightFence(const uint32_t index) const {
        return inFlightFences.at(index);
    }

    /** @brief Increments and wraps the current frame index. */
    void advanceFrame() {
        currentFrame = (currentFrame + 1U) % MAX_FRAMES_IN_FLIGHT;
    }

    /** @brief Returns the command buffer for the frame currently being processed. */
    VkCommandBuffer getCurrentCommandBuffer() const {
        return commandBuffers.at(currentFrame);
    }

    uint32_t getCurrentFrame() const { return currentFrame; }

private:
    uint32_t currentFrame = 0U;

    std::vector<VkCommandBuffer> commandBuffers{};
    std::vector<VkSemaphore> imageAvailableSemaphores{};
    std::vector<VkSemaphore> renderFinishedSemaphores{};
    std::vector<VkFence> inFlightFences{};
};

} // namespace GE::Graphics