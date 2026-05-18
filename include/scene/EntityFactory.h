#pragma once

/* parasoft-begin-suppress ALL */
#include <cstdint>
/* parasoft-end-suppress ALL */

#include <glm/glm.hpp>

namespace GE::ECS  { class EntityManager; using EntityID = uint32_t; }
namespace GE::Scene::FB { struct PrefabTemplate; }

namespace GE::Scene {

    /**
     * @class EntityFactory
     * @brief Creates fully-initialised ECS entities from a PrefabTemplate at runtime.
     * Called by SpawnerSystem::OnUpdate() for each scheduled spawn event.
     * No GPU uploads occur — the prefab's shared mesh was uploaded at scene load time.
     */
    class EntityFactory {
    public:
        /**
         * @brief Instantiate a prefab at the given world position.
         * @param tmpl Prefab definition (shape, physics params, shared mesh).
         * @param position World position for the spawned entity's Transform.
         * @param rotationDeg Euler rotation in degrees (YXZ: yaw, pitch, roll).
         * @param linVel Initial linear velocity (m/s).
         * @param angVelDeg Initial angular velocity (deg/s) converted to rad/s internally.
         * @param ownerPeerId Network peer that owns this entity (0 = unowned).
         * @param em EntityManager to create the entity in.
         * @return The new EntityID, or UINT32_MAX if creation failed.
         */
        static GE::ECS::EntityID InstantiatePrefab(
            const GE::Scene::FB::PrefabTemplate& tmpl,
            const glm::vec3& position,
            const glm::vec3& rotationDeg,
            const glm::vec3& linVel,
            const glm::vec3& angVelDeg,
            uint8_t ownerPeerId,
            uint8_t colorOwnerIdx,  // 0–3; selects ownerMesh or falls back to materialMesh
            GE::ECS::EntityManager* em
        );

    private:
        static uint32_t s_instanceCounter;
    };

} // namespace GE::Scene
