#include "Experience.h"

/* parasoft-begin-suppress ALL */
#include <iostream>
#include <stdexcept>
#include <array>
#include <fstream>
#include <cstring>
/* parasoft-end-suppress ALL */

/**
 * @namespace SceneKeys
 * @brief String identifiers for scene entities to avoid literal duplication.
 */
namespace SceneKeys {
    static constexpr char const* DESERT_QUEEN = "DesertQueen";
    static constexpr char const* MAGIC_CIRCLE = "MagicCircle";
    static constexpr char const* OASIS = "Oasis";
    static constexpr char const* MAIN_LIGHT = "MainLight";
    static constexpr char const* VIKING_HOUSE = "VikingHouse";
}

// ========================================================================
// SECTION 1: CONSTRUCTOR & DESTRUCTOR
// ========================================================================

/**
 * @brief Constructor: Orchestrates the initialization of all engine sub-systems.
 */
Experience::Experience(const uint32_t width, const uint32_t height, char const* const title)
    : WINDOW_WIDTH(width),
    WINDOW_HEIGHT(height),
    framebufferResized(false),
    currentFrame(EngineConstants::INDEX_ZERO)
{
    // Step 1: Foundation
    initWindow(title);
    context = std::make_unique<VulkanContext>();
    ServiceLocator::Provide(context.get());
    vulkanEngine = std::make_unique<VulkanEngine>(window);

    entityManager = std::make_unique<GE::ECS::EntityManager>();

    // Initialize with 1000 entities and space for 32 component types
    entityManager->Initialize(1000, 32);

    // Register our component types
    entityManager->RegisterComponent<GE::Scene::Components::Transform>();
    entityManager->RegisterComponent<GE::Scene::Components::Tag>();
    entityManager->RegisterComponent<GE::Components::MeshRenderer>();
    entityManager->RegisterComponent<GE::Components::LightComponent>();
    entityManager->RegisterComponent<GE::Components::PhysicsComponent>();

	ServiceLocator::Provide(entityManager.get());

    // Step 2: Infrastructure & Factory Setup
    resources = std::make_unique<VulkanResourceManager>();
    ServiceLocator::Provide(resources.get());

    // NEW: Initialize and Provide the Factory
    systemFactory = std::make_unique<SystemFactory>();
    ServiceLocator::Provide(systemFactory.get());

    timeManager = std::make_unique<TimeManager>();
    statsManager = std::make_unique<StatsManager>();
    assetManager = std::make_unique<AssetManager>();

    // Step 3: Hardware Linkage
    resources->init(vulkanEngine.get(), MAX_FRAMES_IN_FLIGHT);
    assetManager->setDescriptorPool(resources->getDescriptorPool());

    // Step 4: Logic Layers
    imagesInFlight.resize(vulkanEngine->getSwapChainImageCount(), VK_NULL_HANDLE);
    renderer = std::make_unique<Renderer>();
    scene = std::make_unique<GE::Scene::Scene>();
    inputManager = std::make_unique<InputManager>(window, timeManager.get());
    ServiceLocator::Provide(inputManager.get());

    uiManager = std::make_unique<IMGUIManager>();
    climateManager = std::make_unique<ClimateManager>();

    // Step 5: Configuration
    cachedConfig = ConfigLoader::loadConfig("./config/config.txt");
    if (cachedConfig.empty()) {
        throw std::runtime_error("Experience: Failed to load config.txt!");
    }

    // ============================================================
    // Step 6: SYSTEM REGISTRATION (OCP Refactor)
    // ============================================================

    // 6.1 Post-Processing Recipe
    systemFactory->registerSystem("PostProcessor", [&]() -> std::unique_ptr<ISystem> {
        return std::make_unique<PostProcessor>(
            vulkanEngine->getSwapChainExtent().width,
            vulkanEngine->getSwapChainExtent().height,
            vulkanEngine->getSwapChainFormat(),
            vulkanEngine->getFinalRenderPass(),
            vulkanEngine->getMsaaSamples()
        );
    });

    // 6.2 Light Recipe
    const ObjectTransform& lightCfg = cachedConfig.at("MainLight");
    systemFactory->registerSystem("MainLight", [&]() -> std::unique_ptr<ISystem> {
        return std::make_unique<PointLight>(
            lightCfg.pos,
            lightCfg.color,
            lightCfg.params.at("intensity")
        );
    });

    // 6.3 Particle System Recipe Template
    auto registerParticle = [&](const std::string& name, const std::string& comp, const std::string& vert, const std::string& frag, glm::vec3 pos, uint32_t count) {
        systemFactory->registerSystem(name, [&, comp, vert, frag, pos, count]() -> std::unique_ptr<ISystem> {
            return std::make_unique<ParticleSystem>(
                postProcessor->getTransparentRenderPass(),
                context->globalSetLayout,
                comp, vert, frag, pos, count, vulkanEngine->getMsaaSamples()
            );
        });
    };

    // ============================================================
    // Step 7: SYSTEM CREATION
    // ============================================================

    // Create PostProcessor first (needed for particle render passes)
    auto genericPP = systemFactory->create("PostProcessor");
    postProcessor.reset(static_cast<PostProcessor*>(genericPP.release()));
    postProcessor->resize(vulkanEngine->getSwapChainExtent());

    // Create Light
    auto genericLight = systemFactory->create("MainLight");
    mainLight.reset(static_cast<PointLight*>(genericLight.release()));

    // Register and Create Particles
    registerParticle("Dust", "./shaders/dust_comp.spv", "./shaders/dust_vert.spv", "./shaders/dust_frag.spv", { 0.0f, 1.2f, 0.0f }, 1000U);
    registerParticle("Fire", "./shaders/fire_comp.spv", "./shaders/fire_vert.spv", "./shaders/fire_frag.spv", { -0.8f, -0.15f, -0.5f }, 500U);
    registerParticle("Smoke", "./shaders/smoke_comp.spv", "./shaders/smoke_vert.spv", "./shaders/smoke_frag.spv", { -0.8f, -0.15f, -0.5f }, 500U);
    registerParticle("Rain", "./shaders/rain_comp.spv", "./shaders/rain_vert.spv", "./shaders/rain_frag.spv", { 0.0f, 1.8f, 0.0f }, 5000U);
    registerParticle("Snow", "./shaders/snow_comp.spv", "./shaders/snow_vert.spv", "./shaders/snow_frag.spv", { 0.0f, 1.8f, 0.0f }, 3000U);

    dustParticleSystem.reset(static_cast<ParticleSystem*>(systemFactory->create("Dust").release()));
    fireParticleSystem.reset(static_cast<ParticleSystem*>(systemFactory->create("Fire").release()));
    smokeParticleSystem.reset(static_cast<ParticleSystem*>(systemFactory->create("Smoke").release()));
    rainParticleSystem.reset(static_cast<ParticleSystem*>(systemFactory->create("Rain").release()));
    snowParticleSystem.reset(static_cast<ParticleSystem*>(systemFactory->create("Snow").release()));

    // Step 8: Finalization
    syncWeatherToggles(); // Use existing helper to align UI/Climate
    initVulkan();
    uiManager->init(window, vulkanEngine.get());
    initSkybox();
    loadAssets();
}

/**
 * @brief Destructor: Ensures safe termination of the hardware context.
 */
Experience::~Experience() {
    try {
        cleanup();
    }
    catch (...) {
        // Standard safety: suppress exceptions during shutdown to prevent std::terminate
    }
}

// ========================================================================
// SECTION 2: CORE ORCHESTRATION
// ========================================================================

/**
 * @brief Triggers the creation of pipelines and synchronization of descriptor sets.
 */
void Experience::initVulkan() {
    createGraphicsPipelines();
    resources->updateDescriptorSets(vulkanEngine.get(), postProcessor.get());
}

/**
 * @brief Enters the main execution loop.
 */
void Experience::run() {
    while (glfwWindowShouldClose(window) == GLFW_FALSE) {
        // Step 1: Poll OS Events
        glfwPollEvents();

        // Step 2: Update simulation timers and telemetry
        timeManager->update();
        statsManager->update(timeManager->getDelta());

        // Step 3: Record and submit frame
        drawFrame();
    }

    // Step 4: Final sync to ensure GPU is idle before resource teardown
    if ((context != nullptr) && (context->device != VK_NULL_HANDLE)) {
        static_cast<void>(vkDeviceWaitIdle(context->device));
    }
}

/**
 * @brief Configures the OS window for Vulkan.
 */
void Experience::initWindow(char const* const title) {
    if (glfwInit() == GLFW_FALSE) {
        throw std::runtime_error("Experience: Failed to initialize GLFW");
    }

    // Configure for Vulkan (No OpenGL API)
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window = glfwCreateWindow(static_cast<int>(WINDOW_WIDTH), static_cast<int>(WINDOW_HEIGHT), title, nullptr, nullptr);
    if (window == nullptr) {
        throw std::runtime_error("Experience: Failed to create GLFW window");
    }

    // Register user pointer and hardware callbacks
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetCursorPosCallback(window, mouseCallback);
}

// ========================================================================
// SECTION 3: STATIC CALLBACK BRIDGES
// ========================================================================

/** @brief Redirects window resize events. */
void Experience::framebufferResizeCallback(GLFWwindow* pWindow, int width, int height) {
    auto* const app = reinterpret_cast<Experience*>(glfwGetWindowUserPointer(pWindow));
    if (app != nullptr) {
        app->framebufferResized = true;
    }
}

/** @brief Redirects keyboard input events. */
void Experience::keyCallback(GLFWwindow* pWindow, int key, int scancode, int action, int mods) {
    auto* const app = reinterpret_cast<Experience*>(glfwGetWindowUserPointer(pWindow));
    if ((app != nullptr) && (app->inputManager != nullptr)) {
        app->inputManager->handleKeyEvent(key, scancode, action, mods);
    }
}

/** @brief Redirects mouse movement events. */
void Experience::mouseCallback(GLFWwindow* pWindow, double xpos, double ypos) {
    auto* const app = reinterpret_cast<Experience*>(glfwGetWindowUserPointer(pWindow));

    // Step 1: Validate the application pointer and the existence of the input manager
    if ((app != nullptr) && (app->inputManager != nullptr)) {
        // Step 2: Delegate the hardware mouse event to the simulation's input manager
        app->inputManager->handleMouseEvent(xpos, ypos);
    }
}

// ========================================================================
// SECTION 4: LOGICAL RESOURCE INITIALIZATION
// ========================================================================

/**
 * @brief Initializes the graphics pipelines for all scene materials.
 * Logic: Transparent objects (Glass/Water) are configured NOT to write
 * to the depth buffer to ensure internal particles remain visible.
 */
void Experience::createGraphicsPipelines() {
    VulkanContext* context = ServiceLocator::GetContext();

    // Step 1: Hardware validation
    if (postProcessor == nullptr) {
        throw std::runtime_error("Experience: Cannot create pipelines, PostProcessor is null!");
    }

    const VkRenderPass offscreenPass = postProcessor->getOffscreenRenderPass();
    const VkRenderPass transPass = postProcessor->getTransparentRenderPass();
    const VkSampleCountFlagBits msaa = vulkanEngine->getMsaaSamples();

    // Step 2: Clear existing state to prevent resource leaks during hot-reloads
    shaderModules.clear();
    pipelines.clear();

    // Step 3: Load Shader Modules (Managed by RAII unique_ptr)
    shaderModules.push_back(std::make_unique<ShaderModule>("./shaders/phong_vert.spv", VK_SHADER_STAGE_VERTEX_BIT));
    shaderModules.push_back(std::make_unique<ShaderModule>("./shaders/phong_frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT));
    shaderModules.push_back(std::make_unique<ShaderModule>("./shaders/sand_frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT));
    shaderModules.push_back(std::make_unique<ShaderModule>("./shaders/base_frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT));
    shaderModules.push_back(std::make_unique<ShaderModule>("./shaders/glass_frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT));
    shaderModules.push_back(std::make_unique<ShaderModule>("./shaders/transparent_frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT));
    shaderModules.push_back(std::make_unique<ShaderModule>("./shaders/water_vert.spv", VK_SHADER_STAGE_VERTEX_BIT));
    shaderModules.push_back(std::make_unique<ShaderModule>("./shaders/water_frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT));
    shaderModules.push_back(std::make_unique<ShaderModule>("./shaders/shadow_vert.spv", VK_SHADER_STAGE_VERTEX_BIT));
    shaderModules.push_back(std::make_unique<ShaderModule>("./shaders/shadow_frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT));

    // Aliases for readability (Parasoft: Use const pointers for aliases)
    ShaderModule* const phongVert = shaderModules[0].get();
    ShaderModule* const phongFrag = shaderModules[1].get();
    ShaderModule* const sandFrag = shaderModules[2].get();
    ShaderModule* const baseFrag = shaderModules[3].get();
    ShaderModule* const glassFrag = shaderModules[4].get();
    ShaderModule* const alphaFrag = shaderModules[5].get();
    ShaderModule* const waterVert = shaderModules[6].get();
    ShaderModule* const waterFrag = shaderModules[7].get();
    ShaderModule* const shadowVert = shaderModules[8].get();
    ShaderModule* const shadowFrag = shaderModules[9].get();

    // Step 4: Instantiate Pipelines in RAII container
    // Arguments: (context, renderPass, layout, vert, frag, depthTest, depthWrite, stencil, msaa)

    // Opaque Pipelines (Require Depth Writing)
    pipelines.push_back(std::make_unique<Pipeline>(offscreenPass, context->materialSetLayout, phongVert, phongFrag, true, true, true, msaa));
    pipelines.push_back(std::make_unique<Pipeline>(offscreenPass, context->materialSetLayout, phongVert, sandFrag, true, true, true, msaa));
    pipelines.push_back(std::make_unique<Pipeline>(offscreenPass, context->materialSetLayout, phongVert, baseFrag, true, true, true, msaa));

    // Transparent/Fluid Pipelines (depthWrite = FALSE)
    pipelines.push_back(std::make_unique<Pipeline>(transPass, context->materialSetLayout, phongVert, glassFrag, true, false, false, msaa));
    pipelines.push_back(std::make_unique<Pipeline>(offscreenPass, context->materialSetLayout, phongVert, alphaFrag, false, false, true, msaa));
    pipelines.push_back(std::make_unique<Pipeline>(transPass, context->materialSetLayout, waterVert, waterFrag, true, false, false, msaa));

    // Shadow Map Pipeline (Requires 1x Sample Count)
    pipelines.push_back(std::make_unique<Pipeline>(
        resources->getShadowRenderPass(),
        context->materialSetLayout,
        shadowVert,
        shadowFrag,
        true, true, true,
        VK_SAMPLE_COUNT_1_BIT
    ));

    // Step 5: Final Pass Link - Orchestrate the Post-Processor's final screen pipeline
    postProcessor->createPipeline(vulkanEngine->getFinalRenderPass());
}

/**
 * @brief Loads all 3D geometry, textures, and materials via the Data-Driven Scene Loader.
 * This replaces the previous ~150 lines of manual asset loading.
 */
void Experience::loadAssets() {
    VulkanContext* context = ServiceLocator::GetContext();

    // Step 1: Setup GPU Command Recording for Transfer
    const VkCommandBuffer setupCmd = VulkanUtils::beginSingleTimeCommands(context->device, context->graphicsCommandPool);
    std::vector<VkBuffer> stagingBuffers{};
    std::vector<VkDeviceMemory> stagingMemories{};

    // Step 2: Reset Scene and Registries
    // Note: Namespacing to GE::Scene
    scene = std::make_unique<GE::Scene::Scene>();

    meshes.clear();
    transparentMeshes.clear();
    ownedModels.clear();

    // Step 3: DATA-DRIVEN LOADING (Step 5 of ECSPlan)
    GE::Scene::SceneLoader loader;
    loader.load(
        "./config/snow_globe.ini",
        entityManager.get(),
        assetManager.get(),
        scene.get(),
        pipelines,
        setupCmd,
        stagingBuffers,
        stagingMemories,
        ownedModels // Collects models to keep Mesh raw pointers valid
    );

    // Step 4: Final GPU Submission & Staging Cleanup
    VulkanUtils::endSingleTimeCommands(context->device, context->graphicsCommandPool, context->graphicsQueue, setupCmd);

    for (size_t i = 0U; i < stagingBuffers.size(); ++i) {
        vkDestroyBuffer(context->device, stagingBuffers[i], nullptr);
        vkFreeMemory(context->device, stagingMemories[i], nullptr);
    }

    // Step 5: Finalize Environmental FX
    initSkybox();
}

/**
 * @brief Initializes the environmental skybox.
 * Loads the 6 faces of the cubemap and prepares the Skybox pipeline.
 */
void Experience::initSkybox() {
    // 1. Define the paths for the cubemap faces
    const std::vector<std::string> skyboxFaces = {
        "./textures/cubemaps/cubemap_0(+X).jpg",
        "./textures/cubemaps/cubemap_1(-X).jpg",
        "./textures/cubemaps/cubemap_2(+Y).jpg",
        "./textures/cubemaps/cubemap_3(-Y).jpg",
        "./textures/cubemaps/cubemap_4(+Z).jpg",
        "./textures/cubemaps/cubemap_5(-Z).jpg"
    };

    // 2. Initialize the GPU Cubemap texture (RAII managed)
    skyboxTexture = std::make_unique<Cubemap>(skyboxFaces);

    // 3. Initialize the Skybox rendering logic
    // We pass the offscreen render pass to allow the skybox to be processed by Bloom/HDR
    skybox = std::make_unique<Skybox>(
        postProcessor->getOffscreenRenderPass(),
        skyboxTexture.get(),
        vulkanEngine->getMsaaSamples()
    );
}

// ========================================================================
// SECTION 5: FRAME LOGIC
// ========================================================================

/**
 * @brief Master frame orchestration: Synchronizes CPU/GPU, updates logic,
 * records commands, and presents the resulting image.
 */
void Experience::drawFrame() {
    VulkanContext* context = ServiceLocator::GetContext();
    auto* em = entityManager.get(); // Cache for ECS access

    // Step 1: CPU-GPU Throttling - Wait for the previous frame's GPU execution to finish
    SyncManager* const sync = resources->getSyncManager();
    const VkFence currentFence = sync->getInFlightFence(currentFrame);
    static_cast<void>(vkWaitForFences(context->device, 1U, &currentFence, VK_TRUE, UINT64_MAX));

    const float dt = timeManager->getDelta();
    const float totalTime = timeManager->getTotal();

    // Step 2: High-Level Logic Updates - Update input and scene state
    inputManager->update(dt);
    if (scene != nullptr) {
        scene->update(dt); // Now uses ECS internal lookups
    }

    // Step 3: Presentation Handshake - Acquire an image from the Swapchain
    uint32_t imageIndex{ 0U };
    const VkResult acquireResult = vkAcquireNextImageKHR(
        context->device,
        vulkanEngine->getSwapChain(),
        UINT64_MAX,
        sync->getImageAvailableSemaphore(currentFrame),
        VK_NULL_HANDLE,
        &imageIndex
    );

    // Handle Window Resizing/Minimization
    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
        vulkanEngine->recreateSwapChain(window);
        if (postProcessor != nullptr) {
            postProcessor->resize(vulkanEngine->getSwapChainExtent());
        }
        resources->updateDescriptorSets(vulkanEngine.get(), postProcessor.get());
        return;
    }

    // Ensure the acquired image is not being used by a previous in-flight frame
    if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        static_cast<void>(vkWaitForFences(context->device, 1U, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX));
    }
    imagesInFlight[imageIndex] = sync->getInFlightFence(currentFrame);

    // Reset the fence for the current frame to signal start of work
    const VkFence frameFence = sync->getInFlightFence(currentFrame);
    static_cast<void>(vkResetFences(context->device, 1U, &frameFence));

    // Step 4: Command Buffer Recording
    updateUniformBuffer(imageIndex);

    const VkCommandBuffer cb = resources->getSyncManager()->getCommandBuffer(currentFrame);
    static_cast<void>(vkResetCommandBuffer(cb, 0U));

    const VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    static_cast<void>(vkBeginCommandBuffer(cb, &beginInfo));

    // NEW: ECS lookup for Particle Emitters (e.g., finding the fire origin on Cactus1)
    glm::vec3 fireOrigin(-0.8f, -0.15f, -0.5f);
    if (scene && scene->hasEntity("Cactus1")) {
        auto* trans = em->GetTIComponent<GE::Scene::Components::Transform>(scene->getEntityID("Cactus1"));
        if (trans) fireOrigin = trans->m_position;
    }

    // Update Particle Systems
    if (dustParticleSystem) dustParticleSystem->update(cb, timeManager->getDelta(), inputManager->getDustEnabled(), timeManager->getTotal(), currentUBO.lightColor);
    if (fireParticleSystem) fireParticleSystem->update(cb, timeManager->getDelta(), inputManager->getFireEnabled(), timeManager->getTotal(), currentUBO.lightColor, fireOrigin);
    if (smokeParticleSystem) smokeParticleSystem->update(cb, timeManager->getDelta(), inputManager->getSmokeEnabled(), timeManager->getTotal(), currentUBO.lightColor, fireOrigin);
    if (rainParticleSystem) rainParticleSystem->update(cb, timeManager->getDelta(), inputManager->getRainEnabled(), timeManager->getTotal(), currentUBO.lightColor);
    if (snowParticleSystem) snowParticleSystem->update(cb, timeManager->getDelta(), inputManager->getSnowEnabled(), timeManager->getTotal(), currentUBO.lightColor);

    // Record Draw Calls via Refactored Renderer
    std::vector<Pipeline*> rawPipelines;
    for (const auto& p : pipelines) rawPipelines.push_back(p.get());
    renderer->recordFrame(
        cb, vulkanEngine->getSwapChainExtent(), skybox.get(),
        dustParticleSystem.get(), fireParticleSystem.get(), smokeParticleSystem.get(),
        rainParticleSystem.get(), snowParticleSystem.get(), postProcessor.get(),
        resources->getDescriptorSet(imageIndex), resources->getShadowRenderPass(), resources->getShadowFramebuffer(),
        rawPipelines, inputManager->getDustEnabled(), inputManager->getFireEnabled(), inputManager->getSmokeEnabled(),
        inputManager->getRainEnabled(), inputManager->getSnowEnabled()
    );

    // Step 5: Final Display Pass - Bloom, UI, and Color Correction
    VkRenderPassBeginInfo finalPassInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    finalPassInfo.renderPass = vulkanEngine->getFinalRenderPass();
    finalPassInfo.framebuffer = vulkanEngine->getFramebuffer(imageIndex);
    finalPassInfo.renderArea.extent = vulkanEngine->getSwapChainExtent();

    std::array<VkClearValue, 2U> clearValues{};
    clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
    clearValues[1].depthStencil = { 1.0f, 0U };
    finalPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    finalPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(cb, &finalPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    if (postProcessor != nullptr) {
        postProcessor->draw(cb, inputManager->getBloomEnabled());
    }

    uiManager->update(inputManager.get(), statsManager.get(), mainLight.get(), timeManager.get(), climateManager.get());
    uiManager->draw(cb);

    vkCmdEndRenderPass(cb);
    static_cast<void>(vkEndCommandBuffer(cb));

    // Step 6: Submission - Send recorded commands to the Graphics Queue
    VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    submitInfo.waitSemaphoreCount = 1U;
    VkSemaphore waitSem = sync->getImageAvailableSemaphore(currentFrame);
    submitInfo.pWaitSemaphores = &waitSem;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1U;
    submitInfo.pCommandBuffers = &cb;
    submitInfo.signalSemaphoreCount = 1U;
    VkSemaphore signalSem = sync->getRenderFinishedSemaphore(imageIndex);
    submitInfo.pSignalSemaphores = &signalSem;

    if (vkQueueSubmit(context->graphicsQueue, 1U, &submitInfo, frameFence) != VK_SUCCESS) {
        throw std::runtime_error("Experience: Queue submit failed!");
    }

    // Step 7: Presentation - Submit the final image to the OS for display
    VkPresentInfoKHR presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    presentInfo.waitSemaphoreCount = 1U;
    VkSemaphore renderWaitSem = sync->getRenderFinishedSemaphore(imageIndex);
    presentInfo.pWaitSemaphores = &renderWaitSem;
    presentInfo.swapchainCount = 1U;
    VkSwapchainKHR swapchains[] = { vulkanEngine->getSwapChain() };
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;

    const VkResult presentResult = vkQueuePresentKHR(context->presentQueue, &presentInfo);

    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR || framebufferResized) {
        framebufferResized = false;
        vulkanEngine->recreateSwapChain(window);
        postProcessor->resize(vulkanEngine->getSwapChainExtent());
        resources->updateDescriptorSets(vulkanEngine.get(), postProcessor.get());
        imagesInFlight.resize(vulkanEngine->getSwapChainImageCount(), VK_NULL_HANDLE);
    }

    currentFrame = (currentFrame + 1U) % MAX_FRAMES_IN_FLIGHT;
}

/**
 * @brief Syncs CPU simulation state to the GPU Uniform Buffer.
 * Orchestrates climate simulation, manual user overrides, and hardware buffer updates.
 */
void Experience::updateUniformBuffer(const uint32_t currentImage) {
    void* const mappedData = resources->getMappedBuffer(currentImage);
    if (!mappedData) return;

    auto* em = entityManager.get();
    const float dt = timeManager->getDelta();
    const float totalTime = timeManager->getTotal();
    const ObjectTransform& lightCfg = cachedConfig.at("MainLight");

    // Climate Simulation
    climateManager->update(dt, totalTime, inputManager->getAutoOrbit(), lightCfg.params.at("orbitRadius"), lightCfg.params.at("orbitSpeed"), lightCfg.params.at("intensity"));
    if (climateManager->checkTransition()) syncWeatherToggles();

    const float cactusMultiplier = climateManager->getCactusScale();
    for (uint32_t i = 1U; i <= 3U; ++i) {
        std::string key = "Cactus" + std::to_string(i);
        if (scene->hasEntity(key)) {
            auto* trans = em->GetTIComponent<GE::Scene::Components::Transform>(scene->getEntityID(key));
            if (trans) {
                trans->m_scale = cachedConfig.at(key).scale * cactusMultiplier;
                trans->m_state = GE::Scene::Components::Transform::TransformState::Dirty;
            }
        }
    }

    if (scene->hasEntity("Oasis")) {
        auto* trans = em->GetTIComponent<GE::Scene::Components::Transform>(scene->getEntityID("Oasis"));
        if (trans) {
            trans->m_scale = cachedConfig.at("Oasis").scale * climateManager->getWaterScale();
            trans->m_position = cachedConfig.at("Oasis").pos + glm::vec3(0.0f, climateManager->getWaterOffset(), 0.0f);
            trans->m_state = GE::Scene::Components::Transform::TransformState::Dirty;
        }
    }

    // Step 4: Synchronize global light data (Sun Position, Color, Intensity)
    mainLight->setPosition(inputManager->getAutoOrbit() ? climateManager->getSunPosition() : mainLight->getPosition());
    mainLight->setColor(climateManager->getAmbientColor() * inputManager->getColorMod());
    mainLight->setIntensity(climateManager->getSunIntensity() * inputManager->getIntensityMod());

    // Step 5: Populate the UBO struct and perform the memory copy to the GPU
    UniformBufferObject ubo{};
    const VkExtent2D extent = vulkanEngine->getSwapChainExtent();
    const float aspect = static_cast<float>(extent.width) / static_cast<float>(extent.height);

    ubo.view = inputManager->getActiveCamera()->getViewMatrix();
    ubo.proj = glm::perspective(glm::radians(inputManager->getActiveCamera()->getZoom()), aspect, 0.1f, 100.0f);
    ubo.proj[1][1] *= -1.0f; // Vulkan NDC inversion

    ubo.lightPos = mainLight->getPosition();
    ubo.viewPos = inputManager->getActiveCamera()->getPosition();
    ubo.lightColor = mainLight->getLightValue();
    ubo.lightSpaceMatrix = mainLight->getLightSpaceMatrix();
    ubo.useGouraud = inputManager->getGouraudEnabled() ? EngineConstants::SHADER_TRUE : EngineConstants::SHADER_FALSE;
    ubo.time = totalTime;

    // Synchronize dynamic Fire/Spark lights from the particle simulation
    if (fireParticleSystem != nullptr && inputManager->getFireEnabled()) {
        const auto sparkData = fireParticleSystem->getLightData();
        for (uint32_t i = 0U; i < EngineConstants::MAX_SPARK_LIGHTS; ++i) {
            ubo.sparks[i] = sparkData[i];
        }
    }
    else {
        for (uint32_t i = 0U; i < EngineConstants::MAX_SPARK_LIGHTS; ++i) {
            ubo.sparks[i].color = glm::vec3(0.0f);
        }
    }

    static_cast<void>(std::memcpy(mappedData, &ubo, sizeof(UniformBufferObject)));
    this->currentUBO = ubo;
}

// ========================================================================
// SECTION 6: RESOURCE MAINTENANCE & RESET
// ========================================================================

/**
 * @brief Performs a controlled shutdown of all engine components.
 */
void Experience::cleanup() {
    // Step 1: Wait for GPU to finish all pending work
    if (context && context->device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(context->device);
    }

    // Step 2: Destroy high-level systems (UI, Renderer, Managers)
    uiManager.reset();
    renderer.reset();
    assetManager.reset();
    postProcessor.reset();

    // Step 3: Destroy particle systems
    dustParticleSystem.reset();
    fireParticleSystem.reset();
    smokeParticleSystem.reset();
    rainParticleSystem.reset();
    snowParticleSystem.reset();

    // Step 4: Destroy scene-specific resources
    skybox.reset();
    skyboxTexture.reset();
    mainLight.reset();
    scene.reset();
    statsManager.reset();
    timeManager.reset();
    climateManager.reset();

    // Step 5: Clear registries and core hardware contexts
    ownedModels.clear();
    pipelines.clear();
    shaderModules.clear();
    meshes.clear();
    transparentMeshes.clear();

    if (resources) {
        resources.reset();
    }
    vulkanEngine.reset();

    // Step 6: Final OS Window destruction
    if (window != nullptr) {
        glfwDestroyWindow(window);
    }
    glfwTerminate();
    context.reset();
}

/**
 * @brief Resets the simulation and UI to initial default values.
 * Ensures particle systems are re-synchronized with the current climate season.
 */
void Experience::performFullReset() const {
    // Step 1: Reset the climate simulation to its starting time/season
    if (climateManager != nullptr) {
        climateManager->reset();
    }

    // Step 2: Reset manual UI overrides (Gouraud, Bloom, Intensity, etc.)
    if (inputManager != nullptr) {
        inputManager->resetDefaults();

        // Step 3: MANDATORY SYNC - Force the InputManager to adopt the 
        // ClimateManager's particle toggles for the current (reset) season.
        syncWeatherToggles();
    }
}

/**
 * @brief Synchronizes the InputManager (UI) toggles with the ClimateManager state.
 */
void Experience::syncWeatherToggles() const {
    if (inputManager != nullptr && climateManager != nullptr) {
        inputManager->setDustEnabled(climateManager->isDustEnabled());
        inputManager->setFireEnabled(climateManager->isFireEnabled());
        inputManager->setSmokeEnabled(climateManager->isSmokeEnabled());
        inputManager->setRainEnabled(climateManager->isRainEnabled());
        inputManager->setSnowEnabled(climateManager->isSnowEnabled());
    }
}