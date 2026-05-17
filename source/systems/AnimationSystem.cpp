#include "systems/AnimationSystem.h"
#include "core/ServiceLocator.h"
#include "ecs/EntityManager.h"
#include "components/AnimationComponent.h"
#include "components/Transform.h"

/* parasoft-begin-suppress ALL */
#include <glm/glm.hpp>
#include <cmath>   // std::fmod
/* parasoft-end-suppress ALL */

namespace GE::Systems {

    AnimationSystem::AnimationSystem() {
        m_typeID = GE::ECS::IECSystem::GetUniqueISystemTypeID<AnimationSystem>();
        m_stage  = GE::ECS::ESystemStage::Animation;
        m_state  = SystemState::Running;
    }

    ERROR_CODE AnimationSystem::Shutdown() {
        m_state = SystemState::ShuttingDown;
        return ERROR_CODE::OK;
    }

    // -------------------------------------------------------------------------
    // Easing helper
    // -------------------------------------------------------------------------

    static float applyEasing(float t, GE::Components::EasingType easing) {
        if (easing == GE::Components::EasingType::SMOOTHSTEP) {
            return t * t * (3.0f - 2.0f * t);
        }
        return t;  // LINEAR
    }

    // -------------------------------------------------------------------------
    // OnUpdate — waypoint interpolation
    // -------------------------------------------------------------------------

    void AnimationSystem::OnUpdate(float dt) {
        auto* em = ServiceLocator::GetEntityManager();
        if (!em) return;
        auto& animArr = em->GetCompArr<GE::Components::AnimatedObjectComponent>();

        for (uint32_t i = 0; i < animArr.GetCount(); ++i) {
            auto&                        ac = animArr.Data()[i];
            const GE::ECS::EntityID      id = animArr.Index()[i];
            auto* const tr = em->TryGetTIComponent<GE::Components::Transform>(id);

            if (!tr || ac.waypoints.empty()) continue;

            // 1. Save position from before this frame (used by PhysicsSystem for kinematic velocity)
            ac.prevPosition = tr->m_localPosition;

            // 2. Advance elapsed time (reversed flag controls direction)
            if (!ac.reversed) { ac.elapsed += dt; }
            else              { ac.elapsed -= dt; }

            // 3. PathMode enforcement
            switch (ac.pathMode) {
            case GE::Components::PathMode::STOP:
                ac.elapsed = glm::clamp(ac.elapsed, 0.0f, ac.totalDuration);
                break;

            case GE::Components::PathMode::LOOP:
                if (ac.totalDuration > 0.0f) {
                    ac.elapsed = std::fmod(ac.elapsed, ac.totalDuration);
                    if (ac.elapsed < 0.0f) { ac.elapsed += ac.totalDuration; }
                }
                break;

            case GE::Components::PathMode::REVERSE:
                if (!ac.reversed && ac.elapsed >= ac.totalDuration) {
                    ac.elapsed  = ac.totalDuration;
                    ac.reversed = true;
                } else if (ac.reversed && ac.elapsed <= 0.0f) {
                    ac.elapsed  = 0.0f;
                    ac.reversed = false;
                }
                break;
            }

            // 4. Waypoint interpolation
            const auto& wps = ac.waypoints;
            glm::vec3 pos = wps.front().position;
            glm::vec3 rot = wps.front().rotDeg;

            if (wps.size() == 1u) {
                pos = wps[0].position;
                rot = wps[0].rotDeg;
            } else if (ac.elapsed >= wps.back().time) {
                // After the last waypoint
                if (ac.pathMode == GE::Components::PathMode::LOOP) {
                    // Loop-back segment: last waypoint → first waypoint
                    const float segDur = ac.totalDuration - wps.back().time;
                    if (segDur > 1e-6f) {
                        float t = (ac.elapsed - wps.back().time) / segDur;
                        t = glm::clamp(t, 0.0f, 1.0f);
                        t = applyEasing(t, ac.easing);
                        pos = glm::mix(wps.back().position, wps.front().position, t);
                        rot = glm::mix(wps.back().rotDeg,   wps.front().rotDeg,   t);
                    } else {
                        pos = wps.front().position;
                        rot = wps.front().rotDeg;
                    }
                } else {
                    pos = wps.back().position;
                    rot = wps.back().rotDeg;
                }
            } else if (ac.elapsed < wps.front().time) {
                pos = wps.front().position;
                rot = wps.front().rotDeg;
            } else {
                // Normal case: find the segment [k, k+1] containing elapsed
                for (std::size_t k = 0; k + 1 < wps.size(); ++k) {
                    if (ac.elapsed >= wps[k].time && ac.elapsed < wps[k + 1].time) {
                        const float segDur = wps[k + 1].time - wps[k].time;
                        float t = (segDur > 1e-6f)
                            ? (ac.elapsed - wps[k].time) / segDur
                            : 1.0f;
                        t = glm::clamp(t, 0.0f, 1.0f);
                        t = applyEasing(t, ac.easing);
                        pos = glm::mix(wps[k].position, wps[k + 1].position, t);
                        rot = glm::mix(wps[k].rotDeg,   wps[k + 1].rotDeg,   t);
                        break;
                    }
                }
            }

            // 5. Write to Transform
            tr->m_localPosition = pos;
            tr->m_localRotation = rot;
            tr->m_state         = GE::Components::Transform::TransformState::Dirty;
        }
    }

} // namespace GE::Systems
