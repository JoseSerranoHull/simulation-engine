#pragma once
#include <vector>
#include <string>
#include <glm/glm.hpp>

namespace GE::Components {

    enum class EasingType : uint8_t { LINEAR, SMOOTHSTEP };
    enum class PathMode   : uint8_t { STOP, LOOP, REVERSE };

    struct FBWaypoint {
        glm::vec3 position { 0.0f };
        glm::vec3 rotDeg   { 0.0f };  // yaw/pitch/roll in degrees
        float     time     { 0.0f };  // absolute arrival time (seconds)
    };

    struct AnimatedObjectComponent {
        std::vector<FBWaypoint> waypoints;
        float      totalDuration { 1.0f };
        EasingType easing        { EasingType::LINEAR };
        PathMode   pathMode      { PathMode::STOP };
        float      elapsed       { 0.0f };  // runtime accumulator (unused this iteration)
    };

    struct PhysicsMaterialTag {
        std::string name;
        float       density { 1.0f };
    };

}
