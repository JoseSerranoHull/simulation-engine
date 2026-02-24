#pragma once

/* parasoft-begin-suppress ALL */
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
/* parasoft-end-suppress ALL */
#include <glm/glm.hpp>

// Engine Includes
#include "assets/Mesh.h"
#include "assets/Model.h"
#include "graphics/Skybox.h"
#include "graphics/PostProcessBackend.h"
#include "graphics/GraphicsPipeline.h"
#include "graphics/VulkanContext.h"
#include "ecs/EntityManager.h"

namespace GE::Graphics {

/**
 * @class Renderer
 * @brief Agnostic Frame Orchestrator.
 * * Manages the multi-pass rendering sequence by querying the ECS for
 * MeshRenderers, Lights, and future ParticleComponents.
 */
class Renderer final {
public:
    // --- Functional Constants ---
    static constexpr float DEPTH_CLEAR_VAL = 1.0f;

    /** @brief Constructor: Standard initialization. */
    explicit Renderer() {}
    ~Renderer() = default;

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    /**
     * @brief Orchestrates the full frame recording sequence using the ECS.
     * Fulfills Requirement: Agnostic rendering of any scenario entities.
     */
    void recordFrame(
        const VkCommandBuffer cb,
        const VkExtent2D& extent,
        const Skybox* const skybox,
        GE::ECS::EntityManager* const em, // The source of all draw data
        const PostProcessBackend* const postProcessor,
        const VkDescriptorSet globalDescriptorSet,
        const VkRenderPass shadowPass,
        const VkFramebuffer shadowFramebuffer,
        const std::vector<GraphicsPipeline*>& materialPipelines,
        const GraphicsPipeline* shadowPipeline,
        const glm::vec4& clearColor                   = glm::vec4{ 0.0f, 0.0f, 0.0f, 1.0f },
        const GraphicsPipeline* checkerboardPipeline  = nullptr,
        const void*  checkerboardPushData             = nullptr,
        uint32_t     checkerboardPushDataSize         = 0U
    ) const;

private:
    // --- Private Pass-Specific Recorders ---

    /** @brief Records the depth-only pass for shadow map generation. */
    void recordShadowPass(
        const VkCommandBuffer cb,
        const VkRenderPass renderPass,
        const VkFramebuffer framebuffer,
        const GraphicsPipeline* const shadowPipeline,
        const VkDescriptorSet globalSet,
        GE::ECS::EntityManager* const em
    ) const;

    /** @brief Records the main forward rendering pass for opaque geometry. */
    void recordOpaquePass(
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
    ) const;

    /** @brief Records alpha-blended geometry and dispatches particles. */
    void recordTransparentPass(
        const VkCommandBuffer cb,
        const VkExtent2D& extent,
        const PostProcessBackend* const postProcessor,
        const VkDescriptorSet globalSet,
        const std::vector<GraphicsPipeline*>& pipelines,
        GE::ECS::EntityManager* const em
    ) const;

    /** @brief Generic Particle Dispatcher: Renamed as per your naming choice. */
    void recordParticles(
        const VkCommandBuffer cb,
        const VkDescriptorSet globalSet,
        GE::ECS::EntityManager* const em
    ) const;
};

} // namespace GE::Graphics