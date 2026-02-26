#include "services/DebugOverlay.h"
#include "scene/GenericScenario.h"
#include "core/EngineOrchestrator.h"
#include "components/Tag.h"
#include "components/Transform.h"

using namespace GE::Graphics;
using namespace GE::Assets;

/* parasoft-begin-suppress ALL */
#include <stdexcept>
/* parasoft-end-suppress ALL */

/**
 * @brief Constructor: Links the UI manager to the centralized Vulkan context.
 */
DebugOverlay::DebugOverlay()
    : imguiPool(VK_NULL_HANDLE) { }

/**
 * @brief Destructor: Triggers the cleanup of ImGui and Vulkan handles.
 */
DebugOverlay::~DebugOverlay() {
    try {
        cleanup();
    }
    catch (...) {
        // Standard safety: suppress exceptions during shutdown to prevent std::terminate.
    }
}

/**
 * @brief Initializes the ImGui library and creates a dedicated descriptor pool.
 */
void DebugOverlay::init(GLFWwindow* const window, const VulkanDevice* const engine) {
    // Step 1: Define Descriptor Pool for UI logic.
    // We allocate a large pool to accommodate potential external textures in the UI.
    static constexpr uint32_t POOL_SIZE = 1000U;
    const VkDescriptorPoolSize pool_sizes[] = {
        { VK_DESCRIPTOR_TYPE_SAMPLER, POOL_SIZE },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, POOL_SIZE },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, POOL_SIZE },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, POOL_SIZE },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, POOL_SIZE },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, POOL_SIZE },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, POOL_SIZE },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, POOL_SIZE },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, POOL_SIZE },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, POOL_SIZE },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, POOL_SIZE }
    };

    VkDescriptorPoolCreateInfo pool_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = POOL_SIZE;
    pool_info.poolSizeCount = static_cast<uint32_t>(sizeof(pool_sizes) / sizeof(pool_sizes[0]));
    pool_info.pPoolSizes = pool_sizes;

    VulkanContext* context = ServiceLocator::GetContext();

    if (vkCreateDescriptorPool(context->device, &pool_info, nullptr, &imguiPool) != VK_SUCCESS) {
        throw std::runtime_error("DebugOverlay: Failed to create descriptor pool!");
    }

    // Step 2: Initialize ImGui Context and GLFW link.
    IMGUI_CHECKVERSION();
    static_cast<void>(ImGui::CreateContext());
    ImGui::StyleColorsDark();

    static_cast<void>(ImGui_ImplGlfw_InitForVulkan(window, true));

    // Step 3: Initialize Vulkan Backend for UI rendering.
    // Rendering must happen in the Final Pass (1x MSAA).
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = context->instance;
    init_info.PhysicalDevice = context->physicalDevice;
    init_info.Device = context->device;
    init_info.QueueFamily = engine->getQueueFamilyIndices().graphicsFamily.value();
    init_info.Queue = context->graphicsQueue;
    init_info.DescriptorPool = imguiPool;
    init_info.MinImageCount = 2U;
    init_info.ImageCount = engine->getSwapChainImageCount();
    init_info.ApiVersion = static_cast<uint32_t>(VK_API_VERSION_1_0);
    init_info.PipelineInfoMain.RenderPass = engine->getFinalRenderPass();
    init_info.PipelineInfoMain.Subpass = 0U;
    init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    if (!ImGui_ImplVulkan_Init(&init_info)) {
        throw std::runtime_error("DebugOverlay: Failed to initialize Vulkan backend!");
    }
}

/**
 * @brief Constructs the diagnostic UI widgets for the current frame.
 */
 /**
  * @brief Constructs the diagnostic UI widgets for the current frame.
  * Refactored to properly synchronize local UI state with InputService setters.
  */
void DebugOverlay::update(InputService* const input, const PerformanceTracker* const stats,
    PointLightSource* const light, const TimeService* const time,
    ClimateService* const climate) const
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // --- 1. Top-Level Main Menu Bar (Agnostic Orchestration) ---
    DrawMainMenuBar(input, light, climate);

    // --- 2. Hierarchy Window ---
    if (ImGui::Begin("Hierarchy", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {

        // Performance (at top)
        if (ImGui::CollapsingHeader("Performance")) {
            ImGui::Text("FPS: %.1f", static_cast<double>(ImGui::GetIO().Framerate));
            if (stats != nullptr) {
                ImGui::PlotLines("History", stats->getHistoryData(),
                    static_cast<int>(stats->getCount()),
                    static_cast<int>(stats->getOffset()), nullptr, 0.0f, 165.0f, ImVec2(0, 50));
            }
        }

        ImGui::Separator();

        // Entity Hierarchy
        try {
            GE::ECS::EntityManager* em = ServiceLocator::GetEntityManager();
            auto& transforms = em->GetCompArr<GE::Components::Transform>();
            for (uint32_t i = 0; i < transforms.GetCount(); ++i) {
                const GE::ECS::EntityID id = transforms.Index()[i];
                if (transforms.Data()[i].m_parentEntityID == UINT32_MAX) {
                    DrawEntityNode(id, em);
                }
            }
        }
        catch (...) {
            ImGui::TextDisabled("No entities");
        }
    }

    ImGui::End();
    ImGui::Render();
}

/**
 * @brief Implementation of the top-level Menu Bar.
 */
void DebugOverlay::DrawMainMenuBar(InputService* const input, PointLightSource* const light, ClimateService* const climate) const {
    auto* experience = ServiceLocator::GetExperience();

    // Fulfills Requirement: ImGui Main Menu Bar
    if (ImGui::BeginMainMenuBar()) {

        // --- SCENARIO SELECTOR ---
        if (ImGui::BeginMenu("Scenario")) {

            if (experience->GetCurrentScenario() != nullptr) {
                if (ImGui::MenuItem("Reset Current Scenario", "F5")) {
                    std::string currentPath = experience->GetCurrentScenario()->GetConfigPath();

                    // We reload the scenario by creating a fresh instance with the same path
                    experience->changeScenario(std::make_unique<GE::GenericScenario>(currentPath));

                    GE_LOG_INFO("UI: Resetting scenario from " + currentPath);
                }
                ImGui::Separator();
            }

            // Fulfills Requirement: Load/Unload easily
            if (ImGui::MenuItem("Snow Globe Scenario")) {
                experience->requestScenarioChange("./config/snow_globe.ini");
            }
            if (ImGui::MenuItem("Simulation Lab 2 Scenario")) {
                experience->requestScenarioChange("./config/simulation_lab2.ini");
            }
            if (ImGui::MenuItem("Simulation Lab 3 Scenario")) {
                experience->requestScenarioChange("./config/simulation_lab3.ini");
            }
            ImGui::EndMenu();
        }

        // --- CAMERA CONTROLS ---
        // Fulfills Requirement: Orthographic/Perspective Toggle
        if (ImGui::BeginMenu("Camera")) {
            Camera* activeCam = input->getActiveCamera();

            bool isOrtho = (activeCam->getProjectionMode() == Camera::ProjectionMode::ORTHOGRAPHIC);
            if (ImGui::MenuItem("Toggle Orthographic", nullptr, &isOrtho)) {
                activeCam->setProjectionMode(isOrtho ?
                    Camera::ProjectionMode::ORTHOGRAPHIC :
                    Camera::ProjectionMode::PERSPECTIVE);
            }

            ImGui::Separator();
            // Fulfills Requirement: View from Axis-Aligned positions
            if (ImGui::MenuItem("Top View (Y-Axis)")) {
                activeCam->setPosition({ 0, 10, 0 }); activeCam->setPitch(-89.9f); activeCam->setYaw(-90.0f);
            }
            if (ImGui::MenuItem("Side View (X-Axis)")) {
                activeCam->setPosition({ 10, 0, 0 }); activeCam->setPitch(0.0f); activeCam->setYaw(180.0f);
            }
            ImGui::EndMenu();
        }

        // --- ENVIRONMENT ---
        if (climate != nullptr && ImGui::BeginMenu("Environment")) {
            static const char* modeNames[] = { "Normal (Auto)", "Summer (Lock)", "Rain (Lock)", "Snow (Lock)" };
            int currentModeIdx = static_cast<int>(climate->getWeatherMode());
            if (ImGui::Combo("Season Mode", &currentModeIdx, modeNames, static_cast<int>(IM_ARRAYSIZE(modeNames)))) {
                climate->setWeatherMode(static_cast<WeatherMode>(currentModeIdx));
            }

            ImVec4 seasonColor{ 1.0f, 0.4f, 0.1f, 1.0f };
            switch (climate->getWeatherState()) {
            case WeatherState::RAIN: seasonColor = ImVec4(0.2f, 0.5f, 1.0f, 1.0f); break;
            case WeatherState::SNOW: seasonColor = ImVec4(0.6f, 0.8f, 1.0f, 1.0f); break;
            default: break;
            }
            ImGui::TextColored(seasonColor, "Season: %s", climate->getSeasonLabel());

            ImGui::Separator();

            bool orbit = input->getAutoOrbit();
            if (ImGui::MenuItem("Auto-Orbit Sun", nullptr, &orbit)) { input->setAutoOrbit(orbit); }

            if (light != nullptr && !input->getAutoOrbit()) {
                glm::vec3 pos = light->getPosition();
                if (ImGui::SliderFloat3("Manual Sun", &pos.x, -5.0f, 5.0f)) { light->setPosition(pos); }
            }

            ImGui::EndMenu();
        }

        // --- VISUAL OVERRIDE ---
        if (ImGui::BeginMenu("Visual Override")) {
            bool bloom = input->getBloomEnabled();
            if (ImGui::MenuItem("Post-Process Bloom", nullptr, &bloom)) { input->setBloomEnabled(bloom); }

            bool gouraud = input->getGouraudEnabled();
            if (ImGui::MenuItem("Gouraud Shading", nullptr, &gouraud)) { input->setGouraudEnabled(gouraud); }

            ImGui::Separator();

            float intensity = input->getIntensityMod();
            if (ImGui::SliderFloat("Global Intensity", &intensity, 0.0f, 5.0f)) {
                input->setIntensityMod(intensity);
            }

            glm::vec3 tint = input->getColorMod();
            if (ImGui::ColorEdit3("Scene Color Tint", &tint[0])) { input->setColorMod(tint); }

            ImGui::Separator();

            bool castShadows = input->getGlobalShadowsEnabled();
            if (ImGui::MenuItem("Cast Shadows (Global)", nullptr, &castShadows)) {
                input->setGlobalShadowsEnabled(castShadows);
            }

            ImGui::EndMenu();
        }

        // --- SCENARIO-SPECIFIC UI HOOK ---
        // Fulfills Requirement: Scenarios add UI controls unique to them
        if (experience->GetCurrentScenario()) {
            experience->GetCurrentScenario()->OnGUI();
        }

        ImGui::EndMainMenuBar();
    }
}

/**
 * @brief Recursively draws an entity and its children in the hierarchy tree.
 */
void DebugOverlay::DrawEntityNode(const GE::ECS::EntityID entityID, GE::ECS::EntityManager* const em) const {
    using namespace GE::Components;

    auto* tag = em->TryGetTIComponent<Tag>(entityID);
    const char* name = (tag != nullptr) ? tag->m_name.c_str() : "Entity";

    // Scan for children
    auto& transforms = em->GetCompArr<Transform>();
    bool hasChildren = false;
    for (uint32_t i = 0; i < transforms.GetCount(); ++i) {
        if (transforms.Data()[i].m_parentEntityID == entityID) {
            hasChildren = true;
            break;
        }
    }

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (!hasChildren) {
        // Leaf + NoTreePushOnOpen: node is always visible but does NOT push to the ID stack,
        // so no matching TreePop() is needed.
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }

    const bool open = ImGui::TreeNodeEx(
        reinterpret_cast<void*>(static_cast<uintptr_t>(entityID)),
        flags, "%s  [%u]", name, entityID);

    if (open && hasChildren) {
        for (uint32_t i = 0; i < transforms.GetCount(); ++i) {
            if (transforms.Data()[i].m_parentEntityID == entityID) {
                DrawEntityNode(transforms.Index()[i], em);
            }
        }
        ImGui::TreePop();
    }
}

/**
 * @brief Records ImGui render commands into the current frame's command buffer.
 */
void DebugOverlay::draw(const VkCommandBuffer cb) const {
    ImDrawData* const data = ImGui::GetDrawData();
    if (data != nullptr) {
        ImGui_ImplVulkan_RenderDrawData(data, cb);
    }
}

/**
 * @brief Shuts down ImGui and destroys the local descriptor pool.
 */
void DebugOverlay::cleanup() {
    VulkanContext* context = ServiceLocator::GetContext();

    if ((context != nullptr) && (context->device != VK_NULL_HANDLE)) {
        static_cast<void>(vkDeviceWaitIdle(context->device));

        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        if (imguiPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(context->device, imguiPool, nullptr);
            imguiPool = VK_NULL_HANDLE;
        }
    }
}