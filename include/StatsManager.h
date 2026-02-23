#pragma once

/* parasoft-begin-suppress ALL */
#include <vector>
#include <numeric>
#include <algorithm>
/* parasoft-end-suppress ALL */

/**
 * @class PerformanceTracker
 * @brief Encapsulates performance metrics and history for UI visualization.
 * Provides a circular buffer for tracking FPS history over a fixed window of frames.
 */
class PerformanceTracker final {
public:
    /** @brief Number of frames stored for historical plotting. */
    static constexpr uint32_t HISTORY_SIZE = 100U;

    /**
     * @brief Constructor: Initializes the history buffer with zeros.
     */
    PerformanceTracker() {
        fpsHistory.assign(static_cast<size_t>(HISTORY_SIZE), 0.0f);
    }

    ~PerformanceTracker() = default;

    // RAII: PerformanceTracker is a simple data container, but we prevent copies to ensure 
    // a single source of truth for engine performance metrics.
    PerformanceTracker(const PerformanceTracker&) = delete;
    PerformanceTracker& operator=(const PerformanceTracker&) = delete;

    /**
     * @brief Updates the history with the latest frame metrics.
     * @param deltaTime Elapsed time of the last frame in seconds.
     */
    void update(const float deltaTime) {
        // Prevent division by zero and extreme outliers
        if (deltaTime > 0.0f) {
            const float currentFps = 1.0f / deltaTime;

            fpsHistory[static_cast<size_t>(offset)] = currentFps;

            // Circular buffer logic using unsigned arithmetic
            offset = (offset + 1U) % HISTORY_SIZE;
        }
    }

    // --- Accessors for ImGui Plotting ---

    /** @brief Returns a pointer to the raw history array. */
    const float* getHistoryData() const { return fpsHistory.data(); }

    /** @brief Returns the current write head in the circular buffer. */
    uint32_t getOffset() const { return offset; }

    /** @brief Returns the total capacity of the buffer. */
    uint32_t getCount() const { return HISTORY_SIZE; }

    /**
     * @brief Computes the average FPS across the stored history.
     */
    float getAverageFPS() const {
        const float sum = std::accumulate(fpsHistory.begin(), fpsHistory.end(), 0.0f);
        return sum / static_cast<float>(HISTORY_SIZE);
    }

private:
    std::vector<float> fpsHistory{};
    uint32_t offset{ 0U };
};