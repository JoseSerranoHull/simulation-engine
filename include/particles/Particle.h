#pragma once

/* parasoft-begin-suppress ALL */
#include "core/libs.h"
/* parasoft-end-suppress ALL */

/**
 * @enum Particle
 * @brief Represents a single GPU-managed particle.
 */
struct Particle {
    static constexpr float DEFAULT_POS_VAL = 0.0f;
    static constexpr float DEFAULT_SIZE = 1.0f;
    static constexpr float INITIAL_VELOCITY = 0.0f;
    static constexpr float INITIAL_LIFE = 1.0f;
    static constexpr float FULL_OPACITY = 1.0f;

    // x, y, z = Position | w = Size
    alignas(16) glm::vec4 m_position{ DEFAULT_POS_VAL, DEFAULT_POS_VAL, DEFAULT_POS_VAL, DEFAULT_SIZE };

    // x, y, z = Velocity | w = Remaining Life
    alignas(16) glm::vec4 m_velocity{ INITIAL_VELOCITY, INITIAL_VELOCITY, INITIAL_VELOCITY, INITIAL_LIFE };

    // r, g, b, a = Color/Transparency
    alignas(16) glm::vec4 m_color{ 1.0f, 1.0f, 1.0f, FULL_OPACITY };
};