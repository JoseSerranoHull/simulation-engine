#pragma once

/* parasoft-begin-suppress ALL */
#include <memory>
#include <glm/glm.hpp>
/* parasoft-end-suppress ALL */

#include "Transform.h"
#include "Tag.h"

// Forward declarations for your existing classes
namespace GE::Assets { class Mesh; class Material; }

namespace GE::Components {
    /**
     * @struct SubMesh
     * @brief A single drawable part of an entity, pairing a mesh with its material.
     */
    struct SubMesh {
        GE::Assets::Mesh* mesh = nullptr;
        GE::Assets::Material* material = nullptr;
    };

    /**
     * @struct MeshRenderer
     * @brief ECS Component managing all renderable parts of an entity.
     */
    struct MeshRenderer {
        std::vector<SubMesh> subMeshes; // Support for complex multi-material models
    };

    /**
     * @struct LightComponent
     * @brief ECS Component for scene illumination.
     */
    struct LightComponent {
        glm::vec3 color = glm::vec3(1.0f);
        float intensity = 1.0f;
        bool isStatic = false;
    };

    /**
     * @struct PhysicsComponent
     * @brief ECS Component for the upcoming Simulation module.
     */
    struct PhysicsComponent {
        glm::vec3 velocity{ 0.0f };
        float mass = 1.0f;
        // Collision shape type can be added here later
    };
}