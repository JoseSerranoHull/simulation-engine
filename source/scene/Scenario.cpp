#include "scene/Scenario.h"
#include "core/ServiceLocator.h"
#include "core/EngineOrchestrator.h"
#include "graphics/GpuResourceManager.h"
#include "graphics/VulkanContext.h"

namespace GE {

/**
 * @brief Builds the fixed set of material pipelines for this scenario.
 * Extracts render pass handles from the PostProcessBackend and constructs the
 * six material pipelines (phong, sand, base, glass, transparent, water).
 * The shadow pipeline remains engine-scoped and is NOT created here.
 */
void Scenario::createMaterialPipelines() {
    using namespace GE::Graphics;

    VulkanContext* ctx = ServiceLocator::GetContext();
    auto* experience = ServiceLocator::GetExperience();

    const VkRenderPass offscreenPass = experience->GetPostProcessBackend()->getOffscreenRenderPass();
    const VkRenderPass transPass     = experience->GetPostProcessBackend()->getTransparentRenderPass();
    const VkSampleCountFlagBits msaa = ctx->msaaSamples;

    m_shaderModules.clear();
    m_pipelines.clear();

    // --- Shader Modules (indices 0-7, material pipelines only) ---
    m_shaderModules.push_back(std::make_unique<ShaderModule>("./shaders/phong_vert.spv",           VK_SHADER_STAGE_VERTEX_BIT));   // [0]
    m_shaderModules.push_back(std::make_unique<ShaderModule>("./shaders/phong_frag.spv",           VK_SHADER_STAGE_FRAGMENT_BIT)); // [1]
    m_shaderModules.push_back(std::make_unique<ShaderModule>("./shaders/sand_frag.spv",            VK_SHADER_STAGE_FRAGMENT_BIT)); // [2]
    m_shaderModules.push_back(std::make_unique<ShaderModule>("./shaders/base_frag.spv",            VK_SHADER_STAGE_FRAGMENT_BIT)); // [3]
    m_shaderModules.push_back(std::make_unique<ShaderModule>("./shaders/glass_frag.spv",           VK_SHADER_STAGE_FRAGMENT_BIT)); // [4]
    m_shaderModules.push_back(std::make_unique<ShaderModule>("./shaders/transparent_frag.spv",     VK_SHADER_STAGE_FRAGMENT_BIT)); // [5]
    m_shaderModules.push_back(std::make_unique<ShaderModule>("./shaders/water_vert.spv",           VK_SHADER_STAGE_VERTEX_BIT));   // [6]
    m_shaderModules.push_back(std::make_unique<ShaderModule>("./shaders/water_frag.spv",           VK_SHADER_STAGE_FRAGMENT_BIT)); // [7]
    m_shaderModules.push_back(std::make_unique<ShaderModule>("./shaders/checkerboard_vert.spv",    VK_SHADER_STAGE_VERTEX_BIT));   // [8]
    m_shaderModules.push_back(std::make_unique<ShaderModule>("./shaders/checkerboard_frag.spv",    VK_SHADER_STAGE_FRAGMENT_BIT)); // [9]

    // Checkerboard push constant size: mat4 (64) + vec4 (16) + vec4 (16) + float (4) = 100 bytes
    // Both VERTEX and FRAGMENT stages read from this range (vert: model, frag: colorA/B/scale).
    static constexpr uint32_t CHECKER_PC_SIZE =
        static_cast<uint32_t>(sizeof(glm::mat4) + sizeof(glm::vec4) + sizeof(glm::vec4) + sizeof(float));
    static constexpr VkShaderStageFlags CHECKER_PC_STAGES =
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    // --- Material Pipelines (indices 0-5) ---
    // Opaque
    m_pipelines.push_back(std::make_unique<GraphicsPipeline>(offscreenPass, ctx->materialSetLayout, m_shaderModules[0].get(), m_shaderModules[1].get(), true,  true,  true,  msaa)); // [0] Phong
    m_pipelines.push_back(std::make_unique<GraphicsPipeline>(offscreenPass, ctx->materialSetLayout, m_shaderModules[0].get(), m_shaderModules[2].get(), true,  true,  true,  msaa)); // [1] Sand
    m_pipelines.push_back(std::make_unique<GraphicsPipeline>(offscreenPass, ctx->materialSetLayout, m_shaderModules[0].get(), m_shaderModules[3].get(), true,  true,  true,  msaa)); // [2] Base
    // Transparent (depthWrite = false)
    m_pipelines.push_back(std::make_unique<GraphicsPipeline>(transPass,     ctx->materialSetLayout, m_shaderModules[0].get(), m_shaderModules[4].get(), true,  false, false, msaa)); // [3] Glass
    m_pipelines.push_back(std::make_unique<GraphicsPipeline>(offscreenPass, ctx->materialSetLayout, m_shaderModules[0].get(), m_shaderModules[5].get(), false, false, true,  msaa)); // [4] Transparent
    m_pipelines.push_back(std::make_unique<GraphicsPipeline>(transPass,     ctx->materialSetLayout, m_shaderModules[6].get(), m_shaderModules[7].get(), true,  false, false, msaa)); // [5] Water
    // Checkerboard (index 6) — culling disabled so plane is visible from both sides
    m_pipelines.push_back(std::make_unique<GraphicsPipeline>(offscreenPass, ctx->materialSetLayout, m_shaderModules[8].get(), m_shaderModules[9].get(), false, false, true,  msaa, CHECKER_PC_SIZE, CHECKER_PC_STAGES)); // [6] Checkerboard
}

} // namespace GE
