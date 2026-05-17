// Pull in only the Win32 threading API (SetThreadAffinityMask, GetCurrentThread).
// NOGDI: prevents wingdi.h from #define-ing DEFAULT_PITCH, FIXED_PITCH etc.
//        which would clash with Camera.h's static constexpr members.
// NOMINMAX: prevents min/max macro conflicts with std::min / std::max.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOGDI
#define NOGDI
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
/* parasoft-begin-suppress ALL */
#include <windows.h>
/* parasoft-end-suppress ALL */

#include "core/EngineOrchestrator.h"
#include "scene/GenericScenario.h"
#include "scene/FlatBuffersScenario.h"
#include "components/PhysicsComponents.h"
#include "components/AnimationComponent.h"
#include "components/SpawnerComponent.h"
#include "components/ClothComponent.h"
#include "components/FlockingComponent.h"
#include "components/ScriptComponent.h"
#include "systems/TransformSystem.h"
#include "systems/ParticleEmitterSystem.h"
#include "systems/ColliderVisualizerSystem.h"
#include "components/SkyboxComponent.h"
#include "components/ParticleComponent.h"
#include "graphics/GpuUploadContext.h"
#include "core/NetworkBridge.h"

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
    entityManager->RegisterComponent<GE::Components::CylinderCollider>();
    entityManager->RegisterComponent<GE::Components::BoxCollider>();
    entityManager->RegisterComponent<GE::Components::CapsuleCollider>();
    entityManager->RegisterComponent<GE::Components::OwnerComponent>();
    entityManager->RegisterComponent<GE::Components::AnimatedObjectComponent>();
    entityManager->RegisterComponent<GE::Components::SpawnerComponent>();
    entityManager->RegisterComponent<GE::Components::PhysicsMaterialTag>();

    entityManager->RegisterComponent<GE::Components::RigidBody2D>();
    entityManager->RegisterComponent<GE::Components::CircleCollider2D>();
    entityManager->RegisterComponent<GE::Components::BoxCollider2D>();

    entityManager->RegisterComponent<GE::Components::ScriptComponent>();
    entityManager->RegisterComponent<GE::Components::ClothComponent>();
    entityManager->RegisterComponent<GE::Components::FlockingComponent>();

    ServiceLocator::Provide(entityManager.get());

    // --- Step 3: Global Systems ---
    entityManager->RegisterSystem(new GE::Systems::TransformSystem());
    auto* particleSystem = new GE::Systems::ParticleEmitterSystem();
    entityManager->RegisterSystem(particleSystem);
    ServiceLocator::Provide(particleSystem);

    // Lab 7: spring force generator — runs before PhysicsSystem each frame.
    m_springSystem = std::make_unique<GE::Systems::SpringSystem>();
    ServiceLocator::Provide(m_springSystem.get());

    // --- Stage 3.0: Networking Foundation ---
    // NetworkService is constructed here; Init() is deferred until after
    // the user chooses a port in the ImGui "Network" menu.
    m_networkService = std::make_unique<GE::Networking::NetworkService>();
    m_networkBridge  = std::make_unique<GE::NetworkBridge>(
        m_networkService.get(), entityManager.get());
    ServiceLocator::Provide(m_networkBridge.get());

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

    changeScenario(std::make_unique<GE::FlatBuffersScenario>("./config/flatbufferConfig/showcases/01_multiplayer.bin"));
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
 * @brief Enters the multi-threaded execution loop.
 *
 * Thread map — spec uses 1-indexed cores; masks use 0-indexed bits:
 *   Core 1 (0x01, bit 0) — main thread:       GLFW poll + Vulkan render + ImGui
 *   Core 2–3 (0x06, bits 1–2) — networking:   UDP poll + NetworkBridge::ApplyReceivedState
 *   Core 4   (0x08, bit 3)    — simulation:    fixed-timestep accumulator + ECS physics
 */
void EngineOrchestrator::run() {
    // --- Spawn physics thread (Core 4, affinity bit 3) ---
    m_physicsThread = std::jthread([this](std::stop_token st) { runPhysicsLoop(st); });
    SetThreadAffinityMask(
        reinterpret_cast<HANDLE>(m_physicsThread.native_handle()), 0x08);

    // --- Spawn networking thread (Cores 2-3, bits 1-2) ---
    m_networkingThread = std::jthread([this](std::stop_token st) {
        GE_LOG_INFO("NetworkingThread started, CPU " + std::to_string(GetCurrentProcessorNumber()));
        while (!st.stop_requested()) {
            if (m_networkBridge != nullptr) {
                m_networkBridge->GetService()->Poll(
                    [this](uint8_t senderId, const uint8_t* data, std::size_t size,
                           uint32_t senderAddr, uint16_t senderPort) {
                        m_networkBridge->ApplyReceivedState(senderId, data, size,
                                                            senderAddr, senderPort);
                    });
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        GE_LOG_INFO("NetworkingThread stopped.");
    });
    SetThreadAffinityMask(
        reinterpret_cast<HANDLE>(m_networkingThread.native_handle()), 0x06);

    // --- Pin main thread to Core 1 (bit 0) ---
    SetThreadAffinityMask(GetCurrentThread(), 0x01);
    GE_LOG_INFO("MainThread on CPU " + std::to_string(GetCurrentProcessorNumber()));

    using Clock = std::chrono::steady_clock;
    auto frameStart = Clock::now();

    // --- Graphics loop ---
    while (glfwWindowShouldClose(window) == GLFW_FALSE) {
        glfwPollEvents();
        timeManager->update();
        statsManager->update(timeManager->getDelta());
        drawFrame();

        // Cap graphics frame rate to graphicsHz (0 = uncapped)
        if (graphicsHz > 0.0f) {
            const auto frameBudget =
                std::chrono::duration_cast<Clock::duration>(
                    std::chrono::duration<float>(1.0f / graphicsHz));
            const auto elapsed = Clock::now() - frameStart;
            if (elapsed < frameBudget) {
                std::this_thread::sleep_for(frameBudget - elapsed);
            }
        }
        frameStart = Clock::now();
    }

    // Stop background threads before GPU teardown
    m_physicsThread.request_stop();
    m_networkingThread.request_stop();
    m_physicsThread.join();
    m_networkingThread.join();

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

    // Pick up any network-triggered scene change (written by the networking thread).
    if (m_networkBridge != nullptr) {
        auto pending = m_networkBridge->PollPendingSceneChange();
        if (pending.has_value()) {
            requestScenarioChange(*pending);
        }
    }

    if (!m_pendingScenarioPath.empty()) {
        // Block the physics thread while we tear down and rebuild the scenario.
        std::unique_lock<std::mutex> sceneLock(m_simMutex);
        vkDeviceWaitIdle(ctx->device);

        std::string path = m_pendingScenarioPath;
        const bool useOwnerColors = m_pendingUseOwnerColors;
        m_pendingScenarioPath = "";
        m_pendingUseOwnerColors = true;

        const bool isFlatBuffers = path.size() > 4U &&
                                  path.substr(path.size() - 4U) == ".bin";
        changeScenario(isFlatBuffers
            ? std::unique_ptr<GE::Scenario>(std::make_unique<GE::FlatBuffersScenario>(path, useOwnerColors))
            : std::unique_ptr<GE::Scenario>(std::make_unique<GE::GenericScenario>(path)));

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
    const VkCommandBuffer cb = sync->getCommandBuffer(currentFrame);
    static_cast<void>(vkResetCommandBuffer(cb, 0U));

    const VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    static_cast<void>(vkBeginCommandBuffer(cb, &beginInfo)); // <--- BUFFER IS NOW OPEN

    // --- Thread-safe ECS update ---
    // The physics thread owns CPU-stage ECS updates (via runPhysicsLoop).
    // The main thread holds m_simMutex here so that:
    //   a) updateUniformBuffer reads Transform/LightComponent without racing
    //      the physics thread which may concurrently write those same arrays.
    //   b) We apply the latest physics snapshot to ECS Transform world matrices.
    //   c) We run GPU-stage systems (particles) and record render commands while
    //      the physics thread cannot concurrently modify the component arrays.
    {
        std::lock_guard<std::mutex> lock(m_simMutex);

        // a) UBO upload — must be inside the mutex because it reads ECS Transform
        //    and LightComponent arrays that the physics thread may also be writing.
        updateUniformBuffer(imageIndex);

        // b) Apply the physics snapshot → ECS Transform (so the renderer reads
        //    consistent world matrices produced by the last completed physics tick).
        {
            const int frontIdx = m_frontSimIdx.load(std::memory_order_acquire);
            std::lock_guard<std::mutex> snapLock(m_simBuffers[frontIdx].mutex);
            for (const auto& snap : m_simBuffers[frontIdx].snapshots) {
                auto* tr = em->TryGetTIComponent<GE::Components::Transform>(snap.id);
                if (tr != nullptr) {
                    tr->m_worldMatrix = snap.worldMatrix;
                }
            }
        }

        // c) GPU-stage systems (ParticleEmitterSystem) need the live command buffer.
        if (activeScenario && !activeScenario->IsPaused()) {
            em->UpdateGpuStages(scaledDelta, cb);
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

        // d) Record Draw Calls: Renderer queries ECS for meshes (reads Transform world matrices).
        //    Must remain inside the mutex so physics cannot race-write those fields.
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
    } // releases m_simMutex — physics thread may resume

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
            ubo.lightPos = trans->m_worldPosition;
            ubo.lightColor = lightComp.color * lightComp.intensity * inputManager->getColorMod();

            // Compute light-space matrix for shadow mapping
            const glm::vec3 shadowTarget{ 0.0f, 0.0f, 0.0f };
            const glm::vec3 lightDir = glm::normalize(shadowTarget - trans->m_worldPosition);
            const glm::vec3 up = (glm::abs(lightDir.x) < 0.001f && glm::abs(lightDir.z) < 0.001f)
                ? glm::vec3(0.0f, 0.0f, 1.0f)
                : glm::vec3(0.0f, 1.0f, 0.0f);
            const glm::mat4 lightView = glm::lookAt(trans->m_worldPosition, shadowTarget, up);
            glm::mat4 lightProj = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, 0.1f, 20.0f);
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
        for (auto& buf : m_simBuffers) {
            std::lock_guard<std::mutex> lk(buf.mutex);
            buf.snapshots.clear();
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
        // Run only CPU-stage systems; GPU systems (particles) require a live command buffer.
        m_springSystem->OnUpdate(fixedStep);
        entityManager->UpdateCpuStages(fixedStep);
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

    // 2. Unload active scenario first — it may call disconnectNetwork() which
    //    needs the NetworkBridge/Service to still be alive in the ServiceLocator.
    if (activeScenario) {
        activeScenario->OnUnload();
    }
    activeScenario.reset();

    // 3. Networking shutdown (after scenario unload, before ECS / Vulkan teardown)
    if (m_networkService != nullptr) {
        m_networkService->Shutdown();
    }
    m_networkBridge.reset();
    m_networkService.reset();

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
// SECTION 8: PHYSICS THREAD
// ========================================================================

/**
 * @brief Physics thread body — fixed-timestep accumulator.
 *
 * Runs on Core 4 (affinity set in run()).  Each tick:
 *   1. Holds m_simMutex to prevent concurrent ECS access from the render thread.
 *   2. Runs SpringSystem then all CPU-stage ECS systems (Animation, Physics, Transform, etc.).
 *   3. Copies resulting Transform world-matrices into the SimulationState back buffer.
 *   4. Atomically publishes the new front index so the render thread sees a
 *      complete, consistent snapshot next frame.
 */
void EngineOrchestrator::runPhysicsLoop(std::stop_token st) {
    GE_LOG_INFO("PhysicsThread started, CPU " + std::to_string(GetCurrentProcessorNumber()));

    using Clock    = std::chrono::steady_clock;
    using FloatSec = std::chrono::duration<float>;

    auto   prevTime     = Clock::now();
    float  accumulator  = 0.0f;

    while (!st.stop_requested()) {
        const auto  now    = Clock::now();
        const float realDt = std::chrono::duration_cast<FloatSec>(now - prevTime).count();
        prevTime = now;

        // Guard against spiral-of-death on hitches (clamp to 8 missed ticks).
        // At high Hz (e.g. 2000) this prevents accumulator from growing unbounded.
        const float clampedDt  = std::min(realDt, 8.0f / std::max(physicsHz, 1.0f));
        accumulator += clampedDt;

        // Read Hz once per outer loop iteration so ImGui changes take effect next cycle
        const float fixedDt = 1.0f / std::max(physicsHz, 1.0f);

        while (accumulator >= fixedDt) {
            auto* const em = entityManager.get();

            {
                std::lock_guard<std::mutex> lock(m_simMutex);

                // Skip if no scenario or scenario is paused.
                // Guard must be inside the mutex so the render thread cannot
                // concurrently destroy activeScenario via changeScenario().
                if ((activeScenario == nullptr) || activeScenario->IsPaused()) {
                    accumulator = 0.0f;
                    break;
                }

                // 1. Spring forces must be accumulated before PhysicsSystem integrates.
                m_springSystem->OnUpdate(fixedDt);

                // 2. CPU-stage ECS systems: TransformSystem, AnimationSystem,
                //    PhysicsSystem, SpawnerSystem, etc.
                em->UpdateCpuStages(fixedDt);

                // 2b. Broadcast owned entity states to peers (throttled to ~60/sec).
                if (m_networkBridge != nullptr) {
                    m_networkBridge->BroadcastOwnedStates();
                    // 2c. Apply dead-reckoned positions to remote entities, overwriting
                    //     whatever PhysicsSystem integrated for them this tick.
                    m_networkBridge->UpdateRemoteEntities(fixedDt);
                }

                // 3. Copy all Transform world-matrices to the SimulationState back buffer.
                const int backIdx = 1 - m_frontSimIdx.load(std::memory_order_relaxed);
                {
                    std::lock_guard<std::mutex> snapLock(m_simBuffers[backIdx].mutex);
                    auto& transforms = em->GetCompArr<GE::Components::Transform>();
                    const uint32_t count = transforms.GetCount();
                    m_simBuffers[backIdx].snapshots.resize(count);
                    for (uint32_t i = 0U; i < count; ++i) {
                        m_simBuffers[backIdx].snapshots[i] = {
                            transforms.Index()[i],
                            transforms.Data()[i].m_worldMatrix
                        };
                    }
                }

                // 4. Atomically publish the new front (release so render thread
                //    sees fully written snapshot data).
                m_frontSimIdx.store(backIdx, std::memory_order_release);
            }

            accumulator -= fixedDt;

            // Yield between ticks so the render thread can acquire m_simMutex.
            // Without this, at high Hz the physics thread can starve the renderer
            // by re-acquiring the lock immediately after releasing it.
            std::this_thread::yield();
        }

        // Sleep for the remainder of the fixed step to avoid busy-spinning.
        const auto tickEnd    = Clock::now();
        const auto tickElapsed = tickEnd - prevTime;
        const auto tickBudget  =
            std::chrono::duration_cast<Clock::duration>(FloatSec(fixedDt));
        if (tickElapsed < tickBudget) {
            std::this_thread::sleep_for(tickBudget - tickElapsed);
        }
    }
    GE_LOG_INFO("PhysicsThread stopped.");
}

// ========================================================================
// SECTION 9: GRAPHICS & ASSET HELPERS
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