#include "graphics/Renderer.h"
#include "core/ServiceLocator.h"
#include "ecs/EntityManager.h"
#include "components/Components.h"
#include "components/ParticleComponent.h"
#include "systems/ParticleEmitterSystem.h"

/* parasoft-begin-suppress ALL */
#include <array>
/* parasoft-end-suppress ALL */

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
    const PostProcessBackend* const postProcessor,
    const VkDescriptorSet globalDescriptorSet,
    const VkRenderPass shadowPass,
    const VkFramebuffer shadowFramebuffer,
    const std::vector<GraphicsPipeline*>& materialPipelines,
    const GraphicsPipeline* shadowPipeline,
    const glm::vec4& clearColor,
    const GraphicsPipeline* checkerboardPipeline,
    const void*  checkerboardPushData,
    uint32_t     checkerboardPushDataSize
) const {
    // Step 1: Shadow Mapping Pass (Depth-only)
    recordShadowPass(cb, shadowPass, shadowFramebuffer,
        shadowPipeline, globalDescriptorSet, em);

    // Step 2: Main Opaque Pass (Scene geometry + Skybox)
    recordOpaquePass(cb, extent, skybox, postProcessor, globalDescriptorSet, materialPipelines, em,
        clearColor, checkerboardPipeline, checkerboardPushData, checkerboardPushDataSize);

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
    recordTransparentPass(cb, extent, postProcessor, globalDescriptorSet, materialPipelines, em);
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
    const GraphicsPipeline* const shadowPipeline,
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

    // Global shadow toggle: read from InputService via ServiceLocator
    const bool globalShadowsEnabled = ServiceLocator::GetInput()->getGlobalShadowsEnabled();

    // Agnostic Iteration: Draw only entities that cast shadows
    for (uint32_t i = 0; i < meshRenderers.GetCount(); ++i) {
        const auto& mr = meshRenderers.Data()[i];
        const GE::ECS::EntityID entityID = meshRenderers.Index()[i];

        auto* const transform = em->GetTIComponent<GE::Components::Transform>(entityID);

        if (transform != nullptr) {
            for (const auto& sub : mr.subMeshes) {
                // Logic: Does the material permit shadows? (Defined in .ini)
                if (sub.m_mesh && sub.m_material && globalShadowsEnabled && sub.m_material->GetCastsShadows()) {
                    sub.m_mesh->draw(cb, globalSet, shadowPipeline, transform->m_worldMatrix);
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
    const PostProcessBackend* const postProcessor,
    const VkDescriptorSet globalSet,
    const std::vector<GraphicsPipeline*>& pipelines,
    GE::ECS::EntityManager* const em,
    const glm::vec4& clearColor,
    const GraphicsPipeline* checkerboardPipeline,
    const void*  checkerboardPushData,
    uint32_t     checkerboardPushDataSize
) const {
    auto& meshRenderers = em->GetCompArr<GE::Components::MeshRenderer>();

    VkRenderPassBeginInfo opaquePassInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    opaquePassInfo.renderPass = postProcessor->getOffscreenRenderPass();
    opaquePassInfo.framebuffer = postProcessor->getOffscreenFramebuffer();
    opaquePassInfo.renderArea.extent = extent;

    std::array<VkClearValue, 3U> clearValues{};
    clearValues[0].color = { { clearColor.r, clearColor.g, clearColor.b, clearColor.a } };
    clearValues[1].depthStencil = { DEPTH_CLEAR_VAL, 0U };
    clearValues[2].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };  // Resolve attachment — overwritten by MSAA blit

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
                if (sub.m_mesh && sub.m_material && sub.m_material->GetPassType() == RenderPassType::Opaque) {
                    // Detect checkerboard pipeline: inject colour push constants (offset 64, FRAG stage)
                    // after the pipeline bind but before the draw (handled inside Mesh::draw via ExtraPushConstants).
                    GE::Assets::Mesh::ExtraPushConstants checkerPush{};
                    const GE::Assets::Mesh::ExtraPushConstants* extraPushPtr = nullptr;

                    if ((checkerboardPipeline != nullptr)
                        && (checkerboardPushData != nullptr)
                        && (checkerboardPushDataSize > 0U)
                        && (sub.m_material->getPipeline() == checkerboardPipeline))
                    {
                        checkerPush.data        = checkerboardPushData;
                        checkerPush.offset      = static_cast<uint32_t>(sizeof(glm::mat4));
                        checkerPush.size        = checkerboardPushDataSize;
                        // The checkerboard pipeline layout declares ONE range [0,100) VERT|FRAG.
                        // Every vkCmdPushConstants call that touches any byte in [0,100) must
                        // include both stages (VUID-vkCmdPushConstants-offset-01796).
                        checkerPush.stages      = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
                        checkerPush.modelStages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
                        extraPushPtr = &checkerPush;
                    }

                    sub.m_mesh->draw(cb, globalSet, nullptr, transform->m_worldMatrix, extraPushPtr);
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
    const PostProcessBackend* const postProcessor,
    const VkDescriptorSet globalSet,
    const std::vector<GraphicsPipeline*>& pipelines,
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
                if (sub.m_mesh && sub.m_material && sub.m_material->GetPassType() == RenderPassType::Transparent) {
                    sub.m_mesh->draw(cb, globalSet, nullptr, transform->m_worldMatrix);
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

    auto* pes = ServiceLocator::GetParticleEmitterSystem();

    for (uint32_t i = 0; i < particleComps.GetCount(); ++i) {
        auto& pc = particleComps.Data()[i];

        if (pc.enabled && pc.emitterIndex != GE::Components::ParticleComponent::INVALID_EMITTER) {
            GpuParticleBackend* backend = pes->GetBackend(pc.emitterIndex);
            if (backend) backend->draw(cb, globalSet);
        }
    }
}

} // namespace GE::Graphics