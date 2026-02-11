#pragma once

/* parasoft-begin-suppress ALL */
#include "libs.h"
#include <memory>
#include <vector>
#include <cstdint>
/* parasoft-end-suppress ALL */

#include "Camera.h"
#include "VulkanContext.h"
#include "Common.h"
#include "TimeManager.h"

/**
 * @class InputManager
 * @brief Handles user interaction, camera switching, and simulation toggles.
 * * Acts as the bridge between raw GLFW window events and engine systems.
 * Refactored to eliminate friend classes and maintain strict encapsulation.
 */
class InputManager final {
public:
    // --- Named Constants ---
    static constexpr uint32_t CAM_IDX_FRONT = 0U;
    static constexpr uint32_t CAM_IDX_BIRD = 1U;
    static constexpr uint32_t CAM_IDX_GLOBE = 2U;
    static constexpr uint32_t CAM_TOTAL = 3U;

    static constexpr float DEFAULT_INTENSITY = 1.0f;
    static constexpr float DIVISOR_HALF = 2.0f;

    // --- Lifecycle ---

    /** @brief Constructor: Initializes managers and sets up the camera array. */
    explicit InputManager(GLFWwindow* const inWindow, TimeManager* const inTime);

    /** @brief Default destructor. */
    ~InputManager() = default;

    // RAII: Prevent duplication to ensure unique ownership of input callbacks.
    InputManager(const InputManager&) = delete;
    InputManager& operator=(const InputManager&) = delete;

    // --- Core API ---

    /** @brief Processes continuous per-frame input (e.g., WASD movement). */
    void update(const float deltaTime) const;

    /** @brief Processes discrete keyboard events triggered by OS interrupts. */
    void handleKeyEvent(const int32_t key, const int32_t scancode, const int32_t action, const int32_t mods);

    /** @brief Processes mouse movement for camera orientation. */
    void handleMouseEvent(const double xpos, const double ypos);

    /** @brief Returns the pointer to the currently active camera. */
    Camera* getActiveCamera() const {
        return cameras.at(static_cast<size_t>(activeCameraIndex)).get();
    }

    /** @brief Returns a string label representing the current camera mode. */
    const char* getActiveCameraLabel() const;

    /** @brief Resets all simulation toggles and light modifiers to defaults. */
    void resetDefaults();

    /** @brief Checks if a simulation reset was requested and clears the flag. */
    bool consumeResetRequest();

    // --- Getters ---
    bool getGouraudEnabled() const { return useGouraud; }
    bool getDustEnabled() const { return dustEnabled; }
    bool getFireEnabled() const { return fireEnabled; }
    bool getSmokeEnabled() const { return smokeEnabled; }
    bool getRainEnabled() const { return rainEnabled; }
    bool getSnowEnabled() const { return snowEnabled; }
    bool getBloomEnabled() const { return bloomEnabled; }
    bool getAutoOrbit() const { return autoOrbit; }
    float getIntensityMod() const { return intensityMod; }
    const glm::vec3& getColorMod() const { return colorMod; }

    // --- Setters ---
    void setGouraudEnabled(const bool v) { useGouraud = v; }
    void setDustEnabled(const bool v) { dustEnabled = v; }
    void setFireEnabled(const bool v) { fireEnabled = v; }
    void setSmokeEnabled(const bool v) { smokeEnabled = v; }
    void setRainEnabled(const bool v) { rainEnabled = v; }
    void setSnowEnabled(const bool v) { snowEnabled = v; }
    void setBloomEnabled(const bool v) { bloomEnabled = v; }
    void setAutoOrbit(const bool v) { autoOrbit = v; }
    void setIntensityMod(const float v) { intensityMod = v; }
    void setColorMod(const glm::vec3& v) { colorMod = v; }

private:
    // --- Hardware Dependencies ---
    GLFWwindow* window;
    TimeManager* timeManager;

    // --- Internal Camera State ---
    std::vector<std::unique_ptr<Camera>> cameras{};
    int32_t activeCameraIndex{ static_cast<int32_t>(EngineConstants::INDEX_ZERO) };

    // --- Mouse Tracking ---
    float lastX;
    float lastY;
    bool firstMouse;
    bool cursorCaptured;

    // --- Simulation & Toggle State ---
    bool useGouraud;
    bool dustEnabled;
    bool fireEnabled;
    bool smokeEnabled;
    bool rainEnabled;
    bool snowEnabled;
    bool bloomEnabled;
    bool autoOrbit;

    // Edge-detection for specific keys
    bool t_pressedLast;
    bool T_pressedLast;

    // Environmental Modifiers
    float intensityMod;
    glm::vec3 colorMod;
    bool resetRequested;

    /** @brief Resets a specific camera to its starting coordinates and orientation. */
    void resetCameraToDefault(uint32_t index) const;
};