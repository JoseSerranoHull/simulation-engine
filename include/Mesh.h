#pragma once

/* parasoft-begin-suppress ALL */
#include "../include/libs.h"
#include <memory>
#include <string>
/* parasoft-end-suppress ALL */

#include "../include/Material.h"
#include "../include/VulkanContext.h"
#include "../include/ServiceLocator.h"

namespace GE::Graphics { class Pipeline; }

/**
 * @class Mesh
 * @brief Encapsulates a single piece of GPU geometry and its associated material.
 * This class handles the recording of draw commands and manages the local model transform.
 */
class Mesh final {
public:
    // --- Rendering Constants ---
    static constexpr uint32_t SET_GLOBAL = 0U;
    static constexpr uint32_t SET_MATERIAL = 1U;
    static constexpr uint32_t SET_COUNT = 2U;
    static constexpr uint32_t BINDING_FIRST = 0U;
    static constexpr uint32_t BUFFER_COUNT_ONE = 1U;
    static constexpr uint32_t INSTANCE_COUNT_ONE = 1U;

private:
    // GPU Resource Handles (Owned by AssetManager)
    VkBuffer     buffer{ VK_NULL_HANDLE };
    uint32_t     indexCount{ 0U };
    VkDeviceSize indexOffset{ 0U };

    // Logic & Transformation
    glm::mat4    modelMatrix{ 1.0f };
    std::string  name{ "Mesh" };
    std::shared_ptr<Material> material{ nullptr };

public:
    /**
     * @brief Constructor: Links the mesh to its vertex/index buffer and material.
     */
    Mesh(const VkBuffer inBuffer, const uint32_t inIndexCount,
        const VkDeviceSize inIndexOffset, std::shared_ptr<Material> inMaterial);

    /**
     * @brief Destructor: Mesh does not own the VkBuffer memory; it only uses the handle.
     */
    ~Mesh();

    // Strict RAII: Prevent accidental copying of hardware-associated state
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    // --- Interface ---

    void setModelMatrix(const glm::mat4& matrix);
    void setName(const std::string& n) { name = n; }

    /**
     * @brief Records draw commands for this specific mesh.
     */
    void draw(
        VkCommandBuffer commandBuffer,
        VkDescriptorSet globalSet,
        const GE::Graphics::Pipeline* pipelineOverride = nullptr,
        const glm::mat4& worldMatrix = glm::mat4(1.0f) // Pre-calculated matrix from ECS
    ) const;

    // --- Logic Queries ---

    bool hasTransparency() const;
    const std::string& getName() const { return name; }
    Material* getMaterial() const { return material.get(); }
};