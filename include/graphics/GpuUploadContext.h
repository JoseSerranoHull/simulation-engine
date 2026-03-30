#pragma once

/* parasoft-begin-suppress ALL */
#include <vulkan/vulkan.h>
#include <vector>
/* parasoft-end-suppress ALL */

namespace GE::Graphics {
    /**
     * @struct GpuUploadContext
     * @brief Bundles the transient Vulkan resources needed for a GPU upload operation.
     * Passed to Scenario::OnLoad to decouple the scenario lifecycle interface
     * from raw Vulkan staging-buffer management.
     */
    struct GpuUploadContext {
        VkCommandBuffer cmd = VK_NULL_HANDLE;
        std::vector<VkBuffer> stagingBuffers;
        std::vector<VkDeviceMemory> stagingMemories;
    };
}
