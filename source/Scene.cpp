#include "Scene.h"
#include "ServiceLocator.h"
#include "EntityManager.h"

/* parasoft-begin-suppress ALL */
#include <GLFW/glfw3.h>
#include <cmath>
/* parasoft-end-suppress ALL */

namespace GE::Scene {
    Scene::Scene() {}

    void Scene::update(const float deltaTime) {
        // 1. Get the heart of the engine
        auto* em = ServiceLocator::GetEntityManager();
        if (!em) return;

        // Step 1: Procedural Animation for MagicCircle (Rotation)
        if (hasEntity(KEY_MAGIC_CIRCLE)) {
            GE::ECS::EntityID id = entities[KEY_MAGIC_CIRCLE];
            // Access the Transform component directly from the ECS data arrays
            auto* transform = em->GetTIComponent<GE::Scene::Components::Transform>(id);

            if (transform) {
                transform->m_rotation.z += (CIRCLE_ROT_SPEED * deltaTime);
                // Mark as dirty so the TransformSystem knows to update the matrices
                transform->m_state = GE::Scene::Components::Transform::TransformState::Dirty;
            }
        }

        // Step 2: Procedural Animation for DesertQueen (Sinusoidal Hovering)
        if (hasEntity(KEY_DESERT_QUEEN)) {
            GE::ECS::EntityID id = entities[KEY_DESERT_QUEEN];
            auto* transform = em->GetTIComponent<GE::Scene::Components::Transform>(id);

            if (transform) {
                const double time = glfwGetTime();
                const double angle = time * static_cast<double>(QUEEN_HOVER_FREQ);
                const float hoverOffset = static_cast<float>(std::sin(angle)) * QUEEN_HOVER_AMP;

                transform->m_position.y = QUEEN_BASE_HEIGHT + hoverOffset;
                transform->m_state = GE::Scene::Components::Transform::TransformState::Dirty;
            }
        }
    }

    GE::ECS::EntityID Scene::getEntityID(const std::string& key) const {
        const auto it = entities.find(key);
        if (it != entities.end()) {
            return it->second;
        }
        return GE::ECS::INVALID_ENTITY_ID;
    }
}