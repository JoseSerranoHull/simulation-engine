#pragma once

/* parasoft-begin-suppress ALL */
#include "core/libs.h"
#include <glm/glm.hpp>
#include <string>
/* parasoft-end-suppress ALL */

#include "core/Common.h"

/** * @enum WeatherState
 * @brief Internal operational states for the procedural weather simulation.
 */
enum class WeatherState { SUMMER, RAIN, SNOW };

/** * @enum WeatherMode
 * @brief User-interface facing modes for manual or automatic weather control.
 */
enum class WeatherMode : int { AUTO = 0, SUMMER = 1, RAIN = 2, SNOW = 3 };

/**
 * @class ClimateService
 * @brief Manages scene-wide environmental effects, including weather cycles and orbital lighting.
 * * Orchestrates the transition between different weather states, calculates time-of-day
 * sun positions, and maintains interpolation targets for various visual components.
 */
class ClimateService final {
public:
    // --- Core Lifecycle & Updates ---

    /**
     * @brief Updates the internal weather lifecycle and atmospheric calculations.
     */
    void update(const float dt, const float totalTime, const bool autoOrbit,
        const float orbitRadius, const float orbitSpeed, const float configIntensity);

    /**
     * @brief Resets the manager to factory defaults (Clear Summer, Auto-cycle).
     */
    void reset() {
        // Step 1: Reset high-level control states
        activeMode = WeatherMode::AUTO;
        currentState = WeatherState::SUMMER;
        stateTimer = 0.0f;
        isLocked = false;
        autoCycle = true;

        // Step 2: Reset visual influence and interpolation values
        currentStormInfluence = 0.0f;
        currentCactusGrowth = 1.0f;
        currentWaterScale = glm::vec3(1.0f);
        currentWaterOffset = 0.0f;
        weatherTint = glm::vec3(1.0f);

        // Step 3: Flag a state transition to notify dependent systems
        transitionTriggered = true;
    }

    // --- State Queries for Orchestration ---

    bool isRainEnabled()  const { return currentState == WeatherState::RAIN; }
    bool isSnowEnabled()  const { return currentState == WeatherState::SNOW; }
    bool isDustEnabled()  const { return currentState == WeatherState::SUMMER; }
    bool isFireEnabled()  const { return currentState == WeatherState::SUMMER || currentState == WeatherState::SNOW; }
    bool isSmokeEnabled() const { return true; } // Smoke is always active per design requirements

    // --- Interpolation Accessors ---

    float getCactusScale() const { return currentCactusGrowth; }

    /** @brief Returns water scale by const reference to satisfy OPT.14. */
    const glm::vec3& getWaterScale() const { return currentWaterScale; }

    float getWaterOffset() const { return currentWaterOffset; }

    /** @brief Returns tint by const reference to satisfy OPT.14. */
    const glm::vec3& getTint() const { return weatherTint; }

    /** @brief Returns sun position by const reference to satisfy OPT.14. */
    const glm::vec3& getSunPosition() const { return currentSunPos; }

    /** @brief Returns ambient color by const reference to satisfy OPT.14. */
    const glm::vec3& getAmbientColor() const { return currentAmbientColor; }

    float getSunIntensity() const { return currentSunIntensity; }

    // --- Configuration Interface ---

    void setWeather(const WeatherState state) {
        currentState = state;
        stateTimer = 0.0f;
    }

    /**
     * @brief Sets the global weather mode (Auto-cycle vs. Manual Overrides).
     */
    void setWeatherMode(const WeatherMode mode) {
        activeMode = mode;

        if (mode == WeatherMode::AUTO) {
            // Step 1: Enable autonomous cycling
            isLocked = false;
            autoCycle = true;
        }
        else {
            // Step 2: Apply manual override and lock the cycle
            isLocked = true;
            autoCycle = false;
            // Map WeatherMode enum to WeatherState enum
            currentState = static_cast<WeatherState>(static_cast<int>(mode) - 1);
        }

        // Step 3: Signal that the environment state has changed
        transitionTriggered = true;
    }

    void toggleAutoCycle() { autoCycle = !autoCycle; }
    void setLocked(const bool locked) { isLocked = locked; autoCycle = !locked; }

    bool getAutoCycle()  const { return autoCycle; }
    bool getLocked()     const { return isLocked; }

    WeatherState getWeatherState() const { return currentState; }
    WeatherMode  getWeatherMode()  const { return activeMode; }

    const char* getSeasonLabel()   const;

    /**
     * @brief Checks if a state transition occurred and resets the trigger.
     * @return True if a transition was triggered since the last check.
     */
    bool checkTransition() {
        const bool triggered = transitionTriggered;
        transitionTriggered = false;
        return triggered;
    }

private:
    // --- Control State ---
    WeatherMode  activeMode{ WeatherMode::AUTO };
    WeatherState currentState{ WeatherState::SUMMER };

    bool  isLocked{ false };
    float stateTimer{ 0.0f };
    bool  autoCycle{ true };
    bool  transitionTriggered{ false };
    float currentStormInfluence{ 0.0f };

    // --- Interpolation Targets ---
    float     currentCactusGrowth{ 1.0f };
    glm::vec3 currentWaterScale{ 1.0f };
    float     currentWaterOffset{ 0.0f };
    glm::vec3 weatherTint{ 1.0f };

    // --- Atmospheric State ---
    glm::vec3 currentSunPos{ 0.0f };
    glm::vec3 currentAmbientColor{ 1.0f };
    float     currentSunIntensity{ 1.0f };
};