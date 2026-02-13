#include "../include/SnowGlobeScenario.h"
#include "../include/ServiceLocator.h"
#include "../include/SceneLoader.h"
#include "../include/Components.h"

namespace GE {

    void SnowGlobeScenario::OnLoad(VkCommandBuffer cmd, std::vector<VkBuffer>& sb, std::vector<VkDeviceMemory>& sm) {
        m_climate = std::make_unique<ClimateManager>();

        auto* em = ServiceLocator::GetEntityManager();
        auto* am = ServiceLocator::GetAssetManager();
        auto* scene = ServiceLocator::GetScene();

        // Note: SceneLoader will now parse [LightComponent] blocks from the .ini
        Scene::SceneLoader loader;
        // loader.load("./config/snow_globe.ini", em, am, scene, ...);

        GE_LOG_INFO("SnowGlobeScenario: Assets Loaded Successfully.");
    }

    void SnowGlobeScenario::OnUpdate(float dt, float totalTime) {
        auto* em = ServiceLocator::GetEntityManager();
        auto* input = ServiceLocator::GetInput();

        // 1. Update Simulation State
        m_climate->update(dt, totalTime, input->getAutoOrbit(), 4.0f, 0.3f, 2.5f);

        // 2. Drive ECS Light Components (Correction: Use GetCompArr)
        auto& lightArray = em->GetCompArr<GE::Components::LightComponent>();

        for (uint32_t i = 0; i < lightArray.GetCount(); ++i) {
            GE::ECS::EntityID id = lightArray.Index()[i];
            auto& lightComp = lightArray.Data()[i];

            auto* tag = em->GetTIComponent<Scene::Components::Tag>(id);
            auto* trans = em->GetTIComponent<Scene::Components::Transform>(id);

            // orbital and seasonal logic for the 'MainSun'
            if (tag && tag->m_name == "MainSun" && !lightComp.isStatic) {
                if (trans && input->getAutoOrbit()) {
                    float radius = 4.0f;
                    float speed = 0.3f;
                    trans->m_position.x = std::sin(totalTime * speed) * radius;
                    trans->m_position.y = radius;
                    trans->m_position.z = std::cos(totalTime * speed) * radius;
                    trans->m_state = Scene::Components::Transform::TransformState::Dirty;
                }

                lightComp.color = m_climate->getAmbientColor();
                lightComp.intensity = m_climate->getSunIntensity();
            }
        }

        updateDynamicEntityScaling();
    }

    void SnowGlobeScenario::updateDynamicEntityScaling() {
        auto* em = ServiceLocator::GetEntityManager();
        auto* scene = ServiceLocator::GetScene();

        const float cactusMult = m_climate->getCactusScale();
        // Correction: WaterScale is a vec3
        const glm::vec3 waterScaleVec = m_climate->getWaterScale();
        const float waterOffset = m_climate->getWaterOffset();

        auto scaleEntity = [&](const std::string& key, float baseScale) {
            if (scene->hasEntity(key)) {
                auto id = scene->getEntityID(key);
                auto* trans = em->GetTIComponent<Scene::Components::Transform>(id);
                if (trans) {
                    trans->m_scale = glm::vec3(baseScale) * cactusMult;
                    trans->m_state = Scene::Components::Transform::TransformState::Dirty;
                }
            }
            };

        scaleEntity("Cactus1", 0.005f);
        scaleEntity("Cactus2", 0.004f);

        if (scene->hasEntity("Oasis")) {
            auto id = scene->getEntityID("Oasis");
            auto* trans = em->GetTIComponent<Scene::Components::Transform>(id);
            if (trans) {
                // Apply the vec3 scale multiplier
                trans->m_scale = glm::vec3(0.1f) * waterScaleVec;
                trans->m_position.y = -0.1f + waterOffset;
                trans->m_state = Scene::Components::Transform::TransformState::Dirty;
            }
        }
    }

    void SnowGlobeScenario::OnUnload() {
        // Clearing owned models triggers the AssetManager cleanup for this scene
        m_ownedModels.clear();
        GE_LOG_INFO("SnowGlobeScenario: Unloaded.");
    }
}