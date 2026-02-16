#pragma once

/* parasoft-begin-suppress ALL */
#include "libs.h"
/* parasoft-end-suppress ALL */

#include "SimpleAllocator.h"

/**
 * @struct VulkanContext
 * @brief Centralized container for Vulkan hardware handles and global settings.
 * Fulfills the "GameObject" vision by providing a shared state for all ECS systems.
 */
struct VulkanContext {
    // --- 1. Core Instance & Windowing ---
    VkInstance instance{ VK_NULL_HANDLE };
    VkDebugUtilsMessengerEXT debugMessenger{ VK_NULL_HANDLE };
    VkSurfaceKHR surface{ VK_NULL_HANDLE };

    // --- 2. Device Selection & Capabilities ---
    VkPhysicalDevice physicalDevice{ VK_NULL_HANDLE };
    VkDevice device{ VK_NULL_HANDLE };

    /** @brief Global MSAA setting used by Renderer, PostProcessor, and Skybox. */
    VkSampleCountFlagBits msaaSamples{ VK_SAMPLE_COUNT_1_BIT };

    // --- 3. Command Queues ---
    VkQueue graphicsQueue{ VK_NULL_HANDLE };
    VkQueue presentQueue{ VK_NULL_HANDLE };
    VkQueue transferQueue{ VK_NULL_HANDLE };
    VkQueue computeQueue{ VK_NULL_HANDLE }; // Future-proofing for GPU Particles/Physics

    // --- 4. Resource Pools ---
    /** @brief Primary pool for per-frame rendering commands. */
    VkCommandPool graphicsCommandPool{ VK_NULL_HANDLE };

    /** @brief Dedicated pool for the SceneLoader to upload meshes/textures without blocking graphics. */
    VkCommandPool transferCommandPool{ VK_NULL_HANDLE };

    /** @brief Global pool for descriptor set allocations. */
    VkDescriptorPool descriptorPool{ VK_NULL_HANDLE };

    // --- 5. Shared Descriptor Layouts (Agnostic Blueprints) ---
    /** @brief Layout for global data: View, Proj, Lights, Time. */
    VkDescriptorSetLayout globalSetLayout{ VK_NULL_HANDLE };

    /** @brief Layout for material-specific data: Textures, PBR parameters. */
    VkDescriptorSetLayout materialSetLayout{ VK_NULL_HANDLE };

    // --- 6. Memory Management ---
    /** @brief Unified sub-allocation system for VRAM buffers and images. */
    SimpleAllocator allocator{};

    // --- RAII Safety (MRM.49 Compliance) ---
    VulkanContext() = default;
    ~VulkanContext() = default;

    // Prevent copies to avoid double-freeing hardware handles
    VulkanContext(const VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;

    // Move semantics (useful if we transfer ownership between Manager classes)
    VulkanContext(VulkanContext&&) noexcept = default;
    VulkanContext& operator=(VulkanContext&&) noexcept = default;
};