#include "../include/Skybox.h"

/* parasoft-begin-suppress ALL */
#include <stdexcept>
#include <array>
#include <cstring>
#include "../include/Logger.h"
/* parasoft-end-suppress ALL */

/**
 * @brief Constructor: Initializes skybox geometry, descriptors, and the graphics pipeline.
 */
Skybox::Skybox(const VkRenderPass renderPass, Cubemap* const inTexture, const VkSampleCountFlagBits inMsaa)
    : msaaSamples(inMsaa)
{
    // If a texture was passed (old style), we wrap it. 
    // Otherwise, we wait for the SceneLoader to call loadTextures().
    if (inTexture != nullptr) {
        cubemapTexture.reset(inTexture);
    }

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
 /**
  * @brief Records the draw command for the skybox, including the 128-byte push constant.
  */
void Skybox::draw(
    const VkCommandBuffer commandBuffer,
    const VkDescriptorSet globalDescriptorSet,
    const glm::mat4& quadrantVP // NEW: Pass the pre-calculated VP matrix
) const {
    if (pipeline == VK_NULL_HANDLE) {
        return;
    }

    // 1. Bind Pipeline
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    // 2. NEW: Push 128-byte Constants (VP + Padding)
    // We only need the VP (mvp) for skybox positioning. 
    // The second 64 bytes is padding to satisfy the 128-byte shader layout.
    struct SkyboxPushConstants {
        glm::mat4 vp;      // 64 bytes
        glm::mat4 padding; // 64 bytes
    } constants;

    constants.vp = quadrantVP;
    constants.padding = glm::mat4(1.0f); // Identity padding

    vkCmdPushConstants(
        commandBuffer,
        pipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT,
        0,
        sizeof(SkyboxPushConstants), // 128 bytes
        &constants
    );

    // 3. Bind Vertex Buffers
    const VkBuffer vertexBuffers[1] = { vertexBuffer };
    const VkDeviceSize offsets[1] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

    // 4. Bind Descriptors (Set 0: Global, Set 1: Cubemap)
    const uint32_t setCount = 2U;
    const VkDescriptorSet sets[setCount] = { globalDescriptorSet, descriptorSet };
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
        0, setCount, sets, 0, nullptr);

    // 5. Draw
    const uint32_t vertexCount = static_cast<uint32_t>(vertices.size() / VERT_FLOATS_PER_POS);
    vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
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

    // Step 1: Create the Descriptor Set Layout (Must happen regardless of texture)
    VkDescriptorSetLayoutBinding samplerLayoutBinding{
        EngineConstants::INDEX_ZERO, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        EngineConstants::COUNT_ONE, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr
    };

    VkDescriptorSetLayoutCreateInfo layoutInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    layoutInfo.bindingCount = EngineConstants::COUNT_ONE;
    layoutInfo.pBindings = &samplerLayoutBinding;
    if (vkCreateDescriptorSetLayout(context->device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("Skybox: Failed to create descriptor set layout!");
    }

    // Step 2: Initialize a dedicated Descriptor Pool (Must happen regardless of texture)
    VkDescriptorPoolSize poolSize{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, EngineConstants::COUNT_ONE };
    VkDescriptorPoolCreateInfo poolInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    poolInfo.poolSizeCount = EngineConstants::COUNT_ONE;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = EngineConstants::COUNT_ONE;
    if (vkCreateDescriptorPool(context->device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("Skybox: Failed to create descriptor pool!");
    }

    // Step 3: Allocate the Descriptor Set
    VkDescriptorSetAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = EngineConstants::COUNT_ONE;
    allocInfo.pSetLayouts = &descriptorSetLayout;
    if (vkAllocateDescriptorSets(context->device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
        throw std::runtime_error("Skybox: Failed to allocate descriptor set!");
    }

    // --- GUARD START ---
    // We only perform the 'Update' if we actually have a texture pointer.
    // If cubemapTexture is nullptr (initial state), we skip this.
    // loadTextures() will call vkUpdateDescriptorSets later when the .ini is read.
    if (cubemapTexture != nullptr) {
        VkDescriptorImageInfo imageInfo{
            cubemapTexture->getSampler(),
            cubemapTexture->getImageView(),
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
    // --- GUARD END ---
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

    // Guard Checks...
    if (renderPass == VK_NULL_HANDLE) {
        throw std::runtime_error("Skybox: renderPass is VK_NULL_HANDLE.");
    }
    if (context->globalSetLayout == VK_NULL_HANDLE || descriptorSetLayout == VK_NULL_HANDLE) {
        throw std::runtime_error("Skybox: Descriptor layouts not initialized.");
    }

    // --- FIX: Define the 128-byte Push Constant Range ---
    // This resolves the VUID-VkGraphicsPipelineCreateInfo-layout-07987 error.
    const VkPushConstantRange pushConstantRange{
        VK_SHADER_STAGE_VERTEX_BIT,
        0U,
        static_cast<uint32_t>(sizeof(glm::mat4) * 2) // 128 bytes total
    };

    const std::array<VkDescriptorSetLayout, 2U> layouts = {
        context->globalSetLayout,
        descriptorSetLayout
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
    pipelineLayoutInfo.pSetLayouts = layouts.data();

    // NEW: Add the push constant range to the layout info
    pipelineLayoutInfo.pushConstantRangeCount = 1U;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(context->device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Skybox: Failed to create pipeline layout!");
    }

    // Step 3: Define Fixed-Function States
    const uint32_t stride = static_cast<uint32_t>(VERT_FLOATS_PER_POS * sizeof(float));
    VkVertexInputBindingDescription bindingDescription{ 0, stride, VK_VERTEX_INPUT_RATE_VERTEX };
    VkVertexInputAttributeDescription attributeDescription{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
    vertexInputInfo.vertexBindingDescriptionCount = 1U;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = 1U;
    vertexInputInfo.pVertexAttributeDescriptions = &attributeDescription;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewportState{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
    viewportState.viewportCount = 1U;
    viewportState.scissorCount = 1U;

    const std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    const VkPipelineDynamicStateCreateInfo dynamicState = VulkanUtils::prepareDynamicState(dynamicStates);
    const VkPipelineRasterizationStateCreateInfo rasterizer = VulkanUtils::prepareRasterizer(VK_CULL_MODE_NONE);
    const VkPipelineMultisampleStateCreateInfo multisampling = VulkanUtils::prepareMultisampling(msaaSamples);
    const VkPipelineDepthStencilStateCreateInfo depthStencil = VulkanUtils::prepareDepthStencil(VK_FALSE);

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = (VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT);
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
    colorBlending.attachmentCount = 1U;
    colorBlending.pAttachments = &colorBlendAttachment;

    // Step 4: Finalize Pipeline Creation
    const VkGraphicsPipelineCreateInfo pipelineInfo = VulkanUtils::preparePipelineCreateInfo(
        shaderStages, &vertexInputInfo, &inputAssembly, &viewportState, &rasterizer,
        &multisampling, &depthStencil, &colorBlending, &dynamicState,
        pipelineLayout, renderPass
    );

    if (vkCreateGraphicsPipelines(context->device, VK_NULL_HANDLE, 1U, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
        throw std::runtime_error("Skybox: Failed to create graphics pipeline!");
    }
}

/**
 * @brief Updates the GPU cubemap and refreshes the sampler descriptors.
 */
void Skybox::loadTextures(const std::vector<std::string>& facePaths) {
    if (facePaths.size() != 6U) {
        throw std::runtime_error("Skybox: loadTextures requires exactly 6 face paths!");
    }

    VulkanContext* context = ServiceLocator::GetContext();

    // Step 1: Create the new Cubemap object (This handles the GPU image/view creation)
    // We replace the old unique_ptr, which automatically triggers the old Cubemap's destructor
    cubemapTexture = std::make_unique<Cubemap>(facePaths);

    // Step 2: Refresh the Descriptor Set
    // We must tell the GPU to use the handles from the newly loaded texture
    VkDescriptorImageInfo imageInfo{
        cubemapTexture->getSampler(),
        cubemapTexture->getImageView(),
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    VkWriteDescriptorSet descriptorWrite{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    descriptorWrite.dstSet = descriptorSet;
    descriptorWrite.dstBinding = EngineConstants::INDEX_ZERO;
    descriptorWrite.descriptorCount = EngineConstants::COUNT_ONE;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(context->device, EngineConstants::COUNT_ONE, &descriptorWrite, 0U, nullptr);

    m_texturesLoaded = true; // Set this after vkUpdateDescriptorSets

    GE_LOG_INFO("Skybox: Successfully updated cubemap textures from Registry.");
}