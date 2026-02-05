#include "InputManager.h"

/**
 * @brief Constructor: Initializes hardware-dependent mouse offsets and camera perspectives.
 */
InputManager::InputManager(GLFWwindow* const inWindow, VulkanContext* const inContext, TimeManager* const inTime)
    : window(inWindow),
    context(inContext),
    timeManager(inTime),
    activeCameraIndex(static_cast<int32_t>(EngineConstants::INDEX_ZERO)),
    lastX(0.0f),
    lastY(0.0f),
    firstMouse(true),
    cursorCaptured(false),
    useGouraud(false),
    dustEnabled(true),
    fireEnabled(true),
    smokeEnabled(true),
    rainEnabled(false),
    snowEnabled(false),
    bloomEnabled(false),
    autoOrbit(true),
    t_pressedLast(false),
    T_pressedLast(false),
    intensityMod(DEFAULT_INTENSITY),
    colorMod(glm::vec3(1.0f, 1.0f, 1.0f)),
    resetRequested(false)
{
    // Step 1: Initial Mouse State Calculation
    int32_t width{ 0 };
    int32_t height{ 0 };
    glfwGetWindowSize(window, &width, &height);

    // Center the initial cursor coordinates based on window dimensions
    lastX = static_cast<float>(width) / DIVISOR_HALF;
    lastY = static_cast<float>(height) / DIVISOR_HALF;

    // Step 2: Initial UI state - Start with cursor free for UI interaction
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    // Step 3: Camera Array Initialization
    cameras.reserve(static_cast<size_t>(CAM_TOTAL));

    // Cam 1: Standard Frontal View
    cameras.push_back(std::make_unique<Camera>(glm::vec3(0.0f, 2.0f, 8.0f), glm::vec3(0.0f, 0.0f, -1.0f)));

    // Cam 2: Isometric Side View
    cameras.push_back(std::make_unique<Camera>(glm::vec3(8.0f, 5.0f, 8.0f), glm::vec3(-1.0f, -0.5f, -1.0f)));

    // Cam 3: Interior Macro View
    cameras.push_back(std::make_unique<Camera>(glm::vec3(0.0f, 0.5f, 1.5f), glm::vec3(0.0f, 0.0f, -1.0f)));
}

/**
 * @brief Discrete event handler: Processes state-changing key presses.
 */
void InputManager::handleKeyEvent(const int32_t key, const int32_t scancode, const int32_t action, const int32_t mods) {
    static_cast<void>(scancode);

    // Only process discrete presses
    if (action != GLFW_PRESS) {
        return;
    }

    switch (key) {
    case GLFW_KEY_R:
        resetRequested = true;
        break;

    case GLFW_KEY_ESCAPE:
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        break;

    case GLFW_KEY_C:
        // Step 1: Toggle cursor capture mode
        cursorCaptured = !cursorCaptured;
        glfwSetInputMode(window, GLFW_CURSOR, cursorCaptured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
        firstMouse = true;
        break;

    case GLFW_KEY_L:
        useGouraud = !useGouraud;
        break;

    case GLFW_KEY_F1:
        resetCameraToDefault(CAM_IDX_FRONT);
        activeCameraIndex = static_cast<int32_t>(CAM_IDX_FRONT);
        break;

    case GLFW_KEY_F2:
        resetCameraToDefault(CAM_IDX_BIRD);
        activeCameraIndex = static_cast<int32_t>(CAM_IDX_BIRD);
        break;

    case GLFW_KEY_F3:
        resetCameraToDefault(CAM_IDX_GLOBE);
        activeCameraIndex = static_cast<int32_t>(CAM_IDX_GLOBE);
        break;

    case GLFW_KEY_T:
        // Step 2: Handle time-scaling logic with Shift-modifier detection
        if (timeManager != nullptr) {
            const bool isShift = (mods & GLFW_MOD_SHIFT) != 0;
            if (isShift) {
                if (T_pressedLast) {
                    timeManager->resetScale();
                    T_pressedLast = false;
                }
                else {
                    timeManager->resetScale();
                    timeManager->speedUp();
                    T_pressedLast = true;
                    t_pressedLast = false;
                }
            }
            else {
                if (t_pressedLast) {
                    timeManager->resetScale();
                    t_pressedLast = false;
                }
                else {
                    timeManager->resetScale();
                    timeManager->slowDown();
                    t_pressedLast = true;
                    T_pressedLast = false;
                }
            }
        }
        break;

    default:
        break;
    }
}

/**
 * @brief Continuous update: Processes WASD movement if the cursor is captured.
 */
void InputManager::update(const float deltaTime) const {
    if (!cursorCaptured) {
        return;
    }

    Camera* const cam = getActiveCamera();
    if (cam == nullptr) {
        return;
    }

    // Step 1: Process Movement with standard FPS controls
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) { cam->processKeyboard(CameraMovement::FORWARD, deltaTime); }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) { cam->processKeyboard(CameraMovement::BACKWARD, deltaTime); }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) { cam->processKeyboard(CameraMovement::LEFT, deltaTime); }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) { cam->processKeyboard(CameraMovement::RIGHT, deltaTime); }

    // Step 2: Vertical Panning
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) { cam->processKeyboard(CameraMovement::UP, deltaTime); }
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) { cam->processKeyboard(CameraMovement::DOWN, deltaTime); }
}

/**
 * @brief Mouse displacement handler: Converts raw delta values into Euler angles.
 */
void InputManager::handleMouseEvent(const double xpos, const double ypos) {
    if (!cursorCaptured) {
        return;
    }

    // Step 1: Handle first-frame mouse jump prevention
    if (firstMouse) {
        lastX = static_cast<float>(xpos);
        lastY = static_cast<float>(ypos);
        firstMouse = false;
    }

    // Step 2: Calculate delta displacement
    const float xoffset = static_cast<float>(xpos) - lastX;
    const float yoffset = lastY - static_cast<float>(ypos);

    lastX = static_cast<float>(xpos);
    lastY = static_cast<float>(ypos);

    // Step 3: Pass deltas to active camera
    Camera* const cam = getActiveCamera();
    if (cam != nullptr) {
        cam->processMouseMovement(xoffset, yoffset);
    }
}

/**
 * @brief Returns a diagnostic label for the active camera state.
 */
const char* InputManager::getActiveCameraLabel() const {
    switch (static_cast<uint32_t>(activeCameraIndex)) {
    case CAM_IDX_FRONT: return "Frontal Perspective";
    case CAM_IDX_BIRD:  return "High-Angle Overview";
    case CAM_IDX_GLOBE: return "Macro Interior";
    default:            return "Unknown System";
    }
}

/**
 * @brief Restores UI toggles and lighting modifiers to their original engine state.
 * This is triggered by the 'R' key and allows the ClimateManager to reclaim
 * control over the simulation settings.
 */
 /**
  * @brief Restores UI toggles and lighting modifiers to their original engine state.
  */
void InputManager::resetDefaults() {
    useGouraud = false;
    bloomEnabled = false;
    autoOrbit = true;
    intensityMod = DEFAULT_INTENSITY;
    colorMod = glm::vec3(1.0f, 1.0f, 1.0f);

    // Note: We don't hardcode particle booleans here because 
    // Experience::syncWeatherToggles() will overwrite them immediately after.
    resetRequested = false;
}

/**
 * @brief Thread-safe style consumption of the reset flag.
 * Returns true only once per request to prevent infinite reset loops.
 */
bool InputManager::consumeResetRequest() {
    bool requestActive = false;

    if (resetRequested) {
        resetRequested = false;
        requestActive = true;
    }

    return requestActive;
}

/**
 * @brief Resets the specified camera to its original factory settings.
 */
void InputManager::resetCameraToDefault(uint32_t index) const {
    if (index >= cameras.size()) return;
    Camera* cam = cameras.at(index).get();
    if (!cam) return;

    if (index == CAM_IDX_FRONT) {
        cam->setPosition(glm::vec3(0.0f, 2.0f, 8.0f));
        cam->setYaw(-90.0f); // Looking straight down -Z
        cam->setPitch(0.0f);
    }
    else if (index == CAM_IDX_BIRD) {
        cam->setPosition(glm::vec3(8.0f, 5.0f, 8.0f));
        // This looks toward (-1, -0.5, -1)
        cam->setYaw(-135.0f);
        cam->setPitch(-20.0f);
    }
    else if (index == CAM_IDX_GLOBE) {
        cam->setPosition(glm::vec3(0.0f, 0.5f, 1.5f));
        cam->setYaw(-90.0f);
        cam->setPitch(0.0f);
    }
}