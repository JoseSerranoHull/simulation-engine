#include "services/DebugOverlay.h"
#include "scene/GenericScenario.h"
#include "core/EngineOrchestrator.h"

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
    PointLight* const light, const TimeService* const time,
    ClimateService* const climate) const
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // --- 1. Top-Level Main Menu Bar (Agnostic Orchestration) ---
    DrawMainMenuBar(input);

    // --- 2. Central Diagnostics Window ---
    // We keep your existing logic here, but wrap it in a window.
    if (ImGui::Begin("Engine Diagnostics", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {

        // Show Telemetry
        if (ImGui::CollapsingHeader("Performance")) {
            ImGui::Text("FPS: %.1f", static_cast<double>(ImGui::GetIO().Framerate));
            if (stats != nullptr) {
                ImGui::PlotLines("History", stats->getHistoryData(),
                    static_cast<int>(stats->getCount()),
                    static_cast<int>(stats->getOffset()), nullptr, 0.0f, 165.0f, ImVec2(0, 50));
            }
        }

        // --- 2. Camera Settings (Step 3 Implementation) ---
        if (ImGui::CollapsingHeader("Camera Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
            Camera* activeCam = input->getActiveCamera();

            ImGui::Text("Active View: %s", input->getActiveCameraLabel());

            // Fulfills Lab Requirement: Orthographic Camera Toggle
            bool isOrtho = (activeCam->getProjectionMode() == Camera::ProjectionMode::ORTHOGRAPHIC);
            if (ImGui::Checkbox("Orthographic Projection", &isOrtho)) {
                activeCam->setProjectionMode(isOrtho ?
                    Camera::ProjectionMode::ORTHOGRAPHIC :
                    Camera::ProjectionMode::PERSPECTIVE);
            }

            // Fulfills Lab Requirement: View from different positions/aligned to axis
            if (isOrtho) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.2f, 1.0f));
                ImGui::TextWrapped("Debug Axis Alignment:");
                ImGui::PopStyleColor();

                if (ImGui::Button("Top (Y-Axis)")) {
                    activeCam->setPosition(glm::vec3(0.0f, 10.0f, 0.0f));
                    activeCam->setPitch(-89.9f); // Looking straight down
                    activeCam->setYaw(-90.0f);
                }
                ImGui::SameLine();
                if (ImGui::Button("Side (X-Axis)")) {
                    activeCam->setPosition(glm::vec3(10.0f, 0.0f, 0.0f));
                    activeCam->setPitch(0.0f);
                    activeCam->setYaw(180.0f); // Looking toward origin from +X
                }
                ImGui::SameLine();
                if (ImGui::Button("Front (Z-Axis)")) {
                    activeCam->setPosition(glm::vec3(0.0f, 0.0f, 10.0f));
                    activeCam->setPitch(0.0f);
                    activeCam->setYaw(-90.0f); // Looking toward origin from +Z
                }
            }
        }

        // --- 3. Performance Telemetry ---
        if (ImGui::CollapsingHeader("Telemetry")) {
            ImGui::Text("Performance: %.1f FPS", static_cast<double>(ImGui::GetIO().Framerate));
            if (stats != nullptr) {
                ImGui::PlotLines("FPS History", stats->getHistoryData(),
                    static_cast<int>(stats->getCount()),
                    static_cast<int>(stats->getOffset()), nullptr, 0.0f, 165.0f, ImVec2(0, 80));
            }
        }

        // --- 4. Simulation Control (Lab Requirement: Start/Stop/Pause/Timestep) ---
        if (ImGui::CollapsingHeader("Simulation Logic")) {
            if (time != nullptr) {
                ImGui::Text("Global Timestep: %.2fx", static_cast<double>(time->getScale()));
                if (ImGui::Button("Reset Clock")) { /* Logic handled via InputService 'r' */ }
            }

            // Note: Once the activeScenario is fully integrated, 
            // we can add a Scenario::Pause toggle here.
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "Status: Running");
        }

        // --- 5. Scenario-Specific UI (Climate/Weather) ---
        // This section will eventually move into a virtual method within the Scenario class
        if (climate != nullptr) {
            if (ImGui::CollapsingHeader("Environment (Snow Globe)")) {
                static const char* modeNames[] = { "Normal (Auto)", "Summer (Lock)", "Rain (Lock)", "Snow (Lock)" };
                int currentModeIdx = static_cast<int>(climate->getWeatherMode());

                if (ImGui::Combo("Season Mode", &currentModeIdx, modeNames, static_cast<int>(IM_ARRAYSIZE(modeNames)))) {
                    climate->setWeatherMode(static_cast<WeatherMode>(currentModeIdx));
                }

                ImVec4 seasonColor{ 1.0f, 1.0f, 1.0f, 1.0f };
                switch (climate->getWeatherState()) {
                case WeatherState::RAIN:   seasonColor = ImVec4(0.2f, 0.5f, 1.0f, 1.0f); break;
                case WeatherState::SNOW:   seasonColor = ImVec4(0.6f, 0.8f, 1.0f, 1.0f); break;
                default:                   seasonColor = ImVec4(1.0f, 0.4f, 0.1f, 1.0f); break;
                }
                ImGui::TextColored(seasonColor, "Current Season: %s", climate->getSeasonLabel());

                ImGui::Separator();

                bool orbit = input->getAutoOrbit();
                if (ImGui::Checkbox("Auto-Orbit Sun", &orbit)) { input->setAutoOrbit(orbit); }

                if (light != nullptr && !input->getAutoOrbit()) {
                    glm::vec3 pos = light->getPosition();
                    if (ImGui::SliderFloat3("Manual Sun", &pos.x, -5.0f, 5.0f)) {
                        light->setPosition(pos);
                    }
                }
            }
        }

        // --- 6. Global Rendering Overrides ---
        if (ImGui::CollapsingHeader("Visual Overrides")) {
            bool bloom = input->getBloomEnabled();
            if (ImGui::Checkbox("Post-Process Bloom", &bloom)) { input->setBloomEnabled(bloom); }

            bool gouraud = input->getGouraudEnabled();
            if (ImGui::Checkbox("Gouraud Shading", &gouraud)) { input->setGouraudEnabled(gouraud); }

            float intensity = input->getIntensityMod();
            if (ImGui::SliderFloat("Global Intensity", &intensity, 0.0f, 5.0f)) {
                input->setIntensityMod(intensity);
            }

            glm::vec3 currentColor = input->getColorMod();
            if (ImGui::ColorEdit3("Scene Color Tint", &currentColor[0])) {
                input->setColorMod(currentColor);
            }
        }
    }

    ImGui::End();
    ImGui::Render();
}

/**
 * @brief Implementation of the top-level Menu Bar.
 */
void DebugOverlay::DrawMainMenuBar(InputService* const input) const {
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

        // --- SCENARIO-SPECIFIC UI HOOK ---
        // Fulfills Requirement: Scenarios add UI controls unique to them
        if (experience->GetCurrentScenario()) {
            experience->GetCurrentScenario()->OnGUI();
        }

        ImGui::EndMainMenuBar();
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