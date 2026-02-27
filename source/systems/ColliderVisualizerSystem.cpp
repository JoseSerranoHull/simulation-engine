#include "systems/ColliderVisualizerSystem.h"
#include "assets/GeometryUtils.h"
#include "graphics/VulkanUtils.h"
#include "graphics/VulkanContext.h"
#include "core/ServiceLocator.h"
#include "ecs/EntityManager.h"
#include "components/Transform.h"
#include "components/PhysicsComponents.h"

/* parasoft-begin-suppress ALL */
#include <cstring>
#include <glm/gtc/matrix_transform.hpp>
/* parasoft-end-suppress ALL */

namespace GE::Systems {

ColliderVisualizerSystem::ColliderVisualizerSystem(GE::Graphics::GpuUploadContext& ctx) {
    m_typeID = ECS::IECSystem::GetUniqueISystemTypeID<ColliderVisualizerSystem>();
    m_stage  = ECS::ESystemStage::Render;
    m_state  = SystemState::Running;

    const auto sphereData = GE::Assets::GeometryUtils::generateWireSphere(32U, WIRE_COLOR);
    const auto planeData  = GE::Assets::GeometryUtils::generateWirePlane(WIRE_PLANE_HALF_SIZE, WIRE_COLOR);

    m_sphereIdxCount = static_cast<uint32_t>(sphereData.indices.size());
    m_planeIdxCount  = static_cast<uint32_t>(planeData.indices.size());

    uploadGeometry(ctx, sphereData.vertices, sphereData.indices,
        m_sphereVertBuf, m_sphereVertMem, m_sphereIdxBuf, m_sphereIdxMem);

    uploadGeometry(ctx, planeData.vertices, planeData.indices,
        m_planeVertBuf, m_planeVertMem, m_planeIdxBuf, m_planeIdxMem);
}

ColliderVisualizerSystem::~ColliderVisualizerSystem() {
    GE::Graphics::VulkanContext* ctx = ServiceLocator::GetContext();
    if ((ctx == nullptr) || (ctx->device == VK_NULL_HANDLE)) { return; }

    vkDestroyBuffer(ctx->device, m_sphereVertBuf, nullptr);
    vkFreeMemory   (ctx->device, m_sphereVertMem, nullptr);
    vkDestroyBuffer(ctx->device, m_sphereIdxBuf,  nullptr);
    vkFreeMemory   (ctx->device, m_sphereIdxMem,  nullptr);

    vkDestroyBuffer(ctx->device, m_planeVertBuf,  nullptr);
    vkFreeMemory   (ctx->device, m_planeVertMem,  nullptr);
    vkDestroyBuffer(ctx->device, m_planeIdxBuf,   nullptr);
    vkFreeMemory   (ctx->device, m_planeIdxMem,   nullptr);
}

// ============================================================================
// SECTION: GPU Buffer Upload
// ============================================================================

void ColliderVisualizerSystem::uploadGeometry(
    GE::Graphics::GpuUploadContext& ctx,
    const std::vector<GE::Assets::Vertex>& vertices,
    const std::vector<uint32_t>& indices,
    VkBuffer& outVertBuf, VkDeviceMemory& outVertMem,
    VkBuffer& outIdxBuf,  VkDeviceMemory& outIdxMem)
{
    using namespace GE::Graphics;
    VulkanContext* vkCtx         = ServiceLocator::GetContext();
    const VkDevice device        = vkCtx->device;
    const VkPhysicalDevice phDev = vkCtx->physicalDevice;

    // --- Vertex buffer ---
    const VkDeviceSize vertSize = sizeof(GE::Assets::Vertex) * vertices.size();

    VkBuffer       stagingVertBuf;
    VkDeviceMemory stagingVertMem;
    VulkanUtils::createBuffer(device, phDev, vertSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingVertBuf, stagingVertMem);

    void* mapped = nullptr;
    vkMapMemory(device, stagingVertMem, 0, vertSize, 0, &mapped);
    std::memcpy(mapped, vertices.data(), static_cast<size_t>(vertSize));
    vkUnmapMemory(device, stagingVertMem);

    VulkanUtils::createBuffer(device, phDev, vertSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        outVertBuf, outVertMem);

    const VkBufferCopy vertCopy{ 0U, 0U, vertSize };
    vkCmdCopyBuffer(ctx.cmd, stagingVertBuf, outVertBuf, 1U, &vertCopy);

    ctx.stagingBuffers.push_back(stagingVertBuf);
    ctx.stagingMemories.push_back(stagingVertMem);

    // --- Index buffer ---
    const VkDeviceSize idxSize = sizeof(uint32_t) * indices.size();

    VkBuffer       stagingIdxBuf;
    VkDeviceMemory stagingIdxMem;
    VulkanUtils::createBuffer(device, phDev, idxSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingIdxBuf, stagingIdxMem);

    vkMapMemory(device, stagingIdxMem, 0, idxSize, 0, &mapped);
    std::memcpy(mapped, indices.data(), static_cast<size_t>(idxSize));
    vkUnmapMemory(device, stagingIdxMem);

    VulkanUtils::createBuffer(device, phDev, idxSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        outIdxBuf, outIdxMem);

    const VkBufferCopy idxCopy{ 0U, 0U, idxSize };
    vkCmdCopyBuffer(ctx.cmd, stagingIdxBuf, outIdxBuf, 1U, &idxCopy);

    ctx.stagingBuffers.push_back(stagingIdxBuf);
    ctx.stagingMemories.push_back(stagingIdxMem);
}

// ============================================================================
// SECTION: Draw Pass
// ============================================================================

void ColliderVisualizerSystem::RecordPass(
    VkCommandBuffer cb,
    VkDescriptorSet globalSet,
    const GE::Graphics::GraphicsPipeline* wirePipeline
) const {
    wirePipeline->bind(cb);

    // Bind Set 0 only — wire pipeline layout has no Set 1 (avoids validation errors)
    vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS,
        wirePipeline->getPipelineLayout(), 0U, 1U, &globalSet, 0U, nullptr);

    auto* em = ServiceLocator::GetEntityManager();
    const VkDeviceSize zeroOffset = 0U;

    // --- SphereColliders ---
    auto& sphereArr = em->GetCompArr<GE::Components::SphereCollider>();

    if (sphereArr.GetCount() > 0U) {
        vkCmdBindVertexBuffers(cb, 0U, 1U, &m_sphereVertBuf, &zeroOffset);
        vkCmdBindIndexBuffer(cb, m_sphereIdxBuf, 0U, VK_INDEX_TYPE_UINT32);

        for (uint32_t i = 0U; i < sphereArr.GetCount(); ++i) {
            const auto& sphereCol  = sphereArr.Data()[i];
            const GE::ECS::EntityID id = sphereArr.Index()[i];
            const auto* transform  = em->GetTIComponent<GE::Components::Transform>(id);
            if (transform == nullptr) { continue; }

            // Strip entity visual scale, apply collider radius.
            // glm::scale(m_worldMatrix, r) would compound entity scale — must not use that.
            const glm::vec3 worldPos = glm::vec3(transform->m_worldMatrix[3]);
            glm::mat3 rotOnly = glm::mat3(transform->m_worldMatrix);
            rotOnly[0] = glm::normalize(rotOnly[0]);
            rotOnly[1] = glm::normalize(rotOnly[1]);
            rotOnly[2] = glm::normalize(rotOnly[2]);

            glm::mat4 model = glm::mat4(rotOnly);
            model[3] = glm::vec4(worldPos, 1.0f);
            model = glm::scale(model, glm::vec3(sphereCol.radius));

            vkCmdPushConstants(cb, wirePipeline->getPipelineLayout(),
                VK_SHADER_STAGE_VERTEX_BIT, 0U,
                static_cast<uint32_t>(sizeof(glm::mat4)), &model);

            vkCmdDrawIndexed(cb, m_sphereIdxCount, 1U, 0U, 0, 0U);
        }
    }

    // --- PlaneColliders ---
    auto& planeArr = em->GetCompArr<GE::Components::PlaneCollider>();

    if (planeArr.GetCount() > 0U) {
        vkCmdBindVertexBuffers(cb, 0U, 1U, &m_planeVertBuf, &zeroOffset);
        vkCmdBindIndexBuffer(cb, m_planeIdxBuf, 0U, VK_INDEX_TYPE_UINT32);

        for (uint32_t i = 0U; i < planeArr.GetCount(); ++i) {
            const auto& plane      = planeArr.Data()[i];
            const GE::ECS::EntityID id = planeArr.Index()[i];
            const auto* transform  = em->GetTIComponent<GE::Components::Transform>(id);
            if (transform == nullptr) { continue; }

            // Build orientation matrix: align local +Y axis to the plane normal,
            // then translate to (entity world pos + normal * offset).
            const glm::vec3 norm   = glm::normalize(plane.normal);
            const glm::vec3 center = glm::vec3(transform->m_worldMatrix[3]) + norm * plane.offset;

            // Choose a reference up vector that is not parallel to the plane normal
            const glm::vec3 worldUp = (glm::abs(glm::dot(glm::vec3(0.0f, 1.0f, 0.0f), norm)) > 0.999f)
                ? glm::vec3(1.0f, 0.0f, 0.0f)
                : glm::vec3(0.0f, 1.0f, 0.0f);

            const glm::vec3 right   = glm::normalize(glm::cross(worldUp, norm));
            const glm::vec3 forward = glm::normalize(glm::cross(norm, right));

            const glm::mat4 model(
                glm::vec4(right,   0.0f),
                glm::vec4(norm,    0.0f),
                glm::vec4(forward, 0.0f),
                glm::vec4(center,  1.0f)
            );

            vkCmdPushConstants(cb, wirePipeline->getPipelineLayout(),
                VK_SHADER_STAGE_VERTEX_BIT, 0U,
                static_cast<uint32_t>(sizeof(glm::mat4)), &model);

            vkCmdDrawIndexed(cb, m_planeIdxCount, 1U, 0U, 0, 0U);
        }
    }
}

} // namespace GE::Systems
