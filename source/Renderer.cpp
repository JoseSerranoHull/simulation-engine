#include "../include/Renderer.h"
#include "../include/ServiceLocator.h"
#include "../include/EntityManager.h"
#include "../include/Components.h"

/* parasoft-begin-suppress ALL */
#include <array>
/* parasoft-end-suppress ALL */

/**
 * @brief Orchestrates the multi-pass command recording sequence for a single frame.
 */
void Renderer::recordFrame(
    const VkCommandBuffer cb,
    const VkExtent2D& extent,
    const Skybox* const skybox,
    const ParticleSystem* const dustSystem,
    const ParticleSystem* const fireSystem,
    const ParticleSystem* const smokeSystem,
    const ParticleSystem* const rainSystem,
    const ParticleSystem* const snowSystem,
    const PostProcessor* const postProcessor,
    const VkDescriptorSet globalDescriptorSet,
    const VkRenderPass shadowPass,
    const VkFramebuffer shadowFramebuffer,
    const std::vector<Pipeline*>& pipelines,
    const bool enableDust,
    const bool enableFire,
    const bool enableSmoke,
    const bool enableRain,
    const bool enableSnow
) const {
    // Step 1: Shadow Mapping Pass
    recordShadowPass(cb, shadowPass, shadowFramebuffer,
        pipelines.at(PIPELINE_IDX_SHADOW), globalDescriptorSet);

    // Step 2: Main Opaque Pass
    recordOpaquePass(cb, extent, skybox, postProcessor, globalDescriptorSet, pipelines);

    // Step 3: Resolve Synchronization Barrier
    const VkImageMemoryBarrier resolveBarrier{
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        nullptr,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        VK_ACCESS_TRANSFER_READ_BIT,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        postProcessor->getResolveImage(),
        { VK_IMAGE_ASPECT_COLOR_BIT, 0U, 1U, 0U, 1U }
    };

    vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, 0U, 0U, nullptr, 0U, nullptr, 1U, &resolveBarrier);

    // Step 4: Refraction Bridge
    if (postProcessor != nullptr) {
        postProcessor->copyScene(cb);
    }

    // Step 5: Transparent & Particle Pass
    recordTransparentPass(cb, extent, dustSystem, fireSystem, smokeSystem,
        rainSystem, snowSystem, postProcessor, globalDescriptorSet, pipelines,
        enableDust, enableFire, enableSmoke, enableRain, enableSnow);
}

/**
 * @brief Records the depth-only shadow pass for all shadow-casting models.
 */
void Renderer::recordShadowPass(
    const VkCommandBuffer cb,
    const VkRenderPass renderPass,
    const VkFramebuffer framebuffer,
    const Pipeline* const shadowPipeline,
    const VkDescriptorSet globalSet
) const {
    auto* em = ServiceLocator::GetEntityManager();
    auto& meshRenderers = em->GetCompArr<GE::Components::MeshRenderer>();

    VkRenderPassBeginInfo shadowPassInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    shadowPassInfo.renderPass = renderPass;
    shadowPassInfo.framebuffer = framebuffer;
    shadowPassInfo.renderArea.extent = { EngineConstants::SHADOW_MAP_RES, EngineConstants::SHADOW_MAP_RES };

    VkClearValue shadowClear{};
    shadowClear.depthStencil = { DEPTH_CLEAR_VAL, 0U };
    shadowPassInfo.clearValueCount = 1U;
    shadowPassInfo.pClearValues = &shadowClear;

    vkCmdBeginRenderPass(cb, &shadowPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    const float shadowRes = static_cast<float>(EngineConstants::SHADOW_MAP_RES);
    const VkViewport sv{ 0.0f, 0.0f, shadowRes, shadowRes, 0.0f, 1.0f };
    vkCmdSetViewport(cb, 0U, 1U, &sv);

    const VkRect2D ss{ {0, 0}, {EngineConstants::SHADOW_MAP_RES, EngineConstants::SHADOW_MAP_RES} };
    vkCmdSetScissor(cb, 0U, 1U, &ss);

    // ECS Iteration: Draw only shadow-casters
    for (uint32_t i = 0; i < meshRenderers.GetCount(); ++i) {
        const auto& mr = meshRenderers.Data()[i];
        GE::ECS::EntityID entityID = meshRenderers.Index()[i];
        auto* transform = em->GetTIComponent<GE::Scene::Components::Transform>(entityID);

        if (transform) {
            // Nested loop for sub-meshes
            for (const auto& sub : mr.subMeshes) {
                // Logic: Only draw if the mesh exists AND the material permits shadows
                if (sub.mesh && sub.material && sub.material->GetCastsShadows()) {
                    sub.mesh->draw(cb, globalSet, shadowPipeline, transform->m_worldMatrix);
                }
            }
        }
    }

    vkCmdEndRenderPass(cb);
}

/**
 * @brief Records the primary opaque rendering pass (Scene geometry + Skybox).
 */
void Renderer::recordOpaquePass(
    const VkCommandBuffer cb,
    const VkExtent2D& extent,
    const Skybox* const skybox,
    const PostProcessor* const postProcessor,
    const VkDescriptorSet globalSet,
    const std::vector<Pipeline*>& pipelines
) const {
    auto* em = ServiceLocator::GetEntityManager();
    auto& meshRenderers = em->GetCompArr<GE::Components::MeshRenderer>();

    VkRenderPassBeginInfo opaquePassInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    opaquePassInfo.renderPass = postProcessor->getOffscreenRenderPass();
    opaquePassInfo.framebuffer = postProcessor->getOffscreenFramebuffer();
    opaquePassInfo.renderArea.extent = extent;

    std::array<VkClearValue, 3U> clearValues{};
    clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
    clearValues[1].depthStencil = { 1.0f, 0U };
    clearValues[2].color = { {0.0f, 0.0f, 0.0f, 1.0f} };

    opaquePassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    opaquePassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(cb, &opaquePassInfo, VK_SUBPASS_CONTENTS_INLINE);

    const VkViewport vp{ 0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f };
    vkCmdSetViewport(cb, 0U, 1U, &vp);

    const VkRect2D sc{ {0, 0}, extent };
    vkCmdSetScissor(cb, 0U, 1U, &sc);

    if (skybox != nullptr) {
        skybox->draw(cb, globalSet);
    }

    // ECS Iteration: Filter for Opaque objects
    for (uint32_t i = 0; i < meshRenderers.GetCount(); ++i) {
        const auto& mr = meshRenderers.Data()[i];
        GE::ECS::EntityID entityID = meshRenderers.Index()[i];
        auto* transform = em->GetTIComponent<GE::Scene::Components::Transform>(entityID);

        if (transform) {
            for (const auto& sub : mr.subMeshes) {
                // Logic: Only draw if the material explicitly asks for the Opaque pass
                if (sub.mesh && sub.material && sub.material->GetPassType() == RenderPassType::Opaque) {
                    sub.mesh->draw(cb, globalSet, nullptr, transform->m_worldMatrix);
                }
            }
        }
    }

    vkCmdEndRenderPass(cb);
}

/**
 * @brief Helper to record draw calls for active environmental particle systems.
 */
void Renderer::recordParticlePass(
    const VkCommandBuffer cb,
    const ParticleSystem* const dust,
    const ParticleSystem* const fire,
    const ParticleSystem* const smoke,
    const ParticleSystem* const rain,
    const ParticleSystem* const snow,
    const bool dustEnabled,
    const bool fireEnabled,
    const bool smokeEnabled,
    const bool rainEnabled,
    const bool snowEnabled,
    const VkDescriptorSet globalSet
) const {
    if (dustEnabled && (dust != nullptr)) { dust->draw(cb, globalSet); }
    if (fireEnabled && (fire != nullptr)) { fire->draw(cb, globalSet); }
    if (smokeEnabled && (smoke != nullptr)) { smoke->draw(cb, globalSet); }
    if (rainEnabled && (rain != nullptr)) { rain->draw(cb, globalSet); }
    if (snowEnabled && (snow != nullptr)) { snow->draw(cb, globalSet); }
}

/**
 * @brief Records the transparent rendering pass.
 */
void Renderer::recordTransparentPass(
    const VkCommandBuffer cb,
    const VkExtent2D& extent,
    const ParticleSystem* const dust,
    const ParticleSystem* const fire,
    const ParticleSystem* const smoke,
    const ParticleSystem* const rain,
    const ParticleSystem* const snow,
    const PostProcessor* const postProcessor,
    const VkDescriptorSet globalSet,
    const std::vector<Pipeline*>& pipelines,
    const bool dustEnabled,
    const bool fireEnabled,
    const bool smokeEnabled,
    const bool rainEnabled,
    const bool snowEnabled
) const {
    auto* em = ServiceLocator::GetEntityManager();
    auto& meshRenderers = em->GetCompArr<GE::Components::MeshRenderer>();

    VkRenderPassBeginInfo transPassInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    transPassInfo.renderPass = postProcessor->getTransparentRenderPass();
    transPassInfo.framebuffer = postProcessor->getOffscreenFramebuffer();
    transPassInfo.renderArea.extent = extent;

    // Depth/Color already cleared by Opaque Pass; we reuse attachments here
    transPassInfo.clearValueCount = 0U;
    transPassInfo.pClearValues = nullptr;

    vkCmdBeginRenderPass(cb, &transPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    const VkViewport vp{ 0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f };
    vkCmdSetViewport(cb, 0U, 1U, &vp);

    const VkRect2D sc{ {0, 0}, extent };
    vkCmdSetScissor(cb, 0U, 1U, &sc);

    // ECS Iteration: Filter for Transparent objects (Glass/Water)
    for (uint32_t i = 0; i < meshRenderers.GetCount(); ++i) {
        const auto& mr = meshRenderers.Data()[i];
        auto* transform = em->GetTIComponent<GE::Scene::Components::Transform>(meshRenderers.Index()[i]);

        if (transform) {
            for (const auto& sub : mr.subMeshes) {
                // Logic: Only draw if the material explicitly asks for the Transparent pass
                if (sub.mesh && sub.material && sub.material->GetPassType() == RenderPassType::Transparent) {
                    sub.mesh->draw(cb, globalSet, nullptr, transform->m_worldMatrix);
                }
            }
        }
    }

    recordParticlePass(cb, dust, fire, smoke, rain, snow, dustEnabled, fireEnabled, smokeEnabled, rainEnabled, snowEnabled, globalSet);

    vkCmdEndRenderPass(cb);
}