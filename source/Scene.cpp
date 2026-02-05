#include "Scene.h"

/* parasoft-begin-suppress ALL */
#include <GLFW/glfw3.h>
#include <cmath>
/* parasoft-end-suppress ALL */

/**
 * @brief Constructor: Links the scene to the Vulkan hardware context.
 */
Scene::Scene(VulkanContext* const inContext) : context(inContext) {}

/**
 * @brief Updates all procedural animations and model transforms for the frame.
 */
void Scene::update(const float deltaTime) {
    // Step 1: Procedural Animation for MagicCircle (Z-axis continuous rotation)
    const auto itCircle = models.find(KEY_MAGIC_CIRCLE);
    if (itCircle != models.end()) {
        Model* const model = itCircle->second.get();
        if (model != nullptr) {
            glm::vec3 rot = model->getRotation();
            rot.z += (CIRCLE_ROT_SPEED * deltaTime);
            model->setRotation(rot);
        }
    }

    // Step 2: Procedural Animation for DesertQueen (Sinusoidal Y-axis hovering)
    const auto itQueen = models.find(KEY_DESERT_QUEEN);
    if (itQueen != models.end()) {
        Model* const model = itQueen->second.get();
        if (model != nullptr) {
            const double time = glfwGetTime();
            glm::vec3 pos = model->getPosition();

            const double angle = time * static_cast<double>(QUEEN_HOVER_FREQ);
            const float hoverOffset = static_cast<float>(std::sin(angle)) * QUEEN_HOVER_AMP;

            pos.y = QUEEN_BASE_HEIGHT + hoverOffset;
            model->setPosition(pos);
        }
    }
}

/**
 * @brief Records draw calls for every model registered in the scene registry.
 */
void Scene::draw(VkCommandBuffer cb, VkDescriptorSet globalSet, const Pipeline* const pipelineOverride) {
    // Iterate through the model map and trigger individual draw calls
    for (const auto& [name, model] : models) {
        if (model != nullptr) {
            model->draw(cb, globalSet, pipelineOverride);
        }
    }
}

/**
 * @brief Retrieves a raw pointer to a model by its string key.
 */
Model* Scene::getModel(const std::string& key) {
    const auto it = models.find(key);
    if (it != models.end()) {
        return it->second.get();
    }
    return nullptr;
}