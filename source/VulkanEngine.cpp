#include "../include/VulkanEngine.h"
#include "../include/VulkanUtils.h"

/* parasoft-begin-suppress ALL */
#include <iostream>
#include <cstring>
/* parasoft-end-suppress ALL */

// --- Static Configuration ---
const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

namespace GE::Graphics {

/**
 * @brief Global debug callback for the Vulkan Validation Layers.
 */
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    static_cast<void>(pUserData);

    if (pCallbackData != nullptr) {
        std::cerr << "Vulkan Hardware Layer: " << pCallbackData->pMessage << std::endl;
    }

    return VK_FALSE;
}

// ========================================================================
// SECTION 1: LIFECYCLE MANAGEMENT
// ========================================================================

/**
 * @brief Constructor: Orchestrates the sequential initialization of the Vulkan hardware stack.
 * Satisfies INIT.06 by explicitly initializing all primitives and handles.
 */
VulkanDevice::VulkanDevice(GLFWwindow* const window)
    : depthFormat(VK_FORMAT_UNDEFINED),
    msaaSamples(VK_SAMPLE_COUNT_1_BIT)
{
    VulkanContext* context = ServiceLocator::GetContext();

    // Initialize the SwapChain container immediately to ensure safe tracking
    swapChainObj = std::make_unique<SwapChain>();
    initVulkan(window);
}

/**
 * @brief Destructor: Ensures the GPU is idle before releasing RAII unique_ptrs.
 */
VulkanDevice::~VulkanDevice() {
    VulkanContext* context = ServiceLocator::GetContext();
    if (context != nullptr && context->device != VK_NULL_HANDLE) {
        // Step 1: Wait for GPU to finish all in-flight work
        static_cast<void>(vkDeviceWaitIdle(context->device));
    }
}

/**
 * @brief Primary initialization sequence.
 * Follows strict Vulkan dependency order: Instance -> Surface -> Physical Device -> Logical Device.
 */
void VulkanDevice::initVulkan(GLFWwindow* const window) {
    createInstance();
    setupDebugMessenger();
    createSurface(window);
    pickPhysicalDevice();
    createLogicalDevice();

    // Step 1: Detect hardware-specific capabilities
    depthFormat = findDepthFormat();
    initAllocator();

    // Step 2: Initialize presentation resources
    createSwapChain(window);
    createImageViews();
    createDepthResources();
    createRenderPass();
    createFramebuffers();
}

// ========================================================================
// SECTION 2: CORE INITIALIZATION
// ========================================================================

/**
 * @brief Creates the Vulkan Instance with application metadata and extensions.
 */
void VulkanDevice::createInstance() const {
    if (enableValidationLayers && !checkValidationLayerSupport()) {
        throw std::runtime_error("VulkanDevice: Validation layers requested but unavailable.");
    }

    // Step 1: Define Application Metadata
    VkApplicationInfo appInfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
    appInfo.pApplicationName = "Sandy-Snow Globe Engine";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "VulkanLab Custom Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    createInfo.pApplicationInfo = &appInfo;

    // Step 2: Resolve GLFW and Debug extensions
    uint32_t glfwExtensionCount = 0U;
    const char** const glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    // Step 3: Configure Validation Layers
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else {
        createInfo.enabledLayerCount = 0U;
    }

    VulkanContext* context = ServiceLocator::GetContext();

    if (vkCreateInstance(&createInfo, nullptr, &context->instance) != VK_SUCCESS) {
        throw std::runtime_error("VulkanDevice: Critical failure during instance creation.");
    }
}

/**
 * @brief Sets up the debug messenger for validation layer output.
 */
void VulkanDevice::setupDebugMessenger() const {
    if (!enableValidationLayers) {
        return;
    }

    VulkanContext* context = ServiceLocator::GetContext();

    VkDebugUtilsMessengerCreateInfoEXT createInfo{ VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;

    const auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(context->instance, "vkCreateDebugUtilsMessengerEXT")
        );

    if (func != nullptr) {
        static_cast<void>(func(context->instance, &createInfo, nullptr, &context->debugMessenger));
    }
}

/**
 * @brief Creates the WSI surface to connect Vulkan to the GLFW window.
 */
void VulkanDevice::createSurface(GLFWwindow* const window) const {
    VulkanContext* context = ServiceLocator::GetContext();

    if (glfwCreateWindowSurface(context->instance, window, nullptr, &context->surface) != VK_SUCCESS) {
        throw std::runtime_error("VulkanDevice: Failed to create window surface.");
    }
}

// ========================================================================
// SECTION 3: PHYSICAL & LOGICAL DEVICE MANAGEMENT
// ========================================================================

/**
 * @brief Selects a GPU that supports the required queue families and extensions.
 * Evaluates available hardware and selects the most capable device based on MSAA support.
 */
void VulkanDevice::pickPhysicalDevice() {
    uint32_t deviceCount = 0U;

    VulkanContext* context = ServiceLocator::GetContext();

    static_cast<void>(vkEnumeratePhysicalDevices(context->instance, &deviceCount, nullptr));

    if (deviceCount == 0U) {
        throw std::runtime_error("VulkanDevice: No compatible GPUs found with Vulkan support.");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    static_cast<void>(vkEnumeratePhysicalDevices(context->instance, &deviceCount, devices.data()));

    // Step 1: Evaluate each device for suitablity
    for (const auto& device : devices) {
        const QueueFamilyIndices indices = findQueueFamilies(device);

        // We ensure the device supports Graphics, Presentation, and Transfer queues
        if (indices.isComplete()) {
            context->physicalDevice = device;

            // Step 2: Cache hardware limits for MSAA (Multi-Sampling)
            msaaSamples = getMaxUsableSampleCount();
            break;
        }
    }

    if (context->physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("VulkanDevice: Failed to identify a suitable GPU.");
    }
}

/**
 * @brief Allocates a Logical Device and retrieves Command Queue handles.
 * Configures required device extensions and enables anisotropy for high-quality sampling.
 */
void VulkanDevice::createLogicalDevice() {
    VulkanContext* context = ServiceLocator::GetContext();
    queueIndices = findQueueFamilies(context->physicalDevice);

    // Step 1: Create queue configuration for unique family indices
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};
    const std::set<uint32_t> uniqueQueueFamilies = {
        queueIndices.graphicsFamily.value(),
        queueIndices.presentFamily.value(),
        queueIndices.transferFamily.value()
    };

    const float queuePriority = 1.0f;
    for (const uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = GE::EngineConstants::COUNT_ONE;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    // Step 2: Enable required hardware features
    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE; // Critical for PBR texture quality

    // Step 3: Define Logical Device creation info
    VkDeviceCreateInfo createInfo{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else {
        createInfo.enabledLayerCount = 0U;
    }

    if (vkCreateDevice(context->physicalDevice, &createInfo, nullptr, &context->device) != VK_SUCCESS) {
        throw std::runtime_error("VulkanDevice: Failed to allocate logical hardware device.");
    }

    // Step 4: Retrieve hardware handles for queue submissions
    vkGetDeviceQueue(context->device, queueIndices.graphicsFamily.value(), 0U, &context->graphicsQueue);
    vkGetDeviceQueue(context->device, queueIndices.presentFamily.value(), 0U, &context->presentQueue);
    vkGetDeviceQueue(context->device, queueIndices.transferFamily.value(), 0U, &context->transferQueue);
}

/**
 * @brief Reserves a global VRAM pool for the engine's linear sub-allocator.
 */
void VulkanDevice::initAllocator() {
    VulkanContext* context = ServiceLocator::GetContext();
    context->allocator.init(context->device, context->physicalDevice, GE::EngineConstants::VRAM_POOL_SIZE);
}

void VulkanDevice::createCommandPool() {
    VulkanContext* context = ServiceLocator::GetContext();

    VkCommandPoolCreateInfo poolInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueIndices.graphicsFamily.value();

    if (vkCreateCommandPool(context->device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("VulkanDevice: Failed to create graphics command pool.");
    }
}

// ========================================================================
// SECTION 4: SWAPCHAIN & PRESENTATION INFRASTRUCTURE
// ========================================================================

/**
 * @brief Negotiates hardware capabilities and creates the VkSwapchainKHR.
 */
void VulkanDevice::createSwapChain(GLFWwindow* const window) {
    VulkanContext* context = ServiceLocator::GetContext();
    // Step 1: Query current hardware support and negotiate surface settings
    const SwapChainSupportDetails swapChainSupport = querySwapChainSupport(context->physicalDevice);

    // Negotiate Surface Format (SRGB preferred)
    VkSurfaceFormatKHR surfaceFormat = swapChainSupport.formats.at(0U);
    for (const auto& availableFormat : swapChainSupport.formats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surfaceFormat = availableFormat;
            break;
        }
    }

    // Negotiate Presentation Mode (Mailbox for "Triple Buffering" low-latency)
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (const auto& availablePresentMode : swapChainSupport.presentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            presentMode = availablePresentMode;
            break;
        }
    }

    // Step 2: Determine resolution extent based on GLFW window size
    int width{ 0 }, height{ 0 };
    glfwGetFramebufferSize(window, &width, &height);
    VkExtent2D extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

    extent.width = std::clamp(extent.width,
        swapChainSupport.capabilities.minImageExtent.width, swapChainSupport.capabilities.maxImageExtent.width);
    extent.height = std::clamp(extent.height,
        swapChainSupport.capabilities.minImageExtent.height, swapChainSupport.capabilities.maxImageExtent.height);

    // Step 3: Determine optimal image count
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1U;
    if (swapChainSupport.capabilities.maxImageCount > 0U && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    // Step 4: Assemble Swapchain Creation Info
    VkSwapchainCreateInfoKHR createInfo{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    createInfo.surface = context->surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = GE::EngineConstants::COUNT_ONE;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    if (queueIndices.graphicsFamily != queueIndices.presentFamily) {
        const uint32_t qIndices[] = { queueIndices.graphicsFamily.value(), queueIndices.presentFamily.value() };
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2U;
        createInfo.pQueueFamilyIndices = qIndices;
    }
    else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    VkSwapchainKHR newSwapChain{ VK_NULL_HANDLE };
    if (vkCreateSwapchainKHR(context->device, &createInfo, nullptr, &newSwapChain) != VK_SUCCESS) {
        throw std::runtime_error("VulkanDevice: Failed to create hardware swap chain.");
    }

    // Step 5: Transfer ownership to the SwapChain RAII object
    swapChainObj->setHandle(newSwapChain);
    swapChainObj->setFormat(surfaceFormat.format);
    swapChainObj->setExtent(extent);

    uint32_t actualImageCount = 0U;
    static_cast<void>(vkGetSwapchainImagesKHR(context->device, newSwapChain, &actualImageCount, nullptr));
    std::vector<VkImage> tempImages(actualImageCount);
    static_cast<void>(vkGetSwapchainImagesKHR(context->device, newSwapChain, &actualImageCount, tempImages.data()));
    swapChainObj->initImages(tempImages);
}

/**
 * @brief Constructs the final Graphics Render Pass for the presentation engine.
 */
void VulkanDevice::createRenderPass() {
    // [0] Color Attachment: Target for tone-mapping
    VkAttachmentDescription colorAttr{};
    colorAttr.format = swapChainObj->getFormat();
    colorAttr.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttr.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttr.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttr.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttr.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // [1] Depth Attachment: Final UI/Depth resolve
    VkAttachmentDescription depthAttr{};
    depthAttr.format = depthFormat;
    depthAttr.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttr.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttr.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttr.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttr.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorRef{ 0U, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    VkAttachmentReference depthRef{ 1U, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = GE::EngineConstants::COUNT_ONE;
    subpass.pColorAttachments = &colorRef;
    subpass.pDepthStencilAttachment = &depthRef;

    std::array<VkAttachmentDescription, 2U> attachments = { colorAttr, depthAttr };
    VkRenderPassCreateInfo passInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    passInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    passInfo.pAttachments = attachments.data();
    passInfo.subpassCount = GE::EngineConstants::COUNT_ONE;
    passInfo.pSubpasses = &subpass;

    VulkanContext* context = ServiceLocator::GetContext();

    VkRenderPass rawPass{ VK_NULL_HANDLE };
    if (vkCreateRenderPass(context->device, &passInfo, nullptr, &rawPass) != VK_SUCCESS) {
        throw std::runtime_error("VulkanDevice: Failed to construct presentation RenderPass.");
    }

    finalPass = std::make_unique<RenderPass>(rawPass);
}

// ========================================================================
// SECTION 5: HARDWARE QUERIES & HELPERS
// ========================================================================

/**
 * @brief Queries hardware for the highest supported Multisample Count (MSAA).
 */
VkSampleCountFlagBits VulkanDevice::getMaxUsableSampleCount() const {
    VkPhysicalDeviceProperties props;
    VulkanContext* context = ServiceLocator::GetContext();
    vkGetPhysicalDeviceProperties(context->physicalDevice, &props);

    const VkSampleCountFlags counts = props.limits.framebufferColorSampleCounts &
        props.limits.framebufferDepthSampleCounts;

    if ((counts & VK_SAMPLE_COUNT_64_BIT) != 0U) { return VK_SAMPLE_COUNT_64_BIT; }
    if ((counts & VK_SAMPLE_COUNT_32_BIT) != 0U) { return VK_SAMPLE_COUNT_32_BIT; }
    if ((counts & VK_SAMPLE_COUNT_16_BIT) != 0U) { return VK_SAMPLE_COUNT_16_BIT; }
    if ((counts & VK_SAMPLE_COUNT_8_BIT) != 0U) { return VK_SAMPLE_COUNT_8_BIT; }
    if ((counts & VK_SAMPLE_COUNT_4_BIT) != 0U) { return VK_SAMPLE_COUNT_4_BIT; }
    if ((counts & VK_SAMPLE_COUNT_2_BIT) != 0U) { return VK_SAMPLE_COUNT_2_BIT; }

    return VK_SAMPLE_COUNT_1_BIT;
}

/**
 * @brief Retrieves hardware-specific depth format support.
 */
VkFormat VulkanDevice::findDepthFormat() const {
    return findSupportedFormat(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

/**
 * @brief Recreates the swapchain and all resolution-dependent resources.
 */
void VulkanDevice::recreateSwapChain(GLFWwindow* const window) {
    int width{ 0 }, height{ 0 };
    glfwGetFramebufferSize(window, &width, &height);

    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    VulkanContext* context = ServiceLocator::GetContext();

    vkDeviceWaitIdle(context->device);
    cleanupSwapChain();

    createSwapChain(window);
    createImageViews();
    createDepthResources();
    createFramebuffers();
}

// ========================================================================
// SECTION 6: PRESENTATION & DEPTH INFRASTRUCTURE
// ========================================================================

/**
 * @brief Creates image views for the acquired swapchain images.
 * Adheres to MISRA2008.9_3_2_b by using the addImageView interface.
 */
void VulkanDevice::createImageViews() {
    const auto& images = swapChainObj->getImages();

    VulkanContext* context = ServiceLocator::GetContext();

    for (size_t i = 0U; i < images.size(); ++i) {
        const VkImageView view = VulkanUtils::createImageView(
            context->device,
            images[i],
            swapChainObj->getFormat(),
            VK_IMAGE_ASPECT_COLOR_BIT,
            GE::EngineConstants::COUNT_ONE
        );

        // Push the handle into the RAII object for managed cleanup
        swapChainObj->addImageView(view);
    }
}

/**
 * @brief Allocates the Hardware Depth Buffer for the final presentation pass.
 */
void VulkanDevice::createDepthResources() {
    VulkanContext* context = ServiceLocator::GetContext();

    depthBuffer = std::make_unique<Image>(
        swapChainObj->getExtent().width,
        swapChainObj->getExtent().height,
        VK_SAMPLE_COUNT_1_BIT,
        depthFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        VK_IMAGE_ASPECT_DEPTH_BIT
    );
}

/**
 * @brief Creates the final framebuffers for window presentation.
 */
void VulkanDevice::createFramebuffers() {
    const auto& views = swapChainObj->getImageViews();
    const auto& extent = swapChainObj->getExtent();

    for (size_t i = 0U; i < views.size(); ++i) {
        // Step 1: Link Color Target and Depth Target views
        const std::array<VkImageView, 2U> attachments = {
            views.at(i),
            depthBuffer->getView()
        };

        VkFramebufferCreateInfo fbInfo{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
        fbInfo.renderPass = finalPass->getHandle();
        fbInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        fbInfo.pAttachments = attachments.data();
        fbInfo.width = extent.width;
        fbInfo.height = extent.height;
        fbInfo.layers = GE::EngineConstants::COUNT_ONE;

        VkFramebuffer newFramebuffer{ VK_NULL_HANDLE };

        VulkanContext* context = ServiceLocator::GetContext();

        if (vkCreateFramebuffer(context->device, &fbInfo, nullptr, &newFramebuffer) != VK_SUCCESS) {
            throw std::runtime_error("VulkanDevice: Framebuffer allocation failed.");
        }

        // Transfer handle to SwapChain container
        swapChainObj->addFramebuffer(newFramebuffer);
    }
}

/**
 * @brief Safely releases resolution-dependent GPU memory and RAII objects.
 */
void VulkanDevice::cleanupSwapChain() {
    if (swapChainObj) {
        swapChainObj->cleanup();
    }

    // Resetting unique_ptr triggers the Image destructor (vkDestroyImage)
    depthBuffer.reset();
}

// ========================================================================
// SECTION 7: HARDWARE QUERY HELPERS (CONST)
// ========================================================================

/**
 * @brief Identifies support for Graphics, Presentation, and Transfer queues on a GPU.
 */
QueueFamilyIndices VulkanDevice::findQueueFamilies(const VkPhysicalDevice device) const {
    QueueFamilyIndices indices;
    uint32_t count = 0U;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);

    std::vector<VkQueueFamilyProperties> families(count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, families.data());

    for (uint32_t i = 0U; i < count; i++) {
        // Step 1: Check Graphics Support
        if ((families.at(i).queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0U) {
            indices.graphicsFamily = i;
        }

        VulkanContext* context = ServiceLocator::GetContext();

        // Step 2: Check Surface Presentation Support
        VkBool32 presentSupport = VK_FALSE;
        static_cast<void>(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, context->surface, &presentSupport));
        if (presentSupport == VK_TRUE) {
            indices.presentFamily = i;
        }

        // Step 3: Check Dedicated Transfer (DMA) Support
        if ((families.at(i).queueFlags & VK_QUEUE_TRANSFER_BIT) != 0U) {
            indices.transferFamily = i;
        }

        if (indices.isComplete()) {
            break;
        }
    }
    return indices;
}

/**
 * @brief Queries the physical device for surface format and presentation mode support.
 */
SwapChainSupportDetails VulkanDevice::querySwapChainSupport(const VkPhysicalDevice device) const {
    SwapChainSupportDetails details;
    VulkanContext* context = ServiceLocator::GetContext();
    static_cast<void>(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, context->surface, &details.capabilities));

    uint32_t formatCount = 0U;
    static_cast<void>(vkGetPhysicalDeviceSurfaceFormatsKHR(device, context->surface, &formatCount, nullptr));
    if (formatCount != 0U) {
        details.formats.resize(static_cast<size_t>(formatCount));
        static_cast<void>(vkGetPhysicalDeviceSurfaceFormatsKHR(device, context->surface, &formatCount, details.formats.data()));
    }

    uint32_t modeCount = 0U;
    static_cast<void>(vkGetPhysicalDeviceSurfacePresentModesKHR(device, context->surface, &modeCount, nullptr));
    if (modeCount != 0U) {
        details.presentModes.resize(static_cast<size_t>(modeCount));
        static_cast<void>(vkGetPhysicalDeviceSurfacePresentModesKHR(device, context->surface, &modeCount, details.presentModes.data()));
    }
    return details;
}

/**
 * @brief Searches the GPU for an image format that supports specific tiling and features.
 * Matches the const signature in VulkanEngine.h to resolve LNK2019.
 */
VkFormat VulkanDevice::findSupportedFormat(
    const std::vector<VkFormat>& candidates,
    const VkImageTiling tiling,
    const VkFormatFeatureFlags features) const
{
    // Step 1: Iterate through candidate formats in order of preference
    for (const VkFormat format : candidates) {
        VkFormatProperties props;
        VulkanContext* context = ServiceLocator::GetContext();
        vkGetPhysicalDeviceFormatProperties(context->physicalDevice, format, &props);

        // Step 2: Check for linear or optimal tiling support based on required features
        if ((tiling == VK_IMAGE_TILING_LINEAR) && ((props.linearTilingFeatures & features) == features)) {
            return format;
        }

        if ((tiling == VK_IMAGE_TILING_OPTIMAL) && ((props.optimalTilingFeatures & features) == features)) {
            return format;
        }
    }

    throw std::runtime_error("VulkanDevice: Failed to identify a hardware-supported image format.");
}

/**
 * @brief Verifies that the hardware/driver supports the requested Vulkan Validation Layers.
 * Matches the const signature in VulkanEngine.h to resolve LNK2019.
 */
bool VulkanDevice::checkValidationLayerSupport() const {
    uint32_t layerCount = 0U;
    static_cast<void>(vkEnumerateInstanceLayerProperties(&layerCount, nullptr));

    std::vector<VkLayerProperties> availableLayers(layerCount);
    static_cast<void>(vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data()));

    // Step 1: Cross-reference requested layers against the system's available layers
    for (const char* const layerName : validationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (std::strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

} // namespace GE::Graphics