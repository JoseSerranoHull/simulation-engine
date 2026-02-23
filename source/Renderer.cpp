#include "../include/Renderer.h"
#include "../include/ServiceLocator.h"
#include "../include/EntityManager.h"
#include "../include/Components.h"
#include "../include/ParticleComponent.h"

/* parasoft-begin-suppress ALL */
#include <array>
/* parasoft-end-suppress ALL */

// ========================================================================
// SECTION 1: MASTER ORCHESTRATION
// ========================================================================

namespace GE::Components
{
	struct ParticleComponent;
}

using namespace GE::Assets;

namespace GE::Graphics {

/**
 * @brief Orchestrates the multi-pass command recording sequence for a single frame.
 * Refactored to be agnostic: It no longer knows about specific particle systems.
 */
void Renderer::recordFrame(
    const VkCommandBuffer cb,
    const VkExtent2D& extent,
    const Skybox* const skybox,
    GE::ECS::EntityManager* const em,
    const PostProcessor* const postProcessor,
    const VkDescriptorSet globalDescriptorSet,
    const VkRenderPass shadowPass,
    const VkFramebuffer shadowFramebuffer,
    const std::vector<Pipeline*>& pipelines
) const {
    // Step 1: Shadow Mapping Pass (Depth-only)
    // Pulls data from MeshRenderers with "CastsShadows" flag
    recordShadowPass(cb, shadowPass, shadowFramebuffer,
        pipelines.at(PIPELINE_IDX_SHADOW), globalDescriptorSet, em);

    // Step 2: Main Opaque Pass (Scene geometry + Skybox)
    recordOpaquePass(cb, extent, skybox, postProcessor, globalDescriptorSet, pipelines, em);

    // Step 3: Resolve Synchronization Barrier
    // Required before copying the scene for Refraction/Glass effects
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
    // Copies the Opaque result into a texture for glass shaders to sample
    if (postProcessor != nullptr) {
        postProcessor->copyScene(cb);
    }

    // Step 5: Transparent Pass
    // Records alpha-blended objects and generic Particle entities
    recordTransparentPass(cb, extent, postProcessor, globalDescriptorSet, pipelines, em);
}

// ========================================================================
// SECTION 2: SHADOW PASS
// ========================================================================

/**
 * @brief Records the depth-only shadow pass for all shadow-casting entities.
 */
void Renderer::recordShadowPass(
    const VkCommandBuffer cb,
    const VkRenderPass renderPass,
    const VkFramebuffer framebuffer,
    const Pipeline* const shadowPipeline,
    const VkDescriptorSet globalSet,
    GE::ECS::EntityManager* const em
) const {
    auto& meshRenderers = em->GetCompArr<GE::Components::MeshRenderer>();

    VkRenderPassBeginInfo shadowPassInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    shadowPassInfo.renderPass = renderPass;
    shadowPassInfo.framebuffer = framebuffer;
    shadowPassInfo.renderArea.extent = { GE::EngineConstants::SHADOW_MAP_RES, GE::EngineConstants::SHADOW_MAP_RES };

    VkClearValue shadowClear{};
    shadowClear.depthStencil = { DEPTH_CLEAR_VAL, 0U };
    shadowPassInfo.clearValueCount = 1U;
    shadowPassInfo.pClearValues = &shadowClear;

    vkCmdBeginRenderPass(cb, &shadowPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    const float shadowRes = static_cast<float>(GE::EngineConstants::SHADOW_MAP_RES);
    const VkViewport sv{ 0.0f, 0.0f, shadowRes, shadowRes, 0.0f, 1.0f };
    vkCmdSetViewport(cb, 0U, 1U, &sv);

    const VkRect2D ss{ {0, 0}, {GE::EngineConstants::SHADOW_MAP_RES, GE::EngineConstants::SHADOW_MAP_RES} };
    vkCmdSetScissor(cb, 0U, 1U, &ss);

    // Agnostic Iteration: Draw only entities that cast shadows
    for (uint32_t i = 0; i < meshRenderers.GetCount(); ++i) {
        const auto& mr = meshRenderers.Data()[i];
        const GE::ECS::EntityID entityID = meshRenderers.Index()[i];

        auto* const transform = em->GetTIComponent<GE::Components::Transform>(entityID);

        if (transform != nullptr) {
            for (const auto& sub : mr.subMeshes) {
                // Logic: Does the material permit shadows? (Defined in .ini)
                if (sub.mesh && sub.material && sub.material->GetCastsShadows()) {
                    sub.mesh->draw(cb, globalSet, shadowPipeline, transform->m_worldMatrix);
                }
            }
        }
    }

    vkCmdEndRenderPass(cb);
}

// ========================================================================
// SECTION 3: OPAQUE & SKYBOX PASS
// ========================================================================

/**
 * @brief Records the primary opaque rendering pass (Scene geometry + Skybox).
 */
void Renderer::recordOpaquePass(
    const VkCommandBuffer cb,
    const VkExtent2D& extent,
    const Skybox* const skybox,
    const PostProcessor* const postProcessor,
    const VkDescriptorSet globalSet,
    const std::vector<Pipeline*>& pipelines,
    GE::ECS::EntityManager* const em
) const {
    auto& meshRenderers = em->GetCompArr<GE::Components::MeshRenderer>();

    VkRenderPassBeginInfo opaquePassInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    opaquePassInfo.renderPass = postProcessor->getOffscreenRenderPass();
    opaquePassInfo.framebuffer = postProcessor->getOffscreenFramebuffer();
    opaquePassInfo.renderArea.extent = extent;

    std::array<VkClearValue, 3U> clearValues{};
    clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
    clearValues[1].depthStencil = { DEPTH_CLEAR_VAL, 0U };
    clearValues[2].color = { {0.0f, 0.0f, 0.0f, 1.0f} };

    opaquePassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    opaquePassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(cb, &opaquePassInfo, VK_SUBPASS_CONTENTS_INLINE);

    const VkViewport vp{ 0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f };
    vkCmdSetViewport(cb, 0U, 1U, &vp);

    const VkRect2D sc{ {0, 0}, extent };
    vkCmdSetScissor(cb, 0U, 1U, &sc);

    // 1. Draw Environment (Background)
    if (skybox != nullptr && skybox->isLoaded()) { // NEW: Only draw if loaded
        skybox->draw(cb, globalSet);
    }

    // 2. Agnostic ECS Iteration: Filter for Opaque objects
    for (uint32_t i = 0; i < meshRenderers.GetCount(); ++i) {
        const auto& mr = meshRenderers.Data()[i];
        const GE::ECS::EntityID id = meshRenderers.Index()[i];
        auto* const transform = em->GetTIComponent<GE::Components::Transform>(id);

        if (transform != nullptr) {
            for (const auto& sub : mr.subMeshes) {
                // Draw if the material is configured for the Opaque pass
                if (sub.mesh && sub.material && sub.material->GetPassType() == RenderPassType::Opaque) {
                    sub.mesh->draw(cb, globalSet, nullptr, transform->m_worldMatrix);
                }
            }
        }
    }

    vkCmdEndRenderPass(cb);
}

// ========================================================================
// SECTION 4: TRANSPARENCY & PARTICLES
// ========================================================================

/**
 * @brief Records the transparent rendering pass and dispatches agnostic particles.
 */
void Renderer::recordTransparentPass(
    const VkCommandBuffer cb,
    const VkExtent2D& extent,
    const PostProcessor* const postProcessor,
    const VkDescriptorSet globalSet,
    const std::vector<Pipeline*>& pipelines,
    GE::ECS::EntityManager* const em
) const {
    auto& meshRenderers = em->GetCompArr<GE::Components::MeshRenderer>();

    VkRenderPassBeginInfo transPassInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    transPassInfo.renderPass = postProcessor->getTransparentRenderPass();
    transPassInfo.framebuffer = postProcessor->getOffscreenFramebuffer();
    transPassInfo.renderArea.extent = extent;

    // Logic: Depth/Color already cleared by Opaque Pass; we reuse the existing content.
    transPassInfo.clearValueCount = 0U;
    transPassInfo.pClearValues = nullptr;

    vkCmdBeginRenderPass(cb, &transPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    const VkViewport vp{ 0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f };
    vkCmdSetViewport(cb, 0U, 1U, &vp);

    const VkRect2D sc{ {0, 0}, extent };
    vkCmdSetScissor(cb, 0U, 1U, &sc);

    // 1. Agnostic ECS Iteration: Filter for Transparent objects (Glass, Water, Ice)
    for (uint32_t i = 0; i < meshRenderers.GetCount(); ++i) {
        const auto& mr = meshRenderers.Data()[i];
        const GE::ECS::EntityID id = meshRenderers.Index()[i];
        auto* const transform = em->GetTIComponent<GE::Components::Transform>(id);

        if (transform != nullptr) {
            for (const auto& sub : mr.subMeshes) {
                if (sub.mesh && sub.material && sub.material->GetPassType() == RenderPassType::Transparent) {
                    sub.mesh->draw(cb, globalSet, nullptr, transform->m_worldMatrix);
                }
            }
        }
    }

    // 2. Generic Particle Pass
    // Fulfills Requirement: Dynamic particle systems defined in .ini files
    recordParticles(cb, globalSet, em);

    vkCmdEndRenderPass(cb);
}

/**
 * @brief Generic Particle Dispatcher: Iterates over all entities with ParticleComponents.
 */
void Renderer::recordParticles(
    const VkCommandBuffer cb,
    const VkDescriptorSet globalSet,
    GE::ECS::EntityManager* const em
) const {
    auto& particleComps = em->GetCompArr<GE::Components::ParticleComponent>();

    for (uint32_t i = 0; i < particleComps.GetCount(); ++i) {
        auto& pc = particleComps.Data()[i];

        // Only draw if the entity is active and enabled
        if (pc.enabled && pc.system) {
            pc.system->draw(cb, globalSet);
        }
    }
}

} // namespace GE::Graphics