#include "scene/EntityFactory.h"
#include "scene/fb/FBSceneContext.h"
#include "ecs/EntityManager.h"
#include "components/Transform.h"
#include "components/Tag.h"
#include "components/Components.h"
#include "components/PhysicsComponents.h"
#include "components/AnimationComponent.h"
#include "components/SpawnerComponent.h"
#include "components/ScriptComponent.h"
#include "scripts/ScriptFactory.h"
#include "assets/Mesh.h"

/* parasoft-begin-suppress ALL */
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
/* parasoft-end-suppress ALL */

namespace GE::Scene {

uint32_t EntityFactory::s_instanceCounter = 0;

GE::ECS::EntityID EntityFactory::InstantiatePrefab(
    const GE::Scene::FB::PrefabTemplate& tmpl,
    const glm::vec3& position,
    const glm::vec3& rotationDeg,
    const glm::vec3& linVel,
    const glm::vec3& angVelDeg,
    uint8_t ownerPeerId,
    uint8_t colorOwnerIdx,
    GE::ECS::EntityManager* em)
{
    if (em == nullptr) return UINT32_MAX;

    const GE::ECS::EntityID id = em->CreateEntity();

    // 1. Transform — world-space position; m_parentEntityID stays UINT32_MAX (root entity).
    // Spawned physics entities must be root nodes so PhysicsSystem treats m_position as
    // world-space coordinates. Hierarchy grouping is handled via SpawnerComponent::spawnedEntityIds.
    GE::Components::Transform tr;
    tr.m_localPosition = position;
    tr.m_localRotation = rotationDeg;
    tr.m_localScale = glm::vec3(1.0f);

    // Pre-compute matrices immediately so the entity is at the correct world position
    // even before TransformSystem runs. TransformSystem uses ESystemStage::Transform (=1),
    // which runs before GameLogic (=6) where SpawnerSystem creates entities. Without this
    // pre-computation, the entity would have identity worldMatrix for one full tick,
    // causing a one-frame flash at the world origin (0,0,0).
    {
        glm::mat4 m = glm::translate(glm::mat4(1.0f), position);
        m = glm::rotate(m, glm::radians(rotationDeg.y), { 0.0f, 1.0f, 0.0f });
        m = glm::rotate(m, glm::radians(rotationDeg.x), { 1.0f, 0.0f, 0.0f });
        m = glm::rotate(m, glm::radians(rotationDeg.z), { 0.0f, 0.0f, 1.0f });
        tr.m_localMatrix = m;
        tr.m_worldMatrix = m;       // root entity: world == local
        tr.m_worldPosition = position;
        tr.m_worldScale    = glm::vec3(1.0f);
        tr.m_state = GE::Components::Transform::TransformState::Clean;
    }

    em->AddComponent(id, tr);

    // 2. Tag (unique name per instance)
    GE::Components::Tag tag;
    tag.m_name = tmpl.name + "_" + std::to_string(s_instanceCounter++);
    em->AddComponent(id, tag);

    // 3. OwnerComponent (networking peer assignment)
    if (ownerPeerId >= 1 && ownerPeerId <= 4) {
        GE::Components::OwnerComponent oc;
        oc.owner = static_cast<GE::Components::OwnerType>(ownerPeerId - 1);
        em->AddComponent(id, oc);
    }

    // 4. Collider (shape-specific; isTrigger = false by default)
    using SK = GE::Scene::FB::PrefabShapeKind;
    switch (tmpl.shapeKind) {
    case SK::Sphere:
        em->AddComponent(id, GE::Components::SphereCollider{ tmpl.radius, false });
        break;
    case SK::Capsule:
        em->AddComponent(id, GE::Components::CapsuleCollider{ tmpl.radius, tmpl.height, false });
        break;
    case SK::Cylinder:
        em->AddComponent(id, GE::Components::CylinderCollider{ tmpl.radius, tmpl.height, false });
        break;
    case SK::Cuboid:
        em->AddComponent(id, GE::Components::BoxCollider{ tmpl.size.x, tmpl.size.y, tmpl.size.z, false });
        break;
    default:
        break;
    }

    // 5. RigidBody (active physics from birth)
    GE::Components::RigidBody rb;
    rb.mass = (tmpl.mass > 0.0f) ? tmpl.mass : 1.0f;
    rb.inverseMass = 1.0f / rb.mass;
    rb.restitution = tmpl.restitution;
    rb.isStatic = false;
    rb.useGravity = true;
    rb.velocity = linVel;
    rb.angularVelocity = glm::radians(angVelDeg);
    rb.invInertiaTensor     = tmpl.invInertia;
    rb.invInertiaTensorWorld = tmpl.invInertia;
    em->AddComponent(id, rb);

    // 5b. PhysicsMaterialTag — enables per-material-pair restitution lookups via MaterialInteractionRegistry
    if (!tmpl.physicsMaterialName.empty()) {
        em->AddComponent(id, GE::Components::PhysicsMaterialTag{
            tmpl.physicsMaterialName, tmpl.density });
    }

    // 6. MeshRenderer — select owner-colored or material-appearance mesh (no GPU upload)
    {
        GE::Assets::Mesh* meshToUse = nullptr;
        if (colorOwnerIdx < 4 && tmpl.ownerMeshes[colorOwnerIdx] != nullptr) {
            meshToUse = tmpl.ownerMeshes[colorOwnerIdx];   // owner-color mode
        } else {
            meshToUse = tmpl.materialMesh;                 // material-color mode fallback
        }
        if (meshToUse != nullptr) {
            GE::Components::SubMesh sm;
            sm.m_mesh = meshToUse;
            sm.m_material = meshToUse->getMaterial();
            GE::Components::MeshRenderer mr;
            mr.subMeshes.push_back(sm);
            em->AddComponent(id, mr);
        }
    }

    // 7. ScriptComponent (only if prefab specifies a script type)
    if (!tmpl.scriptType.empty()) {
        auto script = GE::Scripts::CreateScript(tmpl.scriptType);
        if (script) {
            script->SetEntityID(id);
            em->AddComponent(id, GE::Components::ScriptComponent{ std::move(script) });
        }
    }

    return id;
}

} // namespace GE::Scene
