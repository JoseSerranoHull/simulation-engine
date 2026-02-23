#pragma once

/* parasoft-begin-suppress ALL */
#include "core/libs.h"
#include <array>
#include <vector>
#include <stdexcept>
/* parasoft-end-suppress ALL */

#include "assets/Vertex.h"
#include "graphics/ShaderModule.h"
#include "graphics/VulkanContext.h"
#include "core/ServiceLocator.h"

namespace GE::Graphics {

/**
 * @class GraphicsPipeline
 * @brief Manages a Vulkan Graphics GraphicsPipeline and its associated Layout.
 * Handles state configuration for rasterization, blending, depth testing, and MSAA.
 */
class GraphicsPipeline final {
public:
    // --- Static GraphicsPipeline Constants ---
    static constexpr uint32_t BINDING_COUNT_ONE = 1U;
    static constexpr uint32_t VIEWPORT_COUNT_ONE = 1U;
    static constexpr uint32_t SCISSOR_COUNT_ONE = 1U;
    static constexpr uint32_t ATTACHMENT_COUNT_ONE = 1U;
    static constexpr uint32_t PUSH_CONSTANT_COUNT = 1U;
    static constexpr uint32_t PIPELINE_COUNT_ONE = 1U;

    static constexpr uint32_t SET_INDEX_GLOBAL = 0U;
    static constexpr uint32_t SET_INDEX_MATERIAL = 1U;
    static constexpr uint32_t LAYOUT_SET_COUNT = 2U;

    static constexpr float    DEFAULT_LINE_WIDTH = 1.0f;
    static constexpr uint32_t PUSH_CONSTANT_OFFSET = 0U;

private:
    VkPipeline        pipeline{ VK_NULL_HANDLE };
    VkPipelineLayout  pipelineLayout{ VK_NULL_HANDLE };
    VkDescriptorSetLayout materialLayout{ VK_NULL_HANDLE };

public:
    /**
     * @brief Constructs a specialized graphics pipeline.
     */
    GraphicsPipeline(
        const VkRenderPass renderPass,
        const VkDescriptorSetLayout inMaterialLayout,
        ShaderModule* const vertShader,
        ShaderModule* const fragShader,
        const bool enableCulling = true,
        const bool enableBlending = false,
        const bool enableDepthWrite = true,
        const VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT
    ) :  materialLayout(inMaterialLayout)
    {
        // 1. Shader Stages Initialization
        std::vector<VkPipelineShaderStageCreateInfo> shaderStages{};
        if (vertShader != nullptr) {
            shaderStages.push_back(vertShader->getStageInfo());
        }
        if (fragShader != nullptr) {
            shaderStages.push_back(fragShader->getStageInfo());
        }

        // 2. Vertex Input Configuration
        const VkVertexInputBindingDescription bindingDescription = GE::Assets::Vertex::getBindingDescription();
        const std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions = GE::Assets::Vertex::getAttributeDescriptions();

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
        vertexInputInfo.vertexBindingDescriptionCount = BINDING_COUNT_ONE;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        // 3. Viewport & Dynamic State setup
        VkPipelineViewportStateCreateInfo viewportState{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
        viewportState.viewportCount = VIEWPORT_COUNT_ONE;
        viewportState.scissorCount = SCISSOR_COUNT_ONE;

        const std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dynamicState{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        // 4. Rasterizer Configuration
        VkPipelineRasterizationStateCreateInfo rasterizer{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = DEFAULT_LINE_WIDTH;
        rasterizer.cullMode = enableCulling ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_NONE;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

        // 5. Multisampling Alignment
        VkPipelineMultisampleStateCreateInfo multisampling{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
        multisampling.rasterizationSamples = msaaSamples;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.minSampleShading = 1.0f;
        multisampling.pSampleMask = nullptr;
        multisampling.alphaToCoverageEnable = VK_FALSE;
        multisampling.alphaToOneEnable = VK_FALSE;

        // 6. Depth Stencil Configuration
        VkPipelineDepthStencilStateCreateInfo depthStencil{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = enableDepthWrite ? VK_TRUE : VK_FALSE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;

        // 7. Color Blending Logic
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        if (enableBlending) {
            colorBlendAttachment.blendEnable = VK_TRUE;
            colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
            colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        }

        VkPipelineColorBlendStateCreateInfo colorBlending{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
        colorBlending.attachmentCount = ATTACHMENT_COUNT_ONE;
        colorBlending.pAttachments = &colorBlendAttachment;

        // 8. GraphicsPipeline Layout (Global UBO + Material Set + Push Constants)
        const VkPushConstantRange pushConstantRange{
            VK_SHADER_STAGE_VERTEX_BIT,
            PUSH_CONSTANT_OFFSET,
            static_cast<uint32_t>(sizeof(glm::mat4))
        };

        VulkanContext* context = ServiceLocator::GetContext();

        const std::array<VkDescriptorSetLayout, LAYOUT_SET_COUNT> layouts = {
            context->globalSetLayout,
            materialLayout
        };

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
        pipelineLayoutInfo.pSetLayouts = layouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = PUSH_CONSTANT_COUNT;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

        if (vkCreatePipelineLayout(context->device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("GraphicsPipeline: Failed to create pipeline layout!");
        }

        // 9. Graphics GraphicsPipeline Assembly
        VkGraphicsPipelineCreateInfo pipelineInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
        pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
        pipelineInfo.pStages = shaderStages.data();
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderPass;

        if (vkCreateGraphicsPipelines(context->device, VK_NULL_HANDLE, PIPELINE_COUNT_ONE, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
            throw std::runtime_error("GraphicsPipeline: Failed to create graphics pipeline!");
        }
    }

    /**
     * @brief Destructor: Explicitly destroys the pipeline and its layout.
     */
    ~GraphicsPipeline() {
        VulkanContext* context = ServiceLocator::GetContext();

        if ((context != nullptr) && (context->device != VK_NULL_HANDLE)) {
            vkDestroyPipeline(context->device, pipeline, nullptr);
            vkDestroyPipelineLayout(context->device, pipelineLayout, nullptr);
            pipeline = VK_NULL_HANDLE;
            pipelineLayout = VK_NULL_HANDLE;
        }
    }

    // RAII: Prevent duplication of pipeline handles
    GraphicsPipeline(const GraphicsPipeline&) = delete;
    GraphicsPipeline& operator=(const GraphicsPipeline&) = delete;

    /**
     * @brief Binds this pipeline to the current command buffer.
     * Note: Marked as 'const' to allow calls from const Mesh instances.
     */
    void bind(const VkCommandBuffer commandBuffer) const {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    }

    /** @brief Returns the layout for descriptor and push constant mapping. */
    VkPipelineLayout getPipelineLayout() const { return pipelineLayout; }

    /** @brief Returns a specific set layout based on index (0: Global, 1: Material). */
    VkDescriptorSetLayout getDescriptorSetLayout(const uint32_t setIndex) const {
        VkDescriptorSetLayout layout{ VK_NULL_HANDLE };

        if (setIndex == SET_INDEX_GLOBAL) {
            VulkanContext* context = ServiceLocator::GetContext();
            layout = context->globalSetLayout;
        }
        else if (setIndex == SET_INDEX_MATERIAL) {
            layout = materialLayout;
        }

        return layout;
    }
};

} // namespace GE::Graphics