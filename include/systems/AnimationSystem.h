#pragma once

#include "ecs/IECSystem.h"

namespace GE::Systems {

    /**
     * @class AnimationSystem
     * @brief Stub ECS system for waypoint-based animation (AnimatedObjectComponent).
     *        Registered by FlatBuffersScenario. The component data is populated by
     *        FBSceneAdapter; the tick logic is deferred to a future iteration.
     */
    class AnimationSystem final : public GE::ECS::ICpuSystem {
    public:
        AnimationSystem() {
            m_typeID = GE::ECS::IECSystem::GetUniqueISystemTypeID<AnimationSystem>();
            m_stage  = GE::ECS::ESystemStage::Animation;
            m_state  = SystemState::Running;
        }

        ~AnimationSystem() override = default;

        // Deferred: animation tick not yet implemented
        void OnUpdate(float /*dt*/) override {}

        ERROR_CODE Shutdown() override {
            m_state = SystemState::ShuttingDown;
            return ERROR_CODE::OK;
        }
    };

} // namespace GE::Systems
