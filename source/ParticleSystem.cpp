#include "../include/ParticleSystem.h"

/* parasoft-begin-suppress ALL */
#include <stdexcept>
#include <array>
#include <chrono>
#include <cstring>
#include <random>
/* parasoft-end-suppress ALL */

// ========================================================================
// SECTION 1: LIFECYCLE MANAGEMENT
// ========================================================================

/**
 * @brief Constructor: Initializes the Compute simulation and Graphics rendering sub-systems.
 */
ParticleSystem::ParticleSystem(
    const VkRenderPass renderPass,
    const VkDescriptorSetLayout inGlobalSetLayout,
    const std::string& compPath,
    const std::string& vertPath,
    const std::string& fragPath,
    const glm::vec3& spawnPos,
    const uint32_t maxParticles,
    const VkSampleCountFlagBits inMsaa)
    : globalSetLayout(inGlobalSetLayout),
    particleCount(maxParticles),
    msaaSamples(inMsaa)
{
    createBuffers(spawnPos);
    createComputeDescriptors();
    createComputePipeline(compPath);
    createGraphicsPipeline(renderPass, vertPath, fragPath);
}

/**
 * @brief Destructor: Releases all GPU pipelines, layouts, and buffers.
 */
ParticleSystem::~ParticleSystem() {
    VulkanContext* context = ServiceLocator::GetContext();

    if ((context != nullptr) && (context->device != VK_NULL_HANDLE)) {
        vkDestroyPipeline(context->device, computePipeline, nullptr);
        vkDestroyPipelineLayout(context->device, computePipelineLayout, nullptr);
        vkDestroyPipeline(context->device, graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(context->device, graphicsPipelineLayout, nullptr);

        vkDestroyDescriptorPool(context->device, computeDescriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(context->device, computeSetLayout, nullptr);

        vkDestroyBuffer(context->device, storageBuffer, nullptr);
        vkFreeMemory(context->device, storageBufferMemory, nullptr);

        if (uniformBufferMapped != nullptr) {
            vkUnmapMemory(context->device, uniformBufferMemory);
            uniformBufferMapped = nullptr;
        }
        vkDestroyBuffer(context->device, uniformBuffer, nullptr);
        vkFreeMemory(context->device, uniformBufferMemory, nullptr);
    }
}

// ========================================================================
// SECTION 2: SIMULATION & RENDERING
// ========================================================================

/**
 * @brief Records the Compute dispatch and memory barriers to sync with the Graphics pass.
 */
void ParticleSystem::update(const VkCommandBuffer commandBuffer, const float deltaTime, const bool spawnEnabled,
    const float totalTime, const glm::vec3& lightColor, const glm::vec3& emitterPos)
{
    this->lastEmitterPos = emitterPos;

    // Step 1: Update Simulation Parameters (UBO) using std140 alignment
    ParticleUBO ubo{};
    ubo.deltaTime = deltaTime;
    ubo.spawnEnabled = spawnEnabled ? static_cast<float>(EngineConstants::SHADER_TRUE) : static_cast<float>(EngineConstants::SHADER_FALSE);
    ubo.totalTime = totalTime;
    ubo.padding = 0.0f;
    ubo.lightColor = lightColor;
    ubo.padding2 = 0.0f;
    ubo.emitterPos = emitterPos;
    ubo.padding3 = 0.0f;

    static_cast<void>(std::memcpy(uniformBufferMapped, &ubo, sizeof(ParticleUBO)));

    // Step 2: Dispatch Compute Shader for physics simulation
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
    const VkDescriptorSet sets[DESCRIPTOR_COUNT_ONE] = { computeDescriptorSet };
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout,
        EngineConstants::INDEX_ZERO, DESCRIPTOR_COUNT_ONE, sets, EngineConstants::OFFSET_ZERO, nullptr);

    const uint32_t groupCount = (particleCount / COMPUTE_WORKGROUP_SIZE) + EngineConstants::OFFSET_ONE;
    vkCmdDispatch(commandBuffer, groupCount, EngineConstants::COUNT_ONE, EngineConstants::COUNT_ONE);

    // Step 3: Pipeline Barrier - Ensure Compute writes finish before Vertex Input reads the SSBO
    VkBufferMemoryBarrier bufferBarrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
    bufferBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    bufferBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
    bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bufferBarrier.buffer = storageBuffer;
    bufferBarrier.offset = static_cast<VkDeviceSize>(EngineConstants::OFFSET_ZERO);
    bufferBarrier.size = VK_WHOLE_SIZE;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
        static_cast<VkDependencyFlags>(EngineConstants::OFFSET_ZERO), 0U, nullptr,
        EngineConstants::COUNT_ONE, &bufferBarrier, 0U, nullptr);
}

 /**
  * @brief Records drawing commands for the particles.
  * Fulfills Lab 2: Visualization of simulation from 4 views.
  */
void ParticleSystem::draw(
    const VkCommandBuffer commandBuffer,
    const VkDescriptorSet globalDescriptorSet,
    const glm::mat4& quadrantVP
) const {
    if (graphicsPipeline == VK_NULL_HANDLE) { return; }

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

    const VkDescriptorSet sets[1] = { globalDescriptorSet };
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelineLayout,
        0, 1, sets, 0, nullptr);

    // --- FIX: Push the full 128-byte footprint ---
    struct ParticlePushConstants {
        glm::mat4 vp;      // 64 bytes
        glm::mat4 padding; // 64 bytes (matches 'unused' in shaders)
    } constants;

    constants.vp = quadrantVP;
    constants.padding = glm::mat4(1.0f); // Identity padding

    vkCmdPushConstants(
        commandBuffer,
        graphicsPipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT,
        0,
        sizeof(ParticlePushConstants), // 128 bytes
        &constants
    );

    const VkBuffer vertexBuffers[1] = { storageBuffer };
    const VkDeviceSize offsets[1] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

    vkCmdDraw(commandBuffer, particleCount, 1, 0, 0);
}

/**
 * @brief Retrieves dynamic light emitters from the GPU particle buffer for world-space lighting.
 */
std::vector<SparkLight> ParticleSystem::getLightData() const {
    std::vector<SparkLight> lights(EngineConstants::MAX_SPARK_LIGHTS);
    void* data{ nullptr };

    const VkDeviceSize mapSize = static_cast<VkDeviceSize>(EngineConstants::PARTICLE_POOL_SIZE) * sizeof(Particle);
    VulkanContext* context = ServiceLocator::GetContext();
    if (vkMapMemory(context->device, storageBufferMemory, 0ULL, mapSize, 0U, &data) != VK_SUCCESS) {
        return lights;
    }

    const Particle* const gpuParticles = static_cast<const Particle*>(data);

    // Sample specific particle sectors to simulate dynamic flickering lights
    const uint32_t sectors[EngineConstants::MAX_SPARK_LIGHTS] = { 100U, 1000U, 2000U, 3000U };
    static constexpr float LENGTH_THRESHOLD = 0.001f;
    static constexpr float PUSH_FACTOR = 0.15f;

    for (uint32_t i = 0U; i < EngineConstants::MAX_SPARK_LIGHTS; ++i) {
        const uint32_t idx = sectors[i];
        const glm::vec3 pos = glm::vec3(gpuParticles[idx].position);
        const glm::vec2 toCenter{ pos.x - lastEmitterPos.x, pos.z - lastEmitterPos.z };

        if (glm::length(toCenter) > LENGTH_THRESHOLD) {
            const glm::vec2 pushDir = glm::normalize(toCenter);
            lights[i].position = glm::vec3(pos.x + (pushDir.x * PUSH_FACTOR), pos.y, pos.z + (pushDir.y * PUSH_FACTOR));
        }
        else {
            lights[i].position = pos;
        }

        const float life = gpuParticles[idx].velocity.w;
        lights[i].color = glm::vec3(1.0f, 0.45f, 0.1f) * (life * 0.04f);
    }

    vkUnmapMemory(context->device, storageBufferMemory);
    return lights;
}

// ========================================================================
// SECTION 3: INTERNAL INITIALIZATION
// ========================================================================

void ParticleSystem::createBuffers(const glm::vec3& spawnPos) {
    // Step 1: Generate initial particle state on the CPU
    std::vector<Particle> particles(EngineConstants::PARTICLE_POOL_SIZE);
    const auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::default_random_engine rndEngine(static_cast<unsigned>(seed));
    std::uniform_real_distribution<float> rndLife(0.0f, 1.0f);

    for (auto& p : particles) {
        const float initialYOffset = rndLife(rndEngine) * 0.2f;
        p.position = glm::vec4(spawnPos.x, spawnPos.y + initialYOffset, spawnPos.z, 2.0f);
        p.velocity = glm::vec4(0.0f, 0.0f, 0.0f, rndLife(rndEngine));
        p.color = glm::vec4(1.0f);
    }

    const VkDeviceSize bufferSize = static_cast<VkDeviceSize>(sizeof(Particle)) * EngineConstants::PARTICLE_POOL_SIZE;

    // Step 2: Use a staging buffer to transfer particle data to Device Local memory
    VkBuffer stagingBuffer{ VK_NULL_HANDLE };
    VkDeviceMemory stagingMemory{ VK_NULL_HANDLE };

    VulkanContext* context = ServiceLocator::GetContext();

    VulkanUtils::createBuffer(context->device, context->physicalDevice, bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
        stagingBuffer, stagingMemory);

    void* data{ nullptr };
    static_cast<void>(vkMapMemory(context->device, stagingMemory, 0ULL, bufferSize, 0U, &data));
    static_cast<void>(std::memcpy(data, particles.data(), static_cast<size_t>(bufferSize)));
    vkUnmapMemory(context->device, stagingMemory);

    VulkanUtils::createBuffer(context->device, context->physicalDevice, bufferSize,
        (VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT),
        (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
        storageBuffer, storageBufferMemory);

    VulkanUtils::copyBuffer(context->device, context->graphicsCommandPool, context->graphicsQueue, stagingBuffer, storageBuffer, bufferSize);

    vkDestroyBuffer(context->device, stagingBuffer, nullptr);
    vkFreeMemory(context->device, stagingMemory, nullptr);

    // Step 3: Create the Simulation UBO
    VulkanUtils::createBuffer(context->device, context->physicalDevice, static_cast<VkDeviceSize>(sizeof(ParticleUBO)),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
        uniformBuffer, uniformBufferMemory);

    static_cast<void>(vkMapMemory(context->device, uniformBufferMemory, 0ULL, sizeof(ParticleUBO), 0U, &uniformBufferMapped));
}

void ParticleSystem::createComputeDescriptors() {
    const std::array<VkDescriptorSetLayoutBinding, 2> bindings = {
        VkDescriptorSetLayoutBinding{ BINDING_UBO, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1U, VK_SHADER_STAGE_COMPUTE_BIT, nullptr },
        VkDescriptorSetLayoutBinding{ BINDING_STORAGE, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1U, VK_SHADER_STAGE_COMPUTE_BIT, nullptr }
    };

    VkDescriptorSetLayoutCreateInfo layoutInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    VulkanContext* context = ServiceLocator::GetContext();

    static_cast<void>(vkCreateDescriptorSetLayout(context->device, &layoutInfo, nullptr, &computeSetLayout));

    const std::array<VkDescriptorPoolSize, 2> poolSizes = {
        VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1U },
        VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1U }
    };

    VkDescriptorPoolCreateInfo poolInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = 1U;
    static_cast<void>(vkCreateDescriptorPool(context->device, &poolInfo, nullptr, &computeDescriptorPool));

    VkDescriptorSetAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    allocInfo.descriptorPool = computeDescriptorPool;
    allocInfo.descriptorSetCount = 1U;
    allocInfo.pSetLayouts = &computeSetLayout;
    static_cast<void>(vkAllocateDescriptorSets(context->device, &allocInfo, &computeDescriptorSet));

    const VkDescriptorBufferInfo uboInfo{ uniformBuffer, 0ULL, sizeof(ParticleUBO) };
    const VkDescriptorBufferInfo storageInfo{ storageBuffer, 0ULL, VK_WHOLE_SIZE };

    std::array<VkWriteDescriptorSet, 2> writes{};
    writes[0] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, computeDescriptorSet, BINDING_UBO, 0U, 1U, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr, &uboInfo, nullptr };
    writes[1] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, computeDescriptorSet, BINDING_STORAGE, 0U, 1U, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &storageInfo, nullptr };

    vkUpdateDescriptorSets(context->device, static_cast<uint32_t>(writes.size()), writes.data(), 0U, nullptr);
}

void ParticleSystem::createComputePipeline(const std::string& path) {
    const ShaderModule compShader(path, VK_SHADER_STAGE_COMPUTE_BIT);

    VkPipelineLayoutCreateInfo layoutInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    layoutInfo.setLayoutCount = 1U;
    layoutInfo.pSetLayouts = &computeSetLayout;

    VulkanContext* context = ServiceLocator::GetContext();

    static_cast<void>(vkCreatePipelineLayout(context->device, &layoutInfo, nullptr, &computePipelineLayout));

    VkComputePipelineCreateInfo pipelineInfo{ VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
    pipelineInfo.stage = compShader.getStageInfo();
    pipelineInfo.layout = computePipelineLayout;

    if (vkCreateComputePipelines(context->device, VK_NULL_HANDLE, 1U, &pipelineInfo, nullptr, &computePipeline) != VK_SUCCESS) {
        throw std::runtime_error("ParticleSystem: Failed to create compute pipeline!");
    }
}

/**
 * @brief Constructs the graphics pipeline for rendering particles.
 */
void ParticleSystem::createGraphicsPipeline(const VkRenderPass renderPass, const std::string& vPath, const std::string& fPath) {
    // Step 1: Shader Module Assembly
    const ShaderModule vertShader(vPath, VK_SHADER_STAGE_VERTEX_BIT);
    const ShaderModule fragShader(fPath, VK_SHADER_STAGE_FRAGMENT_BIT);
    const VkPipelineShaderStageCreateInfo shaderStages[2] = { vertShader.getStageInfo(), fragShader.getStageInfo() };

    // Step 2: Define Layout with global and material descriptor sets
    VulkanContext * context = ServiceLocator::GetContext();

    // --- NEW: Define Push Constant Range for Multiview Matrix ---
    const VkPushConstantRange pushConstantRange{
        VK_SHADER_STAGE_VERTEX_BIT,
        0U,
        static_cast<uint32_t>(sizeof(glm::mat4) * 2) // CHANGE: Match the 128-byte shader block
    };

    const std::array<VkDescriptorSetLayout, 2> layouts = { globalSetLayout, context->materialSetLayout };
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
    pipelineLayoutInfo.pSetLayouts = layouts.data();

    // --- FIX: Register the push constant range in the layout ---
    pipelineLayoutInfo.pushConstantRangeCount = 1U;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(context->device, &pipelineLayoutInfo, nullptr, &graphicsPipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("ParticleSystem: Failed to create graphics pipeline layout!");
    }

    // Step 3: Vertex Input - Reading directly from the simulated Storage Buffer
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0U;
    bindingDescription.stride = static_cast<uint32_t>(sizeof(Particle));
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    const std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {
        VkVertexInputAttributeDescription{ 0U, 0U, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Particle, position) },
        VkVertexInputAttributeDescription{ 1U, 0U, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Particle, velocity) },
        VkVertexInputAttributeDescription{ 2U, 0U, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Particle, color) }
    };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
    vertexInputInfo.vertexBindingDescriptionCount = 1U;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    // Step 4: Fixed-Function Assembly State
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
    viewportState.viewportCount = 1U;
    viewportState.scissorCount = 1U;

    const std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    const VkPipelineDynamicStateCreateInfo dynamicState = VulkanUtils::prepareDynamicState(dynamicStates);

    const VkPipelineRasterizationStateCreateInfo rasterizer = VulkanUtils::prepareRasterizer(VK_CULL_MODE_NONE);
    const VkPipelineMultisampleStateCreateInfo multisampling = VulkanUtils::prepareMultisampling(msaaSamples);
    const VkPipelineDepthStencilStateCreateInfo depthStencil = VulkanUtils::prepareDepthStencil(VK_FALSE);

    // Step 5: Color Blending for Alpha-transparent Particles
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1U;
    colorBlending.pAttachments = &colorBlendAttachment;

    // Step 6: Final Pipeline Creation
    const VkGraphicsPipelineCreateInfo pipelineInfo = VulkanUtils::preparePipelineCreateInfo(
        shaderStages, &vertexInputInfo, &inputAssembly, &viewportState, &rasterizer,
        &multisampling, &depthStencil, &colorBlending, &dynamicState,
        graphicsPipelineLayout, renderPass
    );

    if (vkCreateGraphicsPipelines(context->device, VK_NULL_HANDLE, 1U, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("ParticleSystem: Failed to create graphics pipeline!");
    }
}