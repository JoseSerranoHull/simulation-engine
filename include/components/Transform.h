#pragma once

/* parasoft-begin-suppress ALL */
#include <cstdint>
#include <glm/fwd.hpp>
#include <glm/vec3.hpp>
/* parasoft-end-suppress ALL */

namespace GE::Components {
    struct Transform {
        enum class TransformState : uint8_t {
            Clean,  // Up to date
            Dirty   // Needs recalculation
        };

        // --- Local Transform (stored; authored by scene loaders, Animation, Inspector) ---
        glm::vec3 m_localPosition { 0.0f };
        glm::vec3 m_localRotation { 0.0f };  // Euler degrees YXZ
        glm::vec3 m_localScale { 1.0f };

        // --- World Transform (computed by TransformSystem each frame — read-only for all other systems) ---
        glm::vec3 m_worldPosition { 0.0f };
        glm::vec3 m_worldRotation { 0.0f };// Euler degrees, extracted from worldMatrix (display only)
        glm::vec3 m_worldScale { 1.0f };

        // --- Matrices ---
        glm::mat4 m_localMatrix { 1.0f };
        glm::mat4 m_worldMatrix { 1.0f };

        // --- Hierarchy ---
        uint32_t m_parentEntityID = UINT32_MAX; // UINT32_MAX = root entity

        TransformState m_state = TransformState::Dirty;
    };
} // namespace GE::Components
