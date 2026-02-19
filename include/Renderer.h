#pragma once

/* parasoft-begin-suppress ALL */
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
/* parasoft-end-suppress ALL */

// Engine Includes
#include "../include/Mesh.h"
#include "../include/Model.h"
#include "../include/Skybox.h"
#include "PostProcessor.h"
#include "Pipeline.h"
#include "../include/VulkanContext.h"
#include "../include/EntityManager.h"

/**
 * @class Renderer
 * @brief Agnostic Frame Orchestrator.
 */
class Renderer final {
public:
    static constexpr uint32_t PIPELINE_IDX_SHADOW = 6U;
    static constexpr float    DEPTH_CLEAR_VAL = 1.0f;

    explicit Renderer() {}
    ~Renderer() = default;

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    /** @brief Orchestrates the multi-pass recording, now supporting Multiview split-screen. */
    void recordFrame(
        const VkCommandBuffer cb,
        const VkExtent2D& extent,
        const Skybox* const skybox,
        GE::ECS::EntityManager* const em,
        const PostProcessor* const postProcessor,
        const VkDescriptorSet globalDescriptorSet,
        const VkRenderPass shadowPass,
        const VkFramebuffer shadowFramebuffer,
        const std::vector<Pipeline*>& pipelines,
        bool multiview // NEW: Toggle parameter
    ) const;

private:
    void recordShadowPass(
        const VkCommandBuffer cb,
        const VkRenderPass renderPass,
        const VkFramebuffer framebuffer,
        const Pipeline* const shadowPipeline,
        const VkDescriptorSet globalSet,
        GE::ECS::EntityManager* const em
    ) const;

    /** @brief Main forward pass updated to handle 4-way viewport splitting. */
    void recordOpaquePass(
        const VkCommandBuffer cb,
        const VkExtent2D& extent,
        const Skybox* const skybox,
        const PostProcessor* const postProcessor,
        const VkDescriptorSet globalSet,
        const std::vector<Pipeline*>& pipelines,
        GE::ECS::EntityManager* const em,
        bool multiview // NEW
    ) const;

    /** @brief Transparency pass updated to match multiview quadrants. */
    void recordTransparentPass(
        const VkCommandBuffer cb,
        const VkExtent2D& extent,
        const PostProcessor* const postProcessor,
        const VkDescriptorSet globalSet,
        const std::vector<Pipeline*>& pipelines,
        GE::ECS::EntityManager* const em,
        bool multiview // NEW
    ) const;

    void recordParticles(
        const VkCommandBuffer cb,
        const VkDescriptorSet globalSet,
        GE::ECS::EntityManager* const em,
        const glm::mat4& quadrantVP // NEW
    ) const;
};