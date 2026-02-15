#pragma once
#include <glm/glm.hpp>
#include "Line.h"

namespace GE::Physics {
    class Collider {
    public:
        virtual ~Collider() = default;
        virtual bool IsInside(const glm::vec3& point) const = 0;
        virtual bool Intersects(const Line& line) const = 0;
    };
}