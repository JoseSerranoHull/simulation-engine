#include "../include/Skybox.h"

/* parasoft-begin-suppress ALL */
#include <stdexcept>
#include <array>
#include <cstring>
/* parasoft-end-suppress ALL */

/**
 * @brief Constructor: Initializes skybox geometry, descriptors, and the graphics pipeline.
 */
Skybox::Skybox(const VkRenderPass renderPass, Cubemap* const inTexture, const VkSampleCountFlagBits inMsaa)
    : cubemapTexture(inTexture), msaaSamples(inMsaa)
{
    createVertexBuffer();
    createDescriptorResources();
    createPipeline(renderPass);
}

/**
 * @brief Destructor: Releases all allocated GPU resources.
 */
Skybox::~Skybox() {
    VulkanContext* context = ServiceLocator::GetContext();
    if (context != nullptr && context->device != VK_NULL_HANDLE) {
        vkDestroyBuffer(context->device, vertexBuffer, nullptr);
        vkFreeMemory(context->device, vertexMemory, nullptr);
        vkDestroyDescriptorPool(context->device, descriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(context->device, descriptorSetLayout, nullptr);
        vkDestroyPipeline(context->device, pipeline, nullptr);
        vkDestroyPipelineLayout(context->device, pipelineLayout, nullptr);
    }
}

/**
 * @brief Records the draw command for the skybox.
 */
void Skybox::draw(const VkCommandBuffer commandBuffer, const VkDescriptorSet globalDescriptorSet) const {
    if (pipeline == VK_NULL_HANDLE) {
        return;
    }

    // Step 1: Bind the specialized skybox pipeline and vertex data
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    const VkBuffer vertexBuffers[EngineConstants::COUNT_ONE] = { vertexBuffer };
    const VkDeviceSize offsets[EngineConstants::COUNT_ONE] = { EngineConstants::SZ_ZERO };
    vkCmdBindVertexBuffers(commandBuffer, EngineConstants::INDEX_ZERO, EngineConstants::COUNT_ONE, vertexBuffers, offsets);

    // Step 2: Bind descriptors (Global UBO and Skybox Cubemap)
    const uint32_t setCount = 2U;
    const VkDescriptorSet sets[setCount] = { globalDescriptorSet, descriptorSet };
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
        EngineConstants::INDEX_ZERO, setCount, sets, EngineConstants::OFFSET_ZERO, nullptr);

    // Step 3: Dispatch the draw call for the cube geometry
    const uint32_t vertexCount = static_cast<uint32_t>(vertices.size() / VERT_FLOATS_PER_POS);
    vkCmdDraw(commandBuffer, vertexCount, EngineConstants::COUNT_ONE, EngineConstants::OFFSET_ZERO, EngineConstants::OFFSET_ZERO);
}

/**
 * @brief Allocates and populates the GPU vertex buffer.
 */
void Skybox::createVertexBuffer() const {
    const VkDeviceSize bufferSize = static_cast<VkDeviceSize>(sizeof(float) * vertices.size());

    // Step 1: Create a staging buffer to transfer CPU data to GPU memory
    VkBuffer stagingBuffer{ VK_NULL_HANDLE };
    VkDeviceMemory stagingMemory{ VK_NULL_HANDLE };

    VulkanContext* context = ServiceLocator::GetContext();

    VulkanUtils::createBuffer(context->device, context->physicalDevice, bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
        stagingBuffer, stagingMemory);

    void* data{ nullptr };
    static_cast<void>(vkMapMemory(context->device, stagingMemory, EngineConstants::SZ_ZERO, bufferSize, 0U, &data));
    static_cast<void>(std::memcpy(data, vertices.data(), static_cast<size_t>(bufferSize)));
    vkUnmapMemory(context->device, stagingMemory);

    // Step 2: Create the Device-Local vertex buffer for optimal rendering performance
    VulkanUtils::createBuffer(context->device, context->physicalDevice, bufferSize,
        (VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT),
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexMemory);

    // Step 3: Execute the buffer copy command
    VulkanUtils::copyBuffer(context->device, context->graphicsCommandPool, context->graphicsQueue,
        stagingBuffer, vertexBuffer, bufferSize);

    vkDestroyBuffer(context->device, stagingBuffer, nullptr);
    vkFreeMemory(context->device, stagingMemory, nullptr);
}

/**
 * @brief Sets up descriptors for the Cubemap sampler.
 */
void Skybox::createDescriptorResources() const {
    VulkanContext* context = ServiceLocator::GetContext();

    // Step 1: Create the Descriptor Set Layout for the fragment sampler
    VkDescriptorSetLayoutBinding samplerLayoutBinding{
        EngineConstants::INDEX_ZERO, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        EngineConstants::COUNT_ONE, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr
    };

    VkDescriptorSetLayoutCreateInfo layoutInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    layoutInfo.bindingCount = EngineConstants::COUNT_ONE;
    layoutInfo.pBindings = &samplerLayoutBinding;
    static_cast<void>(vkCreateDescriptorSetLayout(context->device, &layoutInfo, nullptr, &descriptorSetLayout));

    // Step 2: Initialize a dedicated Descriptor Pool
    VkDescriptorPoolSize poolSize{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, EngineConstants::COUNT_ONE };
    VkDescriptorPoolCreateInfo poolInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    poolInfo.poolSizeCount = EngineConstants::COUNT_ONE;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = EngineConstants::COUNT_ONE;
    static_cast<void>(vkCreateDescriptorPool(context->device, &poolInfo, nullptr, &descriptorPool));

    // Step 3: Allocate and update the Descriptor Set with cubemap handles
    VkDescriptorSetAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = EngineConstants::COUNT_ONE;
    allocInfo.pSetLayouts = &descriptorSetLayout;
    static_cast<void>(vkAllocateDescriptorSets(context->device, &allocInfo, &descriptorSet));

    VkDescriptorImageInfo imageInfo{
        cubemapTexture->getSampler(), cubemapTexture->getImageView(),
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    VkWriteDescriptorSet descriptorWrite{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    descriptorWrite.dstSet = descriptorSet;
    descriptorWrite.dstBinding = EngineConstants::INDEX_ZERO;
    descriptorWrite.descriptorCount = EngineConstants::COUNT_ONE;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(context->device, EngineConstants::COUNT_ONE, &descriptorWrite, 0U, nullptr);
}

/**
 * @brief Constructs the specialized Graphics Pipeline for the Skybox.
 */
void Skybox::createPipeline(const VkRenderPass renderPass) const {
    // Step 1: Load Skybox Shaders
    const ShaderModule vertShader("./shaders/skybox_vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    const ShaderModule fragShader("./shaders/skybox_frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    const VkPipelineShaderStageCreateInfo shaderStages[2U] = {
        vertShader.getStageInfo(),
        fragShader.getStageInfo()
    };

    VulkanContext* context = ServiceLocator::GetContext();

    // Step 2: Pipeline Layout Creation (Primary exception boundary)
    const std::array<VkDescriptorSetLayout, 2U> layouts = {
        context->globalSetLayout,
        descriptorSetLayout
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
    pipelineLayoutInfo.pSetLayouts = layouts.data();

    if (vkCreatePipelineLayout(context->device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Skybox: Failed to create pipeline layout!");
    }

    // Step 3: Define Fixed-Function States (Postponed for safety)
    const uint32_t stride = static_cast<uint32_t>(VERT_FLOATS_PER_POS * sizeof(float));
    VkVertexInputBindingDescription bindingDescription{ EngineConstants::INDEX_ZERO, stride, VK_VERTEX_INPUT_RATE_VERTEX };
    VkVertexInputAttributeDescription attributeDescription{
        EngineConstants::INDEX_ZERO, EngineConstants::INDEX_ZERO,
        VK_FORMAT_R32G32B32_SFLOAT, EngineConstants::OFFSET_ZERO
    };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
    vertexInputInfo.vertexBindingDescriptionCount = EngineConstants::COUNT_ONE;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = EngineConstants::COUNT_ONE;
    vertexInputInfo.pVertexAttributeDescriptions = &attributeDescription;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewportState{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
    viewportState.viewportCount = EngineConstants::COUNT_ONE;
    viewportState.scissorCount = EngineConstants::COUNT_ONE;

    // Use VulkanUtils for deduplicated boilerplate states
    const std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    const VkPipelineDynamicStateCreateInfo dynamicState = VulkanUtils::prepareDynamicState(dynamicStates);
    const VkPipelineRasterizationStateCreateInfo rasterizer = VulkanUtils::prepareRasterizer(VK_CULL_MODE_NONE);
    const VkPipelineMultisampleStateCreateInfo multisampling = VulkanUtils::prepareMultisampling(msaaSamples);

    // Skybox specifically disables depth writes to remain in the background
    const VkPipelineDepthStencilStateCreateInfo depthStencil = VulkanUtils::prepareDepthStencil(VK_FALSE);

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = (VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT);
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
    colorBlending.attachmentCount = EngineConstants::COUNT_ONE;
    colorBlending.pAttachments = &colorBlendAttachment;

    // Step 4: Finalize Pipeline Creation
    const VkGraphicsPipelineCreateInfo pipelineInfo = VulkanUtils::preparePipelineCreateInfo(
        shaderStages, &vertexInputInfo, &inputAssembly, &viewportState, &rasterizer,
        &multisampling, &depthStencil, &colorBlending, &dynamicState,
        pipelineLayout, renderPass
    );

    if (vkCreateGraphicsPipelines(context->device, VK_NULL_HANDLE, EngineConstants::COUNT_ONE,
        &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
        throw std::runtime_error("Skybox: Failed to create graphics pipeline!");
    }
}