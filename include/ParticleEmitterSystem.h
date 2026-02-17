#pragma once
#include "../include/EntityManager.h"
#include "../include/ParticleComponent.h"
#include "../include/TransformSystem.h" 
#include "../include/TimeManager.h"
#include "../include/Experience.h"
#include "../include/ServiceLocator.h"

namespace GE::Systems {
    /**
     * @class ParticleEmitterSystem
     * @brief Agnostic ECS System that manages GPU Compute dispatches for particles.
     */
    class ParticleEmitterSystem : public ECS::IECSystem {
    public:
        // ADDED CONSTRUCTOR: This registers the system type and stage
        ParticleEmitterSystem() {
            m_typeID = ECS::IECSystem::GetUniqueISystemTypeID<ParticleEmitterSystem>();
            m_stage = ECS::ESystemStage::Particle; // Correctly place it in the Particle stage
            m_state = SystemState::Running;
        }

        /** @brief Heartbeat logic: Updates all active particle components. */
        void OnUpdate(float dt) override {
            auto* em = ServiceLocator::GetEntityManager();
            auto* res = ServiceLocator::GetResourceManager();
            auto* exp = ServiceLocator::GetExperience();

            // Get the active command buffer for the current frame
            VkCommandBuffer cb = res->getSyncManager()->getCurrentCommandBuffer();
            auto& particles = em->GetCompArr<Components::ParticleComponent>();

            for (uint32_t i = 0; i < particles.GetCount(); ++i) {
                auto& pc = particles.Data()[i];

                if (!pc.enabled || !pc.system) continue;

                // Sync particle origin to Entity Position
                GE::ECS::EntityID id = particles.Index()[i];
                auto* transform = em->GetTIComponent<GE::Scene::Components::Transform>(id);
                glm::vec3 worldPos = transform ? (transform->m_position + pc.localOffset) : pc.localOffset;

                // Execute Compute Shader logic
                pc.system->update(
                    cb,
                    dt,
                    pc.enabled,
                    ServiceLocator::GetTimeManager()->getTotal(),
                    exp->GetCurrentUBO().lightColor,
                    worldPos
                );
            }
        }

        /** @brief Satisfies the pure virtual requirement using the 'OK' code. */
        GE::ERROR_CODE Shutdown() override {
            m_state = SystemState::ShuttingDown;
            return GE::ERROR_CODE::OK;
        }
    };
}