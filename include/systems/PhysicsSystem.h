#pragma once
#include "ecs/IECSystem.h"
#include "ecs/EntityManager.h"

namespace GE::Systems {
    /**
     * @class PhysicsSystem
     * @brief ECS System responsible for Newtonian integration and collision resolution.
     * Fulfills Simulation Lab 3 Requirements Q2, Q3, and Q4.
     */
    class PhysicsSystem : public ECS::IECSystem {
    public:
        PhysicsSystem() {
            m_typeID = ECS::IECSystem::GetUniqueISystemTypeID<PhysicsSystem>();
            m_stage = ECS::ESystemStage::Physics;
            m_state = SystemState::Running;
        }

        ~PhysicsSystem() override = default;

        /** @brief Heartbeat logic: Orchestrates integration then collision resolution. */
        void OnUpdate(float dt, VkCommandBuffer cb) override;

        ERROR_CODE Shutdown() override {
            m_state = SystemState::ShuttingDown;
            return ERROR_CODE::OK;
        }

    private:
        /** @brief Fulfills Q2 & Q3: Applies gravity and integrates velocity/position. */
        void Integrate(float dt);

        /** @brief Fulfills Q4: Detects and resolves sphere-plane intersections. */
        void ResolveCollisions();
    };
}