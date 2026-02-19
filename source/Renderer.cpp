#include "../include/Renderer.h"
#include "../include/ServiceLocator.h"
#include "../include/EntityManager.h"
#include "../include/Components.h"
#include "../include/ParticleComponent.h"
#include "../include/Camera.h"
#include "../include/InputManager.h"

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
    const std::vector<Pipeline*>& pipelines,
    bool multiview // Passed from Experience.cpp
) const {
    recordShadowPass(cb, shadowPass, shadowFramebuffer,
        pipelines.at(PIPELINE_IDX_SHADOW), globalDescriptorSet, em);

    // Pass the multiview toggle to the geometry passes
    recordOpaquePass(cb, extent, skybox, postProcessor, globalDescriptorSet, pipelines, em, multiview);

    // Barrier logic remains identical...
    const VkImageMemoryBarrier resolveBarrier{
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, nullptr,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
        postProcessor->getResolveImage(),
        { VK_IMAGE_ASPECT_COLOR_BIT, 0U, 1U, 0U, 1U }
    };
    vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, 0U, 0U, nullptr, 0U, nullptr, 1U, &resolveBarrier);

    if (postProcessor != nullptr) {
        postProcessor->copyScene(cb);
    }

    recordTransparentPass(cb, extent, postProcessor, globalDescriptorSet, pipelines, em, multiview);
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

    // 1. Setup Render Pass Info
    VkRenderPassBeginInfo shadowPassInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    shadowPassInfo.renderPass = renderPass;
    shadowPassInfo.framebuffer = framebuffer;
    shadowPassInfo.renderArea.extent = { EngineConstants::SHADOW_MAP_RES, EngineConstants::SHADOW_MAP_RES };

    VkClearValue shadowClear{};
    shadowClear.depthStencil = { DEPTH_CLEAR_VAL, 0U };
    shadowPassInfo.clearValueCount = 1U;
    shadowPassInfo.pClearValues = &shadowClear;

    // 2. Begin Pass and Set Fixed-Function State
    vkCmdBeginRenderPass(cb, &shadowPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    const float shadowRes = static_cast<float>(EngineConstants::SHADOW_MAP_RES);
    const VkViewport sv{ 0.0f, 0.0f, shadowRes, shadowRes, 0.0f, 1.0f };
    vkCmdSetViewport(cb, 0U, 1U, &sv);

    const VkRect2D ss{ {0, 0}, {EngineConstants::SHADOW_MAP_RES, EngineConstants::SHADOW_MAP_RES} };
    vkCmdSetScissor(cb, 0U, 1U, &ss);

    // 3. Bind the Shadow Pipeline
    shadowPipeline->bind(cb);

    // 4. Agnostic Iteration: Draw only entities that cast shadows
    for (uint32_t i = 0; i < meshRenderers.GetCount(); ++i) {
        const auto& mr = meshRenderers.Data()[i];
        const GE::ECS::EntityID entityID = meshRenderers.Index()[i];

        auto* const transform = em->GetTIComponent<GE::Scene::Components::Transform>(entityID);

        if (transform != nullptr) {
            for (const auto& sub : mr.subMeshes) {
                if (sub.mesh && sub.material && sub.material->GetCastsShadows()) {
                    // Mesh::draw now handles vkCmdPushConstants internally.
                    // We simply pass the world matrix, which shadow.vert expects.
                    sub.mesh->draw(cb, globalSet, shadowPipeline, transform->m_worldMatrix, glm::mat4(1.0f));
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
    const VkCommandBuffer cb, const VkExtent2D& extent, const Skybox* const skybox,
    const PostProcessor* const postProcessor, const VkDescriptorSet globalSet,
    const std::vector<Pipeline*>& pipelines, GE::ECS::EntityManager* const em,
    bool multiview
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

    uint32_t viewCount = multiview ? 4U : 1U;
    float fullW = static_cast<float>(extent.width);
    float fullH = static_cast<float>(extent.height);

    for (uint32_t v = 0U; v < viewCount; ++v) {
        // 1. Set Viewport for current quadrant
        VkViewport vp{};
        if (!multiview) {
            vp = { 0.0f, 0.0f, fullW, fullH, 0.0f, 1.0f };
        }
        else {
            float hw = fullW * 0.5f; float hh = fullH * 0.5f;
            if (v == 0)      vp = { 0.0f, 0.0f, hw, hh, 0.0f, 1.0f }; // Top-Left
            else if (v == 1) vp = { hw, 0.0f, hw, hh, 0.0f, 1.0f };   // Top-Right
            else if (v == 2) vp = { 0.0f, hh, hw, hh, 0.0f, 1.0f };   // Bottom-Left
            else             vp = { hw, hh, hw, hh, 0.0f, 1.0f };   // Bottom-Right
        }
        vkCmdSetViewport(cb, 0U, 1U, &vp);

        const VkRect2D sc{ {0, 0}, extent };
        vkCmdSetScissor(cb, 0U, 1U, &sc);

        // 2. Resolve View-Projection for this quadrant
        glm::mat4 vpMatrix;
        float aspect = (vp.width / vp.height);
        if (v == 0) {
            auto* cam = ServiceLocator::GetInput()->getActiveCamera();
            vpMatrix = cam->getProjectionMatrix(aspect) * cam->getViewMatrix();
            vpMatrix[1][1] *= -1.0f; // Vulkan correction
        }
        else if (v == 1) vpMatrix = Camera::getStaticVP({ 0, 15, 0 }, -89.9f, -90.0f, aspect);
        else if (v == 2) vpMatrix = Camera::getStaticVP({ 0, 0, 15 }, 0.0f, -90.0f, aspect);
        else             vpMatrix = Camera::getStaticVP({ 15, 0, 0 }, 0.0f, 180.0f, aspect);

		// 3. Draw Skybox (Perspective View only)
        if (v == 0 && skybox != nullptr && skybox->isLoaded()) {
            // NEW: Pass the vpMatrix resolved for the main perspective quadrant
            // This fills the 128-byte push constant expected by skybox_vert.spv
            skybox->draw(cb, globalSet, vpMatrix);
        }

        // 4. Draw Geometry
        for (uint32_t i = 0; i < meshRenderers.GetCount(); ++i) {
            const auto& mr = meshRenderers.Data()[i];
            auto* const transform = em->GetTIComponent<GE::Scene::Components::Transform>(meshRenderers.Index()[i]);

            if (transform != nullptr) {
                for (const auto& sub : mr.subMeshes) {
                    if (sub.mesh && sub.material && sub.material->GetPassType() == RenderPassType::Opaque) {

                        // Fulfills Requirement: Calculate both MVP and Model matrices
                        // MVP is used for screen positioning, Model is used for 3D lighting/normals
                        glm::mat4 modelMatrix = transform->m_worldMatrix;
                        glm::mat4 mvpMatrix = vpMatrix * modelMatrix;

                        // NEW: Update Mesh::draw signature to take both matrices
                        sub.mesh->draw(cb, globalSet, nullptr, mvpMatrix, modelMatrix);
                    }
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
 /**
  * @brief Records the transparent rendering pass, now supporting Multiview quadrants.
  * Fulfills Requirement: Visualise primitive shapes and simulations from multiple views.
  */
  /**
   * @brief Records the transparent rendering pass, supporting Multiview quadrants.
   * Fulfills Requirement: Visualise primitive shapes and simulations from multiple views.
   */
void Renderer::recordTransparentPass(
    const VkCommandBuffer cb,
    const VkExtent2D& extent,
    const PostProcessor* const postProcessor,
    const VkDescriptorSet globalSet,
    const std::vector<Pipeline*>& pipelines,
    GE::ECS::EntityManager* const em,
    bool multiview
) const {
    auto& meshRenderers = em->GetCompArr<GE::Components::MeshRenderer>();

    VkRenderPassBeginInfo transPassInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    transPassInfo.renderPass = postProcessor->getTransparentRenderPass();
    transPassInfo.framebuffer = postProcessor->getOffscreenFramebuffer();
    transPassInfo.renderArea.extent = extent;

    transPassInfo.clearValueCount = 0U;
    transPassInfo.pClearValues = nullptr;

    vkCmdBeginRenderPass(cb, &transPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    uint32_t viewCount = multiview ? 4U : 1U;
    float fullW = static_cast<float>(extent.width);
    float fullH = static_cast<float>(extent.height);

    for (uint32_t v = 0U; v < viewCount; ++v) {
        VkViewport vp{};
        if (!multiview) {
            vp = { 0.0f, 0.0f, fullW, fullH, 0.0f, 1.0f };
        }
        else {
            float hw = fullW * 0.5f; float hh = fullH * 0.5f;
            if (v == 0)      vp = { 0.0f, 0.0f, hw, hh, 0.0f, 1.0f };
            else if (v == 1) vp = { hw, 0.0f, hw, hh, 0.0f, 1.0f };
            else if (v == 2) vp = { 0.0f, hh, hw, hh, 0.0f, 1.0f };
            else             vp = { hw, hh, hw, hh, 0.0f, 1.0f };
        }
        vkCmdSetViewport(cb, 0U, 1U, &vp);

        const VkRect2D sc{ {0, 0}, extent };
        vkCmdSetScissor(cb, 0U, 1U, &sc);

        glm::mat4 vpMatrix;
        float aspect = (vp.width / vp.height);
        if (v == 0) {
            auto* cam = ServiceLocator::GetInput()->getActiveCamera();
            vpMatrix = cam->getProjectionMatrix(aspect) * cam->getViewMatrix();
            vpMatrix[1][1] *= -1.0f;
        }
        else if (v == 1) vpMatrix = Camera::getStaticVP({ 0, 15, 0 }, -89.9f, -90.0f, aspect);
        else if (v == 2) vpMatrix = Camera::getStaticVP({ 0, 0, 15 }, 0.0f, -90.0f, aspect);
        else             vpMatrix = Camera::getStaticVP({ 15, 0, 0 }, 0.0f, 180.0f, aspect);

        // 3. Draw Transparent Meshes (Fixed for 3D depth)
        for (uint32_t i = 0; i < meshRenderers.GetCount(); ++i) {
            const auto& mr = meshRenderers.Data()[i];
            auto* const transform = em->GetTIComponent<GE::Scene::Components::Transform>(meshRenderers.Index()[i]);

            if (transform != nullptr) {
                for (const auto& sub : mr.subMeshes) {
                    if (sub.mesh && sub.material && sub.material->GetPassType() == RenderPassType::Transparent) {

                        // FIX: Pass BOTH matrices to restore 3D lighting/water ripples
                        glm::mat4 modelMatrix = transform->m_worldMatrix;
                        glm::mat4 mvpMatrix = vpMatrix * modelMatrix;

                        sub.mesh->draw(cb, globalSet, nullptr, mvpMatrix, modelMatrix);
                    }
                }
            }
        }

        // 4. Record Particles for this view quadrant
        recordParticles(cb, globalSet, em, vpMatrix);
    }

    vkCmdEndRenderPass(cb);
}

 /**
  * @brief Generic Particle Dispatcher: Iterates over all entities with ParticleComponents.
  * Fulfills Requirement: Dynamic particles visible in all views (Top, Side, Front, Main).
  */
void Renderer::recordParticles(
    const VkCommandBuffer cb,
    const VkDescriptorSet globalSet,
    GE::ECS::EntityManager* const em,
    const glm::mat4& quadrantVP
) const {
    auto& particleComps = em->GetCompArr<GE::Components::ParticleComponent>();

    for (uint32_t i = 0; i < particleComps.GetCount(); ++i) {
        auto& pc = particleComps.Data()[i];

        // Only draw if the system is fully initialized and active
        if (pc.enabled && pc.system) {
            /**
             * For particles, we push the 'quadrantVP' (Projection * View).
             * The shader uses: gl_Position = push.vp * vec4(inPosition.xyz, 1.0);
             *
             */
            pc.system->draw(cb, globalSet, quadrantVP);
        }
    }
}