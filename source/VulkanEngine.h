#pragma once

/* parasoft-begin-suppress ALL */
#include "libs.h"
#include <vector>
#include <optional>
#include <set>
#include <algorithm>
#include <memory>
#include <cstdint>
/* parasoft-end-suppress ALL */

#include "VulkanContext.h"
#include "CommonStructs.h"
#include "SwapChain.h"
#include "Image.h"
#include "RenderPass.h"

/**
 * @struct QueueFamilyIndices
 * @brief Identifies hardware queue support for graphics, presentation, and transfer operations.
 */
struct QueueFamilyIndices final {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    std::optional<uint32_t> transferFamily;

    /** @brief Returns true if all required hardware queues are identified. */
    bool isComplete() const {
        return (graphicsFamily.has_value() && presentFamily.has_value() && transferFamily.has_value());
    }
};

/**
 * @struct SwapChainSupportDetails
 * @brief Encapsulates physical device capabilities for swapchain negotiation.
 */
struct SwapChainSupportDetails final {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

/**
 * @class VulkanEngine
 * @brief The Hardware Abstraction Layer (HAL).
 * * Responsible for physical and logical device management, Swapchain orchestration,
 * and hardware-dependent state such as MSAA levels and Depth formats.
 */
class VulkanEngine final {
public:
    // --- Lifecycle ---

    /** @brief Constructor: Orchestrates the full Vulkan initialization sequence. */
    VulkanEngine(GLFWwindow* const window, VulkanContext* const inContext);

    /** @brief Destructor: Triggers GPU safety checks before releasing hardware handles. */
    ~VulkanEngine();

    // RAII: The hardware engine represents a unique link to the GPU; prevent copying.
    VulkanEngine(const VulkanEngine&) = delete;
    VulkanEngine& operator=(const VulkanEngine&) = delete;

    // --- High-Level Interface ---

    /** @brief Rebuilds the Swapchain and dependent resources after a window resize event. */
    void recreateSwapChain(GLFWwindow* const window);

    /** @brief Safely destroys all resolution-dependent resources during recreation or shutdown. */
    void cleanupSwapChain();

    // --- Hardware & Resource Accessors (Const-Safe) ---

    /** @brief Returns the raw swapchain handle. */
    VkSwapchainKHR getSwapChain() const { return swapChainObj->getHandle(); }

    /** @brief Returns the pixel format used by the presentation engine. */
    VkFormat getSwapChainFormat() const { return swapChainObj->getFormat(); }

    /** @brief Returns the current render resolution. */
    VkExtent2D getSwapChainExtent() const { return swapChainObj->getExtent(); }

    /** @brief Returns the handle for the final presentation render pass. */
    VkRenderPass getFinalRenderPass() const { return finalPass->getHandle(); }

    /** @brief Retrieves the framebuffer for a specific swapchain image index. */
    VkFramebuffer getFramebuffer(const uint32_t index) const { return swapChainObj->getFramebuffer(index); }

    /** @brief Returns the number of images in the swapchain (usually 2 or 3). */
    uint32_t getSwapChainImageCount() const { return swapChainObj->getImageCount(); }

    /** @brief Returns the cached hardware queue indices. */
    const QueueFamilyIndices& getQueueFamilyIndices() const { return queueIndices; }

    /** @brief Returns the best supported depth format for the physical device. */
    VkFormat getDepthFormat() const { return depthFormat; }

    /** @brief Returns the maximum usable MSAA sample count for the current hardware. */
    VkSampleCountFlagBits getMsaaSamples() const { return msaaSamples; }

private:
    // --- Internal State & Hardware Cache ---
    VulkanContext* context;
    VkFormat depthFormat;
    QueueFamilyIndices queueIndices;
    VkSampleCountFlagBits msaaSamples;

    // --- RAII Managed Hardware Objects ---
    std::unique_ptr<SwapChain> swapChainObj;
    std::unique_ptr<Image> depthBuffer;
    std::unique_ptr<RenderPass> finalPass;

    // --- Initialization Pipeline ---
    void initVulkan(GLFWwindow* const window);
    void createInstance() const;
    void setupDebugMessenger() const;
    void createSurface(GLFWwindow* const window) const;
    void pickPhysicalDevice();
    void createLogicalDevice();
    void initAllocator();

    // --- Swapchain Lifecycle Batch ---
    void createSwapChain(GLFWwindow* const window);
    void createImageViews();
    void createDepthResources();
    void createRenderPass();
    void createFramebuffers();

    // --- Hardware Queries & Helpers ---
    QueueFamilyIndices findQueueFamilies(const VkPhysicalDevice device) const;
    SwapChainSupportDetails querySwapChainSupport(const VkPhysicalDevice device) const;
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, const VkImageTiling tiling, const VkFormatFeatureFlags features) const;
    VkFormat findDepthFormat() const;
    VkSampleCountFlagBits getMaxUsableSampleCount() const;
    bool checkValidationLayerSupport() const;
};