#include "../include/ClimateManager.h"

/* parasoft-begin-suppress ALL */
#include <cmath>
#include <algorithm>
/* parasoft-end-suppress ALL */

/**
 * @brief Updates the procedural weather simulation, orbital sun logic, and atmospheric tints.
 */
void ClimateManager::update(const float dt, const float totalTime, const bool autoOrbit,
    const float orbitRadius, const float orbitSpeed, const float configIntensity)
{
    // Step 1: Sanitize delta time to prevent simulation "explosions" during frame drops
    static constexpr float MAX_DT = 0.033f;
    const float clampedDt = glm::min(dt, MAX_DT);

    // Step 2: Season Lifecycle - Transition weather states automatically if not manually locked
    if (autoCycle && !isLocked) {
        stateTimer += clampedDt;
        static constexpr float SEASON_DURATION = 15.0f;

        if (stateTimer >= SEASON_DURATION) {
            stateTimer = 0.0f;
            // Cycle through SUMMER -> RAIN -> SNOW
            currentState = static_cast<WeatherState>((static_cast<int>(currentState) + 1) % 3);
            transitionTriggered = true;
        }
    }

    // Step 3: Define NATURAL Weather Targets (Interpolation goals for the current state)
    float targetGrowth = 1.0f;
    glm::vec3 targetWater = glm::vec3(1.0f);
    float targetWaterOffset = 0.0f;
    float targetStormStrength = 0.0f;
    glm::vec3 tintTarget = glm::vec3(1.0f);

    if (currentState == WeatherState::RAIN) {
        targetGrowth = 1.3f;
        targetWater = glm::vec3(1.4f, 1.0f, 1.4f);
        targetWaterOffset = 0.01f;
        targetStormStrength = 0.7f;                // 70% Storm influence over ambient colors
        tintTarget = glm::vec3(0.8f, 0.82f, 0.85f); // Subtle Grey-Blue
    }
    else if (currentState == WeatherState::SNOW) {
        targetStormStrength = 0.5f;                // 50% Storm influence
        tintTarget = glm::vec3(0.9f, 0.95f, 1.1f);  // Subtle Icy Blue
    }

    // Step 4: Smooth Transitions - Prevent jarring visual pops when weather changes
    static constexpr float LERP_SPEED = 1.5f;
    const float lerpFactor = clampedDt * LERP_SPEED;

    currentCactusGrowth = glm::mix(currentCactusGrowth, targetGrowth, lerpFactor);
    currentWaterScale = glm::mix(currentWaterScale, targetWater, lerpFactor);
    currentWaterOffset = glm::mix(currentWaterOffset, targetWaterOffset, lerpFactor);
    currentStormInfluence = glm::mix(currentStormInfluence, targetStormStrength, lerpFactor);
    weatherTint = glm::mix(weatherTint, tintTarget, lerpFactor);

    // Step 5: Sun Position Calculation - Orbital mechanics
    if (autoOrbit) {
        const double angle = static_cast<double>(totalTime * orbitSpeed);
        currentSunPos.x = GE::EngineConstants::SUN_X_OFFSET;
        currentSunPos.y = static_cast<float>(std::sin(angle)) * orbitRadius;
        currentSunPos.z = static_cast<float>(std::cos(angle)) * orbitRadius;
    }

    // Step 6: NATURAL ATMOSPHERIC MIXING
    // Calculate the base ambient color based on sun height (Day/Sunset/Night)
    const float normalizedHeight = currentSunPos.y / orbitRadius;
    const glm::vec3 baseAmbient = (normalizedHeight > 0.0f)
        ? glm::mix(GE::EngineConstants::COLOR_SUNSET, GE::EngineConstants::COLOR_DAY, normalizedHeight)
        : glm::mix(GE::EngineConstants::COLOR_SUNSET, GE::EngineConstants::COLOR_NIGHT, std::abs(normalizedHeight));

    // Apply the weather-specific tint (e.g., grey for rain) to the natural sunlight
    const glm::vec3 tintedSun = baseAmbient * weatherTint;

    // Mix the pure sun color with the tinted sun color based on the current weather influence
    currentAmbientColor = glm::mix(baseAmbient, tintedSun, currentStormInfluence);

    // Step 7: Global Illumination Dimming
    float weatherDimmer = 1.0f;
    if (currentState == WeatherState::RAIN) {
        weatherDimmer = 0.40f;
    }
    else if (currentState == WeatherState::SNOW) {
        weatherDimmer = 0.80f;
    }

    // Ensure actual night is not pitch black so the scene remains legible
    static constexpr float NIGHT_MINIMUM = 0.02f;
    currentSunIntensity = (normalizedHeight > 0.0f)
        ? (configIntensity * weatherDimmer)
        : NIGHT_MINIMUM;
}

/** * @brief Returns a human-readable label for the current simulated season.
 */
const char* ClimateManager::getSeasonLabel() const {
    switch (currentState) {
    case WeatherState::RAIN:
        return "Rainy Season";
    case WeatherState::SNOW:
        return "Winter Storm";
    case WeatherState::SUMMER:
    default:
        return "Clear Summer";
    }
}