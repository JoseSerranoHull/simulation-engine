#include "particles/FlockGpuBackend.h"

/* parasoft-begin-suppress ALL */
#include <stdexcept>
#include <array>
#include <cstring>
#include <random>
/* parasoft-end-suppress ALL */

using namespace GE::Graphics;

namespace GE::Particles {

// ============================================================================
// SECTION 1: LIFECYCLE
// ============================================================================

FlockGpuBackend::FlockGpuBackend(VkRenderPass transparentRenderPass,
                                 VkDescriptorSetLayout inGlobalSetLayout)
    : m_globalSetLayout(inGlobalSetLayout)
    , m_msaaSamples(ServiceLocator::GetContext()->msaaSamples)
{
    // Build the graphics pipeline immediately — only needs the render pass and msaaSamples.
    // The compute pipeline and buffers are built in Init() once boid count is known.
    createGraphicsPipeline(transparentRenderPass);
}

FlockGpuBackend::~FlockGpuBackend() {
    VulkanContext* ctx = ServiceLocator::GetContext();
    if (ctx == nullptr || ctx->device == VK_NULL_HANDLE) { return; }

    vkDestroyPipeline(ctx->device, m_computePipeline, nullptr);
    vkDestroyPipelineLayout(ctx->device, m_computePipeLayout, nullptr);
    vkDestroyPipeline(ctx->device, m_gfxPipeline, nullptr);
    vkDestroyPipelineLayout(ctx->device, m_gfxPipeLayout, nullptr);

    vkDestroyDescriptorPool(ctx->device, m_descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(ctx->device, m_computeSetLayout, nullptr);

    vkDestroyBuffer(ctx->device, m_ssboA, nullptr);
    vkFreeMemory(ctx->device, m_memA, nullptr);
    vkDestroyBuffer(ctx->device, m_ssboB, nullptr);
    vkFreeMemory(ctx->device, m_memB, nullptr);

    if (m_uboMapped != nullptr) {
        vkUnmapMemory(ctx->device, m_uboMem);
        m_uboMapped = nullptr;
    }
    vkDestroyBuffer(ctx->device, m_ubo, nullptr);
    vkFreeMemory(ctx->device, m_uboMem, nullptr);
}

// ============================================================================
// SECTION 2: INIT  (call once after construction)
// ============================================================================

void FlockGpuBackend::Init(std::vector<GpuBoid> initialBoids) {
    m_boidCount = static_cast<uint32_t>(initialBoids.size());
    if (m_boidCount == 0U) { return; }

    createBuffers(m_boidCount);

    // Upload initial boid state into SSBO A via staging buffer
    const VkDeviceSize boidBytes = static_cast<VkDeviceSize>(m_boidCount) * sizeof(GpuBoid);
    VulkanContext* ctx = ServiceLocator::GetContext();

    VkBuffer       stagingBuf { VK_NULL_HANDLE };
    VkDeviceMemory stagingMem { VK_NULL_HANDLE };
    VulkanUtils::createBuffer(ctx->device, ctx->physicalDevice, boidBytes,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuf, stagingMem);

    void* mapped { nullptr };
    static_cast<void>(vkMapMemory(ctx->device, stagingMem, 0ULL, boidBytes, 0U, &mapped));
    static_cast<void>(std::memcpy(mapped, initialBoids.data(), static_cast<size_t>(boidBytes)));
    vkUnmapMemory(ctx->device, stagingMem);

    VulkanUtils::copyBuffer(ctx->device, ctx->graphicsCommandPool, ctx->graphicsQueue,
                            stagingBuf, m_ssboA, boidBytes);

    vkDestroyBuffer(ctx->device, stagingBuf, nullptr);
    vkFreeMemory(ctx->device, stagingMem, nullptr);

    createComputeDescriptors();
    createComputePipeline();

    m_pingPong = 0U;
}

// ============================================================================
// SECTION 3: SIMULATION & RENDERING
// ============================================================================

void FlockGpuBackend::Dispatch(VkCommandBuffer cb, const FlockUBO& params) {
    if (m_boidCount == 0U || m_computePipeline == VK_NULL_HANDLE) { return; }

    // Upload UBO
    FlockUBO ubo = params;
    ubo.pingPong  = m_pingPong;
    ubo.boidCount = m_boidCount;
    static_cast<void>(std::memcpy(m_uboMapped, &ubo, sizeof(FlockUBO)));

    // Compute dispatch
    vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipeline);
    const VkDescriptorSet sets[DESCRIPTOR_COUNT_ONE] = { m_descriptorSet };
    vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipeLayout,
        GE::EngineConstants::INDEX_ZERO, DESCRIPTOR_COUNT_ONE, sets,
        GE::EngineConstants::OFFSET_ZERO, nullptr);

    const uint32_t groupCount = (m_boidCount + COMPUTE_WORKGROUP_SIZE - 1U) / COMPUTE_WORKGROUP_SIZE;
    vkCmdDispatch(cb, groupCount, GE::EngineConstants::COUNT_ONE, GE::EngineConstants::COUNT_ONE);

    // Barrier: ensure compute writes to the output SSBO are visible to vertex input
    const VkBuffer outputBuf = (m_pingPong == 0U) ? m_ssboB : m_ssboA;
    VkBufferMemoryBarrier barrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
    barrier.srcAccessMask       = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.dstAccessMask       = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.buffer              = outputBuf;
    barrier.offset              = static_cast<VkDeviceSize>(GE::EngineConstants::OFFSET_ZERO);
    barrier.size                = VK_WHOLE_SIZE;

    vkCmdPipelineBarrier(cb,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
        static_cast<VkDependencyFlags>(GE::EngineConstants::OFFSET_ZERO),
        0U, nullptr, GE::EngineConstants::COUNT_ONE, &barrier, 0U, nullptr);

    // Flip ping-pong for next frame
    m_pingPong ^= 1U;
}

void FlockGpuBackend::Draw(VkCommandBuffer cb, VkDescriptorSet globalDescriptorSet) const {
    if (m_boidCount == 0U || m_gfxPipeline == VK_NULL_HANDLE) { return; }

    vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, m_gfxPipeline);

    // Render from the buffer that was LAST written.
    // After Dispatch flips m_pingPong, the last-written buffer is:
    //   m_pingPong == 1 → last dispatch wrote to ssboB (pingPong was 0) → bind ssboB
    //   m_pingPong == 0 → last dispatch wrote to ssboA (pingPong was 1) → bind ssboA
    const VkBuffer renderBuf = (m_pingPong == 1U) ? m_ssboB : m_ssboA;
    const VkBuffer vertexBuffers[DESCRIPTOR_COUNT_ONE] = { renderBuf };
    const VkDeviceSize offsets[DESCRIPTOR_COUNT_ONE] = { 0ULL };
    vkCmdBindVertexBuffers(cb, GE::EngineConstants::INDEX_ZERO,
                           GE::EngineConstants::COUNT_ONE, vertexBuffers, offsets);

    const VkDescriptorSet sets[DESCRIPTOR_COUNT_ONE] = { globalDescriptorSet };
    vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, m_gfxPipeLayout,
        SET_INDEX_GLOBAL, DESCRIPTOR_COUNT_ONE, sets,
        GE::EngineConstants::OFFSET_ZERO, nullptr);

    vkCmdDraw(cb, m_boidCount, GE::EngineConstants::COUNT_ONE,
              GE::EngineConstants::OFFSET_ZERO, GE::EngineConstants::OFFSET_ZERO);
}

void FlockGpuBackend::Restart(glm::vec3 spawnCenter, float spawnRadius) {
    if (m_boidCount == 0U) { return; }

    // Generate new boid state: rejection-sampled uniform sphere (matches FlockingSystem::Restart)
    std::vector<GpuBoid> boids(m_boidCount);
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> axis(-1.0f, 1.0f);
    std::uniform_real_distribution<float> speed(0.3f, 0.8f);

    for (auto& b : boids) {
        glm::vec3 pos;
        do { pos = glm::vec3(axis(rng), axis(rng), axis(rng)); }
        while (glm::length(pos) > 1.0f);

        // Sample a unit-sphere direction for initial velocity; reject near-zero to avoid NaN.
        glm::vec3 dir;
        float     dirLen;
        do {
            dir    = glm::vec3(axis(rng), axis(rng), axis(rng));
            dirLen = glm::length(dir);
        } while (dirLen > 1.0f || dirLen < 1e-4f);

        b.posGroup = glm::vec4(spawnCenter + pos * spawnRadius, 0.0f);
        b.vel      = glm::vec4((dir / dirLen) * speed(rng), 0.0f);
    }

    const VkDeviceSize boidBytes = static_cast<VkDeviceSize>(m_boidCount) * sizeof(GpuBoid);
    VulkanContext* ctx = ServiceLocator::GetContext();

    // Wait for ALL in-flight GPU frames to finish before writing the SSBOs.
    // vkWaitForFences only covers frame N-maxFrames; frame N-1 may still be
    // reading an SSBO when OnGUI fires. vkDeviceWaitIdle is acceptable here
    // because Restart is a one-shot user action, not a per-frame call.
    vkDeviceWaitIdle(ctx->device);

    // Write the new positions to BOTH SSBOs.
    //
    // The already-recorded compute dispatch for the current frame has
    // UBO.pingPong baked in from when FlockGpuSystem::OnUpdate ran earlier
    // this frame (before OnGUI). It will read from whichever SSBO that
    // UBO.pingPong selects (A if 0, B if 1). If we only write to A and zero
    // B, then any dispatch that reads B gets all-zeros → boids collapse to
    // the origin and stall (zero forces at coincident positions). Writing
    // random positions to both SSBOs ensures the correct buffer is populated
    // regardless of the current ping-pong state.
    //
    // Do NOT reset m_pingPong: the current frame's draw call already captured
    // it and will read from the correct output buffer after the dispatch runs.
    void* mapped { nullptr };
    static_cast<void>(vkMapMemory(ctx->device, m_memA, 0ULL, boidBytes, 0U, &mapped));
    static_cast<void>(std::memcpy(mapped, boids.data(), static_cast<size_t>(boidBytes)));
    vkUnmapMemory(ctx->device, m_memA);

    static_cast<void>(vkMapMemory(ctx->device, m_memB, 0ULL, boidBytes, 0U, &mapped));
    static_cast<void>(std::memcpy(mapped, boids.data(), static_cast<size_t>(boidBytes)));
    vkUnmapMemory(ctx->device, m_memB);
}

// ============================================================================
// SECTION 4: INTERNAL INITIALISATION
// ============================================================================

void FlockGpuBackend::createBuffers(uint32_t boidCount) {
    VulkanContext* ctx = ServiceLocator::GetContext();
    const VkDeviceSize boidBytes = static_cast<VkDeviceSize>(boidCount) * sizeof(GpuBoid);

    // Allocate SSBO A and SSBO B (device-local; also bound as vertex buffers)
    const VkBufferUsageFlags ssboUsage =
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT  |
        VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    // Device-local + host-visible so the engine can optionally inspect state
    const VkMemoryPropertyFlags ssboProps =
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    VulkanUtils::createBuffer(ctx->device, ctx->physicalDevice, boidBytes,
                              ssboUsage, ssboProps, m_ssboA, m_memA);
    VulkanUtils::createBuffer(ctx->device, ctx->physicalDevice, boidBytes,
                              ssboUsage, ssboProps, m_ssboB, m_memB);

    // UBO — host-visible, persistent-mapped
    const VkDeviceSize uboBytes = static_cast<VkDeviceSize>(sizeof(FlockUBO));
    VulkanUtils::createBuffer(ctx->device, ctx->physicalDevice, uboBytes,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        m_ubo, m_uboMem);
    static_cast<void>(vkMapMemory(ctx->device, m_uboMem, 0ULL, uboBytes, 0U, &m_uboMapped));
}

void FlockGpuBackend::createComputeDescriptors() {
    VulkanContext* ctx = ServiceLocator::GetContext();

    // Descriptor set layout: binding 0 = UBO, 1 = SSBO A, 2 = SSBO B
    const std::array<VkDescriptorSetLayoutBinding, 3> bindings = {{
        { BINDING_UBO,    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  1U, VK_SHADER_STAGE_COMPUTE_BIT, nullptr },
        { BINDING_SSBO_A, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,  1U, VK_SHADER_STAGE_COMPUTE_BIT, nullptr },
        { BINDING_SSBO_B, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,  1U, VK_SHADER_STAGE_COMPUTE_BIT, nullptr },
    }};

    VkDescriptorSetLayoutCreateInfo layoutInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings    = bindings.data();
    static_cast<void>(vkCreateDescriptorSetLayout(ctx->device, &layoutInfo, nullptr, &m_computeSetLayout));

    // Descriptor pool
    const std::array<VkDescriptorPoolSize, 2> poolSizes = {{
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1U },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2U },
    }};

    VkDescriptorPoolCreateInfo poolInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes    = poolSizes.data();
    poolInfo.maxSets       = 1U;
    static_cast<void>(vkCreateDescriptorPool(ctx->device, &poolInfo, nullptr, &m_descriptorPool));

    // Allocate descriptor set
    VkDescriptorSetAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    allocInfo.descriptorPool     = m_descriptorPool;
    allocInfo.descriptorSetCount = 1U;
    allocInfo.pSetLayouts        = &m_computeSetLayout;
    static_cast<void>(vkAllocateDescriptorSets(ctx->device, &allocInfo, &m_descriptorSet));

    // Write descriptors
    const VkDescriptorBufferInfo uboInfo    { m_ubo,   0ULL, sizeof(FlockUBO)  };
    const VkDescriptorBufferInfo ssboAInfo  { m_ssboA, 0ULL, VK_WHOLE_SIZE     };
    const VkDescriptorBufferInfo ssboBInfo  { m_ssboB, 0ULL, VK_WHOLE_SIZE     };

    const std::array<VkWriteDescriptorSet, 3> writes = {{
        { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, m_descriptorSet, BINDING_UBO,
          0U, 1U, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  nullptr, &uboInfo,   nullptr },
        { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, m_descriptorSet, BINDING_SSBO_A,
          0U, 1U, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,  nullptr, &ssboAInfo, nullptr },
        { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, m_descriptorSet, BINDING_SSBO_B,
          0U, 1U, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,  nullptr, &ssboBInfo, nullptr },
    }};
    vkUpdateDescriptorSets(ctx->device, static_cast<uint32_t>(writes.size()), writes.data(), 0U, nullptr);
}

void FlockGpuBackend::createComputePipeline() {
    VulkanContext* ctx = ServiceLocator::GetContext();

    const ShaderModule compShader("shaders/flock_brute_comp.spv", VK_SHADER_STAGE_COMPUTE_BIT);

    VkPipelineLayoutCreateInfo layoutInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    layoutInfo.setLayoutCount = 1U;
    layoutInfo.pSetLayouts    = &m_computeSetLayout;
    static_cast<void>(vkCreatePipelineLayout(ctx->device, &layoutInfo, nullptr, &m_computePipeLayout));

    VkComputePipelineCreateInfo pipelineInfo{ VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
    pipelineInfo.stage  = compShader.getStageInfo();
    pipelineInfo.layout = m_computePipeLayout;

    if (vkCreateComputePipelines(ctx->device, VK_NULL_HANDLE, 1U, &pipelineInfo, nullptr, &m_computePipeline) != VK_SUCCESS) {
        throw std::runtime_error("FlockGpuBackend: Failed to create compute pipeline!");
    }
}

void FlockGpuBackend::createGraphicsPipeline(VkRenderPass renderPass) {
    VulkanContext* ctx = ServiceLocator::GetContext();

    // Shaders
    const ShaderModule vertShader("shaders/flock_vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    const ShaderModule fragShader("shaders/flock_frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    const VkPipelineShaderStageCreateInfo shaderStages[2] = {
        vertShader.getStageInfo(), fragShader.getStageInfo()
    };

    // Pipeline layout: global descriptor set only (set 0)
    const std::array<VkDescriptorSetLayout, 1> layouts = { m_globalSetLayout };
    VkPipelineLayoutCreateInfo pipeLayoutInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    pipeLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
    pipeLayoutInfo.pSetLayouts    = layouts.data();
    if (vkCreatePipelineLayout(ctx->device, &pipeLayoutInfo, nullptr, &m_gfxPipeLayout) != VK_SUCCESS) {
        throw std::runtime_error("FlockGpuBackend: Failed to create graphics pipeline layout!");
    }

    // Vertex input — reads directly from BoidState SSBO (stride = 32 bytes)
    VkVertexInputBindingDescription binding{};
    binding.binding   = 0U;
    binding.stride    = static_cast<uint32_t>(sizeof(GpuBoid));
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    const std::array<VkVertexInputAttributeDescription, 2> attrs = {{
        { 0U, 0U, VK_FORMAT_R32G32B32A32_SFLOAT, static_cast<uint32_t>(offsetof(GpuBoid, posGroup)) },
        { 1U, 0U, VK_FORMAT_R32G32B32A32_SFLOAT, static_cast<uint32_t>(offsetof(GpuBoid, vel))      },
    }};

    VkPipelineVertexInputStateCreateInfo vertexInput{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
    vertexInput.vertexBindingDescriptionCount   = 1U;
    vertexInput.pVertexBindingDescriptions      = &binding;
    vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrs.size());
    vertexInput.pVertexAttributeDescriptions    = attrs.data();

    // Point-list topology
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
    inputAssembly.topology               = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
    viewportState.viewportCount = 1U;
    viewportState.scissorCount  = 1U;

    const std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    const VkPipelineDynamicStateCreateInfo dynamicState = VulkanUtils::prepareDynamicState(dynamicStates);

    const VkPipelineRasterizationStateCreateInfo rasterizer  = VulkanUtils::prepareRasterizer(VK_CULL_MODE_NONE);
    const VkPipelineMultisampleStateCreateInfo   multisampling = VulkanUtils::prepareMultisampling(m_msaaSamples);
    const VkPipelineDepthStencilStateCreateInfo  depthStencil  = VulkanUtils::prepareDepthStencil(VK_FALSE);

    // Alpha blending (same as particles)
    VkPipelineColorBlendAttachmentState blendAttachment{};
    blendAttachment.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    blendAttachment.blendEnable         = VK_TRUE;
    blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
    blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    blendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1U;
    colorBlending.pAttachments    = &blendAttachment;

    const VkGraphicsPipelineCreateInfo pipelineInfo = VulkanUtils::preparePipelineCreateInfo(
        shaderStages, &vertexInput, &inputAssembly, &viewportState, &rasterizer,
        &multisampling, &depthStencil, &colorBlending, &dynamicState,
        m_gfxPipeLayout, renderPass
    );

    if (vkCreateGraphicsPipelines(ctx->device, VK_NULL_HANDLE, 1U, &pipelineInfo, nullptr, &m_gfxPipeline) != VK_SUCCESS) {
        throw std::runtime_error("FlockGpuBackend: Failed to create graphics pipeline!");
    }
}

} // namespace GE::Particles
