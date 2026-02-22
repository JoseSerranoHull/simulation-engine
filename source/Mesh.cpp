#include "../include/Mesh.h"

/* parasoft-begin-suppress ALL */
#include "../include/Pipeline.h"
#include <utility>
/* parasoft-end-suppress ALL */

using namespace GE::Graphics;

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
void Mesh::draw(VkCommandBuffer cb, VkDescriptorSet globalSet, const Pipeline* pipelineOverride, const glm::mat4& worldMatrix) const {
    // 1. Resolve active pipeline using your existing logic
    const Pipeline* const activePipeline = (pipelineOverride != nullptr)
        ? pipelineOverride
        : ((material != nullptr) ? material->getPipeline() : nullptr);

    if ((activePipeline != nullptr) && (cb != VK_NULL_HANDLE)) {
        // 2. Bind Pipeline State (using your actual API)
        activePipeline->bind(cb);

        // 3. Bind Descriptor Sets (using an array to handle the shared_ptr/r-value correctly)
        const VkDescriptorSet sets[SET_COUNT] = {
            globalSet,
            (material != nullptr) ? material->getDescriptorSet() : VK_NULL_HANDLE
        };

        vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS,
            activePipeline->getPipelineLayout(), SET_GLOBAL, SET_COUNT, sets, 0U, nullptr);

        // 4. Update World Matrix via Push Constants
        // We now use the passed worldMatrix instead of the old internal modelMatrix
        vkCmdPushConstants(cb, activePipeline->getPipelineLayout(),
            VK_SHADER_STAGE_VERTEX_BIT, EngineConstants::OFFSET_ZERO,
            static_cast<uint32_t>(sizeof(glm::mat4)), &worldMatrix);

        // 5. Bind Geometry Buffers
        const VkDeviceSize offsets[BUFFER_COUNT_ONE] = { 0ULL };
        vkCmdBindVertexBuffers(cb, BINDING_FIRST, BUFFER_COUNT_ONE, &buffer, offsets);
        vkCmdBindIndexBuffer(cb, buffer, indexOffset, VK_INDEX_TYPE_UINT32);

        // 6. Draw call
        vkCmdDrawIndexed(cb, indexCount, INSTANCE_COUNT_ONE, 0U, 0, 0U);
    }
}