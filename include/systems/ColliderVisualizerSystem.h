#pragma once

/* parasoft-begin-suppress ALL */
#include <vulkan/vulkan.h>
#include <vector>
/* parasoft-end-suppress ALL */

#include <glm/glm.hpp>
#include "ecs/IECSystem.h"
#include "graphics/GpuUploadContext.h"
#include "graphics/GraphicsPipeline.h"
#include "assets/Vertex.h"

namespace GE::Systems {

/**
 * @class ColliderVisualizerSystem
 * @brief CPU/GPU system that renders wireframe overlays for SphereCollider and PlaneCollider
 * components. Uses a dedicated LINE_LIST pipeline with depth-test-ON / depth-write-OFF so
 * wire lines are correctly occluded by solid geometry without affecting future depth reads.
 */
class ColliderVisualizerSystem final : public ECS::ICpuSystem {
public:
    /// Wire colour: #83d42b green, baked into vertex buffer at construction time.
    static constexpr glm::vec3 WIRE_COLOR{ 0.514f, 0.831f, 0.169f };
    /// Visual half-extent of the plane quad in world units.
    static constexpr float WIRE_PLANE_HALF_SIZE = 5.0f;

    /**
     * @brief Constructs the system and uploads wire geometry to the GPU.
     * @param ctx  Active GpuUploadContext — staging buffers are appended to ctx and freed
     *             by EngineOrchestrator after the command buffer is submitted.
     */
    explicit ColliderVisualizerSystem(GE::Graphics::GpuUploadContext& ctx);

    /** @brief Destructor: Frees sphere and plane GPU buffers. */
    ~ColliderVisualizerSystem() override;

    ERROR_CODE Shutdown() override {
        m_state = SystemState::ShuttingDown;
        return ERROR_CODE::OK;
    }

    /** @brief No per-frame CPU work; all work happens in RecordPass(). */
    void OnUpdate(float) override {}

    /**
     * @brief Records wireframe draw calls into the active command buffer.
     * Binds the wire pipeline, iterates SphereCollider and PlaneCollider component arrays,
     * and issues direct vkCmdDrawIndexed calls (no Set 1 binding).
     *
     * @param cb           Command buffer currently in the recording state.
     * @param globalSet    Set 0 descriptor (global UBO / shadow sampler).
     * @param wirePipeline The LINE_LIST pipeline created in Scenario::createMaterialPipelines().
     */
    void RecordPass(
        VkCommandBuffer cb,
        VkDescriptorSet globalSet,
        const GE::Graphics::GraphicsPipeline* wirePipeline
    ) const;

    /** @brief ImGui toggle — set to false to hide all collider overlays. */
    bool m_enabled{ true };

private:
    /**
     * @brief Uploads one vertex + index buffer pair via the provided command buffer.
     * Staging buffers are appended to ctx so EngineOrchestrator frees them after submit.
     */
    void uploadGeometry(
        GE::Graphics::GpuUploadContext& ctx,
        const std::vector<GE::Assets::Vertex>& vertices,
        const std::vector<uint32_t>& indices,
        VkBuffer& outVertBuf, VkDeviceMemory& outVertMem,
        VkBuffer& outIdxBuf,  VkDeviceMemory& outIdxMem
    );

    // --- Wire sphere GPU buffers (unit sphere, scaled at draw time by collider radius) ---
    VkBuffer       m_sphereVertBuf{ VK_NULL_HANDLE };
    VkDeviceMemory m_sphereVertMem{ VK_NULL_HANDLE };
    VkBuffer       m_sphereIdxBuf { VK_NULL_HANDLE };
    VkDeviceMemory m_sphereIdxMem { VK_NULL_HANDLE };
    uint32_t       m_sphereIdxCount{ 0U };

    // --- Wire plane GPU buffers (border quad + cross, oriented at draw time) ---
    VkBuffer       m_planeVertBuf{ VK_NULL_HANDLE };
    VkDeviceMemory m_planeVertMem{ VK_NULL_HANDLE };
    VkBuffer       m_planeIdxBuf { VK_NULL_HANDLE };
    VkDeviceMemory m_planeIdxMem { VK_NULL_HANDLE };
    uint32_t       m_planeIdxCount{ 0U };
};

} // namespace GE::Systems
