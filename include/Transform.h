#pragma once

/* parasoft-begin-suppress ALL */
#include <cstdint>
#include <glm/fwd.hpp>
#include <glm/vec3.hpp>
/* parasoft-end-suppress ALL */

namespace GE::Scene::Components {
    struct Transform {
        enum class TransformState : uint8_t {
            Clean,   // Up to date
            Dirty    // Needs recalculation
        };

        // --- Local Transform (Data-Driven from .ini) ---
        glm::vec3 m_position{ 0.0f };
        glm::vec3 m_rotation{ 0.0f };
        glm::vec3 m_scale{ 1.0f };

        // --- Matrices ---
        glm::mat4 m_localMatrix{ 1.0f };
        glm::mat4 m_worldMatrix{ 1.0f };

        // --- Hierarchy (Agnostic GameObject Logic) ---
        uint32_t m_parentEntityID = UINT32_MAX;
        std::vector<uint32_t> m_children; // NEW: Top-down traversal for performance

        TransformState m_state = TransformState::Dirty;
    };
}