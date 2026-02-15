#pragma once
#include "IECSystem.h"
#include "EntityManager.h"

namespace GE::Systems {
    class PhysicsSystem : public ECS::IECSystem {
    public:
        PhysicsSystem() {
            // Set the internal state to match your base class requirements
            m_typeID = ECS::IECSystem::GetUniqueISystemTypeID<PhysicsSystem>();
            m_stage = ECS::ESystemStage::Physics; // Matches your enum
            m_state = SystemState::Running;
        }

        ~PhysicsSystem() override = default;

        // Implement the actual required virtual methods
        void OnUpdate(float dt) override;
        ERROR_CODE Shutdown() override { return ERROR_CODE::OK; }

    private:
        void Integrate(float dt);
        void ResolveCollisions();
    };
}