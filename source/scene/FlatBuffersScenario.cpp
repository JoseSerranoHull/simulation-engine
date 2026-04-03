/* parasoft-begin-suppress ALL */
#include <filesystem>
/* parasoft-end-suppress ALL */

#include "scene/FlatBuffersScenario.h"
#include "scene/fb/FBSceneAdapter.h"
#include "scene/fb/FBSceneContext.h"
#include "systems/AnimationSystem.h"
#include "graphics/ShaderModule.h"
#include "graphics/GraphicsPipeline.h"
#include "graphics/GpuUploadContext.h"
#include "graphics/PostProcessBackend.h"
#include "core/ServiceLocator.h"
#include "core/EngineOrchestrator.h"
#include "core/Logger.h"
#include "services/InputService.h"
#include "services/Camera.h"
#include "ecs/EntityManager.h"
#include "assets/AssetManager.h"
#include "scene/Scene.h"

/* parasoft-begin-suppress ALL */
#include "imgui.h"
/* parasoft-end-suppress ALL */

using namespace GE::Graphics;
using namespace GE::Assets;

namespace GE {

// ===========================================================================
// SECTION 1: Constructor
// ===========================================================================

FlatBuffersScenario::FlatBuffersScenario(std::string binaryPath, bool useOwnerColors)
    : m_useOwnerColors(useOwnerColors)
{
    m_configPath = std::move(binaryPath);
}

// ===========================================================================
// SECTION 2: OnLoad
// ===========================================================================

void FlatBuffersScenario::OnLoad(GpuUploadContext& ctx) {
    // 1. Build the base 8 scenario-scoped pipelines (indices 0–7)
    createMaterialPipelines();

    // 2. Append flat-color pipeline at index 8:
    //    - No Set 1 material descriptor (includeMaterialSet = false)
    //    - Only push constant is mat4 model (64 bytes, VERTEX stage only)
    VulkanContext* vkCtx = ServiceLocator::GetContext();
    const PostProcessBackend* pp = ServiceLocator::GetExperience()->GetPostProcessBackend();
    const VkRenderPass offscreenPass = pp->getOffscreenRenderPass();
    const VkSampleCountFlagBits msaa = vkCtx->msaaSamples;

    m_shaderModules.push_back(std::make_unique<ShaderModule>(
        "./shaders/flatcolor_vert.spv", VK_SHADER_STAGE_VERTEX_BIT));   // [12]
    m_shaderModules.push_back(std::make_unique<ShaderModule>(
        "./shaders/flatcolor_frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT)); // [13]

    m_pipelines.push_back(std::make_unique<GraphicsPipeline>(
        offscreenPass,
        VK_NULL_HANDLE,                              // no Set 1 material layout
        m_shaderModules[12].get(),
        m_shaderModules[13].get(),
        true, false, true, msaa,
        static_cast<uint32_t>(sizeof(glm::mat4)),
        VK_SHADER_STAGE_VERTEX_BIT,
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        false));                                      // includeMaterialSet = false

    // 3. Load and adapt the FlatBuffers binary
    GE::Scene::FB::FBSceneAdapter adapter;
    if (!adapter.load(m_configPath)) {
        GE_LOG_ERROR("FlatBuffersScenario: Failed to load: " + m_configPath);
        return;
    }

    // 4. Build adaptation context
    GE::ECS::EntityManager* em = ServiceLocator::GetEntityManager();
    AssetManager*           am = ServiceLocator::GetAssetManager();
    GE::Scene::Scene*     scn  = ServiceLocator::GetScene();

    GE::Scene::FB::FBSceneContext adaptCtx;
    adaptCtx.em          = em;
    adaptCtx.am          = am;
    adaptCtx.scene       = scn;
    adaptCtx.uploadCtx   = &ctx;
    adaptCtx.pipelines   = &m_pipelines;
    adaptCtx.ownedModels = &m_ownedModels;
    adaptCtx.useOwnerColors = m_useOwnerColors;

    // 5. Single-pass adaptation: cameras → materials → objects
    adapter.adaptToECS(adaptCtx);

    // 6. Apply cameras
    m_sceneName = adapter.getSceneName();
    buildCamerasFromContext(adaptCtx);
    if (!m_cameras.empty()) {
        applyActiveCamera();
    }

    // 7. Scan ./config/ for available .bin files
    scanSceneDirectory();

    // 8. Register AnimationSystem stub
    auto* as = new GE::Systems::AnimationSystem();
    m_animationSystem = as;
    em->RegisterSystem(as);

    GE_LOG_INFO("FlatBuffersScenario: Loaded '" + m_sceneName + "' from " + m_configPath);
}

// ===========================================================================
// SECTION 3: OnUpdate
// ===========================================================================

void FlatBuffersScenario::OnUpdate(float /*dt*/, float /*totalTime*/) {
    // No per-frame logic at this iteration — physics/animation deferred
}

// ===========================================================================
// SECTION 4: OnUnload
// ===========================================================================

void FlatBuffersScenario::OnUnload() {
    GE::ECS::EntityManager* em = ServiceLocator::GetEntityManager();

    if ((m_animationSystem != nullptr) && (em != nullptr)) {
        em->UnregisterSystemByID(m_animationSystem->GetID());
        m_animationSystem = nullptr;
    }

    m_cameras.clear();
    m_availableScenes.clear();
    m_ownedModels.clear();
    m_pipelines.clear();
    m_shaderModules.clear();
}

// ===========================================================================
// SECTION 5: OnGUI
// ===========================================================================

void FlatBuffersScenario::OnGUI() {
    // NOTE: Called from within DebugOverlay::DrawMainMenuBar() which already has
    // BeginMainMenuBar() open. Do NOT call BeginMainMenuBar() here — just add menus directly.

    // --- Camera menu ---
    if (ImGui::BeginMenu("Camera")) {
        for (int i = 0; i < static_cast<int>(m_cameras.size()); ++i) {
            const bool isActive = (i == m_activeCameraIndex);
            if (ImGui::MenuItem(m_cameras[i].name.c_str(), nullptr, isActive)) {
                m_activeCameraIndex = i;
                applyActiveCamera();
            }
        }
        if (m_cameras.empty()) {
            ImGui::TextDisabled("No cameras in scene");
        }
        ImGui::EndMenu();
    }

    // --- Display mode menu ---
    if (ImGui::BeginMenu("Display")) {
        if (ImGui::MenuItem("Owner Colors",    nullptr,  m_useOwnerColors)) {
            if (!m_useOwnerColors) {
                auto* exp = ServiceLocator::GetExperience();
                if (exp != nullptr) {
                    exp->requestScenarioChange(m_configPath);
                }
            }
        }
        if (ImGui::MenuItem("Material Colors", nullptr, !m_useOwnerColors)) {
            if (m_useOwnerColors) {
                auto* exp = ServiceLocator::GetExperience();
                if (exp != nullptr) {
                    exp->requestScenarioChange(m_configPath);
                }
            }
        }
        ImGui::EndMenu();
    }

    // --- Scene switcher menu ---
    if (ImGui::BeginMenu("Scene")) {
        for (const auto& path : m_availableScenes) {
            const std::string label = std::filesystem::path(path).filename().string();
            const bool isCurrent = (path == m_configPath);
            if (ImGui::MenuItem(label.c_str(), nullptr, isCurrent)) {
                auto* exp = ServiceLocator::GetExperience();
                if (exp != nullptr) {
                    exp->requestScenarioChange(path);
                }
            }
        }
        if (m_availableScenes.empty()) {
            ImGui::TextDisabled("No .bin files in ./config/flatbufferConfig/");
        }
        ImGui::EndMenu();
    }
}

// ===========================================================================
// SECTION 6: Helpers
// ===========================================================================

void FlatBuffersScenario::buildCamerasFromContext(const GE::Scene::FB::FBSceneContext& ctx) {
    m_cameras.clear();
    for (const auto& rec : ctx.cameras) {
        FBCameraState state;
        state.name      = rec.name;
        state.position  = rec.position;
        state.direction = rec.direction;
        state.fov       = rec.fov;
        state.isOrtho   = rec.isOrtho;
        state.orthoSize = rec.orthoSize;
        m_cameras.push_back(state);
    }
    m_activeCameraIndex = 0;
}

void FlatBuffersScenario::applyActiveCamera() const {
    if (m_cameras.empty()) { return; }

    InputService* input = ServiceLocator::GetInput();
    if (input == nullptr) { return; }

    Camera* cam = input->getActiveCamera();
    if (cam == nullptr) { return; }

    const FBCameraState& c = m_cameras[static_cast<std::size_t>(m_activeCameraIndex)];

    cam->setPosition(c.position);

    // Derive Euler angles from the direction vector (same math as Camera ctor)
    const glm::vec3 dir = glm::normalize(c.direction);
    const float pitch = glm::degrees(static_cast<float>(std::asin(static_cast<double>(dir.y))));
    const float yaw   = glm::degrees(static_cast<float>(
        std::atan2(static_cast<double>(dir.z), static_cast<double>(dir.x))));
    cam->setYaw(yaw);
    cam->setPitch(pitch);

    cam->setProjectionMode(c.isOrtho
        ? Camera::ProjectionMode::ORTHOGRAPHIC
        : Camera::ProjectionMode::PERSPECTIVE);
    cam->setZoom(c.isOrtho ? c.orthoSize : c.fov);
}

void FlatBuffersScenario::scanSceneDirectory() {
    m_availableScenes.clear();

    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator("./config/flatbufferConfig/", ec)) {
        if (ec) { break; }
        if (entry.path().extension() == ".bin") {
            m_availableScenes.push_back(entry.path().string());
        }
    }
}

} // namespace GE
