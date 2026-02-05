#pragma once

/* parasoft-begin-suppress ALL */
#include "libs.h"
#include <vector>
#include <optional>
#include <memory>
#include <map>
#include <string>
#include <fstream>
/* parasoft-end-suppress ALL */

// Module Includes
#include "VulkanContext.h"
#include "VulkanEngine.h" 
#include "TimeManager.h"
#include "InputManager.h"
#include "AssetManager.h"
#include "Renderer.h"
#include "StatsManager.h"

// Scene & System Includes
#include "Scene.h"
#include "Model.h"
#include "PointLight.h"
#include "PostProcessor.h"
#include "Skybox.h"
#include "Cubemap.h"
#include "CommonStructs.h"
#include "ConfigLoader.h"
#include "GeometryUtils.h"
#include "SyncManager.h"
#include "IMGUIManager.h"
#include "ClimateManager.h"
#include "SystemFactory.h"
#include "VulkanResourceManager.h"

/**
 * @class Experience
 * @brief The Master Orchestrator for the Sandy-Snow Globe engine.
 * * Orchestrates the relationship between input, simulation, and hardware-accelerated
 * rendering. It serves as the primary owner for all major engine sub-systems.
 */
class Experience final {
public:
    // --- Named Constants ---
    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2U;
    static constexpr float COLOR_CLEAR_VAL = 0.0f;
    static constexpr float ALPHA_CLEAR_VAL = 1.0f;

    // --- Lifecycle Management ---

    /** @brief Initializes all core systems and windowing. */
    Experience(const uint32_t width, const uint32_t height, char const* const title);

    /** @brief Triggers a full teardown of the engine and GPU resources. */
    ~Experience();

    // RAII: Prevent duplication to ensure unique ownership of hardware handles.
    Experience(const Experience&) = delete;
    Experience& operator=(const Experience&) = delete;

    // --- Core Execution ---

    /** @brief Enters the main simulation and rendering loop. */
    void run();

    // --- Static Callback Bridge ---
    // These link OS/Window events directly to the engine's internal managers.
    static void framebufferResizeCallback(GLFWwindow* pWindow, int width, int height);
    static void keyCallback(GLFWwindow* pWindow, int key, int scancode, int action, int mods);
    static void mouseCallback(GLFWwindow* pWindow, double xpos, double ypos);

private:
    // --- Windowing & Core Infrastructure ---
    GLFWwindow* window;
    const uint32_t WINDOW_WIDTH;
    const uint32_t WINDOW_HEIGHT;
    bool framebufferResized;

    // Core Hardware Contexts
    std::unique_ptr<VulkanContext> context;
    std::unique_ptr<VulkanEngine> vulkanEngine;
    std::unique_ptr<VulkanResourceManager> resources;

    // --- High-Level Logic Managers ---
    std::unique_ptr<TimeManager> timeManager;
    std::unique_ptr<InputManager> inputManager;
    std::unique_ptr<StatsManager> statsManager;
    std::unique_ptr<ClimateManager> climateManager;

    // --- Rendering Sub-Systems ---
    std::unique_ptr<Renderer> renderer;
    std::unique_ptr<IMGUIManager> uiManager;
    std::unique_ptr<AssetManager> assetManager;

    // --- Scene Geometry & Pipeline Registries ---
    std::vector<Mesh*> meshes;
    std::vector<Mesh*> transparentMeshes;
    std::vector<std::unique_ptr<Model>> ownedModels;
    std::vector<std::unique_ptr<ShaderModule>> shaderModules;
    std::vector<std::unique_ptr<Pipeline>> pipelines;

    // --- Global Scene Resources ---
    UniformBufferObject currentUBO;
    std::unique_ptr<Scene> scene;
    std::unique_ptr<PostProcessor> postProcessor;
    std::unique_ptr<Skybox> skybox;
    std::unique_ptr<Cubemap> skyboxTexture;
    std::unique_ptr<PointLight> mainLight;

    // Atmospheric Particle Systems
    std::unique_ptr<ParticleSystem> dustParticleSystem;
    std::unique_ptr<ParticleSystem> fireParticleSystem;
    std::unique_ptr<ParticleSystem> smokeParticleSystem;
    std::unique_ptr<ParticleSystem> rainParticleSystem;
    std::unique_ptr<ParticleSystem> snowParticleSystem;

    // --- Configuration & Command Synchronization ---
    std::map<std::string, ObjectTransform> cachedConfig;
    std::vector<VkFence> imagesInFlight;
    uint32_t currentFrame;

    // --- Internal Initialization Helpers ---

    void initWindow(char const* const title);
    void initVulkan();
    void createGraphicsPipelines();
    void loadAssets();
    void initSkybox();

    // --- Frame Logic & Maintenance ---

    void drawFrame();
    void updateUniformBuffer(const uint32_t currentImage);
    void cleanup();

    /** @brief Resets the climate and UI to default starting values. */
    void performFullReset() const;

    /** @brief Synchronizes the ClimateManager state with InputManager UI toggles. */
    void syncWeatherToggles() const;
};