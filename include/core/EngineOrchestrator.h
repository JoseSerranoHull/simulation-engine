#pragma once

/* parasoft-begin-suppress ALL */
#include "core/libs.h"
#include <vector>
#include <memory>
#include <string>
/* parasoft-end-suppress ALL */

// Core Module Includes
#include "graphics/VulkanContext.h"
#include "graphics/VulkanDevice.h" 
#include "services/TimeService.h"
#include "services/InputService.h"
#include "assets/AssetManager.h"
#include "graphics/Renderer.h"
#include "services/PerformanceTracker.h"
#include "core/ServiceLocator.h"
#include "ecs/EntityManager.h"
#include "components/Components.h"
#include "scene/SceneLoader.h"
#include "scene/Scenario.h"

// Scene & Infrastructure Includes
#include "scene/Scene.h"
#include "assets/Model.h"
#include "graphics/PostProcessor.h"
#include "graphics/Skybox.h"
#include "graphics/Cubemap.h"
#include "core/Common.h"
#include "graphics/FrameSyncManager.h"
#include "services/DebugOverlay.h"
#include "services/ClimateService.h"
#include "systems/EngineServiceRegistry.h"
#include "graphics/GpuResourceManager.h"

/**
 * @class EngineOrchestrator
 * @brief master orchestrator for the Agnostic Game Engine.
 * * Orchestrates the relationship between hardware initialization, ECS execution,
 * and scenario lifecycle. This class no longer contains scenario-specific data
 * like particle systems or lights; those are now managed as entities within the ECS.
 */
class EngineOrchestrator final {
public:
    // --- Named Constants ---
    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2U;
    static constexpr float COLOR_CLEAR_VAL = 0.0f;
    static constexpr float ALPHA_CLEAR_VAL = 1.0f;

    // --- Lifecycle Management ---

    /** @brief Initializes core systems (Vulkan, ECS, Managers) and windows. */
    EngineOrchestrator(const uint32_t width, const uint32_t height, char const* const title);

    /** @brief Triggers a full teardown of the engine and hardware handles. */
    ~EngineOrchestrator();

    // RAII: Unique ownership of hardware handles.
    EngineOrchestrator(const EngineOrchestrator&) = delete;
    EngineOrchestrator& operator=(const EngineOrchestrator&) = delete;

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
    GE::Graphics::VulkanDevice* GetVulkanDevice() const { return vulkanEngine.get(); }

    /** @brief Accessor for the loaded graphics pipelines. */
    const std::vector<std::unique_ptr<GE::Graphics::GraphicsPipeline>>& GetPipelines() const { return pipelines; }

    /** @brief Provides access to the most recent global uniform data for compute systems. */
    const UniformBufferObject& GetCurrentUBO() const { return currentUBO; }

	/** @brief Accessor for the post-processing. */
    GE::Graphics::PostProcessor* GetPostProcessor() const { return postProcessor.get(); }

	/** @brief Accessor for the skybox. */
    GE::Graphics::Skybox* GetSkybox() const { return skybox.get(); }

    /** @brief Requests a scenario transition to be performed at the start of the next frame. */
    void requestScenarioChange(const std::string& path) { m_pendingScenarioPath = path; }

private:
    // --- Windowing & Core Infrastructure ---
    GLFWwindow* window;
    const uint32_t WINDOW_WIDTH;
    const uint32_t WINDOW_HEIGHT;
    bool framebufferResized;

    // --- ECS Core (The Engine's Body) ---
    std::unique_ptr<GE::ECS::EntityManager> entityManager;

    // Core Hardware Contexts
    std::unique_ptr<GE::Graphics::VulkanContext> context;
    std::unique_ptr<GE::Graphics::VulkanDevice> vulkanEngine;
    std::unique_ptr<GE::Graphics::GpuResourceManager> resources;
    std::unique_ptr<EngineServiceRegistry> systemFactory;

    // --- Logic & Orchestration ---
    std::unique_ptr<GE::Scenario> activeScenario; // The "Soul" of the current level
    std::unique_ptr<TimeService> timeManager;
    std::unique_ptr<InputService> inputManager;
    std::unique_ptr<PerformanceTracker> statsManager;
    std::unique_ptr<ClimateService> climateManager;

    // --- Rendering Sub-Systems ---
    std::unique_ptr<GE::Graphics::Renderer> renderer;
    std::unique_ptr<DebugOverlay> uiManager;
    std::unique_ptr<AssetManager> assetManager;

    // --- Shared Registries (Cleared on Scenario Change) ---
    std::vector<GE::Assets::Mesh*> meshes;
    std::vector<std::unique_ptr<GE::Assets::Model>> ownedModels;
    std::vector<std::unique_ptr<GE::Graphics::ShaderModule>> shaderModules;
    std::vector<std::unique_ptr<GE::Graphics::GraphicsPipeline>> pipelines;

    // --- Global Scene Resources ---
    UniformBufferObject currentUBO;
    std::unique_ptr<GE::Scene::Scene> scene;
    std::unique_ptr<GE::Graphics::PostProcessor> postProcessor;
    std::unique_ptr<GE::Graphics::Skybox> skybox;
    std::unique_ptr<GE::Graphics::Cubemap> skyboxTexture;

    // --- Configuration & Command Synchronization ---
    std::vector<VkFence> imagesInFlight;
    uint32_t currentFrame;
    std::string m_pendingScenarioPath = ""; // Stores the path for the next frame

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