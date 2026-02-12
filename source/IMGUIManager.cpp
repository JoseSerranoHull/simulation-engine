#include "../include/IMGUIManager.h"

/* parasoft-begin-suppress ALL */
#include <stdexcept>
/* parasoft-end-suppress ALL */

/**
 * @brief Constructor: Links the UI manager to the centralized Vulkan context.
 */
IMGUIManager::IMGUIManager()
    : imguiPool(VK_NULL_HANDLE) { }

/**
 * @brief Destructor: Triggers the cleanup of ImGui and Vulkan handles.
 */
IMGUIManager::~IMGUIManager() {
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
void IMGUIManager::init(GLFWwindow* const window, const VulkanEngine* const engine) {
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
        throw std::runtime_error("IMGUIManager: Failed to create descriptor pool!");
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
        throw std::runtime_error("IMGUIManager: Failed to initialize Vulkan backend!");
    }
}

/**
 * @brief Constructs the diagnostic UI widgets for the current frame.
 */
 /**
  * @brief Constructs the diagnostic UI widgets for the current frame.
  * Refactored to properly synchronize local UI state with InputManager setters.
  */
void IMGUIManager::update(InputManager* const input, const StatsManager* const stats,
    PointLight* const light, const TimeManager* const time,
    ClimateManager* const climate) const
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    static_cast<void>(ImGui::Begin("Snow Globe Engine Diagnostics"));

    if (input != nullptr) {
        // --- 1. Control Documentation ---
        if (ImGui::CollapsingHeader("Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::BulletText("'F1', 'F2', 'F3': Switch Cameras");
            ImGui::BulletText("'c/C': Toggle Cursor Capture");
            ImGui::BulletText("WASD: Move | QE: Pan | Mouse: Look");
            ImGui::BulletText("'l/L': Toggle Shading Model");
            ImGui::BulletText("'t/T': Simulation Speed Scaling");
            ImGui::BulletText("'r/R': Reset Simulation State");
        }

        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Active Camera: %s", input->getActiveCameraLabel());

        // --- 2. Performance Telemetry ---
        ImGui::Separator();
        ImGui::Text("Performance: %.1f FPS", static_cast<double>(ImGui::GetIO().Framerate));
        if (stats != nullptr) {
            ImGui::PlotLines("FPS History", stats->getHistoryData(),
                static_cast<int>(stats->getCount()),
                static_cast<int>(stats->getOffset()), nullptr, 0.0f, 165.0f, ImVec2(0, 80));
        }

        // --- 3. Simulation Scaling ---
        if (time != nullptr) {
            ImGui::Separator();
            ImGui::Text("Simulation Scale: %.2fx", static_cast<double>(time->getScale()));
        }

        // --- 4. Weather & Climate Management ---
        if (climate != nullptr) {
            ImGui::Separator();
            ImGui::Text("Weather Management");

            static const char* modeNames[] = { "Normal (Auto)", "Summer (Lock)", "Rain (Lock)", "Snow (Lock)" };
            int currentModeIdx = static_cast<int>(climate->getWeatherMode());

            if (ImGui::Combo("Mode", &currentModeIdx, modeNames, static_cast<int>(IM_ARRAYSIZE(modeNames)))) {
                climate->setWeatherMode(static_cast<WeatherMode>(currentModeIdx));
            }

            ImVec4 seasonColor{ 1.0f, 1.0f, 1.0f, 1.0f };
            switch (climate->getWeatherState()) {
            case WeatherState::RAIN:   seasonColor = ImVec4(0.2f, 0.5f, 1.0f, 1.0f); break;
            case WeatherState::SNOW:   seasonColor = ImVec4(0.6f, 0.8f, 1.0f, 1.0f); break;
            case WeatherState::SUMMER:
            default:                   seasonColor = ImVec4(1.0f, 0.4f, 0.1f, 1.0f); break;
            }
            ImGui::TextColored(seasonColor, "Season: %s", climate->getSeasonLabel());
        }

        // --- 5. System Overrides (Corrected to use Setters) ---
        ImGui::Separator();

        // Gouraud Shading
        bool gouraud = input->getGouraudEnabled();
        if (ImGui::Checkbox("Gouraud Shading", &gouraud)) { input->setGouraudEnabled(gouraud); }

        // Particle Toggles
        bool dust = input->getDustEnabled();
        if (ImGui::Checkbox("Simulate Dust", &dust)) { input->setDustEnabled(dust); }

        bool fire = input->getFireEnabled();
        if (ImGui::Checkbox("Simulate Fire", &fire)) { input->setFireEnabled(fire); }

        bool smoke = input->getSmokeEnabled();
        if (ImGui::Checkbox("Simulate Smoke", &smoke)) { input->setSmokeEnabled(smoke); }

        bool rain = input->getRainEnabled();
        if (ImGui::Checkbox("Simulate Rain", &rain)) { input->setRainEnabled(rain); }

        bool snow = input->getSnowEnabled();
        if (ImGui::Checkbox("Simulate Snow", &snow)) { input->setSnowEnabled(snow); }

        bool bloom = input->getBloomEnabled();
        if (ImGui::Checkbox("Post-Process Bloom", &bloom)) { input->setBloomEnabled(bloom); }

        // --- 6. Lighting Control ---
        ImGui::Separator();
        bool orbit = input->getAutoOrbit();
        if (ImGui::Checkbox("Auto-Orbit Sun", &orbit)) { input->setAutoOrbit(orbit); }

        if (light != nullptr && !input->getAutoOrbit()) {
            glm::vec3 pos = light->getPosition();
            if (ImGui::SliderFloat3("Manual Sun", &pos.x, -5.0f, 5.0f)) {
                light->setPosition(pos);
            }
        }

        float intensity = input->getIntensityMod();
        if (ImGui::SliderFloat("Intensity", &intensity, 0.0f, 5.0f)) {
            input->setIntensityMod(intensity);
        }

        glm::vec3 currentColor = input->getColorMod();
        if (ImGui::ColorEdit3("Color Tint", &currentColor[0])) {
            input->setColorMod(currentColor);
        }
    }

    ImGui::End();
    ImGui::Render();
}

/**
 * @brief Records ImGui render commands into the current frame's command buffer.
 */
void IMGUIManager::draw(const VkCommandBuffer cb) const {
    ImDrawData* const data = ImGui::GetDrawData();
    if (data != nullptr) {
        ImGui_ImplVulkan_RenderDrawData(data, cb);
    }
}

/**
 * @brief Shuts down ImGui and destroys the local descriptor pool.
 */
void IMGUIManager::cleanup() {
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