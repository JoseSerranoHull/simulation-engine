#include "VulkanResourceManager.h"

/* parasoft-begin-suppress ALL */
#include <array>
#include <stdexcept>
/* parasoft-end-suppress ALL */

// ========================================================================
// SECTION 1: LIFECYCLE & INFRASTRUCTURE
// ========================================================================

/**
 * @brief Constructor: Links the manager to the centralized Vulkan hardware context.
 */
VulkanResourceManager::VulkanResourceManager()
    : descriptorPool(VK_NULL_HANDLE),
    transferCommandPool(VK_NULL_HANDLE),
    shadowImage(VK_NULL_HANDLE),
    shadowImageMemory(VK_NULL_HANDLE),
    shadowImageView(VK_NULL_HANDLE),
    shadowSampler(VK_NULL_HANDLE),
    shadowRenderPass(VK_NULL_HANDLE),
    shadowFramebuffer(VK_NULL_HANDLE)
{
}

/**
 * @brief Destructor: Triggers the full cleanup of allocated GPU handles.
 */
VulkanResourceManager::~VulkanResourceManager() {
    try {
        cleanup();
    }
    catch (...) {
        // Suppression for destructor safety (Standard practice for RAII)
    }
}

/**
 * @brief Orchestrates the primary allocation sequence for global engine resources.
 */
void VulkanResourceManager::init(const VulkanEngine* const engine, const uint32_t maxFrames) {
    // Step 1: Establish Layouts and Infrastructure Pools
    createLayouts();
    createPools(engine);

    // Step 2: Allocate Dedicated Shadow Mapping Hardware
    createShadowResources(engine);

    // Step 3: Prepare Uniform Buffers (one per swapchain image for overlapping frames)
    const uint32_t imageCount = engine->getSwapChainImageCount();
    createUniformBuffers(imageCount);

    // Step 4: Initialize CPU-GPU Synchronization (SyncManager)
    syncManager = std::make_unique<SyncManager>();
    syncManager->init(maxFrames, imageCount);

    // Step 5: Allocate Primary Graphics Command Buffers
    VulkanContext* context = ServiceLocator::GetContext();
    syncManager->allocateCommandBuffers(context->graphicsCommandPool, maxFrames);
}

// ========================================================================
// SECTION 2: DESCRIPTOR & POOL MANAGEMENT
// ========================================================================

/**
 * @brief Creates global descriptor set layouts for scene and material data.
 */
void VulkanResourceManager::createLayouts() const {
    // Step 1: Global Set (Set 0) - Shared across all shaders (UBOs, Shadows, Refraction)
    const VkDescriptorSetLayoutBinding uboBinding{
        EngineConstants::BINDING_UBO, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1U,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr
    };

    const VkDescriptorSetLayoutBinding shadowBinding{
        EngineConstants::BINDING_SHADOW_SAMPLER, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1U,
        VK_SHADER_STAGE_FRAGMENT_BIT, nullptr
    };

    const VkDescriptorSetLayoutBinding refractionBinding{
        2U, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1U,
        VK_SHADER_STAGE_FRAGMENT_BIT, nullptr
    };

    const std::array<VkDescriptorSetLayoutBinding, 3U> bindings = { uboBinding, shadowBinding, refractionBinding };
    VkDescriptorSetLayoutCreateInfo layoutInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    VulkanContext* context = ServiceLocator::GetContext();

    if (vkCreateDescriptorSetLayout(context->device, &layoutInfo, nullptr, &context->globalSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("VulkanResourceManager: Failed to create global descriptor set layout!");
    }

    // Step 2: Material Set (Set 1) - Mesh-specific PBR Textures
    std::array<VkDescriptorSetLayoutBinding, AssetManager::PBR_TEXTURE_COUNT> matBindings{};
    for (uint32_t i = 0U; i < AssetManager::PBR_TEXTURE_COUNT; ++i) {
        matBindings[i] = { i, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1U, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
    }

    VkDescriptorSetLayoutCreateInfo matLayoutInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    matLayoutInfo.bindingCount = static_cast<uint32_t>(matBindings.size());
    matLayoutInfo.pBindings = matBindings.data();

    if (vkCreateDescriptorSetLayout(context->device, &matLayoutInfo, nullptr, &context->materialSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("VulkanResourceManager: Failed to create material descriptor set layout!");
    }
}

/**
 * @brief Reserves memory pools for command recording and descriptor allocation.
 */
void VulkanResourceManager::createPools(const VulkanEngine* const engine) {
    const uint32_t imageCount = engine->getSwapChainImageCount();

    // Step 1: Descriptor Pool for UBOs and Samplers
    std::array<VkDescriptorPoolSize, 2U> poolSizes{};
    poolSizes[0] = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, imageCount };
    poolSizes[1] = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageCount + (100U * AssetManager::PBR_TEXTURE_COUNT) };

    VkDescriptorPoolCreateInfo descPoolInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    descPoolInfo.maxSets = imageCount + 100U;
    descPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    descPoolInfo.pPoolSizes = poolSizes.data();

    VulkanContext* context = ServiceLocator::GetContext();

    vkCreateDescriptorPool(context->device, &descPoolInfo, nullptr, &descriptorPool);

    // Step 2: Graphics and Transfer Command Pools
    const auto& indices = engine->getQueueFamilyIndices();
    VkCommandPoolCreateInfo graphicsPoolInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    graphicsPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    graphicsPoolInfo.queueFamilyIndex = indices.graphicsFamily.value();
    vkCreateCommandPool(context->device, &graphicsPoolInfo, nullptr, &context->graphicsCommandPool);

    VkCommandPoolCreateInfo transferPoolInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    transferPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    transferPoolInfo.queueFamilyIndex = indices.transferFamily.value();
    vkCreateCommandPool(context->device, &transferPoolInfo, nullptr, &transferCommandPool);
}

// ========================================================================
// SECTION 3: SHADOW & UNIFORM RESOURCES
// ========================================================================

/**
 * @brief Initializes the textures and render passes required for Shadow Mapping.
 */
void VulkanResourceManager::createShadowResources(const VulkanEngine* const engine) {
    const VkFormat shadowFormat = engine->getDepthFormat();
    const uint32_t res = EngineConstants::SHADOW_MAP_RES;

    VulkanContext* context = ServiceLocator::GetContext();

    // Step 1: Create Shadow Map Image and View
    VulkanUtils::createImage(context->device, context->physicalDevice, res, res, 1U,
        VK_SAMPLE_COUNT_1_BIT, shadowFormat, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, shadowImage, shadowImageMemory);

    shadowImageView = VulkanUtils::createImageView(context->device, shadowImage, shadowFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1U);

    // Step 2: Create Shadow Sampler with Depth Comparison enabled
    VkSamplerCreateInfo samplerInfo{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    samplerInfo.compareEnable = VK_TRUE;
    samplerInfo.compareOp = VK_COMPARE_OP_LESS;

    if (vkCreateSampler(context->device, &samplerInfo, nullptr, &shadowSampler) != VK_SUCCESS) {
        throw std::runtime_error("VulkanResourceManager: Failed to create shadow sampler!");
    }

    // Step 3: Finalize Shadow RenderPass and Framebuffer
    shadowRenderPass = VulkanUtils::createDepthRenderPass(context->device, shadowFormat);

    VkFramebufferCreateInfo fbInfo{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
    fbInfo.renderPass = shadowRenderPass;
    fbInfo.attachmentCount = 1U;
    fbInfo.pAttachments = &shadowImageView;
    fbInfo.width = res;
    fbInfo.height = res;
    fbInfo.layers = 1U;

    if (vkCreateFramebuffer(context->device, &fbInfo, nullptr, &shadowFramebuffer) != VK_SUCCESS) {
        throw std::runtime_error("VulkanResourceManager: Failed to create shadow framebuffer!");
    }
}

/**
 * @brief Allocates and maps memory for the per-frame Uniform Buffer Objects (UBO).
 */
void VulkanResourceManager::createUniformBuffers(const uint32_t imageCount) {
    uniformBuffers.resize(static_cast<size_t>(imageCount));
    uniformBuffersMemory.resize(static_cast<size_t>(imageCount));
    uniformBuffersMapped.resize(static_cast<size_t>(imageCount));

    VulkanContext* context = ServiceLocator::GetContext();

    for (uint32_t i = 0U; i < imageCount; ++i) {
        VulkanUtils::createBuffer(context->device, context->physicalDevice, sizeof(UniformBufferObject),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            uniformBuffers[i], uniformBuffersMemory[i]);

        vkMapMemory(context->device, uniformBuffersMemory[i], 0U, sizeof(UniformBufferObject), 0U, &uniformBuffersMapped[i]);
    }
}

/**
 * @brief Links allocated UBOs and shadow maps to the GPU Descriptor Sets.
 */
void VulkanResourceManager::updateDescriptorSets(const VulkanEngine* const engine, const PostProcessor* const postProcessor) {
    const uint32_t imageCount = engine->getSwapChainImageCount();

    VulkanContext* context = ServiceLocator::GetContext();

    // Step 1: Allocate Global Descriptor Sets if registry is empty
    if (descriptorSets.empty()) {
        std::vector<VkDescriptorSetLayout> layouts(imageCount, context->globalSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = imageCount;
        allocInfo.pSetLayouts = layouts.data();

        descriptorSets.resize(static_cast<size_t>(imageCount));
        if (vkAllocateDescriptorSets(context->device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("VulkanResourceManager: Failed to allocate global descriptor sets!");
        }
    }

    // Step 2: Update each set with its respective UBO, Shadow, and Refraction textures
    for (uint32_t i = 0U; i < imageCount; ++i) {
        VkDescriptorBufferInfo bInfo{ uniformBuffers[i], 0U, sizeof(UniformBufferObject) };
        VkDescriptorImageInfo sInfo{ shadowSampler, shadowImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
        VkDescriptorImageInfo rInfo{ postProcessor->getBackgroundSampler(), postProcessor->getBackgroundImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

        std::array<VkWriteDescriptorSet, 3U> writes{};
        writes[0] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, descriptorSets[i], 0U, 0U, 1U, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr, &bInfo, nullptr };
        writes[1] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, descriptorSets[i], 1U, 0U, 1U, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &sInfo, nullptr, nullptr };
        writes[2] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, descriptorSets[i], 2U, 0U, 1U, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &rInfo, nullptr, nullptr };

        vkUpdateDescriptorSets(context->device, static_cast<uint32_t>(writes.size()), writes.data(), 0U, nullptr);
    }
}

// ========================================================================
// SECTION 4: CLEANUP
// ========================================================================

/**
 * @brief Safely releases all managed Vulkan handles and mapped memory.
 */
void VulkanResourceManager::cleanup() {
    VulkanContext* context = ServiceLocator::GetContext();

    if ((context == nullptr) || (context->device == VK_NULL_HANDLE)) {
        return;
    }

    // Step 1: Release child managers
    syncManager.reset();

    // Step 2: Destroy Shadow mapping infrastructure
    vkDestroySampler(context->device, shadowSampler, nullptr);
    vkDestroyImageView(context->device, shadowImageView, nullptr);
    vkDestroyImage(context->device, shadowImage, nullptr);
    vkFreeMemory(context->device, shadowImageMemory, nullptr);
    vkDestroyFramebuffer(context->device, shadowFramebuffer, nullptr);
    vkDestroyRenderPass(context->device, shadowRenderPass, nullptr);

    // Step 3: Unmap and release Uniform Buffers
    for (size_t i = 0U; i < uniformBuffers.size(); ++i) {
        vkUnmapMemory(context->device, uniformBuffersMemory[i]);
        vkDestroyBuffer(context->device, uniformBuffers[i], nullptr);
        vkFreeMemory(context->device, uniformBuffersMemory[i], nullptr);
    }
    uniformBuffers.clear();

    // Step 4: Destroy core infrastructure handles
    vkDestroyDescriptorPool(context->device, descriptorPool, nullptr);
    vkDestroyCommandPool(context->device, transferCommandPool, nullptr);
    vkDestroyDescriptorSetLayout(context->device, context->globalSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(context->device, context->materialSetLayout, nullptr);
    vkDestroyCommandPool(context->device, context->graphicsCommandPool, nullptr);
}