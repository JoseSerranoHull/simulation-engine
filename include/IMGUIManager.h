#pragma once

/* parasoft-begin-suppress ALL */
#include "libs.h"
#include <memory>
#include <string>
/* parasoft-end-suppress ALL */

#include "../include/VulkanContext.h"
#include "../include/VulkanEngine.h"
#include "InputManager.h"
#include "StatsManager.h"
#include "TimeManager.h"
#include "../include/ClimateManager.h"
#include "PointLight.h"
#include "ServiceLocator.h"
#include "../include/Scenario.h"

/**
 * @class DebugOverlay
 * @brief Manages the Dear ImGui lifecycle and diagnostic UI widgets.
 * * This manager orchestrates the dedicated ImGui descriptor pool, library
 * initialization for GLFW/Vulkan, and the per-frame recording of debug interfaces.
 */
class DebugOverlay final {
public:
    // --- Lifecycle ---

    /**
     * @brief Constructor: Links the UI manager to the centralized Vulkan context.
     */
    explicit DebugOverlay();

    /** @brief Destructor: Ensures cleanup of hardware and library resources. */
    ~DebugOverlay();

    // RAII: Prevent duplication of the UI context and descriptor pool handles.
    DebugOverlay(const DebugOverlay&) = delete;
    DebugOverlay& operator=(const DebugOverlay&) = delete;

    // --- Core API ---

    /**
     * @brief Initializes the ImGui library for GLFW and Vulkan.
     */
    void init(GLFWwindow* const window, const GE::Graphics::VulkanDevice* const engine);

    /**
     * @brief Records the UI widget state for the current frame.
     * Captures data from managers and lights to drive the visual debug interface.
     */
    void update(
        InputService* const input,
        const PerformanceTracker* const stats,
        PointLight* const light,
        const TimeService* const time,
        ClimateService* const climate
    ) const;

    /**
     * @brief Records the ImGui draw data into the provided command buffer.
     */
    void draw(const VkCommandBuffer cb) const;

    /**
     * @brief Explicitly destroys ImGui resources and the dedicated descriptor pool.
     */
    void cleanup();

private:
    // --- Internal State & GPU Resources ---
    VkDescriptorPool imguiPool;  /**< Dedicated descriptor pool for ImGui textures. */

	// --- Internal Helper Methods ---
    void DrawMainMenuBar(InputService* const input) const;
};