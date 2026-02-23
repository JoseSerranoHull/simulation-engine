#pragma once

#include "ecs/IECSystem.h"
#include "ecs/EntityManager.h"
#include "components/Transform.h"
#include <glm/glm.hpp>

namespace GE::Systems {
    /**
     * @class TransformSystem
     * @brief Agnostic system that resolves local and world matrices for the hierarchy.
     */
    class TransformSystem : public ECS::IECSystem {
    public:
        TransformSystem() {
            m_typeID = ECS::IECSystem::GetUniqueISystemTypeID<TransformSystem>();
            m_stage = ECS::ESystemStage::Transform; // High-priority stage for matrix resolution
            m_state = SystemState::Running;
        }

        ~TransformSystem() override = default;

        /** @brief Standard ECS update loop. */
        void OnUpdate(float dt, VkCommandBuffer cb) override;

        /** @brief Basic cleanup for the system. */
        ERROR_CODE Shutdown() override {
            m_state = SystemState::ShuttingDown;
            return ERROR_CODE::OK;
        }

    private:
        /** @brief Helper to build the local matrix from TRS data. */
        glm::mat4 calculateLocalMatrix(const GE::Components::Transform& trans);

        /** @brief Recursive or iterative helper to resolve the world matrix based on parentage. */
        void resolveWorldMatrix(uint32_t id, GE::Components::Transform& trans, GE::ECS::EntityManager* em);
    };
}