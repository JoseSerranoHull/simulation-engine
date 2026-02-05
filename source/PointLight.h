#pragma once

/* parasoft-begin-suppress ALL */
#include "Light.h"
#include <cmath>
/* parasoft-end-suppress ALL */

/**
 * @class PointLight
 * @brief Represents a dynamic point light source with orbital animation.
 * Refactored to satisfy CODSTA-MCPP.52, OOP.25, and OPT.14.
 */
class PointLight final : public Light {
public:
    // --- Projection Constants ---
    static constexpr float ORTHO_RANGE = 5.0f;
    static constexpr float NEAR_PLANE = 0.1f;
    static constexpr float FAR_PLANE = 20.0f;
    static constexpr float Y_FLIP_FACTOR = -1.0f;
    static constexpr float ZERO_VAL = 0.0f;
    static constexpr float ONE_VAL = 1.0f;

private:
    glm::vec3 position{ ZERO_VAL, ZERO_VAL, ZERO_VAL };

public:
    PointLight(const glm::vec3& inPosition, const glm::vec3& inColor, const float inIntensity)
        : Light(inColor, inIntensity), position(inPosition)
    {
    }

    /** * @brief Destructor marked 'final' to satisfy CODSTA-MCPP.52.
     * Implementation moved to .cpp to satisfy OOP.25.
     */
    ~PointLight() override final;

    PointLight(const PointLight&) = default;
    PointLight& operator=(const PointLight&) = default;

    // --- Accessors ---

    void setPosition(const glm::vec3& pos) { position = pos; }

    /** @brief Returns position by const reference to satisfy OPT.14. */
    const glm::vec3& getPosition() const { return position; }

    void orbit(const float time, const float radius, const float speed, const float height);
    glm::mat4 getLightSpaceMatrix() const;
};