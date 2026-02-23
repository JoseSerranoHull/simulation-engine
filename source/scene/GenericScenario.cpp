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

        // 2. Agnostic System Registration
        // If the path contains "physics", we ensure the PhysicsSystem is active
        if (m_configPath.find("physics") != std::string::npos) {
            // Check if already registered to avoid duplicates
            em->RegisterSystem(new Systems::PhysicsSystem());
        }

        GE_LOG_INFO("GenericScenario: Scenario loaded from " + m_configPath);
    }

    void GenericScenario::OnUpdate(float dt, float totalTime) {
        // Logic updates are handled by the registered ECS Systems
        // This remains empty to stay agnostic.
    }

    void GE::GenericScenario::OnGUI() {
        // Fulfills Requirement: Simulation menu in the Main Menu Bar
        if (ImGui::BeginMenu("Simulation")) {

            // --- 1. Restart Logic (NEW) ---
            // Allows for instantaneous reset of Lab 3 test cases
            if (ImGui::MenuItem("Restart Simulation", "F5")) {
                auto* exp = ServiceLocator::GetExperience();
                // We use the stored config path to reload the same .ini file
                exp->changeScenario(std::make_unique<GE::GenericScenario>(m_configPath));

                GE_LOG_INFO("GenericScenario: Restarting simulation from " + m_configPath);
            }

            ImGui::Separator();

            // --- 2. Start / Pause Toggle ---
            // Fulfills Lab 3 Q2 Requirement: Simulation Start/Stop
            std::string playLabel = m_isPaused ? "Resume Simulation" : "Pause Simulation";
            if (ImGui::MenuItem(playLabel.c_str(), "P")) {
                m_isPaused = !m_isPaused; //
            }

            ImGui::Separator();

            // --- 3. Fixed Timestep (Time Scale) ---
            // Fulfills Lab 3 Q2 Requirement: Adjust the size of the timestep
            ImGui::Text("Time Scale");
            ImGui::PushItemWidth(120);
            // This modifies the multiplier applied to dt in Experience::drawFrame
            if (ImGui::SliderFloat("##timescale", &m_timeScale, 0.0f, 5.0f, "%.2fx")) {
                // Value is clamped between 0 (paused) and 5x speed
            }
            ImGui::PopItemWidth();

            ImGui::Separator();

            // --- 4. Step One Timestep Forward ---
            // Fulfills Lab Requirement: Debug simulation stepping
            // Enabled only when the simulation is paused
            if (ImGui::MenuItem("Step One Frame", "Space", false, m_isPaused)) {
                // standard 60fps fixed delta for a single discrete step
                const float fixedStep = 0.01667f;
                ServiceLocator::GetExperience()->stepSimulation(fixedStep);
            }

            ImGui::EndMenu();
        }
    }

    void GenericScenario::OnUnload() {
        auto* em = ServiceLocator::GetEntityManager();

        // 1. Cleanup Systems unique to this scenario
        em->UnregisterSystemByID(Systems::PhysicsSystem().GetID());

        // 2. Release GPU particle backends (owned by ParticleEmitterSystem pool)
        ServiceLocator::GetParticleEmitterSystem()->ClearBackends();

        // 3. Memory Cleanup: release GPU resources owned by this scenario
        m_ownedModels.clear();
        m_pipelines.clear();
        m_shaderModules.clear();

        GE_LOG_INFO("GenericScenario: Unloaded " + m_configPath);
    }
}