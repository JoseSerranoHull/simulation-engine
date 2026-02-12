#pragma once

/* parasoft-begin-suppress ALL */
#include "libs.h"
/* parasoft-end-suppress ALL */

#include "SimpleAllocator.h"

/**
 * @struct VulkanContext
 * @brief Centralized container for Vulkan hardware handles.
 * Explicitly non-copyable to prevent hardware handle double-destruction (MRM.49).
 */
struct VulkanContext {
    // 1. Core Instance & Debugging
    VkInstance instance{ VK_NULL_HANDLE };
    VkDebugUtilsMessengerEXT debugMessenger{ VK_NULL_HANDLE };
    VkSurfaceKHR surface{ VK_NULL_HANDLE };

    // 2. Physical & Logical Devices
    VkPhysicalDevice physicalDevice{ VK_NULL_HANDLE };
    VkDevice device{ VK_NULL_HANDLE };

    // 3. Command Queues
    VkQueue graphicsQueue{ VK_NULL_HANDLE };
    VkQueue presentQueue{ VK_NULL_HANDLE };
    VkQueue transferQueue{ VK_NULL_HANDLE };

    // 4. Resource Pools
    VkCommandPool graphicsCommandPool{ VK_NULL_HANDLE };
    VkDescriptorPool descriptorPool{ VK_NULL_HANDLE };

    // 5. Global Layouts
    VkDescriptorSetLayout globalSetLayout{ VK_NULL_HANDLE };
    VkDescriptorSetLayout materialSetLayout{ VK_NULL_HANDLE };

    // 6. Sub-Allocation System
    SimpleAllocator allocator{};

    // --- MRM.49 Compliance: Explicitly delete copy operations ---

    /** @brief Default constructor for standard initialization. */
    VulkanContext() = default;

    /** @brief Copy constructor deleted to prevent shallow copies of hardware handles. */
    VulkanContext(const VulkanContext&) = delete;

    /** @brief Copy assignment operator deleted to prevent handle aliasing. */
    VulkanContext& operator=(const VulkanContext&) = delete;

    // Optional: If you want to support Move semantics (C++11/14/17)
    VulkanContext(VulkanContext&&) = delete;
    VulkanContext& operator=(VulkanContext&&) = delete;
};