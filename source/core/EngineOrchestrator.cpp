#include "core/EngineOrchestrator.h"
#include "scene/GenericScenario.h"
#include "components/PhysicsComponents.h"
#include "systems/TransformSystem.h"
#include "systems/ParticleEmitterSystem.h"
#include "systems/ColliderVisualizerSystem.h"
#include "components/SkyboxComponent.h"
#include "components/ParticleComponent.h"
#include "graphics/GpuUploadContext.h"

using namespace GE::Graphics;
using namespace GE::Assets;

// ========================================================================
// SECTION 1: CONSTRUCTOR & DESTRUCTOR
// ========================================================================

EngineOrchestrator::EngineOrchestrator(const uint32_t width, const uint32_t height, char const* const title)
    : WINDOW_WIDTH(width), WINDOW_HEIGHT(height), framebufferResized(false), currentFrame(0)
{
    ServiceLocator::Provide(this);

    // --- Step 1: Hardware Foundation ---
    initWindow(title);
    context = std::make_unique<VulkanContext>();
    ServiceLocator::Provide(context.get());
    vulkanEngine = std::make_unique<VulkanDevice>(window);

    context->graphicsCommandPool = vulkanEngine->getCommandPool();
    context->msaaSamples = vulkanEngine->getMsaaSamples();

    // --- Step 2: ECS Framework Setup ---
    entityManager = std::make_unique<GE::ECS::EntityManager>();
    entityManager->Initialize(2000, 64);

    entityManager->RegisterComponent<GE::Components::Transform>();
    entityManager->RegisterComponent<GE::Components::Tag>();
    entityManager->RegisterComponent<GE::Components::MeshRenderer>();
    entityManager->RegisterComponent<GE::Components::LightComponent>();
    entityManager->RegisterComponent<GE::Components::ParticleComponent>();
    entityManager->RegisterComponent<GE::Components::SkyboxComponent>();

    entityManager->RegisterComponent<GE::Components::RigidBody>();
    entityManager->RegisterComponent<GE::Components::SphereCollider>();
    entityManager->RegisterComponent<GE::Components::PlaneCollider>();

    entityManager->RegisterComponent<GE::Components::RigidBody2D>();
    entityManager->RegisterComponent<GE::Components::CircleCollider2D>();
    entityManager->RegisterComponent<GE::Components::BoxCollider2D>();

    ServiceLocator::Provide(entityManager.get());

    // --- Step 3: Global Systems ---
    entityManager->RegisterSystem(new GE::Systems::TransformSystem());
    auto* particleSystem = new GE::Systems::ParticleEmitterSystem();
    entityManager->RegisterSystem(particleSystem);
    ServiceLocator::Provide(particleSystem);

    // --- Step 4: Engine Infrastructure ---
    resources = std::make_unique<GpuResourceManager>();
    ServiceLocator::Provide(resources.get());

    // AGNOSTIC FIX: EngineServiceRegistry is kept for logic, but Particle Recipes are REMOVED.
    // The SceneLoader now builds ParticleSystems directly from .ini shader paths.
    systemFactory = std::make_unique<EngineServiceRegistry>();
    ServiceLocator::Provide(systemFactory.get());

    timeManager = std::make_unique<TimeService>();
    ServiceLocator::Provide(timeManager.get());
    statsManager = std::make_unique<PerformanceTracker>();
    assetManager = std::make_unique<AssetManager>();
    ServiceLocator::Provide(assetManager.get());

    imagesInFlight.resize(vulkanEngine->getSwapChainImageCount(), VK_NULL_HANDLE);
    resources->init(vulkanEngine.get(), MAX_FRAMES_IN_FLIGHT);
    assetManager->setDescriptorPool(resources->getDescriptorPool());

    // --- Step 5: High-Level Orchestrators ---
    postProcessor = std::make_unique<PostProcessBackend>(
        vulkanEngine->getSwapChainExtent().width, vulkanEngine->getSwapChainExtent().height,
        vulkanEngine->getSwapChainFormat(), vulkanEngine->getFinalRenderPass(), vulkanEngine->getMsaaSamples()
    );
    postProcessor->resize(vulkanEngine->getSwapChainExtent());

    renderer = std::make_unique<Renderer>();
    scene = std::make_unique<GE::Scene::Scene>();
    ServiceLocator::Provide(scene.get());
    inputManager = std::make_unique<InputService>(window, timeManager.get());
    ServiceLocator::Provide(inputManager.get());
    uiManager = std::make_unique<DebugOverlay>();
    climateManager = std::make_unique<ClimateService>();

    // --- Step 6: Final Boot Sequence ---
    initVulkan();
    uiManager->init(window, vulkanEngine.get());

    // Create the empty skybox shell (waiting for .ini textures)
    initSkybox();

    changeScenario(std::make_unique<GE::GenericScenario>("./config/snow_globe.ini"));
}

/**
 * @brief Destructor: Triggers a safe and orderly shutdown.
 */
EngineOrchestrator::~EngineOrchestrator() {
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
void EngineOrchestrator::initVulkan() {
    // Shadow pipeline is engine-lifetime (shared across all scenarios)
    m_shadowVert = std::make_unique<GE::Graphics::ShaderModule>("./shaders/shadow_vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    m_shadowFrag = std::make_unique<GE::Graphics::ShaderModule>("./shaders/shadow_frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    m_shadowPipeline = std::make_unique<GE::Graphics::GraphicsPipeline>(
        resources->getShadowRenderPass(), context->materialSetLayout,
        m_shadowVert.get(), m_shadowFrag.get(),
        true, true, true, VK_SAMPLE_COUNT_1_BIT
    );

    postProcessor->createPipeline(vulkanEngine->getFinalRenderPass());
    resources->updateDescriptorSets(vulkanEngine.get(), postProcessor.get());
}

/**
 * @brief Enters the main execution loop.
 */
void EngineOrchestrator::run() {
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
void EngineOrchestrator::drawFrame() {
    VulkanContext* const ctx = ServiceLocator::GetContext();

    if (!m_pendingScenarioPath.empty()) {
        vkDeviceWaitIdle(ctx->device);

        // Use a local copy to avoid issues during the change
        std::string path = m_pendingScenarioPath;
        m_pendingScenarioPath = "";

        changeScenario(std::make_unique<GE::GenericScenario>(path));

        // CRITICAL: We MUST return here to ensure we don't proceed to draw 
        // with empty registries or stale command buffers!
        return;
    }

    auto* const em = entityManager.get();

    // --- Step 1: CPU-GPU Throttling ---
    FrameSyncManager* const sync = resources->getSyncManager();
    const VkFence currentFence = sync->getInFlightFence(currentFrame);
    static_cast<void>(vkWaitForFences(ctx->device, 1U, &currentFence, VK_TRUE, UINT64_MAX));

    const float dt = timeManager->getDelta();
    const float totalTime = timeManager->getTotal();

    // --- Step 2: Input & Scenario Script Updates (No GPU Commands) ---
    inputManager->update(dt);

    // Scaling delta for scenario timescale logic
    const float scaledDelta = (activeScenario && !activeScenario->IsPaused()) ?
        dt * activeScenario->GetTimeScale() : 0.0f;

    if (activeScenario && !activeScenario->IsPaused()) {
        // Run scenario-specific logic scripts first
        activeScenario->OnUpdate(scaledDelta, totalTime);
    }

    // --- Step 3: Swapchain Acquisition ---
    uint32_t imageIndex{ 0U };
    const VkResult acquireResult = vkAcquireNextImageKHR(
        ctx->device, vulkanEngine->getSwapChain(), UINT64_MAX,
        sync->getImageAvailableSemaphore(currentFrame), VK_NULL_HANDLE, &imageIndex
    );

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

    // --- Step 4: Command Buffer Recording (GPU WORK BEGINS) ---
    updateUniformBuffer(imageIndex);

    const VkCommandBuffer cb = sync->getCommandBuffer(currentFrame);
    static_cast<void>(vkResetCommandBuffer(cb, 0U));

    const VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    static_cast<void>(vkBeginCommandBuffer(cb, &beginInfo)); // <--- BUFFER IS NOW OPEN

    // CRITICAL FIX: Trigger ECS Systems that record GPU commands (Particles) 
    // only while the buffer is in the 'Recording' state.
    if (activeScenario && !activeScenario->IsPaused()) {
        // This triggers ParticleEmitterSystem::OnUpdate which now has a valid 'cb'.
        em->Update(scaledDelta, cb);
    }

    // Collect scenario-scoped material pipelines for this frame
    std::vector<GraphicsPipeline*> rawPipelines;
    if (activeScenario) {
        for (const auto& p : activeScenario->GetPipelines()) {
            rawPipelines.push_back(p.get());
        }
    }

    // Extract per-scenario rendering overrides.
    const glm::vec4 clearColor = activeScenario
        ? activeScenario->GetClearColor()
        : glm::vec4{ 0.0f, 0.0f, 0.0f, 1.0f };

    const GE::Graphics::GraphicsPipeline* checkerPipeline = activeScenario
        ? activeScenario->GetCheckerboardPipeline() : nullptr;
    const void*  checkerPushData = activeScenario
        ? activeScenario->GetCheckerboardPushData() : nullptr;
    const uint32_t checkerPushSize = activeScenario
        ? activeScenario->GetCheckerboardPushDataSize() : 0U;

    const GE::Graphics::GraphicsPipeline* wirePipeline = activeScenario
        ? activeScenario->GetWirePipeline() : nullptr;
    GE::Systems::ColliderVisualizerSystem* visualizer = activeScenario
        ? activeScenario->GetVisualizerSystem() : nullptr;

    // Record Draw Calls: Renderer queries ECS for meshes/particles
    renderer->recordFrame(
        cb, vulkanEngine->getSwapChainExtent(), skybox.get(),
        em, postProcessor.get(), resources->getDescriptorSet(imageIndex),
        resources->getShadowRenderPass(), resources->getShadowFramebuffer(),
        rawPipelines, m_shadowPipeline.get(),
        clearColor, checkerPipeline, checkerPushData, checkerPushSize,
        wirePipeline, visualizer
    );

    // --- Step 5: UI & Final Render Pass ---
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

    uiManager->update(inputManager.get(), statsManager.get(), nullptr, timeManager.get(), climateManager.get());
    uiManager->draw(cb);

    vkCmdEndRenderPass(cb);
    static_cast<void>(vkEndCommandBuffer(cb)); // <--- BUFFER IS NOW CLOSED

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
        throw std::runtime_error("EngineOrchestrator: Queue submit failed!");
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
void EngineOrchestrator::updateUniformBuffer(const uint32_t currentImage) {
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
        auto* const trans = em->GetTIComponent<GE::Components::Transform>(lightEntityID);

        if (trans) {
            ubo.lightPos = trans->m_position;
            ubo.lightColor = lightComp.color * lightComp.intensity * inputManager->getColorMod();

            // Compute light-space matrix for shadow mapping
            const glm::vec3 shadowTarget{ 0.0f, 0.0f, 0.0f };
            const glm::vec3 lightDir = glm::normalize(shadowTarget - trans->m_position);
            const glm::vec3 up = (glm::abs(lightDir.x) < 0.001f && glm::abs(lightDir.z) < 0.001f)
                ? glm::vec3(0.0f, 0.0f, 1.0f)
                : glm::vec3(0.0f, 1.0f, 0.0f);
            const glm::mat4 lightView = glm::lookAt(trans->m_position, shadowTarget, up);
            glm::mat4 lightProj = glm::ortho(-30.0f, 30.0f, -30.0f, 30.0f, 1.0f, 100.0f);
            lightProj[1][1] *= -1.0f; // Vulkan NDC Y-flip
            ubo.lightSpaceMatrix = lightProj * lightView;
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
void EngineOrchestrator::changeScenario(std::unique_ptr<GE::Scenario> newScenario) {
    if (activeScenario) {
        activeScenario->OnUnload();

        // --- NEW: Reset Skybox state on scenario change ---
        if (skybox != nullptr) {
            // If your Skybox class doesn't have a 'clear' method, 
            // you can simply reset the unique_ptr and re-init an empty shell.
            initSkybox();
        }

        scene->clearEntities();
        if (entityManager) {
            entityManager->ClearAllEntities();
        }
    }

    activeScenario = std::move(newScenario);

    if (activeScenario) {
        GE::Graphics::GpuUploadContext ctx;
        ctx.cmd = VulkanUtils::beginSingleTimeCommands(context->device, context->graphicsCommandPool);

        // Data-driven load: Populates ECS from the .ini path provided to the scenario
        activeScenario->OnLoad(ctx);

        VulkanUtils::endSingleTimeCommands(context->device, context->graphicsCommandPool, context->graphicsQueue, ctx.cmd);

        for (size_t i = 0U; i < ctx.stagingBuffers.size(); ++i) {
            vkDestroyBuffer(context->device, ctx.stagingBuffers[i], nullptr);
            vkFreeMemory(context->device, ctx.stagingMemories[i], nullptr);
        }
    }
}

/** * @brief Fulfills Requirement: Debug simulation stepping.
 */
void EngineOrchestrator::stepSimulation(float fixedStep) {
    if (activeScenario && activeScenario->IsPaused()) {
        // Fix: Pass VK_NULL_HANDLE because there is no active frame buffer during a manual step
        entityManager->Update(fixedStep, VK_NULL_HANDLE);
        activeScenario->OnUpdate(fixedStep, timeManager->getTotal());

        GE_LOG_INFO("EngineOrchestrator: Manual simulation step performed.");
    }
}

// ========================================================================
// SECTION 6: TEARDOWN
// ========================================================================

void EngineOrchestrator::cleanup() {
    // 1. Synchronize GPU
    if (context && context->device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(context->device);
    }

    // 2. Destroy High-Level Logic
    if (activeScenario) {
        activeScenario->OnUnload();
    }
    activeScenario.reset();

    // NEW: Explicitly destroy the Skybox while the Context/Device is still alive!
    skybox.reset();

    // 3. Destroy ECS and Components
    // This triggers ParticleSystem destructors
    entityManager.reset();

    // 4. Release orchestrators
    uiManager.reset();
    renderer.reset();
    assetManager.reset();
    postProcessor.reset();
    scene.reset();

    // 5. Engine-scoped shadow pipeline
    m_shadowPipeline.reset();
    m_shadowVert.reset();
    m_shadowFrag.reset();

    // 6. Managers
    statsManager.reset();
    timeManager.reset();
    climateManager.reset();

    // 7. Resources
    resources.reset();
    vulkanEngine.reset();

    if (window != nullptr) {
        glfwDestroyWindow(window);
    }
    glfwTerminate();

    // 8. Foundation dies LAST
    context.reset();

    GE_LOG_INFO("EngineOrchestrator: Agnostic cleanup complete.");
}

// ========================================================================
// SECTION 7: WINDOWING & STATIC CALLBACKS
// ========================================================================

/**
 * @brief Configures the OS window for Vulkan via GLFW.
 */
void EngineOrchestrator::initWindow(char const* const title) {
    if (glfwInit() == GLFW_FALSE) {
        throw std::runtime_error("EngineOrchestrator: Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // No OpenGL
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window = glfwCreateWindow(static_cast<int>(WINDOW_WIDTH), static_cast<int>(WINDOW_HEIGHT), title, nullptr, nullptr);
    if (window == nullptr) {
        throw std::runtime_error("EngineOrchestrator: Failed to create GLFW window");
    }

    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetCursorPosCallback(window, mouseCallback);
}

void EngineOrchestrator::framebufferResizeCallback(GLFWwindow* pWindow, int width, int height) {
    auto* const app = reinterpret_cast<EngineOrchestrator*>(glfwGetWindowUserPointer(pWindow));
    if (app != nullptr) {
        app->framebufferResized = true;
    }
}

void EngineOrchestrator::keyCallback(GLFWwindow* pWindow, int key, int scancode, int action, int mods) {
    auto* const app = reinterpret_cast<EngineOrchestrator*>(glfwGetWindowUserPointer(pWindow));
    if ((app != nullptr) && (app->inputManager != nullptr)) {
        app->inputManager->handleKeyEvent(key, scancode, action, mods);
    }
}

void EngineOrchestrator::mouseCallback(GLFWwindow* pWindow, double xpos, double ypos) {
    auto* const app = reinterpret_cast<EngineOrchestrator*>(glfwGetWindowUserPointer(pWindow));
    if ((app != nullptr) && (app->inputManager != nullptr)) {
        app->inputManager->handleMouseEvent(xpos, ypos);
    }
}

// ========================================================================
// SECTION 8: GRAPHICS & ASSET HELPERS
// ========================================================================

/**
 * @brief Initializes the environmental skybox.
 */
void EngineOrchestrator::initSkybox() {
    skybox = std::make_unique<Skybox>(
        postProcessor->getOffscreenRenderPass(),
        nullptr, // No texture yet
        context->msaaSamples
    );
}