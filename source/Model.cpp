#include "../include/Model.h"

/* parasoft-begin-suppress ALL */
#include <glm/gtc/matrix_transform.hpp>
/* parasoft-end-suppress ALL */

using namespace GE::Graphics;

namespace GE::Assets {

/**
 * @brief Constructor: Initializes the model container.
 */
Model::Model() {}

/**
 * @brief Transfers ownership of a Mesh into the Model's internal registry.
 */
void Model::addMesh(std::unique_ptr<Mesh> mesh) {
    if (mesh != nullptr) {
        meshes.push_back(std::move(mesh));

        // Ensure the new mesh is synchronized with the current model transform
        meshes.back()->setModelMatrix(modelMatrix);
    }
}

// --- Transform Setters ---

void Model::setPosition(const glm::vec3& pos) {
    position = pos;
    updateMatrix();
}

void Model::setRotation(const glm::vec3& rot) {
    rotation = rot;
    updateMatrix();
}

void Model::setScale(const glm::vec3& s) {
    scale = s;
    updateMatrix();
}

/**
 * @brief Recalculates the TRS (Translate, Rotate, Scale) matrix.
 * Propagates the new global matrix to all constituent meshes.
 */
void Model::updateMatrix() {
    // Step 1: Calculate new transform matrix starting from Identity
    glm::mat4 newMatrix = glm::mat4(IDENTITY_VAL);
    newMatrix = glm::translate(newMatrix, position);

    // Step 2: Apply Euler Rotation Order (X -> Y -> Z)
    newMatrix = glm::rotate(newMatrix, glm::radians(rotation.x), AXIS_X);
    newMatrix = glm::rotate(newMatrix, glm::radians(rotation.y), AXIS_Y);
    newMatrix = glm::rotate(newMatrix, glm::radians(rotation.z), AXIS_Z);

    // Step 3: Apply Scale and cache the result
    newMatrix = glm::scale(newMatrix, scale);
    modelMatrix = newMatrix;

    // Step 4: Propagate transform to all child meshes
    for (const auto& mesh : meshes) {
        if (mesh != nullptr) {
            mesh->setModelMatrix(modelMatrix);
        }
    }
}

/**
 * @brief Orchestrates the draw call for all child meshes.
 */
void Model::draw(VkCommandBuffer cb, VkDescriptorSet globalSet, const Pipeline* const pipelineOverride) {
    for (const auto& mesh : meshes) {
        if (mesh != nullptr) {
            // Note: Mesh::draw handles specific material binding and descriptor logic
            mesh->draw(cb, globalSet, pipelineOverride);
        }
    }
}

} // namespace GE::Assets