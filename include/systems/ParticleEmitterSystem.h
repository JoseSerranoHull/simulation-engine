#pragma once
#include "ecs/EntityManager.h"
#include "components/ParticleComponent.h"
#include "systems/TransformSystem.h" 
#include "services/TimeService.h"
#include "core/EngineOrchestrator.h"
#include "core/ServiceLocator.h"

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
        void OnUpdate(float dt, VkCommandBuffer cb) override { // Signature changed
            auto* em = ServiceLocator::GetEntityManager();
            auto* exp = ServiceLocator::GetExperience();

            // REMOVED: No longer asking FrameSyncManager for a buffer
            auto& particles = em->GetCompArr<Components::ParticleComponent>();

            for (uint32_t i = 0; i < particles.GetCount(); ++i) {
                auto& pc = particles.Data()[i];
                if (!pc.enabled || !pc.system) continue;

                GE::ECS::EntityID id = particles.Index()[i];
                auto* transform = em->GetTIComponent<GE::Components::Transform>(id);
                glm::vec3 worldPos = transform ? (transform->m_position + pc.localOffset) : pc.localOffset;

                // Execute Compute Shader logic using the passed 'cb'
                pc.system->update(
                    cb, // Use the parameter
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