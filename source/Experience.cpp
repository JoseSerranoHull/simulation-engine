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
    // Step 1: Foundation - Initialize OS Window and Hardware Contexts
    initWindow(title);
    context = std::make_unique<VulkanContext>();
    ServiceLocator::Provide(context.get());
    vulkanEngine = std::make_unique<VulkanEngine>(window);

    // Step 2: Resource Infrastructure - Initialize Manager and Asset registry
    resources = std::make_unique<VulkanResourceManager>();
    ServiceLocator::Provide(resources.get());
    timeManager = std::make_unique<TimeManager>();
    statsManager = std::make_unique<StatsManager>();
    assetManager = std::make_unique<AssetManager>();

    // Step 3: Hardware Linkage - Connect managers to the Vulkan device
    resources->init(vulkanEngine.get(), MAX_FRAMES_IN_FLIGHT);
    assetManager->setDescriptorPool(resources->getDescriptorPool());

    // Step 4: Logic Layers - Initialize simulation and UI managers
    imagesInFlight.resize(vulkanEngine->getSwapChainImageCount(), VK_NULL_HANDLE);
    renderer = std::make_unique<Renderer>();
    scene = std::make_unique<Scene>();
    inputManager = std::make_unique<InputManager>(window, timeManager.get());
    ServiceLocator::Provide(inputManager.get());
    uiManager = std::make_unique<IMGUIManager>();
    climateManager = std::make_unique<ClimateManager>();

    // Step 5: State Synchronization - Align UI with initial simulation state
    inputManager->setDustEnabled(climateManager->isDustEnabled());
    inputManager->setFireEnabled(climateManager->isFireEnabled());
    inputManager->setSmokeEnabled(climateManager->isSmokeEnabled());
    inputManager->setRainEnabled(climateManager->isRainEnabled());
    inputManager->setSnowEnabled(climateManager->isSnowEnabled());

    // Step 6: Visual Systems - Initialize Post-Processing and Particles
    postProcessor = SystemFactory::createPostProcessingSystem(vulkanEngine.get());
    postProcessor->resize(vulkanEngine->getSwapChainExtent());

    // Step 7: Configuration - Load scene-specific metadata
    cachedConfig = ConfigLoader::loadConfig("./config/config.txt");
    if (cachedConfig.empty()) {
        throw std::runtime_error("Experience: Failed to load config.txt! Check file path.");
    }

    // Step 8: Entity Setup - Initialize light and particle systems
    const ObjectTransform& lightCfg = cachedConfig.at("MainLight");
    mainLight = SystemFactory::createLightSystem(lightCfg.pos, lightCfg.color, lightCfg.params.at("intensity"));

    const VkSampleCountFlagBits msaa = vulkanEngine->getMsaaSamples();
    const VkRenderPass transRP = postProcessor->getTransparentRenderPass();

    dustParticleSystem = SystemFactory::createDustSystem(transRP, msaa);
    fireParticleSystem = SystemFactory::createFireSystem(transRP, msaa);
    smokeParticleSystem = SystemFactory::createSmokeSystem(transRP, msaa);
    rainParticleSystem = SystemFactory::createRainSystem(transRP, msaa);
    snowParticleSystem = SystemFactory::createSnowSystem(transRP, msaa);

    // Step 9: Finalization - Hardware handshake and Asset loading
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
 * @brief Loads all 3D geometry, textures, and materials.
 */
 /**
  * @brief Loads all 3D geometry, textures, and materials.
  * Orchestrates the transfer of data from disk to GPU via staging buffers
  * and handles procedural placement.
  */
void Experience::loadAssets() {
    VulkanContext* context = ServiceLocator::GetContext();

    // Step 1: Setup GPU Command Recording for Transfer and reset registries
    const VkCommandBuffer setupCmd = VulkanUtils::beginSingleTimeCommands(context->device, context->graphicsCommandPool);
    std::vector<VkBuffer> stagingBuffers{};
    std::vector<VkDeviceMemory> stagingMemories{};

    scene = std::make_unique<Scene>();

    meshes.clear();
    transparentMeshes.clear();
    ownedModels.clear();

    // Step 2: Utility Assets - Load shared textures and reusable materials
    const auto whiteTex = assetManager->loadTexture("./textures/white.png");
    const auto blackTex = assetManager->loadTexture("./textures/black.png");
    const auto matteTex = assetManager->loadTexture("./textures/matte_rough.png");
    const auto placeholder = assetManager->loadTexture("./textures/vikingroom/viking_room.png");

    // Bridge PostProcessor background for material use
    const auto sceneTex = std::shared_ptr<Texture>(postProcessor->getBackgroundTexture(), [](Texture*) {
        /* Ownership managed by PostProcessor */
        });

    // Step 3: Define Standard PBR Materials
    const auto sandMat = assetManager->createMaterial(
        assetManager->loadTexture("./textures/sand/GroundSand005_COL_2K.jpg"),
        assetManager->loadTexture("./textures/sand/GroundSand005_NRM_2K.jpg"),
        whiteTex, blackTex, whiteTex, pipelines[1].get());

    const auto rattanMat = assetManager->createMaterial(
        assetManager->loadTexture("./textures/base/Poliigon_RattanWeave_6945_BaseColor.jpg"),
        assetManager->loadTexture("./textures/base/Poliigon_RattanWeave_6945_Normal.jpg"),
        assetManager->loadTexture("./textures/base/Poliigon_RattanWeave_6945_AmbientOcclusion.jpg"),
        blackTex, whiteTex, pipelines[2].get());

    const auto propMat = assetManager->createMaterial(
        assetManager->loadTexture("./textures/vikingroom/viking_room.png"),
        placeholder, whiteTex, blackTex, matteTex, pipelines[0].get());

    const auto cactusMat = assetManager->createMaterial(
        assetManager->loadTexture("./textures/cacti/10436_Cactus_v1_Diffuse.jpg"),
        assetManager->loadTexture("./textures/cacti/10436_Cactus_v1_NormalMap.png"),
        whiteTex, blackTex, matteTex, pipelines[0].get());

    const auto magicMat = assetManager->createMaterial(
        assetManager->loadTexture("./textures/magiccircle/Magic1.png"),
        placeholder, assetManager->loadTexture("./textures/magiccircle/Magic1.png"),
        blackTex, whiteTex, pipelines[4].get());

    const auto grassMat = assetManager->createMaterial(
        assetManager->loadTexture("./textures/grass/Trava Kolosok.jpg"),
        placeholder, assetManager->loadTexture("./textures/grass/Trava Kolosok Cut.jpg"),
        blackTex, whiteTex, pipelines[4].get());

    // Step 4: Load Sorceress-specific Materials
    const auto sBodyMat = assetManager->createMaterial(
        assetManager->loadTexture("./textures/sorceress/Drenai_Body_BaseColor.png"),
        assetManager->loadTexture("./textures/sorceress/Drenai_Body_Normal.jpg"),
        whiteTex, assetManager->loadTexture("./textures/sorceress/Drenai_Body_Metallic.jpg"),
        assetManager->loadTexture("./textures/sorceress/Drenai_Body_Roughness.jpg"), pipelines[0].get());

    const auto sSkirtMat = assetManager->createMaterial(
        assetManager->loadTexture("./textures/sorceress/Drenai_Skirt_BaseColor.jpg"),
        assetManager->loadTexture("./textures/sorceress/Drenai_Skirt_Normal.jpg"),
        whiteTex, assetManager->loadTexture("./textures/sorceress/Drenai_Skirt_Metallic.jpg"),
        assetManager->loadTexture("./textures/sorceress/Drenai_Skirt_Roughness.jpg"), pipelines[0].get());

    const auto sBookMat = assetManager->createMaterial(
        assetManager->loadTexture("./textures/sorceress/book_02_-_Default_BaseColor.jpg"),
        assetManager->loadTexture("./textures/sorceress/book_02_-_Default_Normal.jpg"),
        whiteTex, assetManager->loadTexture("./textures/sorceress/book_02_-_Default_Metallic.jpg"),
        assetManager->loadTexture("./textures/sorceress/book_02_-_Default_Roughness.jpg"), pipelines[0].get());

    const auto orbMat = assetManager->createMaterial(
        assetManager->loadTexture("./textures/sorceress/DefaultMaterial_Base_Color.jpg"),
        placeholder, assetManager->loadTexture("./textures/sorceress/DefaultMaterial_Opacity.jpg"),
        blackTex, whiteTex, pipelines[4].get());

    const auto spellMat = assetManager->createMaterial(
        assetManager->loadTexture("./textures/sorceress/basecolor.jpg"),
        placeholder, assetManager->loadTexture("./textures/sorceress/basecolor.jpg"),
        blackTex, whiteTex, pipelines[4].get());

    // Step 5: Initialize Helper Lambdas for Asset Organization
    auto sorcSelector = [&](const std::string& meshName) -> std::shared_ptr<Material> {
        std::string n = meshName;
        for (char& c : n) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        if (n == "tiles_low") {
            return nullptr;
        }
        if (n == "skirt") {
            return sSkirtMat;
        }
        if (n == "book") {
            return sBookMat;
        }
        if ((n == "object003") || (n == "sphere_low")) {
            return orbMat;
        }
        if (n == "effects") {
            return spellMat;
        }
        return sBodyMat;
        };

    auto categorizeMesh = [&](Mesh* const m) {
        if (m != nullptr) {
            const Pipeline* const p = m->getMaterial()->getPipeline();

            // Glass (3) and Water (5) are categorized as transparent
            if ((p == pipelines[3].get()) || (p == pipelines[5].get())) {
                transparentMeshes.push_back(m);
            }
            else {
                // Opaque and Alpha-blended (Pipeline 4) objects
                meshes.push_back(m);
            }
        }
        };

    // Step 6: Named Constants for Procedural Placement
    static constexpr uint32_t CACTUS_COUNT = 3U;
    static constexpr uint32_t ROCK_COUNT = 11U;
    static constexpr uint32_t GRASS_COUNT = 8U;
    static constexpr float ROCK_PLACEMENT_RADIUS = 1.3f;
    static constexpr float ROCK_Y_OFFSET = -0.12f;
    static constexpr float ROCK_ANGLE_STEP = 0.7f;
    static constexpr float ROCK_UNIFORM_SCALE = 0.0004f;

    // Step 7: Model Loading & Configuration (Sorceress, Magic Circle, Viking House, Oasis)

    // 7.1 Desert Queen
    auto sorceressModel = assetManager->loadModel("./models/sorceress/Pose_Body.obj", sorcSelector, setupCmd, stagingBuffers, stagingMemories);
    for (auto& m : sorceressModel->getMeshes()) {
        categorizeMesh(m.get());
    }
    if (cachedConfig.count(SceneKeys::DESERT_QUEEN) > 0U) {
        sorceressModel->setPosition(cachedConfig.at(SceneKeys::DESERT_QUEEN).pos);
        sorceressModel->setScale(cachedConfig.at(SceneKeys::DESERT_QUEEN).scale);
    }
    scene->addModel(SceneKeys::DESERT_QUEEN, std::move(sorceressModel));

    // 7.2 Magic Circle
    auto magicCircle = assetManager->loadModel("./models/magiccircle/Magic1.obj", [&](const std::string&) {
        return magicMat;
        }, setupCmd, stagingBuffers, stagingMemories);
    for (auto& m : magicCircle->getMeshes()) {
        categorizeMesh(m.get());
    }
    magicCircle->setShadowCasting(false);
    if (cachedConfig.count(SceneKeys::MAGIC_CIRCLE) > 0U) {
        magicCircle->setPosition(cachedConfig.at(SceneKeys::MAGIC_CIRCLE).pos);
        magicCircle->setRotation(cachedConfig.at(SceneKeys::MAGIC_CIRCLE).rot);
        magicCircle->setScale(cachedConfig.at(SceneKeys::MAGIC_CIRCLE).scale);
    }
    scene->addModel(SceneKeys::MAGIC_CIRCLE, std::move(magicCircle));

    // 7.3 Viking House
    auto vikingHouse = assetManager->loadModel("./models/vikingroom/viking_room.obj", [&](const std::string&) {
        return propMat;
        }, setupCmd, stagingBuffers, stagingMemories);
    for (auto& m : vikingHouse->getMeshes()) {
        categorizeMesh(m.get());
    }
    if (cachedConfig.count(SceneKeys::VIKING_HOUSE) > 0U) {
        vikingHouse->setPosition(cachedConfig.at(SceneKeys::VIKING_HOUSE).pos);
        vikingHouse->setScale(cachedConfig.at(SceneKeys::VIKING_HOUSE).scale);
    }
    scene->addModel(SceneKeys::VIKING_HOUSE, std::move(vikingHouse));

    // 7.4 Oasis / Water Cube
    const auto waterNorm = assetManager->loadTexture("./textures/watercube/watermapnormalmap.jpg");
    const auto waterMat = assetManager->createMaterial(placeholder, waterNorm, whiteTex, blackTex, blackTex, pipelines[5].get());
    auto oasisModel = assetManager->loadModel("./models/watercube/wideflatcube.obj", [&](const std::string&) {
        return waterMat;
        }, setupCmd, stagingBuffers, stagingMemories);
    for (auto& m : oasisModel->getMeshes()) {
        categorizeMesh(m.get());
    }
	oasisModel->setShadowCasting(false);
    if (cachedConfig.count(SceneKeys::OASIS) > 0U) {
        oasisModel->setPosition(cachedConfig.at(SceneKeys::OASIS).pos);
        oasisModel->setScale(cachedConfig.at(SceneKeys::OASIS).scale);
    }
    scene->addModel(SceneKeys::OASIS, std::move(oasisModel));

    // Step 8: Procedural Instance Generation (Vegetation and Rocks)

    for (uint32_t i = 0U; i < GRASS_COUNT; ++i) {
        auto grassModel = assetManager->loadModel("./models/grass/Trava Kolosok.obj", [&](const std::string&) {
            return grassMat;
            }, setupCmd, stagingBuffers, stagingMemories);
        const float angle = static_cast<float>(i) * 1.5f;
        const float dist = 0.2f + (static_cast<float>(i) * 0.05f);
        grassModel->setPosition({ (i < 4U ? -0.4f : 0.6f) + std::cos(angle) * dist, -0.1f, (i < 4U ? 0.4f : 0.5f) + std::sin(angle) * dist });
        grassModel->setScale({ 0.00025f, 0.00025f, 0.00025f });
        for (auto& m : grassModel->getMeshes()) {
            categorizeMesh(m.get());
        }
        ownedModels.push_back(std::move(grassModel));
    }

    for (uint32_t i = 1U; i <= CACTUS_COUNT; ++i) {
        const std::string key = "Cactus" + std::to_string(i);
        auto cactus = assetManager->loadModel("./models/cacti/10436_Cactus_v1_max2010_it2.obj", [&](const std::string&) {
            return cactusMat;
            }, setupCmd, stagingBuffers, stagingMemories);
        for (auto& m : cactus->getMeshes()) {
            categorizeMesh(m.get());
        }
        if (cachedConfig.count(key) > 0U) {
            cactus->setPosition(cachedConfig.at(key).pos);
            cactus->setRotation(cachedConfig.at(key).rot);
            cactus->setScale(cachedConfig.at(key).scale);
        }
        scene->addModel(key, std::move(cactus));
    }

    const auto rockNorm = assetManager->loadTexture("./textures/rocks/Rock_Normal.png");
    for (uint32_t i = 0U; i < ROCK_COUNT; ++i) {
        const std::string texPath = "./textures/rocks/Rock" + std::to_string(i == 0U ? 1U : i) + "_Diffuse.png";
        const auto rMat = assetManager->createMaterial(assetManager->loadTexture(texPath), rockNorm, whiteTex, blackTex, matteTex, pipelines[0].get());
        auto rock = assetManager->loadModel("./models/rocks/Rock" + std::to_string(i) + ".obj", [&](const std::string&) {
            return rMat;
            }, setupCmd, stagingBuffers, stagingMemories);
        for (auto& m : rock->getMeshes()) {
            categorizeMesh(m.get());
        }
        const float angle = static_cast<float>(i) * ROCK_ANGLE_STEP;
        rock->setPosition({ std::cos(angle) * ROCK_PLACEMENT_RADIUS, ROCK_Y_OFFSET, std::sin(angle) * ROCK_PLACEMENT_RADIUS });
        rock->setScale({ ROCK_UNIFORM_SCALE, ROCK_UNIFORM_SCALE, ROCK_UNIFORM_SCALE });
        scene->addModel("Rock" + std::to_string(i), std::move(rock));
    }

    // Step 9: Procedural Globe & Base Generation (Geometry Utils)
    static constexpr uint32_t GLOBE_SEGMENTS = 64U;

    auto baseModel = std::make_unique<Model>();
    auto bMesh = assetManager->processMeshData(GeometryUtils::generateCylinder(GLOBE_SEGMENTS, 2.4f, 1.75f, 0.8f), rattanMat, setupCmd, stagingBuffers, stagingMemories);
    categorizeMesh(bMesh.get());
    baseModel->addMesh(std::move(bMesh));
    baseModel->setPosition({ 0.0f, -0.9f, 0.0f });
    scene->addModel("ProceduralBase", std::move(baseModel));

    auto sandModel = std::make_unique<Model>();
    auto sMesh = assetManager->processMeshData(GeometryUtils::generateSandPlug(GLOBE_SEGMENTS, 1.78f, 1.8f, 0.4f), sandMat, setupCmd, stagingBuffers, stagingMemories);
    categorizeMesh(sMesh.get());
    sandModel->addMesh(std::move(sMesh));
    sandModel->setPosition({ 0.0f, -0.1f, 0.0f });
    scene->addModel("ProceduralSand", std::move(sandModel));

    const auto glassMatFinal = assetManager->createMaterial(placeholder, placeholder, whiteTex, blackTex, blackTex, pipelines[3].get());
    auto glassModel = std::make_unique<Model>();
    auto glassMesh = assetManager->processMeshData(GeometryUtils::generateSphere(GLOBE_SEGMENTS, 1.8f, -0.5f), glassMatFinal, setupCmd, stagingBuffers, stagingMemories);
    categorizeMesh(glassMesh.get());
    glassModel->addMesh(std::move(glassMesh));
    glassModel->setPosition({ 0.0f, -0.3f, 0.0f });
    glassModel->setShadowCasting(false);
    ownedModels.push_back(std::move(glassModel));

    // Step 10: Final GPU Submission & Staging Cleanup
    VulkanUtils::endSingleTimeCommands(context->device, context->graphicsCommandPool, context->graphicsQueue, setupCmd);

    for (size_t i = 0U; i < stagingBuffers.size(); ++i) {
        vkDestroyBuffer(context->device, stagingBuffers[i], nullptr);
        vkFreeMemory(context->device, stagingMemories[i], nullptr);
    }

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

    // Step 1: CPU-GPU Throttling - Wait for the previous frame's GPU execution to finish
    SyncManager* const sync = resources->getSyncManager();
    const VkFence currentFence = sync->getInFlightFence(currentFrame);
    static_cast<void>(vkWaitForFences(context->device, 1U, &currentFence, VK_TRUE, UINT64_MAX));

    const float dt = timeManager->getDelta();
    const float totalTime = timeManager->getTotal();

    // Step 2: High-Level Logic Updates - Update input and scene state
    inputManager->update(dt);
    if (scene != nullptr) {
        scene->update(dt);
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

    // Step 4: Command Buffer Recording - Update UBO and record draw calls
    updateUniformBuffer(imageIndex);

    const VkCommandBuffer cb = sync->getCommandBuffer(currentFrame);
    static_cast<void>(vkResetCommandBuffer(cb, 0U));

    const VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    static_cast<void>(vkBeginCommandBuffer(cb, &beginInfo));

    // Update Particle Systems (Physics/Compute steps)
    if (dustParticleSystem != nullptr) {
        dustParticleSystem->update(cb, dt, inputManager->getDustEnabled(), totalTime, currentUBO.lightColor);
    }
    if (fireParticleSystem != nullptr) {
        const glm::vec3 origin = (scene && scene->hasModel("Cactus1"))
            ? scene->getModels().at("Cactus1")->getPosition()
            : glm::vec3(-0.8f, -0.15f, -0.5f);
        fireParticleSystem->update(cb, dt, inputManager->getFireEnabled(), totalTime, currentUBO.lightColor, origin);
    }
    if (smokeParticleSystem != nullptr) {
        const glm::vec3 origin = (scene && scene->hasModel("Cactus1"))
            ? scene->getModels().at("Cactus1")->getPosition()
            : glm::vec3(-0.8f, -0.15f, -0.5f);
        smokeParticleSystem->update(cb, dt, inputManager->getSmokeEnabled(), totalTime, currentUBO.lightColor, origin);
    }
    if (rainParticleSystem != nullptr) {
        rainParticleSystem->update(cb, dt, inputManager->getRainEnabled(), totalTime, currentUBO.lightColor);
    }
    if (snowParticleSystem != nullptr) {
        snowParticleSystem->update(cb, dt, inputManager->getSnowEnabled(), totalTime, currentUBO.lightColor);
    }

    // Record the actual geometry draw calls via the Renderer
    std::vector<Pipeline*> rawPipelines;
    for (const auto& p : pipelines) {
        rawPipelines.push_back(p.get());
    }

    renderer->recordFrame(
        cb, vulkanEngine->getSwapChainExtent(), scene->getModels(), ownedModels, meshes, transparentMeshes,
        skybox.get(), dustParticleSystem.get(), fireParticleSystem.get(), smokeParticleSystem.get(),
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
 */
 /**
  * @brief Syncs CPU simulation state to the GPU Uniform Buffer.
  * Orchestrates climate simulation, manual user overrides, and hardware buffer updates.
  */
void Experience::updateUniformBuffer(const uint32_t currentImage) {
    void* const mappedData = resources->getMappedBuffer(currentImage);
    if (mappedData == nullptr) {
        return;
    }

    const float dt = timeManager->getDelta();
    const float totalTime = timeManager->getTotal();
    const ObjectTransform& lightCfg = cachedConfig.at("MainLight");

    // Step 1: Handle high-level simulation reset requests ('R' Key)
    // This is now the only place where Climate states are forced into the Input toggles.
    if (inputManager->consumeResetRequest()) {
        performFullReset();
    }

    // Step 2: Update Climate simulation
    climateManager->update(dt, totalTime, inputManager->getAutoOrbit(),
        lightCfg.params.at("orbitRadius"),
        lightCfg.params.at("orbitSpeed"),
        lightCfg.params.at("intensity"));

    // we push those new default toggles to the InputManager.
    if (climateManager->checkTransition()) {
        syncWeatherToggles();
    }

    // Step 3: Apply Climate scaling/tints to scene models (Cacti and Water)
    const float cactusMultiplier = climateManager->getCactusScale();

    for (uint32_t i = 1U; i <= 3U; ++i) {
        const std::string key = "Cactus" + std::to_string(i);
        Model* const pCactus = scene->getModel(key);

        if (pCactus != nullptr) {
            pCactus->setScale(cachedConfig.at(key).scale * cactusMultiplier);
        }
    }

    Model* const pOasis = scene->getModel("Oasis");
    if (pOasis != nullptr) {
        pOasis->setScale(cachedConfig.at("Oasis").scale * climateManager->getWaterScale());
        pOasis->setPosition(cachedConfig.at("Oasis").pos + glm::vec3(0.0f, climateManager->getWaterOffset(), 0.0f));
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
    // This now respects the user's manual toggle even if the climate is currently "Summer".
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