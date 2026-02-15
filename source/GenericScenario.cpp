#include "../include/GenericScenario.h"
#include "../include/ServiceLocator.h"
#include "../include/SceneLoader.h"
#include "../include/PhysicsSystem.h"
#include "../include/Experience.h"

namespace GE {

    void GenericScenario::OnLoad(VkCommandBuffer cmd, std::vector<VkBuffer>& sb, std::vector<VkDeviceMemory>& sm) {
        auto* em = ServiceLocator::GetEntityManager();
        auto* am = ServiceLocator::GetAssetManager();
        auto* scene = ServiceLocator::GetScene();
		auto* experience = ServiceLocator::GetExperience();

        // 1. Agnostic Initialization: Load everything from data
        if (!m_configPath.empty()) {
            Scene::SceneLoader loader;
            // The loader populates the EntityManager and Scene registry
            loader.load(m_configPath, em, am, scene,
                experience->GetPipelines(),
                cmd, sb, sm, m_ownedModels);
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

    void GenericScenario::OnGUI() {
        // Fulfills Requirement: Simulation menu in the Main Menu Bar
        if (ImGui::BeginMenu("Simulation")) {

            // 1. Start / Pause Toggle
            std::string playLabel = m_isPaused ? "Resume Simulation" : "Pause Simulation";
            if (ImGui::MenuItem(playLabel.c_str())) {
                m_isPaused = !m_isPaused;
            }

            ImGui::Separator();

            // 2. Fixed Timestep (Time Scale)
            // Allows specifying how fast time moves in the simulation
            ImGui::Text("Time Scale");
            ImGui::PushItemWidth(100);
            ImGui::SliderFloat("##timescale", &m_timeScale, 0.0f, 5.0f, "%.2fx");
            ImGui::PopItemWidth();

            ImGui::Separator();

            // 3. Step One Timestep Forward
            // Useful for debugging collisions while paused
            if (ImGui::MenuItem("Step One Frame", "Space", false, m_isPaused)) {
                // We use a standard fixed delta for the step (e.g., 60fps)
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

        // 2. Memory Cleanup: Clearing m_ownedModels releases GPU buffers
        m_ownedModels.clear();

        GE_LOG_INFO("GenericScenario: Unloaded " + m_configPath);
    }
}