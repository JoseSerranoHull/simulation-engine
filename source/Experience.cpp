#include "../include/Experience.h"
#include "../include/GenericScenario.h"
#include "../include/PhysicsComponents.h"
#include "../include/TransformSystem.h"

// Note: ParticleUpdateSystem would be included here once implemented
// #include "../include/ParticleUpdateSystem.h" 

// ========================================================================
// SECTION 1: CONSTRUCTOR & DESTRUCTOR
// ========================================================================

/**
 * @brief Constructor: Orchestrates the initialization of the agnostic engine.
 * Establishing a strict initialization order is critical for Vulkan stability.
 */
Experience::Experience(const uint32_t width, const uint32_t height, char const* const title)
    : WINDOW_WIDTH(width), WINDOW_HEIGHT(height), framebufferResized(false), currentFrame(0)
{
    ServiceLocator::Provide(this);

    // --- Step 1: Hardware Foundation ---
    initWindow(title);
    context = std::make_unique<VulkanContext>();
    ServiceLocator::Provide(context.get());

    // Initialize core hardware (Instance, Device, Queues, Pools)
    vulkanEngine = std::make_unique<VulkanEngine>(window);

    // CRITICAL: Propagate handles from Hardware to the Central Context
    // Fixes the "Access Violation" in PostProcessor/SceneLoader by providing a valid Command Pool
    context->graphicsCommandPool = vulkanEngine->getCommandPool();
    context->msaaSamples = vulkanEngine->getMsaaSamples();

    // --- Step 2: ECS Framework Setup ---
    entityManager = std::make_unique<GE::ECS::EntityManager>();
    entityManager->Initialize(2000, 64);

    // Complete Component Registration
    // These must be registered BEFORE loading any scenario .ini
    entityManager->RegisterComponent<GE::Scene::Components::Transform>();
    entityManager->RegisterComponent<GE::Scene::Components::Tag>();
    entityManager->RegisterComponent<GE::Components::MeshRenderer>();
    entityManager->RegisterComponent<GE::Components::LightComponent>();

    // 3D Physics components
    entityManager->RegisterComponent<GE::Components::RigidBody>();
    entityManager->RegisterComponent<GE::Components::SphereCollider>();
    entityManager->RegisterComponent<GE::Components::PlaneCollider>();

    // 2D Physics components (Future-proofing)
    entityManager->RegisterComponent<GE::Components::RigidBody2D>();
    entityManager->RegisterComponent<GE::Components::CircleCollider2D>();
    entityManager->RegisterComponent<GE::Components::BoxCollider2D>();

    // Particle-to-ECS Migration: Particles are now generic components
    // entityManager->RegisterComponent<GE::Components::ParticleComponent>();

    ServiceLocator::Provide(entityManager.get());

    // --- Step 3: Global Systems (Universal logic) ---
    // Transform hierarchy resolution is required for all scenarios
    entityManager->RegisterSystem(new GE::Systems::TransformSystem());

    // Future: Add ParticleUpdateSystem here to handle all particle types generically
    // entityManager->RegisterSystem(new GE::Systems::ParticleUpdateSystem());

    // --- Step 4: Engine Infrastructure ---
    resources = std::make_unique<VulkanResourceManager>();
    ServiceLocator::Provide(resources.get());
    systemFactory = std::make_unique<SystemFactory>();
    ServiceLocator::Provide(systemFactory.get());

    timeManager = std::make_unique<TimeManager>();
    statsManager = std::make_unique<StatsManager>();
    assetManager = std::make_unique<AssetManager>();
    ServiceLocator::Provide(assetManager.get());

    // Initialize synchronization tracking for the swapchain images
    // Fixes the "Vector subscript out of range" crash in drawFrame
    imagesInFlight.resize(vulkanEngine->getSwapChainImageCount(), VK_NULL_HANDLE);

    resources->init(vulkanEngine.get(), MAX_FRAMES_IN_FLIGHT);
    assetManager->setDescriptorPool(resources->getDescriptorPool());

    // --- Step 5: High-Level Orchestrators ---
    // PostProcessor must exist before initVulkan() creates graphics pipelines
    postProcessor = std::make_unique<PostProcessor>(
        vulkanEngine->getSwapChainExtent().width,
        vulkanEngine->getSwapChainExtent().height,
        vulkanEngine->getSwapChainFormat(),
        vulkanEngine->getFinalRenderPass(),
        vulkanEngine->getMsaaSamples()
    );
    postProcessor->resize(vulkanEngine->getSwapChainExtent());

    renderer = std::make_unique<Renderer>();
    scene = std::make_unique<GE::Scene::Scene>();
    ServiceLocator::Provide(scene.get());

    inputManager = std::make_unique<InputManager>(window, timeManager.get());
    ServiceLocator::Provide(inputManager.get());

    uiManager = std::make_unique<IMGUIManager>();
    climateManager = std::make_unique<ClimateManager>();

    // --- Step 6: Final Boot Sequence ---
    initVulkan();
    uiManager->init(window, vulkanEngine.get());

    // Initial scenario load (Data-driven and agnostic)
    changeScenario(std::make_unique<GE::GenericScenario>("./config/snow_globe.ini"));
}

/**
 * @brief Destructor: Triggers a safe and orderly shutdown.
 */
Experience::~Experience() {
    try {
        cleanup();
    }
    catch (...) {
        // Suppress errors during shutdown
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

    // Ensure GPU is idle before resource teardown
    if ((context != nullptr) && (context->device != VK_NULL_HANDLE)) {
        static_cast<void>(vkDeviceWaitIdle(context->device));
    }
}

// ========================================================================
// SECTION 3: FRAME LOGIC
// ========================================================================

/**
 * @brief Master frame orchestration: Agnostic version.
 * Now delegates particle updates to ECS systems and uses the Scenario timescale.
 */
void Experience::drawFrame() {
    VulkanContext* const ctx = ServiceLocator::GetContext();
    auto* const em = entityManager.get();

    // --- Step 1: CPU-GPU Throttling ---
    SyncManager* const sync = resources->getSyncManager();
    const VkFence currentFence = sync->getInFlightFence(currentFrame);
    static_cast<void>(vkWaitForFences(ctx->device, 1U, &currentFence, VK_TRUE, UINT64_MAX));

    const float dt = timeManager->getDelta();
    const float totalTime = timeManager->getTotal();

    // --- Step 2: Agnostic Simulation Updates ---
    inputManager->update(dt);

    // Fulfills Lab Requirement: Simulation Start/Pause/Timestep
    if (activeScenario && !activeScenario->IsPaused()) {
        const float scaledDelta = dt * activeScenario->GetTimeScale();

        // This heartbeat triggers ALL registered systems (Physics, Transforms, Particles)
        em->Update(scaledDelta);

        // Scenario-specific custom script logic
        activeScenario->OnUpdate(scaledDelta, totalTime);
    }

    // --- Step 3: Swapchain Acquisition ---
    uint32_t imageIndex{ 0U };
    const VkResult acquireResult = vkAcquireNextImageKHR(
        ctx->device, vulkanEngine->getSwapChain(), UINT64_MAX,
        sync->getImageAvailableSemaphore(currentFrame), VK_NULL_HANDLE, &imageIndex
    );

    // Handle Window Resizing
    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
        vulkanEngine->recreateSwapChain(window);
        postProcessor->resize(vulkanEngine->getSwapChainExtent());
        resources->updateDescriptorSets(vulkanEngine.get(), postProcessor.get());
        imagesInFlight.resize(vulkanEngine->getSwapChainImageCount(), VK_NULL_HANDLE);
        return;
    }

    if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        static_cast<void>(vkWaitForFences(ctx->device, 1U, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX));
    }
    imagesInFlight[imageIndex] = sync->getInFlightFence(currentFrame);

    const VkFence frameFence = sync->getInFlightFence(currentFrame);
    static_cast<void>(vkResetFences(ctx->device, 1U, &frameFence));

    // --- Step 4: Command Buffer Recording ---
    updateUniformBuffer(imageIndex);

    const VkCommandBuffer cb = sync->getCommandBuffer(currentFrame);
    static_cast<void>(vkResetCommandBuffer(cb, 0U));

    const VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    static_cast<void>(vkBeginCommandBuffer(cb, &beginInfo));

    // Prepare raw pipelines for renderer
    std::vector<Pipeline*> rawPipelines;
    for (const auto& p : pipelines) rawPipelines.push_back(p.get());

    // Record Draw Calls: Particles are now Agnostic (Renderer queries ECS)
    renderer->recordFrame(
        cb, vulkanEngine->getSwapChainExtent(), skybox.get(),
        em, // NEW: Pass EntityManager so the renderer can find lights and particles
        postProcessor.get(), resources->getDescriptorSet(imageIndex),
        resources->getShadowRenderPass(), resources->getShadowFramebuffer(),
        rawPipelines
    );

    // --- Step 5: UI & Final Pass ---
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

    // UI Manager draws the Main Menu Bar and delegates to activeScenario->OnGUI()
    uiManager->update(inputManager.get(), statsManager.get(), nullptr, timeManager.get(), climateManager.get());
    uiManager->draw(cb);

    vkCmdEndRenderPass(cb);
    static_cast<void>(vkEndCommandBuffer(cb));

    // --- Step 6: Submission & Presentation ---
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

    if (vkQueueSubmit(ctx->graphicsQueue, 1U, &submitInfo, frameFence) != VK_SUCCESS) {
        throw std::runtime_error("Experience: Queue submit failed!");
    }

    VkPresentInfoKHR presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    presentInfo.waitSemaphoreCount = 1U;
    presentInfo.pWaitSemaphores = &signalSem;
    presentInfo.swapchainCount = 1U;
    VkSwapchainKHR swapchains[] = { vulkanEngine->getSwapChain() };
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;

    const VkResult presentResult = vkQueuePresentKHR(ctx->presentQueue, &presentInfo);

    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR || framebufferResized) {
        framebufferResized = false;
        vulkanEngine->recreateSwapChain(window);
        postProcessor->resize(vulkanEngine->getSwapChainExtent());
        resources->updateDescriptorSets(vulkanEngine.get(), postProcessor.get());
    }

    currentFrame = (currentFrame + 1U) % MAX_FRAMES_IN_FLIGHT;
}

// ========================================================================
// SECTION 4: UNIFORM DATA & ECS QUERIES
// ========================================================================

/**
 * @brief Syncs CPU simulation state to the GPU Uniform Buffer.
 * Now queries the ECS for lighting data rather than using a hardcoded pointer.
 */
void Experience::updateUniformBuffer(const uint32_t currentImage) {
    void* const mappedData = resources->getMappedBuffer(currentImage);
    if (mappedData == nullptr) return;

    auto* const em = ServiceLocator::GetEntityManager();
    const float totalTime = timeManager->getTotal();
    const VkExtent2D extent = vulkanEngine->getSwapChainExtent();
    const float aspect = static_cast<float>(extent.width) / static_cast<float>(extent.height);
    Camera* const activeCam = inputManager->getActiveCamera();

    UniformBufferObject ubo{};
    ubo.view = activeCam->getViewMatrix();
    ubo.proj = activeCam->getProjectionMatrix(aspect);
    ubo.proj[1][1] *= -1.0f; // Vulkan NDC Correction
    ubo.viewPos = activeCam->getPosition();
    ubo.time = totalTime;

    // --- AGNOSTIC LIGHT LOOKUP ---
    // Fulfills Requirement: Data-driven lighting
    auto& lightArray = em->GetCompArr<GE::Components::LightComponent>();

    if (lightArray.GetCount() > 0) {
        // Find the first light defined in the current scenario
        const uint32_t lightEntityID = lightArray.Index()[0];
        const auto& lightComp = lightArray.Data()[0];
        auto* const trans = em->GetTIComponent<GE::Scene::Components::Transform>(lightEntityID);

        if (trans) {
            ubo.lightPos = trans->m_position;
            ubo.lightColor = lightComp.color * lightComp.intensity * inputManager->getColorMod();
        }
    }
    else {
        // Standard engine fallback if no light is present in the scenario
        ubo.lightPos = glm::vec3(0.0f, 10.0f, 0.0f);
        ubo.lightColor = glm::vec3(1.0f);
    }

    static_cast<void>(std::memcpy(mappedData, &ubo, sizeof(UniformBufferObject)));
    this->currentUBO = ubo;
}

// ========================================================================
// SECTION 5: SCENARIO LIFECYCLE
// ========================================================================

/**
 * @brief Transitions the engine to a new state defined by a configuration file.
 * Safely flushes the ECS and Scene registry before loading new data.
 */
void Experience::changeScenario(std::unique_ptr<GE::Scenario> newScenario) {
    if (activeScenario) {
        activeScenario->OnUnload();

        // Requirement: Clear registries to prevent ID conflicts
        scene->clearEntities();
        if (entityManager) {
            entityManager->ClearAllEntities();
        }
    }

    activeScenario = std::move(newScenario);

    if (activeScenario) {
        const VkCommandBuffer setupCmd = VulkanUtils::beginSingleTimeCommands(context->device, context->graphicsCommandPool);
        std::vector<VkBuffer> sb;
        std::vector<VkDeviceMemory> sm;

        // Data-driven load: Populates ECS from the .ini path provided to the scenario
        activeScenario->OnLoad(setupCmd, sb, sm);

        VulkanUtils::endSingleTimeCommands(context->device, context->graphicsCommandPool, context->graphicsQueue, setupCmd);

        for (size_t i = 0U; i < sb.size(); ++i) {
            vkDestroyBuffer(context->device, sb[i], nullptr);
            vkFreeMemory(context->device, sm[i], nullptr);
        }
    }
}

/** * @brief Fulfills Requirement: Debug simulation stepping.
 */
void Experience::stepSimulation(float fixedStep) {
    if (activeScenario && activeScenario->IsPaused()) {
        // Force one heartbeat of the ECS and script logic
        entityManager->Update(fixedStep);
        activeScenario->OnUpdate(fixedStep, timeManager->getTotal());

        GE_LOG_INFO("Experience: Manual simulation step performed.");
    }
}

// ========================================================================
// SECTION 6: TEARDOWN
// ========================================================================

void Experience::cleanup() {
    if (context && context->device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(context->device);
    }

    if (activeScenario) {
        activeScenario->OnUnload();
    }
    activeScenario.reset();

    uiManager.reset();
    renderer.reset();
    assetManager.reset();
    postProcessor.reset();
    scene.reset();

    // registries
    ownedModels.clear();
    pipelines.clear();
    shaderModules.clear();
    meshes.clear();

    entityManager.reset();

    statsManager.reset();
    timeManager.reset();
    climateManager.reset();

    resources.reset();
    vulkanEngine.reset();

    if (window != nullptr) {
        glfwDestroyWindow(window);
    }
    glfwTerminate();
    context.reset();

    GE_LOG_INFO("Experience: Agnostic cleanup complete.");
}

// ========================================================================
// SECTION 7: WINDOWING & STATIC CALLBACKS
// ========================================================================

/**
 * @brief Configures the OS window for Vulkan via GLFW.
 */
void Experience::initWindow(char const* const title) {
    if (glfwInit() == GLFW_FALSE) {
        throw std::runtime_error("Experience: Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // No OpenGL
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window = glfwCreateWindow(static_cast<int>(WINDOW_WIDTH), static_cast<int>(WINDOW_HEIGHT), title, nullptr, nullptr);
    if (window == nullptr) {
        throw std::runtime_error("Experience: Failed to create GLFW window");
    }

    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetCursorPosCallback(window, mouseCallback);
}

void Experience::framebufferResizeCallback(GLFWwindow* pWindow, int width, int height) {
    auto* const app = reinterpret_cast<Experience*>(glfwGetWindowUserPointer(pWindow));
    if (app != nullptr) {
        app->framebufferResized = true;
    }
}

void Experience::keyCallback(GLFWwindow* pWindow, int key, int scancode, int action, int mods) {
    auto* const app = reinterpret_cast<Experience*>(glfwGetWindowUserPointer(pWindow));
    if ((app != nullptr) && (app->inputManager != nullptr)) {
        app->inputManager->handleKeyEvent(key, scancode, action, mods);
    }
}

void Experience::mouseCallback(GLFWwindow* pWindow, double xpos, double ypos) {
    auto* const app = reinterpret_cast<Experience*>(glfwGetWindowUserPointer(pWindow));
    if ((app != nullptr) && (app->inputManager != nullptr)) {
        app->inputManager->handleMouseEvent(xpos, ypos);
    }
}

// ========================================================================
// SECTION 8: GRAPHICS & ASSET HELPERS
// ========================================================================

/**
 * @brief Initializes graphics pipelines for Opaque, Transparent, and Shadow passes.
 */
void Experience::createGraphicsPipelines() {
    VulkanContext* ctx = ServiceLocator::GetContext();

    if (postProcessor == nullptr) {
        throw std::runtime_error("Experience: Cannot create pipelines, PostProcessor is null!");
    }

    const VkRenderPass offscreenPass = postProcessor->getOffscreenRenderPass();
    const VkRenderPass transPass = postProcessor->getTransparentRenderPass();
    const VkSampleCountFlagBits msaa = ctx->msaaSamples;

    shaderModules.clear();
    pipelines.clear();

    // 1. Load Agnostic Shader Modules
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

    // 2. Instantiate Pipelines in RAII container
    // Opaque
    pipelines.push_back(std::make_unique<Pipeline>(offscreenPass, ctx->materialSetLayout, shaderModules[0].get(), shaderModules[1].get(), true, true, true, msaa));
    pipelines.push_back(std::make_unique<Pipeline>(offscreenPass, ctx->materialSetLayout, shaderModules[0].get(), shaderModules[2].get(), true, true, true, msaa));
    pipelines.push_back(std::make_unique<Pipeline>(offscreenPass, ctx->materialSetLayout, shaderModules[0].get(), shaderModules[3].get(), true, true, true, msaa));

    // Transparent (depthWrite = FALSE)
    pipelines.push_back(std::make_unique<Pipeline>(transPass, ctx->materialSetLayout, shaderModules[0].get(), shaderModules[4].get(), true, false, false, msaa));
    pipelines.push_back(std::make_unique<Pipeline>(offscreenPass, ctx->materialSetLayout, shaderModules[0].get(), shaderModules[5].get(), false, false, true, msaa));
    pipelines.push_back(std::make_unique<Pipeline>(transPass, ctx->materialSetLayout, shaderModules[6].get(), shaderModules[7].get(), true, false, false, msaa));

    // Shadow Map (1x Sample Count required)
    pipelines.push_back(std::make_unique<Pipeline>(resources->getShadowRenderPass(), ctx->materialSetLayout,
        shaderModules[8].get(), shaderModules[9].get(), true, true, true, VK_SAMPLE_COUNT_1_BIT));

    postProcessor->createPipeline(vulkanEngine->getFinalRenderPass());
}

/**
 * @brief Initializes the environmental skybox.
 */
void Experience::initSkybox() {
    const std::vector<std::string> skyboxFaces = {
        "./textures/cubemaps/cubemap_0(+X).jpg", "./textures/cubemaps/cubemap_1(-X).jpg",
        "./textures/cubemaps/cubemap_2(+Y).jpg", "./textures/cubemaps/cubemap_3(-Y).jpg",
        "./textures/cubemaps/cubemap_4(+Z).jpg", "./textures/cubemaps/cubemap_5(-Z).jpg"
    };

    skyboxTexture = std::make_unique<Cubemap>(skyboxFaces);
    skybox = std::make_unique<Skybox>(postProcessor->getOffscreenRenderPass(), skyboxTexture.get(), context->msaaSamples);
}