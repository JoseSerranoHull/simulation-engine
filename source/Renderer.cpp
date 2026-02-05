#include "Renderer.h"

/* parasoft-begin-suppress ALL */
#include <array>
/* parasoft-end-suppress ALL */

/**
 * @brief Orchestrates the multi-pass command recording sequence for a single frame.
 */
void Renderer::recordFrame(
    const VkCommandBuffer cb,
    const VkExtent2D& extent,
    const std::map<std::string, std::unique_ptr<Model>>& models,
    const std::vector<std::unique_ptr<Model>>& ownedModels,
    const std::vector<Mesh*>& opaqueMeshes,
    const std::vector<Mesh*>& transparentMeshes,
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
    // Records depth information from the light's perspective into the shadow map.
    recordShadowPass(cb, shadowPass, shadowFramebuffer, models, ownedModels,
        pipelines.at(PIPELINE_IDX_SHADOW), globalDescriptorSet);

    // Step 2: Main Opaque Pass
    // Renders the skybox and all non-transparent scene geometry.
    recordOpaquePass(cb, extent, opaqueMeshes, skybox, postProcessor, globalDescriptorSet);

    // Step 3: Resolve Synchronization Barrier
    // Transition the MSAA resolve target for refractive sampling.
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
    // Copies the resolved opaque scene to a texture for glass/water refraction.
    if (postProcessor != nullptr) {
        postProcessor->copyScene(cb);
    }

    // Step 5: Transparent & Particle Pass
    // Renders glass, liquids, and environmental particles with alpha blending.
    recordTransparentPass(cb, extent, transparentMeshes, dustSystem, fireSystem, smokeSystem,
        rainSystem, snowSystem, postProcessor, globalDescriptorSet,
        enableDust, enableFire, enableSmoke, enableRain, enableSnow);
}

/**
 * @brief Records the depth-only shadow pass for all shadow-casting models.
 */
void Renderer::recordShadowPass(
    const VkCommandBuffer cb,
    const VkRenderPass renderPass,
    const VkFramebuffer framebuffer,
    const std::map<std::string, std::unique_ptr<Model>>& models,
    const std::vector<std::unique_ptr<Model>>& ownedModels,
    const Pipeline* const shadowPipeline,
    const VkDescriptorSet globalSet
) const {
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

    // Step 1: Draw global scene models
    for (const auto& [name, model] : models) {
        if (model->castsShadows()) {
            model->draw(cb, globalSet, shadowPipeline);
        }
    }

    // Step 2: Draw instanced foliage or specialized meshes
    for (const auto& model : ownedModels) {
        if (model && model->castsShadows()) {
            model->draw(cb, globalSet, shadowPipeline);
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
    const std::vector<Mesh*>& opaque,
    const Skybox* const skybox,
    const PostProcessor* const postProcessor,
    const VkDescriptorSet globalSet
) const {
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

    for (Mesh* const mesh : opaque) {
        if (mesh != nullptr) {
            mesh->draw(cb, globalSet);
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
    const std::vector<Mesh*>& transparent,
    const ParticleSystem* const dust,
    const ParticleSystem* const fire,
    const ParticleSystem* const smoke,
    const ParticleSystem* const rain,
    const ParticleSystem* const snow,
    const PostProcessor* const postProcessor,
    const VkDescriptorSet globalSet,
    const bool dustEnabled,
    const bool fireEnabled,
    const bool smokeEnabled,
    const bool rainEnabled,
    const bool snowEnabled
) const {
    VkRenderPassBeginInfo transPassInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    transPassInfo.renderPass = postProcessor->getTransparentRenderPass();
    transPassInfo.framebuffer = postProcessor->getOffscreenFramebuffer();
    transPassInfo.renderArea.extent = extent;

    const std::array<VkClearValue, 3U> dummyValues{};
    transPassInfo.clearValueCount = static_cast<uint32_t>(dummyValues.size());
    transPassInfo.pClearValues = dummyValues.data();

    vkCmdBeginRenderPass(cb, &transPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    const VkViewport vp{ 0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f };
    vkCmdSetViewport(cb, 0U, 1U, &vp);

    const VkRect2D sc{ {0, 0}, extent };
    vkCmdSetScissor(cb, 0U, 1U, &sc);

    for (Mesh* const mesh : transparent) {
        if (mesh != nullptr) {
            mesh->draw(cb, globalSet);
        }
    }

    recordParticlePass(cb, dust, fire, smoke, rain, snow, dustEnabled, fireEnabled, smokeEnabled, rainEnabled, snowEnabled, globalSet);

    vkCmdEndRenderPass(cb);
}