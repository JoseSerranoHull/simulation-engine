#include "PostProcessor.h"

/* parasoft-begin-suppress ALL */
#include <stdexcept>
#include <array>
/* parasoft-end-suppress ALL */

/**
 * @brief Constructor: Orchestrates the allocation of HDR render targets and refraction snapshots.
 */
PostProcessor::PostProcessor(VulkanContext* const inContext, const uint32_t inWidth, const uint32_t inHeight,
    const VkFormat inSwapChainFormat, const VkRenderPass finalRenderPass, const VkSampleCountFlagBits inMsaaSamples)
    : context(inContext), width(inWidth), height(inHeight), swapChainFormat(inSwapChainFormat), msaaSamples(inMsaaSamples)
{
    // 1. Establish the Depth Format (Hardware-specific float precision)
    this->depthFormat = VK_FORMAT_D32_SFLOAT;

    // 2. Resource & Pass Orchestration
    createOffscreenResources();

    // 3. Initialize Background Snapshot Image (Refraction Buffer)
    // Uses 64-bit HDR precision (R16G16B16A16_SFLOAT) to match offscreen targets
    VulkanUtils::createImage(context->device, context->physicalDevice, width, height, EngineConstants::COUNT_ONE,
        VK_SAMPLE_COUNT_1_BIT, hdrFormat, VK_IMAGE_TILING_OPTIMAL,
        (VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT),
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, backgroundImage, backgroundMemory);

    backgroundImageView = VulkanUtils::createImageView(context->device, backgroundImage, hdrFormat,
        VK_IMAGE_ASPECT_COLOR_BIT, EngineConstants::COUNT_ONE);

    // Initial Layout Transition: Ensure background is ready for shader sampling
    const VkCommandBuffer transitionCmd = VulkanUtils::beginSingleTimeCommands(context->device, context->graphicsCommandPool);
    VulkanUtils::recordImageBarrier(transitionCmd, backgroundImage,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        static_cast<VkAccessFlags>(EngineConstants::OFFSET_ZERO), VK_ACCESS_SHADER_READ_BIT,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        EngineConstants::COUNT_ONE);
    VulkanUtils::endSingleTimeCommands(context->device, context->graphicsCommandPool, context->graphicsQueue, transitionCmd);

    VulkanUtils::createTextureSampler(context->device, backgroundSampler, EngineConstants::COUNT_ONE);

    // RAII Wrapper: Encapsulate the background handles for use in the material system
    backgroundTextureWrapper = std::make_unique<Texture>(context, backgroundImage, backgroundImageView, backgroundSampler);

    // 4. Infrastructure Assembly
    createTransparentRenderPass();
    createRenderPass();
    createFramebuffer();
    createDescriptors();
    createPipeline(finalRenderPass);
}

/**
 * @brief Destructor: Ensures ordered cleanup of all GPU resources.
 */
PostProcessor::~PostProcessor() {
    try {
        if ((context != nullptr) && (context->device != VK_NULL_HANDLE)) {
            // 1. Clean up transient frame-dependent resources (Images/Views)
            cleanupResources();

            // 2. Destroy Permanent Pipeline Objects
            if (pipeline != VK_NULL_HANDLE) {
                vkDestroyPipeline(context->device, pipeline, nullptr);
                pipeline = VK_NULL_HANDLE;
            }
            if (pipelineLayout != VK_NULL_HANDLE) {
                vkDestroyPipelineLayout(context->device, pipelineLayout, nullptr);
                pipelineLayout = VK_NULL_HANDLE;
            }

            // 3. Destroy Descriptor Infrastructure
            if (descriptorPool != VK_NULL_HANDLE) {
                vkDestroyDescriptorPool(context->device, descriptorPool, nullptr);
                descriptorPool = VK_NULL_HANDLE;
            }
            if (descriptorSetLayout != VK_NULL_HANDLE) {
                vkDestroyDescriptorSetLayout(context->device, descriptorSetLayout, nullptr);
                descriptorSetLayout = VK_NULL_HANDLE;
            }

            // 4. Destroy Render Pass Infrastructure
            if (transparentRenderPass != VK_NULL_HANDLE) {
                vkDestroyRenderPass(context->device, transparentRenderPass, nullptr);
                transparentRenderPass = VK_NULL_HANDLE;
            }
            if (offscreenRenderPass != VK_NULL_HANDLE) {
                vkDestroyRenderPass(context->device, offscreenRenderPass, nullptr);
                offscreenRenderPass = VK_NULL_HANDLE;
            }
        }
    }
    catch (...) {
        // Exception caught and suppressed to prevent std::terminate() during destruction.
    }
}

/**
 * @brief Reallocates all resolution-dependent GPU resources.
 * Ensures the refraction and offscreen buffers match the new window extent.
 */
void PostProcessor::resize(const VkExtent2D& extent) {
    cleanupResources();
    width = extent.width;
    height = extent.height;

    createOffscreenResources();
    createBackgroundResources();
    createFramebuffer();

    backgroundTextureWrapper = std::make_unique<Texture>(context, backgroundImage, backgroundImageView, backgroundSampler);

    const VkDescriptorImageInfo imageInfo{ offscreenSampler, resolveImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
    const VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, descriptorSet,
        0U, 0U, 1U, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &imageInfo, nullptr, nullptr };

    vkUpdateDescriptorSets(context->device, 1U, &write, 0, nullptr);
}

/**
 * @brief Captures a high-precision snapshot of the opaque scene.
 * This snapshot is used by the refraction shaders in the subsequent transparent pass.
 */
void PostProcessor::copyScene(const VkCommandBuffer cb) const {
    // 1. Transition Background Image: Shader Read -> Transfer Destination
    VulkanUtils::recordImageBarrier(cb, backgroundImage,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        EngineConstants::COUNT_ONE);

    // 2. Execute GPU Copy
    // Source: resolveImage (MSAA flattened HDR)
    // Destination: backgroundImage (Refraction Snapshot)
    VkImageCopy copyRegion{};
    copyRegion.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0U, 0U, 1U };
    copyRegion.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0U, 0U, 1U };
    copyRegion.extent = { width, height, 1U };

    vkCmdCopyImage(cb, resolveImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        backgroundImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1U, &copyRegion);

    // 3. Transition Background Image: Transfer Destination -> Shader Read
    // Ready for consumption by glass and water refraction shaders
    VulkanUtils::recordImageBarrier(cb, backgroundImage,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        EngineConstants::COUNT_ONE);

    // 4. Transition Resolve Image: Transfer Source -> Shader Read
    // Synchronizes the HDR target for the following transparent pass
    VulkanUtils::recordImageBarrier(cb, resolveImage,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        EngineConstants::COUNT_ONE);
}

/**
 * @brief Helper: Specifically initializes resources used for refraction snapshots.
 */
void PostProcessor::createBackgroundResources() {
    // 1. Create Image and View
    VulkanUtils::createImage(context->device, context->physicalDevice, width, height, 1U,
        VK_SAMPLE_COUNT_1_BIT, hdrFormat, VK_IMAGE_TILING_OPTIMAL,
        (VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT),
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, backgroundImage, backgroundMemory);

    backgroundImageView = VulkanUtils::createImageView(context->device, backgroundImage, hdrFormat,
        VK_IMAGE_ASPECT_COLOR_BIT, 1U);

    // 2. Transition Layout
    const VkCommandBuffer transitionCmd = VulkanUtils::beginSingleTimeCommands(context->device, context->graphicsCommandPool);
    VulkanUtils::recordImageBarrier(transitionCmd, backgroundImage,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        0, VK_ACCESS_SHADER_READ_BIT,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 1U);
    VulkanUtils::endSingleTimeCommands(context->device, context->graphicsCommandPool, context->graphicsQueue, transitionCmd);

    // --- FIX: Recreate the sampler if it was destroyed by cleanupResources ---
    if (backgroundSampler == VK_NULL_HANDLE) {
        VulkanUtils::createTextureSampler(context->device, backgroundSampler, 1U);
    }
}
/**
 * @brief Records the final fullscreen post-processing pass.
 * Performs tone-mapping and applies the Bloom effect to the resolved scene.
 */
void PostProcessor::draw(const VkCommandBuffer commandBuffer, const bool enableBloom) const {
    const VkViewport vp{ 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f };
    vkCmdSetViewport(commandBuffer, 0, 1, &vp);

    const VkRect2D sc{ {0, 0}, {width, height} };
    vkCmdSetScissor(commandBuffer, 0, 1, &sc);

    // 2. Bind the Post-FX Pipeline
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    // 3. Bind Descriptors (Set 0: Resolved HDR Scene)
    const VkDescriptorSet sets[EngineConstants::COUNT_ONE] = { descriptorSet };
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
        EngineConstants::INDEX_ZERO, EngineConstants::COUNT_ONE, sets, EngineConstants::OFFSET_ZERO, nullptr);

    // 4. Push Bloom Toggle State
    const int32_t enabled = enableBloom ? EngineConstants::SHADER_TRUE : EngineConstants::SHADER_FALSE;
    vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT,
        EngineConstants::OFFSET_ZERO, static_cast<uint32_t>(sizeof(int32_t)), &enabled);

    // 5. Execute Fullscreen Triangle Draw (3 Vertices, Procedurally generated in shader)
    vkCmdDraw(commandBuffer, FULLSCREEN_TRI_VERTS, EngineConstants::COUNT_ONE, EngineConstants::OFFSET_ZERO, EngineConstants::OFFSET_ZERO);
}

/**
 * @brief Safely releases all frame-dependent GPU memory.
 * This is called during both destruction and window resizing.
 */
void PostProcessor::cleanupResources() {
    // 1. Offscreen HDR Targets & Views
    if (offscreenFramebuffer != VK_NULL_HANDLE) {
        vkDestroyFramebuffer(context->device, offscreenFramebuffer, nullptr);
        offscreenFramebuffer = VK_NULL_HANDLE;
    }
    if (offscreenImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(context->device, offscreenImageView, nullptr);
        offscreenImageView = VK_NULL_HANDLE;
    }
    if (offscreenImage != VK_NULL_HANDLE) {
        vkDestroyImage(context->device, offscreenImage, nullptr);
        offscreenImage = VK_NULL_HANDLE;
    }
    if (offscreenMemory != VK_NULL_HANDLE) {
        vkFreeMemory(context->device, offscreenMemory, nullptr);
        offscreenMemory = VK_NULL_HANDLE;
    }

    // 2. Resolve & Depth Buffers
    if (resolveImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(context->device, resolveImageView, nullptr);
        resolveImageView = VK_NULL_HANDLE;
    }
    if (resolveImage != VK_NULL_HANDLE) {
        vkDestroyImage(context->device, resolveImage, nullptr);
        resolveImage = VK_NULL_HANDLE;
    }
    if (resolveMemory != VK_NULL_HANDLE) {
        vkFreeMemory(context->device, resolveMemory, nullptr);
        resolveMemory = VK_NULL_HANDLE;
    }
    if (internalDepthView != VK_NULL_HANDLE) {
        vkDestroyImageView(context->device, internalDepthView, nullptr);
        internalDepthView = VK_NULL_HANDLE;
    }
    if (internalDepthImage != VK_NULL_HANDLE) {
        vkDestroyImage(context->device, internalDepthImage, nullptr);
        internalDepthImage = VK_NULL_HANDLE;
    }
    if (internalDepthMemory != VK_NULL_HANDLE) {
        vkFreeMemory(context->device, internalDepthMemory, nullptr);
        internalDepthMemory = VK_NULL_HANDLE;
    }

    // 3. Background/Snapshot Resources
    if (backgroundImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(context->device, backgroundImageView, nullptr);
        backgroundImageView = VK_NULL_HANDLE;
    }
    if (backgroundImage != VK_NULL_HANDLE) {
        vkDestroyImage(context->device, backgroundImage, nullptr);
        backgroundImage = VK_NULL_HANDLE;
    }
    if (backgroundMemory != VK_NULL_HANDLE) {
        vkFreeMemory(context->device, backgroundMemory, nullptr);
        backgroundMemory = VK_NULL_HANDLE;
    }

    // 4. Samplers
    if (offscreenSampler != VK_NULL_HANDLE) {
        vkDestroySampler(context->device, offscreenSampler, nullptr);
        offscreenSampler = VK_NULL_HANDLE;
    }
    if (backgroundSampler != VK_NULL_HANDLE) {
        vkDestroySampler(context->device, backgroundSampler, nullptr);
        backgroundSampler = VK_NULL_HANDLE;
    }
}

/**
 * @brief Allocates multi-sampled HDR and Resolve images.
 */
void PostProcessor::createOffscreenResources() {
    // 1. Color Target (MSAA Enabled)
    VulkanUtils::createImage(context->device, context->physicalDevice, width, height, EngineConstants::COUNT_ONE,
        msaaSamples, hdrFormat, VK_IMAGE_TILING_OPTIMAL,
        (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT),
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, offscreenImage, offscreenMemory);

    offscreenImageView = VulkanUtils::createImageView(context->device, offscreenImage, hdrFormat,
        VK_IMAGE_ASPECT_COLOR_BIT, EngineConstants::COUNT_ONE);

    // 2. Depth Target (MSAA Enabled)
    VulkanUtils::createImage(context->device, context->physicalDevice, width, height, EngineConstants::COUNT_ONE,
        msaaSamples, depthFormat, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        internalDepthImage, internalDepthMemory);

    internalDepthView = VulkanUtils::createImageView(context->device, internalDepthImage, depthFormat,
        VK_IMAGE_ASPECT_DEPTH_BIT, EngineConstants::COUNT_ONE);

    // 3. Resolve Target (1x, No MSAA)
    VulkanUtils::createImage(context->device, context->physicalDevice, width, height, EngineConstants::COUNT_ONE,
        VK_SAMPLE_COUNT_1_BIT, hdrFormat, VK_IMAGE_TILING_OPTIMAL,
        (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT),
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, resolveImage, resolveMemory);

    resolveImageView = VulkanUtils::createImageView(context->device, resolveImage, hdrFormat,
        VK_IMAGE_ASPECT_COLOR_BIT, EngineConstants::COUNT_ONE);

    // 4. Samplers for HDR textures
    if (offscreenSampler == VK_NULL_HANDLE) {
        VulkanUtils::createTextureSampler(context->device, offscreenSampler, 1U, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    }
}

/**
 * @brief Initializes the primary offscreen render pass.
 * FIX: Calls deduplicated helper to resolve CDD.DUPC.
 */
void PostProcessor::createRenderPass() {
    internalCreateRenderPass(false);
}

/**
 * @brief Initializes the secondary transparent pass.
 * FIX: Calls deduplicated helper to resolve CDD.DUPC.
 */
void PostProcessor::createTransparentRenderPass() {
    internalCreateRenderPass(true);
}

/**
 * @brief Creates the offscreen framebuffer linking all three HDR attachments.
 */
void PostProcessor::createFramebuffer() {
    const std::array<VkImageView, 3> attachments = {
        offscreenImageView,
        internalDepthView,
        resolveImageView
    };

    VkFramebufferCreateInfo framebufferInfo{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
    framebufferInfo.renderPass = offscreenRenderPass;
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments = attachments.data();
    framebufferInfo.width = width;
    framebufferInfo.height = height;
    framebufferInfo.layers = 1U;

    if (vkCreateFramebuffer(context->device, &framebufferInfo, nullptr, &offscreenFramebuffer) != VK_SUCCESS) {
        throw std::runtime_error("PostProcessor: Failed to create offscreen framebuffer!");
    }
}

/**
 * @brief Sets up descriptors to sample the resolved HDR scene for post-processing.
 */
void PostProcessor::createDescriptors() {
    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 0U;
    samplerLayoutBinding.descriptorCount = 1U;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    layoutInfo.bindingCount = 1U;
    layoutInfo.pBindings = &samplerLayoutBinding;

    if (vkCreateDescriptorSetLayout(context->device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("PostProcessor: Failed to create descriptor set layout!");
    }

    VkDescriptorPoolSize poolSize{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1U };
    VkDescriptorPoolCreateInfo poolInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    poolInfo.poolSizeCount = 1U;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 1U;

    if (vkCreateDescriptorPool(context->device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("PostProcessor: Failed to create descriptor pool!");
    }

    VkDescriptorSetAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1U;
    allocInfo.pSetLayouts = &descriptorSetLayout;

    if (vkAllocateDescriptorSets(context->device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
        throw std::runtime_error("PostProcessor: Failed to allocate descriptor set!");
    }

    if (offscreenSampler == VK_NULL_HANDLE) {
        throw std::runtime_error("PostProcessor: Attempted to update descriptors with an invalid sampler!");
    }

    VkDescriptorImageInfo imageInfo{};
    imageInfo.sampler = offscreenSampler;
    imageInfo.imageView = resolveImageView;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet descriptorWrite{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    descriptorWrite.dstSet = descriptorSet;
    descriptorWrite.dstBinding = 0U;
    descriptorWrite.dstArrayElement = 0U;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1U;
    descriptorWrite.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(context->device, 1U, &descriptorWrite, 0U, nullptr);
}

/**
 * @brief Constructs the final fullscreen graphics pipeline.
 */
void PostProcessor::createPipeline(const VkRenderPass finalRenderPass) {
    const ShaderModule vertShader(context, "./shaders/post_vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    const ShaderModule fragShader(context, "./shaders/post_frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    const VkPipelineShaderStageCreateInfo shaderStages[2] = { vertShader.getStageInfo(), fragShader.getStageInfo() };

    // --- 1. Pipeline Configuration State ---
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewportState{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
    viewportState.viewportCount = 1U;
    viewportState.scissorCount = 1U;

    const std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineRasterizationStateCreateInfo rasterizer{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo multisampling{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
    depthStencil.depthTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
    colorBlending.attachmentCount = 1U;
    colorBlending.pAttachments = &colorBlendAttachment;

    // --- 2. Layout Creation ---
    const std::array<VkDescriptorSetLayout, 2> layouts = { descriptorSetLayout, context->materialSetLayout };
    const VkPushConstantRange pushRange{ VK_SHADER_STAGE_FRAGMENT_BIT, 0U, static_cast<uint32_t>(sizeof(int32_t)) };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
    pipelineLayoutInfo.pSetLayouts = layouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 1U;
    pipelineLayoutInfo.pPushConstantRanges = &pushRange;

    // Logic remains duplicated across Skybox.cpp? 
    // In a master's project, moving this to VulkanUtils::createPipelineLayout would be the next step.
    if (vkCreatePipelineLayout(context->device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("PostProcessor: Failed to create pipeline layout!");
    }

    // --- 3. FINAL DEFINITIONS (Satisfies OPT.20) ---
    // Vertex Input: Positioned exactly before usage to minimize stack lifetime
    const VkPipelineVertexInputStateCreateInfo vertexInputInfo{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };

    const VkGraphicsPipelineCreateInfo pipelineInfo = VulkanUtils::preparePipelineCreateInfo(
        shaderStages, &vertexInputInfo, &inputAssembly, &viewportState,
        &rasterizer, &multisampling, &depthStencil, &colorBlending,
        &dynamicState, pipelineLayout, finalRenderPass
    );

    if (vkCreateGraphicsPipelines(context->device, VK_NULL_HANDLE, 1U, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
        throw std::runtime_error("PostProcessor: Failed to create graphics pipeline!");
    }
}

/**
 * @brief Unified RenderPass factory.
 * Handles the logic switch for MSAA HDR rendering and Resolve targets.
 */
void PostProcessor::internalCreateRenderPass(const bool isTransparent) {
    // 1. Determine Ops based on pass type
    // Opaque pass clears the screen; Transparent pass loads existing data.
    const VkAttachmentLoadOp colorLoadOp = isTransparent ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR;
    const VkAttachmentLoadOp depthLoadOp = isTransparent ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR;

    // Transparent pass starts and ends in COLOR_ATTACHMENT_OPTIMAL layout
    const VkImageLayout initialLayout = isTransparent ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;

    // 2. Multi-sampled Color Attachment (8x HDR)
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = hdrFormat;
    colorAttachment.samples = msaaSamples;
    colorAttachment.loadOp = colorLoadOp;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = initialLayout;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // 3. Multi-sampled Depth Attachment (8x D32)
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = depthFormat;
    depthAttachment.samples = msaaSamples;
    depthAttachment.loadOp = depthLoadOp;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = isTransparent ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // 4. Resolve Attachment (1x HDR)
    // The resolve target is where the MSAA data is flattened for sampling.
    VkAttachmentDescription resolveAttachment{};
    resolveAttachment.format = hdrFormat;
    resolveAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    resolveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    resolveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    resolveAttachment.initialLayout = isTransparent ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;
    resolveAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // 5. Subpass References
    const VkAttachmentReference colorRef{ 0U, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    const VkAttachmentReference depthRef{ 1U, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
    const VkAttachmentReference resolveRef{ 2U, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1U;
    subpass.pColorAttachments = &colorRef;
    subpass.pDepthStencilAttachment = &depthRef;
    subpass.pResolveAttachments = &resolveRef;

    // 6. Dependencies: External synchronization
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0U;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0U;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    // 7. Creation
    const std::array<VkAttachmentDescription, 3U> attachments = { colorAttachment, depthAttachment, resolveAttachment };
    VkRenderPassCreateInfo renderPassInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1U;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1U;
    renderPassInfo.pDependencies = &dependency;

    // Select the correct pointer to the member handle
    VkRenderPass* const targetPass = isTransparent ? &transparentRenderPass : &offscreenRenderPass;

    if (vkCreateRenderPass(context->device, &renderPassInfo, nullptr, targetPass) != VK_SUCCESS) {
        throw std::runtime_error("PostProcessor: Failed to create internal render pass!");
    }
}