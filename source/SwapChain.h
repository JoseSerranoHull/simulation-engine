#pragma once

/* parasoft-begin-suppress ALL */
#include "libs.h"
#include <vector>
/* parasoft-end-suppress ALL */

#include "VulkanContext.h"

/**
 * @class SwapChain
 * @brief Manages the presentation engine state, including images, views, and framebuffers.
 * * This class encapsulates the Vulkan swapchain lifecycle, providing a centralized
 * registry for the images that will eventually be presented to the display.
 */
class SwapChain final {
public:
    // --- Lifecycle ---

    /** @brief Constructor: Links the swapchain state to the Vulkan context. */
    explicit SwapChain(VulkanContext* const inContext);

    /** @brief Destructor: Triggers cleanup of all hardware presentation handles. */
    ~SwapChain();

    // RAII: Prevent copying to maintain unique ownership of presentation resources.
    SwapChain(const SwapChain&) = delete;
    SwapChain& operator=(const SwapChain&) = delete;

    /** @brief Safely releases all image views, framebuffers, and the swapchain handle. */
    void cleanup();

    // --- Accessors (All Const-Safe) ---

    /** @brief Returns the raw VkSwapchainKHR handle. */
    VkSwapchainKHR getHandle() const { return swapChain; }

    /** @brief Returns the chosen pixel format for the presentation images. */
    VkFormat getFormat() const { return swapChainImageFormat; }

    /** @brief Returns the resolution (width/height) of the swapchain images. */
    const VkExtent2D& getExtent() const { return swapChainExtent; }

    /** @brief Returns the total number of images in the presentation queue. */
    uint32_t getImageCount() const { return static_cast<uint32_t>(swapChainImages.size()); }

    /** @brief Returns the list of raw image handles. */
    const std::vector<VkImage>& getImages() const { return swapChainImages; }

    /** @brief Returns the list of image views associated with the swapchain. */
    const std::vector<VkImageView>& getImageViews() const { return swapChainImageViews; }

    /** @brief Returns the collection of framebuffers bound to these images. */
    const std::vector<VkFramebuffer>& getFramebuffers() const { return swapChainFramebuffers; }

    /** @brief Returns a specific framebuffer by swapchain image index. */
    VkFramebuffer getFramebuffer(uint32_t index) const { return swapChainFramebuffers.at(index); }

    // --- State Management API (MISRA-Compliant) ---

    /** @brief Manually sets the swapchain handle during recreation. */
    void setHandle(VkSwapchainKHR handle) { swapChain = handle; }

    /** @brief Sets the image format for the presentation pass. */
    void setFormat(VkFormat format) { swapChainImageFormat = format; }

    /** @brief Sets the current resolution extent. */
    void setExtent(const VkExtent2D& extent) { swapChainExtent = extent; }

    /** @brief Populates the internal image list from the swapchain engine. */
    void initImages(const std::vector<VkImage>& images);

    /** @brief Registers a new image view into the container. */
    void addImageView(VkImageView view);

    /** @brief Registers a new framebuffer into the container. */
    void addFramebuffer(VkFramebuffer buffer);

private:
    // --- Internal State & Hardware Handles ---
    VulkanContext* context;             /**< Pointer to the centralized Vulkan state. */
    VkSwapchainKHR swapChain;           /**< Hardware handle for the swapchain. */
    VkFormat swapChainImageFormat;      /**< Presentation image format. */
    VkExtent2D swapChainExtent;         /**< Current presentation resolution. */

    std::vector<VkImage> swapChainImages;           /**< Handles for the presentation images. */
    std::vector<VkImageView> swapChainImageViews;   /**< Views required for rendering to the images. */
    std::vector<VkFramebuffer> swapChainFramebuffers; /**< Framebuffers linked to swapchain images. */
};