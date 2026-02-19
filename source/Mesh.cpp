#include "../include/Mesh.h"

/* parasoft-begin-suppress ALL */
#include "../include/Pipeline.h"
#include <utility>
/* parasoft-end-suppress ALL */

/**
 * @brief Constructor: Initializes geometry metadata and shared material ownership.
 */
Mesh::Mesh(const VkBuffer inBuffer, const uint32_t inIndexCount,
    const VkDeviceSize inIndexOffset, std::shared_ptr<Material> inMaterial)
    : buffer(inBuffer), indexCount(inIndexCount),
    indexOffset(inIndexOffset), material(std::move(inMaterial))
{
    // Implementation Note: 'buffer' is a handle to a resource managed by AssetManager.
    // The Mesh class acts as a descriptor/accessor, not a primary owner of VRAM.
}

/**
 * @brief Destructor: Resource handles are nullified without explicit destruction.
 * To prevent double-free errors during engine shutdown, the VkBuffer is destroyed
 * centrally by the AssetManager.
 */
Mesh::~Mesh() {
    buffer = VK_NULL_HANDLE;
}

/**
 * @brief Updates the mesh's local transformation matrix.
 */
void Mesh::setModelMatrix(const glm::mat4& matrix) {
    modelMatrix = matrix;
}

/**
 * @brief Queries the material state to determine transparency requirements.
 */
bool Mesh::hasTransparency() const {
    // Check material and internal pipeline validity before querying
    return (material != nullptr) && (material->getPipeline() != nullptr);
}

/**
 * @brief Records the drawing sequence to the provided Vulkan Command Buffer.
 * Orchestrates pipeline binding, descriptor mapping (Global/Material),
 * and indexed draw execution.
 */
void Mesh::draw(VkCommandBuffer cb, VkDescriptorSet globalSet, const Pipeline* pipelineOverride,
    const glm::mat4& mvp, const glm::mat4& model) const {

    const Pipeline* const activePipeline = (pipelineOverride != nullptr)
        ? pipelineOverride
        : ((material != nullptr) ? material->getPipeline() : nullptr);

    if ((activePipeline != nullptr) && (cb != VK_NULL_HANDLE)) {
        activePipeline->bind(cb);

        const VkDescriptorSet sets[SET_COUNT] = {
            globalSet,
            (material != nullptr) ? material->getDescriptorSet() : VK_NULL_HANDLE
        };

        vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS,
            activePipeline->getPipelineLayout(), SET_GLOBAL, SET_COUNT, sets, 0U, nullptr);

        // --- FIX: Push both matrices at once ---
        MeshPushConstants constants{ mvp, model };
        vkCmdPushConstants(cb, activePipeline->getPipelineLayout(),
            VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &constants);

        const VkDeviceSize offsets[1] = { 0ULL };
        vkCmdBindVertexBuffers(cb, 0, 1, &buffer, offsets);
        vkCmdBindIndexBuffer(cb, buffer, indexOffset, VK_INDEX_TYPE_UINT32);

        vkCmdDrawIndexed(cb, indexCount, 1, 0U, 0, 0U);
    }
}