#include "lighting/PointLight.h"

/**
 * @brief Non-inline destructor definition to satisfy OOP.25.
 * Ensures the virtual table for PointLight is established in this translation unit.
 */
PointLight::~PointLight() = default;

/**
 * @brief Updates the light's position along a circular orbital path.
 */
void PointLight::orbit(const float time, const float radius, const float speed, const float height) {
    const double angle = static_cast<double>(time) * static_cast<double>(speed);

    // Step 1: Calculate new position using trigonometric orbital math
    position.x = static_cast<float>(std::sin(angle)) * radius;
    position.y = height;
    position.z = static_cast<float>(std::cos(angle)) * radius;
}

/**
 * @brief Generates the View-Projection matrix for shadow mapping.
 */
glm::mat4 PointLight::getLightSpaceMatrix() const {
    const glm::vec3 center{ ZERO_VAL, ZERO_VAL, ZERO_VAL };
    const glm::vec3 upVector{ ZERO_VAL, ONE_VAL, ZERO_VAL };

    // Step 1: Generate the view matrix looking from the light toward the world origin
    const glm::mat4 lightView = glm::lookAt(position, center, upVector);

    // Step 2: Generate an orthographic projection for the directional-style "sun" light
    glm::mat4 lightProj = glm::ortho(-ORTHO_RANGE, ORTHO_RANGE, -ORTHO_RANGE, ORTHO_RANGE, NEAR_PLANE, FAR_PLANE);

    // Step 3: Apply the Vulkan Y-axis flip factor to the projection matrix
    lightProj[1][1] *= Y_FLIP_FACTOR;

    // Step 4: Return the combined Light-Space matrix (Projection * View)
    return (lightProj * lightView);
}