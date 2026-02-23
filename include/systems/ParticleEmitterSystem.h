#pragma once

/* parasoft-begin-suppress ALL */
#include <vector>
#include <memory>
/* parasoft-end-suppress ALL */

#include "ecs/EntityManager.h"
#include "components/ParticleComponent.h"
#include "particles/GpuParticleBackend.h"
#include "systems/TransformSystem.h"
#include "services/TimeService.h"
#include "core/EngineOrchestrator.h"
#include "core/ServiceLocator.h"

namespace GE::Systems {
    /**
     * @class ParticleEmitterSystem
     * @brief Agnostic ECS System that manages GPU Compute dispatches for particles.
     * Owns the GpuParticleBackend pool; ParticleComponent holds an index into it.
     */
    class ParticleEmitterSystem : public ECS::IGpuSystem {
    public:
        ParticleEmitterSystem() {
            m_typeID = ECS::IECSystem::GetUniqueISystemTypeID<ParticleEmitterSystem>();
            m_stage = ECS::ESystemStage::Particle;
            m_state = SystemState::Running;
        }

        // --- Backend Pool Management ---

        /** @brief Registers a backend and returns its stable pool index. */
        uint32_t RegisterBackend(std::unique_ptr<GpuParticleBackend> backend) {
            const uint32_t idx = static_cast<uint32_t>(m_backends.size());
            m_backends.push_back(std::move(backend));
            return idx;
        }

        /** @brief Returns a raw (non-owning) pointer to a backend by index, or nullptr. */
        GpuParticleBackend* GetBackend(uint32_t index) const {
            if (index >= static_cast<uint32_t>(m_backends.size())) return nullptr;
            return m_backends[index].get();
        }

        /** @brief Destroys all backends — called on scenario unload. */
        void ClearBackends() {
            m_backends.clear();
        }

        // --- IECSystem Interface ---

        /** @brief Dispatches compute updates for all active particle emitters. */
        void OnUpdate(float dt, VkCommandBuffer cb) override {
            auto* em = ServiceLocator::GetEntityManager();
            auto* exp = ServiceLocator::GetExperience();
            auto& particles = em->GetCompArr<Components::ParticleComponent>();

            for (uint32_t i = 0; i < particles.GetCount(); ++i) {
                auto& pc = particles.Data()[i];
                if (!pc.enabled || pc.emitterIndex == Components::ParticleComponent::INVALID_EMITTER) continue;

                GpuParticleBackend* backend = GetBackend(pc.emitterIndex);
                if (!backend) continue;

                GE::ECS::EntityID id = particles.Index()[i];
                auto* transform = em->GetTIComponent<GE::Components::Transform>(id);
                glm::vec3 worldPos = transform ? (transform->m_position + pc.localOffset) : pc.localOffset;

                backend->update(
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

    private:
        std::vector<std::unique_ptr<GpuParticleBackend>> m_backends;
    };
}