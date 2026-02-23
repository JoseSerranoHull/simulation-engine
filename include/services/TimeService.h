#pragma once

/* parasoft-begin-suppress ALL */
#include <chrono>
#include <algorithm>
/* parasoft-end-suppress ALL */

/**
 * @class TimeService
 * @brief Handles frame-timing, global clock accumulation, and simulation scaling.
 * Provides the Delta Time required for frame-independent movement and animations.
 */
class TimeService final {
public:
    /** @brief Delta increment constant for simulation speed adjustments. */
    static constexpr float SCALE_STEP = 0.2f;
    static constexpr float SCALE_MIN = 0.0f;
    static constexpr float SCALE_BASE = 1.0f;

    /**
     * @brief Constructor: Captures the initial timestamp to prevent massive first-frame jumps.
     */
    TimeService() = default;

    /** @brief Destructor: Defaulted as no external heap resources are managed. */
    ~TimeService() = default;

    // RAII: Time is a global singleton-like state; prevent copies to maintain 
    // a consistent clock across all engine systems.
    TimeService(const TimeService&) = delete;
    TimeService& operator=(const TimeService&) = delete;

    /**
     * @brief Calculates elapsed time since the last update and advances the global clock.
     * Uses high_resolution_clock to provide sub-millisecond precision.
     */
    void update() {
        const auto currentTime = std::chrono::high_resolution_clock::now();

        // Calculate raw frame duration in seconds
        const float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(
            currentTime - lastTimestamp).count();

        lastTimestamp = currentTime;

        // Apply time scaling to support slow-motion or fast-forward effects
        deltaTime = frameTime * timeScale;

        // Accumulate total simulation time
        totalTime += deltaTime;
    }

    // --- Mandatory Simulation Controls ---

    /** @brief Increases simulation speed (Default 't'/'T' control). */
    void speedUp() { timeScale += SCALE_STEP; }

    /** @brief Decreases simulation speed, clamped to zero. */
    void slowDown() { timeScale = std::max(SCALE_MIN, timeScale - SCALE_STEP); }

    /** @brief Restores simulation to real-time (1.0x). */
    void resetScale() { timeScale = SCALE_BASE; }

    // --- Accessors ---

    /** @brief Returns scaled delta time for the current frame. */
    float getDelta() const { return deltaTime; }

    /** @brief Returns total accumulated simulation time since engine start. */
    float getTotal() const { return totalTime; }

    /** @brief Returns the current simulation speed multiplier. */
    float getScale() const { return timeScale; }

private:
    std::chrono::high_resolution_clock::time_point lastTimestamp{
        std::chrono::high_resolution_clock::now()
    };

    float deltaTime{ 0.0f };
    float totalTime{ 0.0f };
    float timeScale{ SCALE_BASE };
};