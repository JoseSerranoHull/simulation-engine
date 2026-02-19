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

    void GE::GenericScenario::OnGUI() {
        auto* exp = ServiceLocator::GetExperience();

        // --- 1. Material Menu (Already implemented) ---
        if (ImGui::BeginMenu("Material")) {
            glm::vec4& colA = exp->GetCheckerColorA();
            glm::vec4& colB = exp->GetCheckerColorB();

            if (ImGui::ColorEdit3("Checker Light", &colA[0])) {}
            if (ImGui::ColorEdit3("Checker Dark", &colB[0])) {}

            ImGui::Separator();
            if (ImGui::Button("Reset Defaults")) {
                colA = glm::vec4(0.8f, 0.1f, 0.1f, 1.0f);
                colB = glm::vec4(0.8f, 0.8f, 0.0f, 1.0f);
            }
            ImGui::EndMenu();
        }

        // --- 2. Simulation Menu (Updated for Lab 2 Q3) ---
        if (ImGui::BeginMenu("Simulation")) {

            // Restart logic remains the same
            if (ImGui::MenuItem("Restart Simulation", "F5")) {
                exp->changeScenario(std::make_unique<GE::GenericScenario>(m_configPath));
            }

            ImGui::Separator();

            // Start / Pause Toggle
            std::string playLabel = m_isPaused ? "Resume Simulation" : "Pause Simulation";
            if (ImGui::MenuItem(playLabel.c_str(), "P")) {
                m_isPaused = !m_isPaused;
            }

            ImGui::Separator();

            // Fixed Timestep Configuration
            ImGui::Text("Manual Step Timestep (s)");
            ImGui::PushItemWidth(120);
            // Allows setting specific values like 0.01 or 0.005 for precision testing
            ImGui::InputFloat("##fixedStep", &m_fixedTimestep, 0.001f, 0.01f, "%.4f");
            ImGui::PopItemWidth();

            // Time Scale (Multiplier for live simulation)
            ImGui::Text("Live Time Scale");
            ImGui::PushItemWidth(120);
            ImGui::SliderFloat("##timescale", &m_timeScale, 0.0f, 5.0f, "%.2fx");
            ImGui::PopItemWidth();

            ImGui::Separator();

            // Step One Timestep Forward
            // Now uses the m_fixedTimestep value defined above
            if (ImGui::MenuItem("Step One Frame", "Space", false, m_isPaused)) {
                exp->stepSimulation(m_fixedTimestep);
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