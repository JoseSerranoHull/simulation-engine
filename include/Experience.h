#pragma once

/* parasoft-begin-suppress ALL */
#include "libs.h"
#include <vector>
#include <memory>
#include <string>
/* parasoft-end-suppress ALL */

// Core Module Includes
#include "../include/VulkanContext.h"
#include "../include/VulkanEngine.h" 
#include "TimeManager.h"
#include "InputManager.h"
#include "../include/AssetManager.h"
#include "Renderer.h"
#include "StatsManager.h"
#include "ServiceLocator.h"
#include "../include/EntityManager.h"
#include "../include/Components.h"
#include "../include/SceneLoader.h"
#include "../include/Scenario.h"

// Scene & Infrastructure Includes
#include "Scene.h"
#include "Model.h"
#include "PostProcessor.h"
#include "Skybox.h"
#include "../include/Cubemap.h"
#include "../include/Common.h"
#include "SyncManager.h"
#include "IMGUIManager.h"
#include "../include/ClimateManager.h"
#include "SystemFactory.h"
#include "../include/VulkanResourceManager.h"

/**
 * @class Experience
 * @brief master orchestrator for the Agnostic Game Engine.
 * * Orchestrates the relationship between hardware initialization, ECS execution,
 * and scenario lifecycle. This class no longer contains scenario-specific data
 * like particle systems or lights; those are now managed as entities within the ECS.
 */
class Experience final {
public:
    // --- Named Constants ---
    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2U;
    static constexpr float COLOR_CLEAR_VAL = 0.0f;
    static constexpr float ALPHA_CLEAR_VAL = 1.0f;

    // --- Lifecycle Management ---

    /** @brief Initializes core systems (Vulkan, ECS, Managers) and windows. */
    Experience(const uint32_t width, const uint32_t height, char const* const title);

    /** @brief Triggers a full teardown of the engine and hardware handles. */
    ~Experience();

    // RAII: Unique ownership of hardware handles.
    Experience(const Experience&) = delete;
    Experience& operator=(const Experience&) = delete;

    // --- Core Execution ---

    /** @brief Enters the main simulation and rendering loop. */
    void run();

    // --- Static Callback Bridge ---
    static void framebufferResizeCallback(GLFWwindow* pWindow, int width, int height);
    static void keyCallback(GLFWwindow* pWindow, int key, int scancode, int action, int mods);
    static void mouseCallback(GLFWwindow* pWindow, double xpos, double ypos);

    // --- Agnostic State API ---

    /** @brief Requirement: Ability to change scenarios at runtime by loading new data files. */
    void changeScenario(std::unique_ptr<GE::Scenario> newScenario);

    /** @brief Manual simulation step helper for debugging collisions while paused. */
    void stepSimulation(float fixedStep);

    /** @brief Accessor for the active scenario instance for UI hooks. */
    GE::Scenario* GetCurrentScenario() const { return activeScenario.get(); }

    /** @brief Accessor for the Vulkan Engine hardware layer. */
    VulkanEngine* GetVulkanEngine() const { return vulkanEngine.get(); }

    /** @brief Accessor for the loaded graphics pipelines. */
    const std::vector<std::unique_ptr<Pipeline>>& GetPipelines() const { return pipelines; }

    /** @brief Provides access to the most recent global uniform data for compute systems. */
    const UniformBufferObject& GetCurrentUBO() const { return currentUBO; }

	/** @brief Accessor for the post-processing. */
    PostProcessor* GetPostProcessor() const { return postProcessor.get(); }

	/** @brief Accessor for the skybox. */
    Skybox* GetSkybox() const { return skybox.get(); }

private:
    // --- Windowing & Core Infrastructure ---
    GLFWwindow* window;
    const uint32_t WINDOW_WIDTH;
    const uint32_t WINDOW_HEIGHT;
    bool framebufferResized;

    // --- ECS Core (The Engine's Body) ---
    std::unique_ptr<GE::ECS::EntityManager> entityManager;

    // Core Hardware Contexts
    std::unique_ptr<VulkanContext> context;
    std::unique_ptr<VulkanEngine> vulkanEngine;
    std::unique_ptr<VulkanResourceManager> resources;
    std::unique_ptr<SystemFactory> systemFactory;

    // --- Logic & Orchestration ---
    std::unique_ptr<GE::Scenario> activeScenario; // The "Soul" of the current level
    std::unique_ptr<TimeManager> timeManager;
    std::unique_ptr<InputManager> inputManager;
    std::unique_ptr<StatsManager> statsManager;
    std::unique_ptr<ClimateManager> climateManager;

    // --- Rendering Sub-Systems ---
    std::unique_ptr<Renderer> renderer;
    std::unique_ptr<IMGUIManager> uiManager;
    std::unique_ptr<AssetManager> assetManager;

    // --- Shared Registries (Cleared on Scenario Change) ---
    std::vector<Mesh*> meshes;
    std::vector<std::unique_ptr<Model>> ownedModels;
    std::vector<std::unique_ptr<ShaderModule>> shaderModules;
    std::vector<std::unique_ptr<Pipeline>> pipelines;

    // --- Global Scene Resources ---
    UniformBufferObject currentUBO;
    std::unique_ptr<GE::Scene::Scene> scene;
    std::unique_ptr<PostProcessor> postProcessor;
    std::unique_ptr<Skybox> skybox;
    std::unique_ptr<Cubemap> skyboxTexture;

    // --- Configuration & Command Synchronization ---
    std::vector<VkFence> imagesInFlight;
    uint32_t currentFrame;

    // --- Internal Initialization Helpers ---
    void initWindow(char const* const title);
    void initVulkan();
    void createGraphicsPipelines();
    // void loadAssets(); Due to now being agnostic, the assets are loaded in the SceneLoader
    void initSkybox();

    // --- Frame Logic & Maintenance ---
    void drawFrame();
    void updateUniformBuffer(const uint32_t currentImage);
    void cleanup();
};