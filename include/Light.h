#pragma once

/* parasoft-begin-suppress ALL */
#include "../include/ISystem.h"
#include "../include/libs.h"
/* parasoft-end-suppress ALL */

/**
 * @class Light
 * @brief Abstract base class representing a light source within the 3D scene.
 * Provides core properties for color and intensity, serving as the foundation
 * for Point, Directional, and Spot lights.
 */
class Light : public ISystem {
public:
    static constexpr float DEFAULT_INTENSITY = 1.0f;

private:
    glm::vec3 color{ 1.0f, 1.0f, 1.0f };
    float intensity{ DEFAULT_INTENSITY };

public:
    /**
     * @brief Constructor for base light properties.
     */
    Light(const glm::vec3& inColor, const float inIntensity)
        : color(inColor), intensity(inIntensity)
    {
    }

    /** @brief Virtual destructor ensuring safe cleanup for derived classes. */
    virtual ~Light();

    // Standard RAII: Data-classes allow default copying/assignment 
    // to facilitate easy passing between simulation and UBO updates.
    Light(const Light&) = default;
    Light& operator=(const Light&) = default;

    // --- Accessors ---

    /** @brief Returns the base RGB color of the light. */
    const glm::vec3& getColor() const { return color; }

    /** @brief Returns the raw intensity scalar. */
    float getIntensity() const { return intensity; }

    /** * @brief Returns the calculated radiance value (Color * Intensity).
     * This is the value typically consumed by the GLSL fragment shaders.
     */
    glm::vec3 getLightValue() const {
        return (color * intensity);
    }

    // --- Setters ---

    /** @brief Updates the light color using reference-to-const for performance. */
    void setColor(const glm::vec3& c) {
        color = c;
    }

    /** @brief Updates the intensity scalar. */
    void setIntensity(const float i) {
        intensity = i;
    }

	/** @brief Standard ISystem update override (Unused for this base class, as it requires command buffer context).*/
    void update(float deltaTime) override { /* Interface stub */ }
};