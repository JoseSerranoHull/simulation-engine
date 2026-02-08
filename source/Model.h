#pragma once

/* parasoft-begin-suppress ALL */
#include <vector>
#include <memory>
#include <map>
#include <string>
#include <glm/glm.hpp>
/* parasoft-end-suppress ALL */

#include "Mesh.h"

/**
 * @class Model
 * @brief Represents a complex 3D object consisting of multiple meshes and a global transform.
 * Manages the local-to-world transformation matrix and orchestrates the drawing of sub-meshes.
 */
class Model final {
public:
    // --- Transformation Constants ---
    static constexpr float IDENTITY_VAL = 1.0f;
    static constexpr glm::vec3 AXIS_X{ 1.0f, 0.0f, 0.0f };
    static constexpr glm::vec3 AXIS_Y{ 0.0f, 1.0f, 0.0f };
    static constexpr glm::vec3 AXIS_Z{ 0.0f, 0.0f, 1.0f };

private:
    /** * @brief Collection of geometry pieces forming this model.
     * Meshes are owned by the Model via unique_ptr to ensure strict lifecycle management.
     */
    std::vector<std::unique_ptr<Mesh>> meshes{};

    // --- Transform State ---
    glm::vec3 position{ 0.0f };
    glm::vec3 rotation{ 0.0f }; // Euler angles (Degrees)
    glm::vec3 scale{ IDENTITY_VAL };
    glm::mat4 modelMatrix{ IDENTITY_VAL };

    // --- Logic State ---
    bool canProduceShadows{ true };

    /** @brief Recalculates the internal 4x4 model matrix based on pos/rot/scale. */
    void updateMatrix();

public:
    /**
     * @brief Constructor: Initializes a model within the given Vulkan context.
     */
    explicit Model();

    /** @brief Destructor: Implicitly cleans up meshes via unique_ptr. */
    ~Model() = default;

    // RAII: Prevent duplication of hardware-mapped model state
    Model(const Model&) = delete;
    Model& operator=(const Model&) = delete;

    /** @brief Appends a mesh to the model's internal registry. */
    void addMesh(std::unique_ptr<Mesh> mesh);

    // --- High-Level Transform Interface ---

    void setPosition(const glm::vec3& pos);
    void setRotation(const glm::vec3& rot);
    void setScale(const glm::vec3& s);
    void setShadowCasting(const bool enabled) { canProduceShadows = enabled; }

    /**
     * @brief Iterates through all child meshes and records their draw commands.
     */
    void draw(VkCommandBuffer cb, VkDescriptorSet globalSet, const Pipeline* const pipelineOverride = nullptr);

    /** @brief Returns shadow state (bool is small enough for pass-by-value) */
    bool castsShadows() const { return canProduceShadows; }

    // --- Accessors ---

    /** @brief Returns the position/rotation/scale by const reference */
    const glm::vec3& getPosition() const { return position; }
    const glm::vec3& getRotation() const { return rotation; }
    const glm::vec3& getScale() const { return scale; }

    /** @brief Provides read-only access to meshes for scene categorization logic. */
    const std::vector<std::unique_ptr<Mesh>>& getMeshes() const { return meshes; }
};