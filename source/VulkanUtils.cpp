#include "../include/VulkanUtils.h"

/* parasoft-begin-suppress ALL */
#include <stdexcept>
#include <cstring>
/* parasoft-end-suppress ALL */

// ========================================================================
// SECTION 1: MEMORY & COMMANDS
// ========================================================================

/**
 * @brief Queries the hardware for a memory type that satisfies the filter and properties.
 */
uint32_t VulkanUtils::findMemoryType(const VkPhysicalDevice physicalDevice, const uint32_t typeFilter, const VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties{};
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0U; i < memProperties.memoryTypeCount; ++i) {
        if ((typeFilter & (SHIFT_ONE << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    throw std::runtime_error("VulkanUtils: Failed to find suitable memory type.");
}

/**
 * @brief Allocates and begins a primary command buffer for one-time GPU transfers.
 */
VkCommandBuffer VulkanUtils::beginSingleTimeCommands(const VkDevice device, const VkCommandPool commandPool) {
    VkCommandBufferAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1U;

    VkCommandBuffer commandBuffer{ VK_NULL_HANDLE };
    static_cast<void>(vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer));

    VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    static_cast<void>(vkBeginCommandBuffer(commandBuffer, &beginInfo));
    return commandBuffer;
}

/**
 * @brief Submits, synchronizes, and frees a one-time command buffer.
 */
void VulkanUtils::endSingleTimeCommands(const VkDevice device, const VkCommandPool commandPool, const VkQueue graphicsQueue, const VkCommandBuffer commandBuffer) {
    static_cast<void>(vkEndCommandBuffer(commandBuffer));

    VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submitInfo.commandBufferCount = 1U;
    submitInfo.pCommandBuffers = &commandBuffer;

    static_cast<void>(vkQueueSubmit(graphicsQueue, 1U, &submitInfo, VK_NULL_HANDLE));
    static_cast<void>(vkQueueWaitIdle(graphicsQueue));

    vkFreeCommandBuffers(device, commandPool, 1U, &commandBuffer);
}

// ========================================================================
// SECTION 2: BUFFER MANAGEMENT
// ========================================================================

/**
 * @brief Creates a Vulkan buffer and allocates its backing device memory.
 */
void VulkanUtils::createBuffer(const VkDevice device, const VkPhysicalDevice physicalDevice, const VkDeviceSize size,
    const VkBufferUsageFlags usage, const VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {

    VkBufferCreateInfo bufferInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("VulkanUtils: Failed to create buffer.");
    }

    const VkMemoryRequirements memReqs = getBufferMemoryRequirements(device, buffer);
    VkMemoryAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memReqs.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("VulkanUtils: Failed to allocate buffer memory.");
    }

    static_cast<void>(vkBindBufferMemory(device, buffer, bufferMemory, 0U));
}

/**
 * @brief Executes a GPU-side copy between two buffer resources.
 */
void VulkanUtils::copyBuffer(const VkDevice device, const VkCommandPool commandPool, const VkQueue graphicsQueue,
    const VkBuffer srcBuffer, const VkBuffer dstBuffer, const VkDeviceSize size) {

    const VkCommandBuffer cb = beginSingleTimeCommands(device, commandPool);
    VkBufferCopy copyRegion{};
    copyRegion.size = size;

    vkCmdCopyBuffer(cb, srcBuffer, dstBuffer, 1U, &copyRegion);
    endSingleTimeCommands(device, commandPool, graphicsQueue, cb);
}

// ========================================================================
// SECTION 3: IMAGE & TEXTURE MANAGEMENT
// ========================================================================

/**
 * @brief Creates a Vulkan image and allocates its backing device memory.
 */
void VulkanUtils::createImage(const VkDevice device, const VkPhysicalDevice physicalDevice, const uint32_t width, const uint32_t height,
    const uint32_t mipLevels, const VkSampleCountFlagBits numSamples, const VkFormat format, const VkImageTiling tiling,
    const VkImageUsageFlags usage, const VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory,
    const uint32_t arrayLayers, const VkImageCreateFlags flags) {

    VkImageCreateInfo imageInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = { width, height, 1U };
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = arrayLayers;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = numSamples;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.flags = flags;

    if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("VulkanUtils: Failed to create image.");
    }

    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(device, image, &memReqs);

    VkMemoryAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memReqs.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("VulkanUtils: Failed to allocate image memory.");
    }

    static_cast<void>(vkBindImageMemory(device, image, imageMemory, 0U));
}

/**
 * @brief Procedurally generates a mipmap chain for a texture using blit commands.
 */
void VulkanUtils::generateMipmaps(const VkDevice device, const VkPhysicalDevice physicalDevice, const VkCommandPool commandPool,
    const VkQueue graphicsQueue, const VkImage image, const VkFormat imageFormat, const int32_t texWidth, const int32_t texHeight,
    const uint32_t mipLevels, const uint32_t layerCount) {

    VkFormatProperties formatProps;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProps);
    if (!(formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
        throw std::runtime_error("VulkanUtils: Format does not support linear blitting!");
    }

    const VkCommandBuffer cb = beginSingleTimeCommands(device, commandPool);

    VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0U;
    barrier.subresourceRange.layerCount = layerCount;
    barrier.subresourceRange.levelCount = 1U;

    int32_t mipW = texWidth;
    int32_t mipH = texHeight;

    // Iterate through mip chain levels
    for (uint32_t i = 1U; i < mipLevels; i++) {
        // Step 1: Transition previous mip level to TRANSFER_SRC
        barrier.subresourceRange.baseMipLevel = i - 1U;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0U, 0U, nullptr, 0U, nullptr, 1U, &barrier);

        // Step 2: Blit from previous mip to current mip (downsampling)
        VkImageBlit blit{};
        blit.srcOffsets[1] = { mipW, mipH, 1 };
        blit.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, i - 1U, 0U, layerCount };
        blit.dstOffsets[1] = { mipW > 1 ? mipW / 2 : 1, mipH > 1 ? mipH / 2 : 1, 1 };
        blit.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, i, 0U, layerCount };

        vkCmdBlitImage(cb, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1U, &blit, VK_FILTER_LINEAR);

        // Step 3: Transition previous mip level to SHADER_READ_ONLY
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0U, 0U, nullptr, 0U, nullptr, 1U, &barrier);

        if (mipW > 1) { mipW /= 2; }
        if (mipH > 1) { mipH /= 2; }
    }

    // Step 4: Transition final mip level to SHADER_READ_ONLY
    barrier.subresourceRange.baseMipLevel = mipLevels - 1U;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0U, 0U, nullptr, 0U, nullptr, 1U, &barrier);

    endSingleTimeCommands(device, commandPool, graphicsQueue, cb);
}

// ========================================================================
// SECTION 4: IMAGE VIEWS & TRANSITIONS
// ========================================================================

/**
 * @brief Generates an ImageView for a GPU image.
 */
VkImageView VulkanUtils::createImageView(const VkDevice device, const VkImage image, const VkFormat format,
    const VkImageAspectFlags aspectFlags, const uint32_t mipLevels, const VkImageViewType viewType, const uint32_t layerCount)
{
    VkImageViewCreateInfo viewInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    viewInfo.image = image;
    viewInfo.viewType = viewType;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0U;
    viewInfo.subresourceRange.levelCount = mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0U;
    viewInfo.subresourceRange.layerCount = layerCount;

    VkImageView imageView{ VK_NULL_HANDLE };
    if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("VulkanUtils: Failed to generate hardware image view.");
    }
    return imageView;
}

/**
 * @brief Transitions image layouts via a one-time command buffer.
 */
void VulkanUtils::transitionImageLayout(const VkDevice device, const VkCommandPool commandPool, const VkQueue graphicsQueue,
    const VkImage image, const VkFormat format, const VkImageLayout oldLayout, const VkImageLayout newLayout,
    const uint32_t mipLevels, const uint32_t layerCount)
{
    static_cast<void>(format);

    const VkCommandBuffer cb = beginSingleTimeCommands(device, commandPool);

    VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0U;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0U;
    barrier.subresourceRange.layerCount = layerCount;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if ((oldLayout == VK_IMAGE_LAYOUT_UNDEFINED) && (newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)) {
        barrier.srcAccessMask = 0U;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if ((oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) && (newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else {
        throw std::invalid_argument("VulkanUtils: Unsupported layout transition encountered.");
    }

    vkCmdPipelineBarrier(cb, sourceStage, destinationStage, 0U, 0U, nullptr, 0U, nullptr, 1U, &barrier);
    endSingleTimeCommands(device, commandPool, graphicsQueue, cb);
}

/**
 * @brief Records a memory barrier into an existing command buffer.
 */
void VulkanUtils::recordImageBarrier(VkCommandBuffer cb, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout,
    VkAccessFlags srcAccess, VkAccessFlags dstAccess, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, uint32_t mipLevels)
{
    VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0U;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0U;
    barrier.subresourceRange.layerCount = 1U;
    barrier.srcAccessMask = srcAccess;
    barrier.dstAccessMask = dstAccess;

    vkCmdPipelineBarrier(cb, srcStage, dstStage, 0U, 0U, nullptr, 0U, nullptr, 1U, &barrier);
}

// ========================================================================
// SECTION 5: SAMPLERS & RENDER PASSES
// ========================================================================

/**
 * @brief Configures a texture sampler for the GPU.
 */
void VulkanUtils::createTextureSampler(const VkDevice device, VkSampler& sampler, const uint32_t mipLevels, const VkSamplerAddressMode addressMode) {
    VkSamplerCreateInfo samplerInfo{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = addressMode;
    samplerInfo.addressModeV = addressMode;
    samplerInfo.addressModeW = addressMode;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = DEFAULT_ANISOTROPY;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.minLod = LOD_ZERO;
    samplerInfo.maxLod = static_cast<float>(mipLevels);

    if (vkCreateSampler(device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
        throw std::runtime_error("VulkanUtils: Failed to create texture sampler.");
    }
}

/**
 * @brief Specialized factory for Shadow Map RenderPasses.
 */
VkRenderPass VulkanUtils::createDepthRenderPass(const VkDevice device, const VkFormat depthFormat) {
    VkAttachmentDescription dAttr{};
    dAttr.format = depthFormat;
    dAttr.samples = VK_SAMPLE_COUNT_1_BIT;
    dAttr.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    dAttr.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    dAttr.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    dAttr.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference depthRef{ 0U, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

    VkSubpassDescription sub{};
    sub.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    sub.pDepthStencilAttachment = &depthRef;

    VkRenderPassCreateInfo passInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    passInfo.attachmentCount = 1U;
    passInfo.pAttachments = &dAttr;
    passInfo.subpassCount = 1U;
    passInfo.pSubpasses = &sub;

    VkRenderPass pass{ VK_NULL_HANDLE };
    if (vkCreateRenderPass(device, &passInfo, nullptr, &pass) != VK_SUCCESS) {
        throw std::runtime_error("VulkanUtils: Depth render pass creation failed.");
    }
    return pass;
}

/**
 * @brief Transfers pixel data from a staging buffer into a GPU Image.
 */
void VulkanUtils::copyBufferToImage(const VkDevice device, const VkCommandPool commandPool, const VkQueue graphicsQueue,
    const VkBuffer buffer, const VkImage image, const uint32_t width, const uint32_t height, const uint32_t layerCount)
{
    const VkCommandBuffer cb = beginSingleTimeCommands(device, commandPool);

    VkBufferImageCopy region{};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = layerCount;
    region.imageExtent = { width, height, 1U };

    vkCmdCopyBufferToImage(cb, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1U, &region);

    endSingleTimeCommands(device, commandPool, graphicsQueue, cb);
}

/**
 * @brief Helper to initialize the complex Graphics Pipeline creation structure.
 */
VkGraphicsPipelineCreateInfo VulkanUtils::preparePipelineCreateInfo(
    const VkPipelineShaderStageCreateInfo* shaderStages,
    const VkPipelineVertexInputStateCreateInfo* vertexInput,
    const VkPipelineInputAssemblyStateCreateInfo* inputAssembly,
    const VkPipelineViewportStateCreateInfo* viewport,
    const VkPipelineRasterizationStateCreateInfo* rasterizer,
    const VkPipelineMultisampleStateCreateInfo* multisampling,
    const VkPipelineDepthStencilStateCreateInfo* depthStencil,
    const VkPipelineColorBlendStateCreateInfo* colorBlending,
    const VkPipelineDynamicStateCreateInfo* dynamicState,
    const VkPipelineLayout layout,
    const VkRenderPass renderPass)
{
    VkGraphicsPipelineCreateInfo pipelineInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
    pipelineInfo.stageCount = 2U;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = vertexInput;
    pipelineInfo.pInputAssemblyState = inputAssembly;
    pipelineInfo.pViewportState = viewport;
    pipelineInfo.pRasterizationState = rasterizer;
    pipelineInfo.pMultisampleState = multisampling;
    pipelineInfo.pDepthStencilState = depthStencil;
    pipelineInfo.pColorBlendState = colorBlending;
    pipelineInfo.pDynamicState = dynamicState;
    pipelineInfo.layout = layout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0U;

    return pipelineInfo;
}

VkPipelineDynamicStateCreateInfo VulkanUtils::prepareDynamicState(const std::vector<VkDynamicState>& states) {
    VkPipelineDynamicStateCreateInfo info{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
    info.dynamicStateCount = static_cast<uint32_t>(states.size());
    info.pDynamicStates = states.data();
    return info;
}

VkPipelineRasterizationStateCreateInfo VulkanUtils::prepareRasterizer(VkCullModeFlags cullMode) {
    VkPipelineRasterizationStateCreateInfo info{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
    info.polygonMode = VK_POLYGON_MODE_FILL;
    info.lineWidth = 1.0f;
    info.cullMode = cullMode;
    info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    return info;
}

VkPipelineMultisampleStateCreateInfo VulkanUtils::prepareMultisampling(VkSampleCountFlagBits samples) {
    VkPipelineMultisampleStateCreateInfo info{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
    info.rasterizationSamples = samples;
    return info;
}

VkPipelineDepthStencilStateCreateInfo VulkanUtils::prepareDepthStencil(VkBool32 depthWrite) {
    VkPipelineDepthStencilStateCreateInfo info{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
    info.depthTestEnable = VK_TRUE;
    info.depthWriteEnable = depthWrite;
    info.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    return info;
}