#pragma once

/* parasoft-begin-suppress ALL */
#include "core/libs.h"
/* parasoft-end-suppress ALL */

#include "ecs/EntityManager.h"
#include "components/Transform.h"
#include "components/PhysicsComponents.h"
#include "components/Tag.h"

/**
 * @class Inspector
 * @brief Draws editable component data for a single selected entity.
 */
class Inspector final {
public:
    Inspector() = default;
    ~Inspector() = default;

    Inspector(const Inspector&) = delete;
    Inspector& operator=(const Inspector&) = delete;

    /**
     * @brief Draws the full Inspector panel for the given entity.
     * Shows entity name as a header, then one collapsing section per component present.
     */
    void Draw(GE::ECS::EntityID entityID, GE::ECS::EntityManager* em) const;

private:
    void DrawTransform(GE::Components::Transform* t) const;
    void DrawRigidBody(GE::Components::RigidBody* rb) const;
    void DrawSphereCollider(GE::Components::SphereCollider* sc) const;
    void DrawPlaneCollider(GE::Components::PlaneCollider* pc) const;
};
