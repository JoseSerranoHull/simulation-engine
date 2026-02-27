#include "scene/GenericScenario.h"
#include "core/ServiceLocator.h"
#include "scene/SceneLoader.h"
#include "systems/PhysicsSystem.h"
#include "systems/ParticleEmitterSystem.h"
#include "core/EngineOrchestrator.h"
#include "graphics/GpuUploadContext.h"

using namespace GE::Graphics;
using namespace GE::Assets;

namespace GE {

    void GenericScenario::OnLoad(GE::Graphics::GpuUploadContext& ctx) {
        auto* em = ServiceLocator::GetEntityManager();
        auto* am = ServiceLocator::GetAssetManager();
        auto* scene = ServiceLocator::GetScene();

        // 1. Build scenario-scoped pipelines from the active PostProcessBackend
        createMaterialPipelines();

        // 2. Agnostic Initialization: Load everything from data
        if (!m_configPath.empty()) {
            Scene::SceneLoader loader;
            // The loader populates the EntityManager and Scene registry
            loader.load(m_configPath, em, am, scene, m_pipelines, ctx, m_ownedModels);
        }

        // 2. Always register PhysicsSystem — it is a no-op when no RigidBody
        // components exist, and required for all simulation lab scenarios.
        // Store a non-owning pointer for ImGui integration method selection.
        auto* ps = new Systems::PhysicsSystem();
        m_physicsSystem = ps;
        em->RegisterSystem(ps);

        // 3. Register ColliderVisualizerSystem — uploads wire geometry and draws
        // green wireframe overlays for SphereCollider and PlaneCollider entities.
        auto* vs = new Systems::ColliderVisualizerSystem(ctx);
        m_visualizerSystem = vs;
        em->RegisterSystem(vs);

        GE_LOG_INFO("GenericScenario: Scenario loaded from " + m_configPath);
    }

    void GenericScenario::OnUpdate(float dt, float totalTime) {
        // Logic updates are handled by the registered ECS Systems
        // This remains empty to stay agnostic.
    }

    void GE::GenericScenario::OnGUI() {
        // --- COLOUR MENU (Lab 2 Q1: background colour picker) ---
        if (ImGui::BeginMenu("Colour")) {
            ImGui::ColorEdit4("Background", &m_clearColor.r,
                ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_PickerHueWheel);
            ImGui::EndMenu();
        }

        // --- MATERIAL MENU (Lab 2 Q2: checkerboard colour picker) ---
        if (ImGui::BeginMenu("Material")) {
            ImGui::SeparatorText("Checkerboard");
            ImGui::ColorEdit4("Light squares", &m_checkerConstants.colorA.r,
                ImGuiColorEditFlags_NoAlpha);
            ImGui::ColorEdit4("Dark squares",  &m_checkerConstants.colorB.r,
                ImGuiColorEditFlags_NoAlpha);
            ImGui::SliderFloat("Tile scale", &m_checkerConstants.scale, 0.5f, 10.0f, "%.1f");
            ImGui::EndMenu();
        }

        // Fulfills Requirement: Simulation menu in the Main Menu Bar
        if (ImGui::BeginMenu("Simulation")) {

            // --- 1. Restart Logic ---
            // Deferred via requestScenarioChange so the actual swap happens at the
            // top of the next drawFrame(), after vkDeviceWaitIdle — never mid-frame.
            auto doRestart = [&]() {
                ServiceLocator::GetExperience()->requestScenarioChange(m_configPath);
                GE_LOG_INFO("GenericScenario: Restarting simulation from " + m_configPath);
            };

            if (ImGui::MenuItem("Restart Simulation", "F5")) {
                doRestart();
            }
            if (ImGui::IsKeyPressed(ImGuiKey_F5, /*repeat=*/false)) {
                doRestart();
            }

            ImGui::Separator();

            // --- 2. Start / Pause Toggle ---
            // Fulfills Lab 3 Q2 Requirement: Simulation Start/Stop
            std::string playLabel = m_isPaused ? "Resume Simulation" : "Pause Simulation";
            if (ImGui::MenuItem(playLabel.c_str(), "P")) {
                m_isPaused = !m_isPaused; //
            }

            ImGui::Separator();

            // --- 3. Time Scale Multiplier ---
            ImGui::Text("Time Scale");
            ImGui::PushItemWidth(150);
            ImGui::SliderFloat("##timescale", &m_timeScale, 0.0f, 5.0f, "%.2fx");
            ImGui::PopItemWidth();

            // --- 4. Fixed Timestep Input ---
            // Fulfills Lab 3 Q2: "Change the size of the fixed timestep from your application using ImGui"
            ImGui::Text("Fixed Step (s)");
            ImGui::PushItemWidth(150);
            ImGui::InputFloat("##fixeddt", &m_fixedTimestep, 0.001f, 0.005f, "%.4f");
            m_fixedTimestep = glm::clamp(m_fixedTimestep, 0.0001f, 0.1f);
            ImGui::PopItemWidth();

            ImGui::Separator();

            // --- 5. Integration Method Selector ---
            // Fulfills Lab 3 Q2: "Choose between at least two integration methods"
            if (m_physicsSystem != nullptr) {
                static const char* methodNames[] = { "Euler", "Semi-Implicit Euler", "RK4" };
                int currentMethod = static_cast<int>(m_physicsSystem->m_integrationMethod);
                ImGui::Text("Integration Method");
                ImGui::PushItemWidth(200);
                if (ImGui::Combo("##integration", &currentMethod, methodNames, 3)) {
                    m_physicsSystem->m_integrationMethod =
                        static_cast<Systems::IntegrationMethod>(currentMethod);
                }
                ImGui::PopItemWidth();
            }

            ImGui::Separator();

            // --- 6. Step One Timestep Forward ---
            // Fulfills Lab Requirement: Debug simulation stepping (enabled only while paused)
            if (ImGui::MenuItem("Step One Frame", "Space", false, m_isPaused)) {
                ServiceLocator::GetExperience()->stepSimulation(m_fixedTimestep);
            }

            ImGui::Separator();

            // --- 7. Collider Wireframe Toggle ---
            if (m_visualizerSystem != nullptr) {
                ImGui::Checkbox("Show Collider Wireframes", &m_visualizerSystem->m_enabled);
            }

            ImGui::EndMenu();
        }
    }

    void GenericScenario::OnUnload() {
        auto* em = ServiceLocator::GetEntityManager();

        // 1. Cleanup Systems unique to this scenario
        // Use the stored non-owning pointer — constructing a throwaway system to read its ID
        // risks an ODR-split static counter returning a different value.
        if (m_physicsSystem != nullptr) {
            em->UnregisterSystemByID(m_physicsSystem->GetID());
            m_physicsSystem = nullptr;
        }

        if (m_visualizerSystem != nullptr) {
            em->UnregisterSystemByID(m_visualizerSystem->GetID());
            m_visualizerSystem = nullptr;
        }

        // 2. Release GPU particle backends (owned by ParticleEmitterSystem pool)
        ServiceLocator::GetParticleEmitterSystem()->ClearBackends();

        // 3. Memory Cleanup: release GPU resources owned by this scenario
        m_ownedModels.clear();
        m_pipelines.clear();
        m_shaderModules.clear();

        GE_LOG_INFO("GenericScenario: Unloaded " + m_configPath);
    }
}