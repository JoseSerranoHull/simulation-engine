/* parasoft-begin-suppress ALL */
#include <filesystem>
#include <algorithm>
/* parasoft-end-suppress ALL */

#include "scene/FlatBuffersScenario.h"
#include "scene/fb/FBSceneAdapter.h"
#include "scene/fb/FBSceneContext.h"
#include "systems/AnimationSystem.h"
#include "systems/ClothSystem.h"
#include "systems/FlockingSystem.h"
#include "systems/PhysicsSystem.h"
#include "systems/SpawnerSystem.h"
#include "systems/ScriptSystem.h"
#include "systems/ColliderVisualizerSystem.h"
#include "graphics/ShaderModule.h"
#include "graphics/GraphicsPipeline.h"
#include "graphics/GpuUploadContext.h"
#include "graphics/PostProcessBackend.h"
#include "core/ServiceLocator.h"
#include "core/EngineOrchestrator.h"
#include "graphics/VulkanContext.h"
#include "core/NetworkBridge.h"
#include "core/Logger.h"
#include "services/InputService.h"
#include "services/Camera.h"
#include "ecs/EntityManager.h"
#include "assets/AssetManager.h"
#include "assets/Vertex.h"
#include "components/ClothComponent.h"
#include "components/FlockingComponent.h"
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
    // Clear any stale dead-reckoning state from the previous scene
    // so old entity IDs don't get applied to new scene entities.
    if (auto* nb = ServiceLocator::GetNetworkBridge()) {
        nb->ClearRemoteStates();
    }

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
        false, false, true, msaa,                    // culling OFF — double-sided geometry
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

    // 8. Build MaterialInteraction registry from adapted data
    m_interactionRegistry.Clear();
    for (const auto& rec : adaptCtx.interactions) {
        m_interactionRegistry.Register(
            rec.materialA, rec.materialB,
            rec.restitution, rec.staticFriction, rec.dynamicFriction);
    }

    // 9. Register AnimationSystem first so prevPosition is set before PhysicsSystem resolves collisions
    auto* as = new GE::Systems::AnimationSystem();
    m_animationSystem = as;
    em->RegisterSystem(as);

    // 10a. Register ClothSystem (Physics stage, runs before PhysicsSystem)
    auto* cs = new GE::Systems::ClothSystem();
    m_clothSystem = cs;
    em->RegisterSystem(cs);

    // 10b. Register FlockingSystem (GameLogic stage — runs after Physics)
    auto* fks = new GE::Systems::FlockingSystem();
    m_flockingSystem = fks;
    em->RegisterSystem(fks);

    // 10c. Register ScriptSystem (GameLogic stage — runs after FlockingSystem)
    auto* scs = new GE::Systems::ScriptSystem(nullptr);  // PhysicsSystem ptr provided after registration
    m_scriptSystem = scs;
    em->RegisterSystem(scs);

    // 10d. Register PhysicsSystem and wire up the interaction registry
    auto* ps = new GE::Systems::PhysicsSystem();
    ps->SetRegistry(&m_interactionRegistry);
    m_physicsSystem = ps;
    em->RegisterSystem(ps);

    // 11. Move prefab registry into member (keeps PrefabTemplate* pointers valid for scene lifetime)
    m_prefabRegistry = std::move(adaptCtx.prefabRegistry);

    // 11b. Wire flock spawn config so FlockingSystem::Restart() can scatter agents correctly
    if (m_flockingSystem != nullptr) {
        m_flockingSystem->m_spawnOrigin = adaptCtx.flockSpawnOrigin;
        m_flockingSystem->m_spawnRadius = adaptCtx.flockSpawnRadius;
    }

    // Build SpawnerComponent entities from adapted spawner records
    for (auto& rec : adaptCtx.spawners) {
        const GE::ECS::EntityID spawnerId = em->CreateEntity();
        GE::Components::Transform spawnTr;
        spawnTr.m_localPosition = rec.fixedPos;
        em->AddComponent(spawnerId, spawnTr);

        GE::Components::Tag spawnTag;
        spawnTag.m_name = rec.name;
        em->AddComponent(spawnerId, spawnTag);

        GE::Components::SpawnerComponent sc;
        sc.startTime    = rec.startTime;
        sc.isBurst      = rec.isBurst;
        sc.burstCount   = rec.maxCount;
        sc.maxCount     = rec.maxCount;
        sc.interval     = rec.interval;
        sc.locationType = rec.locationType;
        sc.fixedPos     = rec.fixedPos;
        sc.boxMin       = rec.boxMin;
        sc.boxMax       = rec.boxMax;
        sc.sphereCenter = rec.sphereCenter;
        sc.sphereRadius = rec.sphereRadius;
        sc.linVelMin    = rec.linVelMin;
        sc.linVelMax    = rec.linVelMax;
        sc.angVelMin    = rec.angVelMin;
        sc.angVelMax    = rec.angVelMax;
        sc.ownerPeerId  = rec.ownerPeerId;
        sc.isSequential = rec.isSequential;

        // Resolve prefab pointer from registry (stays valid until OnUnload clears m_prefabRegistry)
        if (!rec.prefabRef.empty()) {
            const auto it = m_prefabRegistry.find(rec.prefabRef);
            sc.prefabTemplate = (it != m_prefabRegistry.end()) ? &it->second : nullptr;
        }

        em->AddComponent(spawnerId, sc);
    }

    // 12. Register SpawnerSystem
    auto* ss = new GE::Systems::SpawnerSystem();
    m_spawnerSystem = ss;
    em->RegisterSystem(ss);

    // 13. Register ColliderVisualizerSystem (debug wireframe overlay; on by default)
    auto* vs = new GE::Systems::ColliderVisualizerSystem(ctx);
    vs->m_enabled = true;   // toggle via Simulation → Show Collider Wireframes
    m_visualizerSystem = vs;
    em->RegisterSystem(vs);

    // Initialise default peer IDs for the three remote slots (2, 3, 4)
    for (int i = 0; i < 3; ++i) {
        m_peerEntries[i].peerId = i + 2;
    }

    GE_LOG_INFO("FlatBuffersScenario: Loaded '" + m_sceneName + "' from " + m_configPath);
    GE_LOG_INFO("FlatBuffersScenario: Network / Simulation / Display menus are only active when a .bin FlatBuffers scene is loaded.");
}

// ===========================================================================
// SECTION 3: OnUpdate
// ===========================================================================

void FlatBuffersScenario::OnUpdate(float dt, float /*totalTime*/) {
    GE::ECS::EntityManager* em = ServiceLocator::GetEntityManager();
    if (em == nullptr) { return; }

    // Fire post-connect ECS syncs the first frame after auto-connect completes.
    // Safe here: OnUpdate runs on the physics thread which owns ECS data.
    {
        GE::NetworkBridge* nb = ServiceLocator::GetNetworkBridge();
        if (nb != nullptr && nb->ConsumePostConnectSync()) {
            nb->BroadcastAnimationStates();
            // Mirror auto-assigned values into the manual UI fields (cosmetic only).
            if (nb->GetService() != nullptr) {
                m_localPeerId = static_cast<int>(nb->GetService()->GetLocalPeerId());
                m_localPort   = 54000 + m_localPeerId - 1;
            }
            m_connectionMethod = ConnectionMethod::Auto;
        }
    }

    // Refresh cloth vertex buffers from current particle positions (HOST_COHERENT — no flush needed).

    auto& clothArr = em->GetCompArr<GE::Components::ClothComponent>();
    const uint32_t count = clothArr.GetCount();
    for (uint32_t i = 0U; i < count; ++i) {
        const GE::Components::ClothComponent& cc = clothArr.Data()[i];
        if (cc.mappedVertices == nullptr || cc.vertexCount == 0U) { continue; }

        auto* verts = static_cast<GE::Assets::Vertex*>(cc.mappedVertices);
        const int R = cc.rows;
        const int C = cc.cols;

        for (uint32_t vi = 0U; vi < cc.vertexCount; ++vi) {
            const int r = static_cast<int>(vi) / C;
            const int c = static_cast<int>(vi) % C;
            const glm::vec3 center = cc.particles[vi].position;

            verts[vi].position = center;

            // Per-vertex normal: central-difference cross product of deformed neighbor positions.
            // At boundary vertices the missing neighbor is replaced by the current vertex (half step).
            const glm::vec3 right = (c < C-1) ? cc.particles[vi+1].position : center;
            const glm::vec3 left  = (c > 0)   ? cc.particles[vi-1].position : center;
            const glm::vec3 above = (r > 0)   ? cc.particles[vi-C].position : center;
            const glm::vec3 below = (r < R-1) ? cc.particles[vi+C].position : center;
            const glm::vec3 dX = right - left;
            const glm::vec3 dY = above - below;
            const glm::vec3 n  = glm::cross(dX, dY);
            verts[vi].normal = (glm::dot(n, n) > 1e-8f) ? glm::normalize(n) : glm::vec3(0.0f, 0.0f, 1.0f);

            // Per-vertex tangent: U direction (toward increasing column) for TBN normal mapping.
            const glm::vec3 tang = (c < C-1) ? (cc.particles[vi+1].position - center)
                                              : (center - cc.particles[vi-1].position);
            verts[vi].tangent = (glm::dot(tang, tang) > 1e-8f) ? glm::normalize(tang) : glm::vec3(1.0f, 0.0f, 0.0f);

            // Heat-based color: cold = cloth color, heating → orange, burned → near-black
            const float heat = cc.particles[vi].heat;
            if (cc.particles[vi].burned) {
                verts[vi].color = glm::vec3{ 0.05f, 0.05f, 0.05f };
            } else if (heat > 0.0f) {
                const glm::vec3 orange{ 1.0f, 0.2f, 0.0f };
                verts[vi].color = glm::mix(cc.color, orange, glm::clamp(heat, 0.0f, 1.0f));
            } else {
                verts[vi].color = cc.color;
            }
        }
    }

    // Rebuild index buffer: replace quads that have any burned corner with degenerate
    // triangles (all indices = 0) so the GPU discards them, creating a visible hole.
    // Buffer is HOST_VISIBLE | HOST_COHERENT — no vkFlushMappedMemoryRanges needed.
    for (uint32_t i = 0U; i < count; ++i) {
        GE::Components::ClothComponent& cc = clothArr.Data()[i];
        if (cc.mappedVertices == nullptr || cc.indexCount == 0U) { continue; }

        const int R = cc.rows, C = cc.cols;
        auto* idxPtr = reinterpret_cast<uint32_t*>(
            static_cast<uint8_t*>(cc.mappedVertices) + cc.indexOffset);

        uint32_t w = 0U;
        for (int r = 0; r < R - 1; ++r) {
            for (int c = 0; c < C - 1; ++c) {
                const uint32_t tl  = static_cast<uint32_t>(r * C + c);
                const uint32_t tr_ = tl + 1U;
                const uint32_t bl  = static_cast<uint32_t>((r + 1) * C + c);
                const uint32_t br  = bl + 1U;

                const bool anyBurned =
                    cc.particles[tl].burned  || cc.particles[tr_].burned ||
                    cc.particles[bl].burned  || cc.particles[br].burned;

                if (anyBurned) {
                    idxPtr[w++]=0U; idxPtr[w++]=0U; idxPtr[w++]=0U;
                    idxPtr[w++]=0U; idxPtr[w++]=0U; idxPtr[w++]=0U;
                } else {
                    idxPtr[w++]=tl;  idxPtr[w++]=bl;  idxPtr[w++]=tr_;
                    idxPtr[w++]=tr_; idxPtr[w++]=bl;  idxPtr[w++]=br;
                }
            }
        }
    }
}

// ===========================================================================
// SECTION 4: OnUnload
// ===========================================================================

void FlatBuffersScenario::OnUnload() {
    // Drop the network connection so each scene starts fresh.
    // Also prevents stale state if the same scene is restarted.
    disconnectNetwork();

    GE::ECS::EntityManager* em = ServiceLocator::GetEntityManager();

    // Destroy cloth GPU resources before models are cleared (Mesh doesn't own the buffer)
    if (em != nullptr) {
        GE::Graphics::VulkanContext* vkCtx = ServiceLocator::GetContext();
        if (vkCtx != nullptr && vkCtx->device != VK_NULL_HANDLE) {
            auto& clothArr = em->GetCompArr<GE::Components::ClothComponent>();
            for (uint32_t i = 0U; i < clothArr.GetCount(); ++i) {
                GE::Components::ClothComponent& cc = clothArr.Data()[i];
                if (cc.mappedVertices != nullptr) {
                    vkUnmapMemory(vkCtx->device, cc.vertexMemory);
                    cc.mappedVertices = nullptr;
                }
                if (cc.vertexBuffer != VK_NULL_HANDLE) {
                    vkDestroyBuffer(vkCtx->device, cc.vertexBuffer, nullptr);
                    cc.vertexBuffer = VK_NULL_HANDLE;
                }
                if (cc.vertexMemory != VK_NULL_HANDLE) {
                    vkFreeMemory(vkCtx->device, cc.vertexMemory, nullptr);
                    cc.vertexMemory = VK_NULL_HANDLE;
                }
            }
        }
    }

    if ((m_scriptSystem != nullptr) && (em != nullptr)) {
        em->UnregisterSystemByID(m_scriptSystem->GetID());
        m_scriptSystem = nullptr;
    }

    if ((m_clothSystem != nullptr) && (em != nullptr)) {
        em->UnregisterSystemByID(m_clothSystem->GetID());
        m_clothSystem = nullptr;
    }

    if ((m_flockingSystem != nullptr) && (em != nullptr)) {
        em->UnregisterSystemByID(m_flockingSystem->GetID());
        m_flockingSystem = nullptr;
    }

    if ((m_physicsSystem != nullptr) && (em != nullptr)) {
        em->UnregisterSystemByID(m_physicsSystem->GetID());
        m_physicsSystem = nullptr;
    }

    if ((m_animationSystem != nullptr) && (em != nullptr)) {
        em->UnregisterSystemByID(m_animationSystem->GetID());
        m_animationSystem = nullptr;
    }

    if ((m_spawnerSystem != nullptr) && (em != nullptr)) {
        em->UnregisterSystemByID(m_spawnerSystem->GetID());
        m_spawnerSystem = nullptr;
    }

    if ((m_visualizerSystem != nullptr) && (em != nullptr)) {
        em->UnregisterSystemByID(m_visualizerSystem->GetID());
        m_visualizerSystem = nullptr;
    }

    m_interactionRegistry.Clear();
    m_prefabRegistry.clear();   // invalidates all sc.prefabTemplate pointers — safe after systems unregistered
    m_cameras.clear();
    m_availableScenes.clear();
    m_ownedModels.clear();      // destroys shared Mesh objects last (prefabTemplate.sharedMesh already cleared)
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

    // --- Display mode menu (LOCAL-ONLY toggle — no network broadcast) ---
    if (ImGui::BeginMenu("Display")) {
        if (ImGui::MenuItem("Owner Colors",    nullptr,  m_useOwnerColors)) {
            if (!m_useOwnerColors) {
                auto* exp = ServiceLocator::GetExperience();
                if (exp != nullptr) {
                    // Deferred local-only reload with owner colors ON
                    exp->requestScenarioChange(m_configPath, true);
                }
            }
        }
        if (ImGui::MenuItem("Material Colors", nullptr, !m_useOwnerColors)) {
            if (m_useOwnerColors) {
                auto* exp = ServiceLocator::GetExperience();
                if (exp != nullptr) {
                    // Deferred local-only reload with owner colors OFF
                    exp->requestScenarioChange(m_configPath, false);
                }
            }
        }
        ImGui::EndMenu();
    }

    // --- Simulation frequency controls ---
    if (ImGui::BeginMenu("Simulation")) {
        auto* exp = ServiceLocator::GetExperience();
        if (exp != nullptr) {
            ImGui::SliderFloat("Physics Hz",  &exp->m_physicsHz,  1.0f,   2000.0f, "%.0f Hz");
            ImGui::SliderFloat("Graphics Hz", &exp->m_graphicsHz, 0.0f,    300.0f, "%.0f Hz");
            ImGui::TextDisabled("Graphics Hz = 0 means uncapped");

            ImGui::Separator();
            ImGui::Text("Actual graphics: %.1f Hz", static_cast<double>(ImGui::GetIO().Framerate));
            ImGui::Text("Actual physics:  %.0f Hz", static_cast<double>(exp->m_physicsHz));
        }

        if (m_physicsSystem != nullptr) {
            ImGui::Separator();
            ImGui::Checkbox("Gravity", &m_physicsSystem->m_gravityEnabled);

            static const char* intMethodNames[] = { "Euler", "Semi-Implicit", "RK4" };
            int methodIdx = static_cast<int>(m_physicsSystem->m_integrationMethod);
            if (ImGui::Combo("Integration", &methodIdx, intMethodNames, 3)) {
                m_physicsSystem->m_integrationMethod = static_cast<GE::Systems::IntegrationMethod>(methodIdx);
            }
            ImGui::SliderInt("Solver Iterations", &m_physicsSystem->m_solverIterations, 1, 8);
        }

        ImGui::Separator();
        bool paused = IsPaused();
        if (ImGui::Checkbox("Pause Simulation", &paused)) {
            SetPaused(paused);
        }
        if (IsPaused()) {
            ImGui::SameLine();
            auto* exp = ServiceLocator::GetExperience();
            if (exp != nullptr && ImGui::Button("Step")) {
                exp->stepSimulation(1.0f / std::max(exp->m_physicsHz, 1.0f));
            }
        }

        if (m_visualizerSystem != nullptr) {
            ImGui::Separator();
            ImGui::Checkbox("Show Collider Wireframes", &m_visualizerSystem->m_enabled);
        }

        ImGui::EndMenu();
    }

    // --- Network menu ---
    if (ImGui::BeginMenu("Network")) {
        GE::NetworkBridge* bridge = ServiceLocator::GetNetworkBridge();
        GE::Networking::NetworkService* svc = (bridge != nullptr)
            ? bridge->GetService() : nullptr;

        // ── Auto Connect ─────────────────────────────────────────────────────
        {
            using ACS = GE::NetworkBridge::AutoConnectState;
            const bool autoConnected  = (m_connectionMethod == ConnectionMethod::Auto);
            const bool manualConnected= (m_connectionMethod == ConnectionMethod::Manual);
            const bool discovering    = (bridge != nullptr) &&
                bridge->GetAutoConnectState() == ACS::Discovering;

            if (autoConnected) {
                // Show Disconnect in red
                ImGui::PushStyleColor(ImGuiCol_Button,        { 0.7f, 0.15f, 0.15f, 1.0f });
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.9f, 0.2f,  0.2f,  1.0f });
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,  { 0.5f, 0.1f,  0.1f,  1.0f });
                if (ImGui::Button("Disconnect##Auto")) { disconnectNetwork(); }
                ImGui::PopStyleColor(3);
            } else {
                // Show Auto Connect, disabled while discovering or locked out by manual
                if (discovering || manualConnected) { ImGui::BeginDisabled(); }
                if (ImGui::Button("Auto Connect") && bridge != nullptr) {
                    bridge->BeginAutoConnect();
                }
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                    ImGui::SetTooltip(manualConnected
                        ? "Disconnect manual connection first."
                        : "Broadcasts a probe to peers 1-4 (ports 54000-54003).\n"
                          "Stagger clicks ~2 s apart to avoid slot conflicts.");
                }
                if (discovering || manualConnected) { ImGui::EndDisabled(); }
            }

            if (bridge != nullptr) {
                const ACS state = bridge->GetAutoConnectState();
                ImVec4 col;
                if      (state == ACS::Done)        { col = { 0.2f, 1.0f, 0.2f, 1.0f }; }
                else if (state == ACS::Failed)      { col = { 1.0f, 0.3f, 0.3f, 1.0f }; }
                else if (state == ACS::Discovering) { col = { 1.0f, 1.0f, 0.3f, 1.0f }; }
                else                                { col = { 0.6f, 0.6f, 0.6f, 1.0f }; }
                ImGui::SameLine(0.0f, 8.0f);
                ImGui::TextColored(col, "%s", bridge->GetAutoConnectStatus().c_str());

                if (autoConnected && svc != nullptr) {
                    ImGui::Indent(8.0f);
                    ImGui::Text("My IP: %s   Port: %d   Peer ID: %d",
                        svc->GetLocalIPString().c_str(),
                        54000 + static_cast<int>(svc->GetLocalPeerId()) - 1,
                        static_cast<int>(svc->GetLocalPeerId()));
                    ImGui::Unindent(8.0f);
                }
            }
        }
        ImGui::Separator();
        ImGui::TextDisabled("── Manual Configuration ──");

        // Shared color palette (matches FBSceneContext::ownerColors exactly)
        static constexpr ImVec4 kPeerColors[4] = {
            { 1.0f, 0.2f, 0.2f, 1.0f },   // Peer 1 — Red
            { 0.2f, 1.0f, 0.2f, 1.0f },   // Peer 2 — Green
            { 0.2f, 0.4f, 1.0f, 1.0f },   // Peer 3 — Blue
            { 1.0f, 1.0f, 0.2f, 1.0f },   // Peer 4 — Yellow
        };
        static constexpr const char* kPeerColorNames[4] = { "Red", "Green", "Blue", "Yellow" };

        // Local peer ID — cycle with - / + buttons (read-only display)
        ImGui::Text("Local Peer");
        ImGui::SameLine(0.0f, 4.0f);
        if (ImGui::Button("-##LPDec")) { m_localPeerId = (m_localPeerId > 1) ? m_localPeerId - 1 : 4; }
        ImGui::SameLine(0.0f, 2.0f);
        ImGui::Text("%d", m_localPeerId);
        ImGui::SameLine(0.0f, 2.0f);
        if (ImGui::Button("+##LPInc")) { m_localPeerId = (m_localPeerId < 4) ? m_localPeerId + 1 : 1; }
        ImGui::SameLine(0.0f, 6.0f);
        {
            const int ci = std::clamp(m_localPeerId - 1, 0, 3);
            ImGui::ColorButton("##mycolor", kPeerColors[ci],
                ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoPicker,
                ImVec2(14.0f, 14.0f));
            ImGui::SameLine(0.0f, 4.0f);
            ImGui::TextDisabled("(%s)", kPeerColorNames[ci]);
        }
        ImGui::SameLine(0.0f, 8.0f);
        ImGui::Text("Port");
        ImGui::SameLine(0.0f, 4.0f);
        ImGui::SetNextItemWidth(95.0f);
        if (ImGui::InputInt("##LocalPort", &m_localPort)) {
            m_localPort = std::clamp(m_localPort, 1024, 65535);
        }

        ImGui::Separator();
        ImGui::TextDisabled("Remote Peers (up to 3)");

        const char* peerLabels[3] = { "Peer A", "Peer B", "Peer C" };
        for (int i = 0; i < 3; ++i) {
            ImGui::PushID(i);

            // Peer label + ID cycle buttons (read-only display)
            ImGui::Text("%s  ID:", peerLabels[i]);
            ImGui::SameLine(0.0f, 4.0f);
            if (ImGui::Button("-##PDec")) {
                m_peerEntries[i].peerId = (m_peerEntries[i].peerId > 1)
                    ? m_peerEntries[i].peerId - 1 : 4;
                if (m_peerEntries[i].peerId == m_localPeerId)
                    m_peerEntries[i].peerId = (m_peerEntries[i].peerId > 1)
                        ? m_peerEntries[i].peerId - 1 : 4;
            }
            ImGui::SameLine(0.0f, 2.0f);
            ImGui::Text("%d", m_peerEntries[i].peerId);
            ImGui::SameLine(0.0f, 2.0f);
            if (ImGui::Button("+##PInc")) {
                m_peerEntries[i].peerId = (m_peerEntries[i].peerId < 4)
                    ? m_peerEntries[i].peerId + 1 : 1;
                if (m_peerEntries[i].peerId == m_localPeerId)
                    m_peerEntries[i].peerId = (m_peerEntries[i].peerId < 4)
                        ? m_peerEntries[i].peerId + 1 : 1;
            }
            ImGui::SameLine(0.0f, 6.0f);
            {
                const int pci = std::clamp(m_peerEntries[i].peerId - 1, 0, 3);
                ImGui::ColorButton("##pc", kPeerColors[pci],
                    ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoPicker,
                    ImVec2(14.0f, 14.0f));
            }
            ImGui::SameLine(0.0f, 6.0f);
            ImGui::Text("IP:");
            ImGui::SameLine(0.0f, 4.0f);
            ImGui::SetNextItemWidth(120.0f);
            ImGui::InputText("##ip", m_peerEntries[i].ip, sizeof(m_peerEntries[i].ip));
            ImGui::SameLine(0.0f, 6.0f);
            ImGui::Text("Port:");
            ImGui::SameLine(0.0f, 4.0f);
            ImGui::SetNextItemWidth(95.0f);
            ImGui::InputInt("##port", &m_peerEntries[i].port);
            m_peerEntries[i].port = std::clamp(m_peerEntries[i].port, 1024, 65535);
            if (m_peerEntries[i].connected) {
                ImGui::SameLine();
                ImGui::TextColored({0.2f, 1.0f, 0.2f, 1.0f}, "[OK]");
            }
            ImGui::PopID();
        }

        ImGui::Separator();
        {
            const bool manualConnected = (m_connectionMethod == ConnectionMethod::Manual);
            const bool autoConnected   = (m_connectionMethod == ConnectionMethod::Auto);

            if (manualConnected) {
                // Show Disconnect in red
                ImGui::PushStyleColor(ImGuiCol_Button,        { 0.7f, 0.15f, 0.15f, 1.0f });
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.9f, 0.2f,  0.2f,  1.0f });
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,  { 0.5f, 0.1f,  0.1f,  1.0f });
                if (ImGui::Button("Disconnect##Manual")) { disconnectNetwork(); }
                ImGui::PopStyleColor(3);
                ImGui::SameLine();
                ImGui::TextColored({0.2f, 1.0f, 0.2f, 1.0f}, "Connected (peer %d)", m_localPeerId);
            } else {
                // Show Connect; disabled while auto-connect is negotiating OR connection is active
                using ACS = GE::NetworkBridge::AutoConnectState;
                const bool discovering = (bridge != nullptr) &&
                    bridge->GetAutoConnectState() == ACS::Discovering;
                const bool canConnect = (svc != nullptr) && !autoConnected && !discovering;
                if (!canConnect) { ImGui::BeginDisabled(); }
                if (ImGui::Button("Connect")) {
                    if (svc->IsConnected()) {
                        m_netInitialised = true;
                        svc->SetLocalPeerId(static_cast<uint8_t>(m_localPeerId));
                    }
                    if (!m_netInitialised) {
                        m_netInitialised = svc->Init(static_cast<uint16_t>(m_localPort));
                        if (m_netInitialised) {
                            svc->SetLocalPeerId(static_cast<uint8_t>(m_localPeerId));
                            GE_LOG_INFO("FlatBuffersScenario: NetworkService initialised on port "
                                        + std::to_string(m_localPort));
                        }
                    }
                    if (m_netInitialised) {
                        for (int i = 0; i < 3; ++i) {
                            const char* ip = m_peerEntries[i].ip;
                            if (ip[0] != '\0') {
                                svc->AddPeer(static_cast<uint8_t>(m_peerEntries[i].peerId), ip,
                                             static_cast<uint16_t>(m_peerEntries[i].port));
                                m_peerEntries[i].connected = true;
                            }
                        }
                        GE::NetworkBridge* nb = ServiceLocator::GetNetworkBridge();
                        if (nb != nullptr) { nb->BroadcastAnimationStates(); }
                        m_connectionMethod = ConnectionMethod::Manual;
                    }
                }
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && !canConnect) {
                    ImGui::SetTooltip(autoConnected
                        ? "Disconnect auto connection first."
                        : "Auto-connect negotiation in progress...");
                }
                if (!canConnect) { ImGui::EndDisabled(); }
            }
        }

        ImGui::Separator();
        ImGui::Text("Dead Reckoning");
        if (bridge != nullptr) {
            ImGui::Text("Tracked remote entities: %zu", bridge->GetRemoteStateCount());
        } else {
            ImGui::TextDisabled("(no network bridge)");
        }

        ImGui::EndMenu();
    }

    // --- Cloth tweaking menu ---
    {
        GE::ECS::EntityManager* em = ServiceLocator::GetEntityManager();
        if (em != nullptr) {
            auto& clothArr = em->GetCompArr<GE::Components::ClothComponent>();
            if (clothArr.GetCount() > 0U && ImGui::BeginMenu("Cloth")) {
                for (uint32_t i = 0U; i < clothArr.GetCount(); ++i) {
                    GE::Components::ClothComponent& cc = clothArr.Data()[i];
                    ImGui::PushID(static_cast<int>(i));
                    if (clothArr.GetCount() > 1U) {
                        ImGui::TextDisabled("Cloth %u (%dx%d)", i, cc.rows, cc.cols);
                    }
                    ImGui::SliderFloat("Spring K",  &cc.springK,  1.0f,  1000.0f);
                    ImGui::SliderFloat("Shear K",   &cc.shearK,   0.0f,   500.0f);
                    ImGui::SliderFloat("Flexion K", &cc.flexionK, 0.0f,   250.0f);
                    ImGui::SliderFloat("Damping",   &cc.damping,  0.0f,     1.0f);
                    ImGui::SliderFloat("Wind X",    &cc.windX,  -10.0f,    10.0f);
                    ImGui::SliderFloat("Wind Z",    &cc.windZ,  -10.0f,    10.0f);

                    ImGui::Separator();
                    ImGui::Text("Tearing");
                    ImGui::SliderFloat("Tear Threshold", &cc.tearThreshold, 1.1f, 10.0f);
                    if (ImGui::Button("Reset Springs")) {
                        for (auto& s : cc.springs) { s.active = true; }
                        for (auto& p : cc.particles) { p.heat = 0.0f; p.burned = false; }
                    }

                    ImGui::Separator();
                    ImGui::Text("Burning");
                    ImGui::Checkbox("Enable Burn", &cc.burnActive);
                    ImGui::SliderFloat("Burn Radius", &cc.burnRadius, 0.1f, 5.0f);
                    ImGui::SliderFloat("Burn Rate",   &cc.burnRate,   0.1f, 5.0f);
                    if (ImGui::Button("Place Burn at Centre")) {
                        if (!cc.particles.empty()) {
                            glm::vec3 avg{ 0.0f };
                            for (const auto& p : cc.particles) { avg += p.position; }
                            cc.burnCenter = avg / static_cast<float>(cc.particles.size());
                        }
                    }
                    ImGui::DragFloat3("Burn Center", &cc.burnCenter.x, 0.05f);

                    ImGui::PopID();
                    if (i + 1U < clothArr.GetCount()) { ImGui::Separator(); }
                }
                ImGui::EndMenu();
            }
        }
    }

    // --- Flocking menu ---
    {
        if (ImGui::BeginMenu("Flocking")) {
            GE::ECS::EntityManager* em = ServiceLocator::GetEntityManager();
            auto& fkArrCheck = em->GetCompArr<GE::Components::FlockingComponent>();
            if (m_flockingSystem == nullptr || fkArrCheck.GetCount() == 0U) {
                ImGui::TextDisabled("No flocking agents in scene");
            } else {

            if (ImGui::Button("Restart Flock")) {
                m_flockingSystem->Restart(ServiceLocator::GetEntityManager());
            }
            ImGui::SameLine();
            ImGui::Checkbox("Freeze", &m_flockingSystem->m_frozen);
            ImGui::Separator();

            // Spatial mode selector
            static const char* modeNames[] = { "Brute Force", "Uniform Grid", "Octree" };
            int modeIdx = static_cast<int>(m_flockingSystem->m_spatialMode);
            if (ImGui::Combo("Spatial Mode", &modeIdx, modeNames, 3)) {
                m_flockingSystem->m_spatialMode = static_cast<GE::Systems::FlockSpatialMode>(modeIdx);
            }
            ImGui::SliderFloat("Grid Cell Size", &m_flockingSystem->m_gridCellSize, 1.0f, 20.0f);

            ImGui::Separator();
            ImGui::Text("Performance (last frame):");
            ImGui::Text("  Neighbour checks: %llu", m_flockingSystem->m_neighbourChecksLastFrame);
            ImGui::Text("  Update time:      %.3f ms", m_flockingSystem->m_lastUpdateMs);

            // Per-agent parameter editing (all FlockingComponents simultaneously)
            if (em != nullptr) {
                auto& fkArr = em->GetCompArr<GE::Components::FlockingComponent>();
                if (fkArr.GetCount() > 0U) {
                    ImGui::Separator();
                    ImGui::Text("Agent Parameters (all agents):");

                    static float sepR   = 1.5f, alignR = 3.0f, cohR   = 5.0f;
                    static float wSep   = 2.0f, wAlign = 1.0f, wCoh   = 1.0f;
                    static float maxSpd = 6.0f, maxF   = 15.0f;
                    bool changed = false;
                    changed |= ImGui::SliderFloat("Sep Radius",   &sepR,   0.1f,  5.0f);
                    changed |= ImGui::SliderFloat("Align Radius", &alignR, 0.5f, 10.0f);
                    changed |= ImGui::SliderFloat("Coh Radius",   &cohR,   1.0f, 20.0f);
                    changed |= ImGui::SliderFloat("W Separation", &wSep,   0.0f,  5.0f);
                    changed |= ImGui::SliderFloat("W Alignment",  &wAlign, 0.0f,  5.0f);
                    changed |= ImGui::SliderFloat("W Cohesion",   &wCoh,   0.0f,  5.0f);
                    changed |= ImGui::SliderFloat("Max Speed",    &maxSpd, 1.0f, 20.0f);
                    changed |= ImGui::SliderFloat("Max Force",    &maxF,   1.0f, 50.0f);
                    if (changed) {
                        for (uint32_t i = 0U; i < fkArr.GetCount(); ++i) {
                            auto& fk = fkArr.Data()[i];
                            fk.separationRadius = sepR;  fk.alignmentRadius = alignR;
                            fk.cohesionRadius   = cohR;  fk.wSeparation     = wSep;
                            fk.wAlignment       = wAlign; fk.wCohesion      = wCoh;
                            fk.maxSpeed         = maxSpd; fk.maxForce       = maxF;
                        }
                    }
                }
            }
            } // end else (has agents)
            ImGui::EndMenu();
        }
    }

    // --- Spawners menu ---
    {
        if (ImGui::BeginMenu("Spawners")) {
            GE::ECS::EntityManager* em = ServiceLocator::GetEntityManager();
            if (em == nullptr || m_spawnerSystem == nullptr) {
                ImGui::TextDisabled("No spawners in scene.");
            } else {
                auto& spawnerArr = em->GetCompArr<GE::Components::SpawnerComponent>();
                if (spawnerArr.GetCount() == 0U) {
                    ImGui::TextDisabled("No spawners in scene.");
                } else {
                    // Table: # | Name | Prefab | Type | Progress | Status | Actions
                    constexpr ImGuiTableFlags kTableFlags =
                        ImGuiTableFlags_Borders    |
                        ImGuiTableFlags_RowBg      |
                        ImGuiTableFlags_SizingFixedFit |
                        ImGuiTableFlags_NoHostExtendX;

                    if (ImGui::BeginTable("spawners_tbl", 7, kTableFlags)) {
                        ImGui::TableSetupColumn("#",        ImGuiTableColumnFlags_WidthFixed, 22.0f);
                        ImGui::TableSetupColumn("Name",     ImGuiTableColumnFlags_WidthFixed, 130.0f);
                        ImGui::TableSetupColumn("Prefab",   ImGuiTableColumnFlags_WidthFixed, 100.0f);
                        ImGui::TableSetupColumn("Type",     ImGuiTableColumnFlags_WidthFixed, 62.0f);
                        ImGui::TableSetupColumn("Progress", ImGuiTableColumnFlags_WidthFixed, 110.0f);
                        ImGui::TableSetupColumn("Status",   ImGuiTableColumnFlags_WidthFixed, 58.0f);
                        ImGui::TableSetupColumn("Actions",  ImGuiTableColumnFlags_WidthFixed, 148.0f);
                        ImGui::TableHeadersRow();

                        for (uint32_t i = 0; i < spawnerArr.GetCount(); ++i) {
                            auto& sc = spawnerArr.Data()[i];
                            ImGui::PushID(static_cast<int>(i));
                            ImGui::TableNextRow();

                            // --- Col 0: index ---
                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("%u", i);

                            // --- Col 1: spawner name (from Tag) ---
                            ImGui::TableSetColumnIndex(1);
                            const GE::ECS::EntityID eid = spawnerArr.Index()[i];
                            const auto* tag = em->TryGetTIComponent<GE::Components::Tag>(eid);
                            const char* spawnerName = (tag != nullptr && !tag->m_name.empty())
                                ? tag->m_name.c_str() : "Spawner";
                            ImGui::TextUnformatted(spawnerName);

                            // --- Col 2: prefab name ---
                            ImGui::TableSetColumnIndex(2);
                            const char* prefabName = (sc.prefabTemplate != nullptr)
                                ? sc.prefabTemplate->name.c_str() : "(none)";
                            ImGui::TextUnformatted(prefabName);

                            // --- Col 3: type ---
                            ImGui::TableSetColumnIndex(3);
                            if (sc.isBurst) {
                                ImGui::Text("Burst x%u", sc.burstCount);
                            } else {
                                ImGui::Text("Rep %.1fs", sc.interval);
                            }

                            // --- Col 4: progress bar ---
                            ImGui::TableSetColumnIndex(4);
                            const float fraction = (sc.maxCount > 0)
                                ? static_cast<float>(sc.spawnedCount) / static_cast<float>(sc.maxCount)
                                : 0.0f;
                            char progressBuf[16];
                            std::snprintf(progressBuf, sizeof(progressBuf),
                                "%u / %u", sc.spawnedCount, sc.maxCount);
                            ImGui::ProgressBar(fraction, ImVec2(-1.0f, 0.0f), progressBuf);

                            // --- Col 5: status (colored) ---
                            ImGui::TableSetColumnIndex(5);
                            const bool done = (sc.spawnedCount >= sc.maxCount);
                            if (done) {
                                ImGui::TextColored({ 0.5f, 0.5f, 0.5f, 1.0f }, "Done");
                            } else if (sc.paused) {
                                ImGui::TextColored({ 1.0f, 0.85f, 0.0f, 1.0f }, "Paused");
                            } else if (sc.elapsed < sc.startTime) {
                                ImGui::TextColored({ 0.6f, 0.6f, 1.0f, 1.0f }, "Waiting");
                            } else {
                                ImGui::TextColored({ 0.3f, 1.0f, 0.3f, 1.0f }, "Running");
                            }

                            // --- Col 6: action buttons ---
                            ImGui::TableSetColumnIndex(6);

                            // Fire
                            const bool canFire = !done && (sc.prefabTemplate != nullptr);
                            if (!canFire) ImGui::BeginDisabled();
                            if (ImGui::SmallButton("Fire"))  { m_spawnerSystem->ForceSpawnOne(sc); }
                            if (!canFire) ImGui::EndDisabled();
                            ImGui::SameLine();

                            // Pause / Resume
                            if (done) ImGui::BeginDisabled();
                            if (sc.paused) {
                                if (ImGui::SmallButton("Resume")) { sc.paused = false; }
                            } else {
                                if (ImGui::SmallButton("Pause"))  { sc.paused = true; }
                            }
                            if (done) ImGui::EndDisabled();
                            ImGui::SameLine();

                            // Stop (exhausts the spawner — marks as fully done)
                            if (done || sc.paused) ImGui::BeginDisabled();
                            if (ImGui::SmallButton("Stop")) { sc.spawnedCount = sc.maxCount; }
                            if (done || sc.paused) ImGui::EndDisabled();
                            ImGui::SameLine();

                            // Reset (restart from scratch; old spawned entities become root nodes)
                            if (ImGui::SmallButton("Reset")) {
                                sc.elapsed            = 0.0f;
                                sc.timeSinceLastSpawn = 0.0f;
                                sc.spawnedCount       = 0;
                                sc.activated          = false;
                                sc.paused             = false;
                                sc.spawnedEntityIds.clear();
                            }

                            ImGui::PopID();
                        }
                        ImGui::EndTable();
                    }
                }
            }
            ImGui::EndMenu();
        }
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

void FlatBuffersScenario::disconnectNetwork() {
    GE::NetworkBridge* bridge = ServiceLocator::GetNetworkBridge();
    GE::Networking::NetworkService* svc = bridge ? bridge->GetService() : nullptr;

    if (svc != nullptr && svc->IsConnected()) {
        svc->Shutdown();
    }
    if (bridge != nullptr) {
        bridge->ClearRemoteStates();
        bridge->ResetAutoConnect();
    }

    m_netInitialised = false;
    m_connectionMethod = ConnectionMethod::None;
    for (auto& entry : m_peerEntries) {
        entry.connected = false;
    }

    GE_LOG_INFO("FlatBuffersScenario: disconnected from network.");
}

void FlatBuffersScenario::scanSceneDirectory() {
    m_availableScenes.clear();

    std::error_code ec;
    for (const auto& entry : std::filesystem::recursive_directory_iterator("./config/flatbufferConfig/", ec)) {
        if (ec) { break; }
        if (entry.path().extension() == ".bin") {
            m_availableScenes.push_back(entry.path().string());
        }
    }
}

} // namespace GE
