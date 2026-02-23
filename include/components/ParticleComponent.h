#pragma once
#include "particles/ParticleSystem.h"
#include <memory>

namespace GE::Components {
    /**
     * @struct ParticleComponent
     * @brief ECS wrapper for a GPU-accelerated particle effect.
     */
    struct ParticleComponent {
        // Changing from unique_ptr to shared_ptr makes the component copyable
        std::shared_ptr<ParticleSystem> system;
        bool enabled = true;
        glm::vec3 localOffset = glm::vec3(0.0f);
    };
}