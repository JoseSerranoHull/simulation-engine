#pragma once

#include "ecs/IECSystem.h"

namespace GE::Systems {

    /**
     * @class AnimationSystem
     * @brief ECS system that drives AnimatedObjectComponent waypoint interpolation.
     *        Supports LINEAR and SMOOTHSTEP easing; STOP, LOOP, and REVERSE path modes.
     *        Stores prevPosition on each component each tick so that PhysicsSystem
     *        can compute kinematic velocity for collision response.
     */
    class AnimationSystem final : public GE::ECS::ICpuSystem {
    public:
        AnimationSystem();
        ~AnimationSystem() override = default;

        void OnUpdate(float dt) override;
        void Shutdown() override;
    };

} // namespace GE::Systems
