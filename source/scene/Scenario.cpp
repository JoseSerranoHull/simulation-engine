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
    m_shaderModules.push_back(std::make_unique<ShaderModule>("./shaders/phong_vert.spv",        VK_SHADER_STAGE_VERTEX_BIT));
    m_shaderModules.push_back(std::make_unique<ShaderModule>("./shaders/phong_frag.spv",        VK_SHADER_STAGE_FRAGMENT_BIT));
    m_shaderModules.push_back(std::make_unique<ShaderModule>("./shaders/sand_frag.spv",         VK_SHADER_STAGE_FRAGMENT_BIT));
    m_shaderModules.push_back(std::make_unique<ShaderModule>("./shaders/base_frag.spv",         VK_SHADER_STAGE_FRAGMENT_BIT));
    m_shaderModules.push_back(std::make_unique<ShaderModule>("./shaders/glass_frag.spv",        VK_SHADER_STAGE_FRAGMENT_BIT));
    m_shaderModules.push_back(std::make_unique<ShaderModule>("./shaders/transparent_frag.spv",  VK_SHADER_STAGE_FRAGMENT_BIT));
    m_shaderModules.push_back(std::make_unique<ShaderModule>("./shaders/water_vert.spv",        VK_SHADER_STAGE_VERTEX_BIT));
    m_shaderModules.push_back(std::make_unique<ShaderModule>("./shaders/water_frag.spv",        VK_SHADER_STAGE_FRAGMENT_BIT));

    // --- Material Pipelines (indices 0-5) ---
    // Opaque
    m_pipelines.push_back(std::make_unique<GraphicsPipeline>(offscreenPass, ctx->materialSetLayout, m_shaderModules[0].get(), m_shaderModules[1].get(), true,  true,  true,  msaa));
    m_pipelines.push_back(std::make_unique<GraphicsPipeline>(offscreenPass, ctx->materialSetLayout, m_shaderModules[0].get(), m_shaderModules[2].get(), true,  true,  true,  msaa));
    m_pipelines.push_back(std::make_unique<GraphicsPipeline>(offscreenPass, ctx->materialSetLayout, m_shaderModules[0].get(), m_shaderModules[3].get(), true,  true,  true,  msaa));
    // Transparent (depthWrite = false)
    m_pipelines.push_back(std::make_unique<GraphicsPipeline>(transPass,     ctx->materialSetLayout, m_shaderModules[0].get(), m_shaderModules[4].get(), true,  false, false, msaa));
    m_pipelines.push_back(std::make_unique<GraphicsPipeline>(offscreenPass, ctx->materialSetLayout, m_shaderModules[0].get(), m_shaderModules[5].get(), false, false, true,  msaa));
    m_pipelines.push_back(std::make_unique<GraphicsPipeline>(transPass,     ctx->materialSetLayout, m_shaderModules[6].get(), m_shaderModules[7].get(), true,  false, false, msaa));
}

} // namespace GE
