#pragma once
#include "core/libs.h"

namespace GE::Components {
    /**
     * @struct ParticleComponent
     * @brief ECS wrapper for a GPU-accelerated particle effect.
     * Holds an index into ParticleEmitterSystem's backend pool — no GPU types here.
     */
    struct ParticleComponent {
        static constexpr uint32_t INVALID_EMITTER = UINT32_MAX;
        uint32_t emitterIndex = INVALID_EMITTER;
        bool enabled = true;
        glm::vec3 localOffset = glm::vec3(0.0f);
    };
}