#pragma once

/* parasoft-begin-suppress ALL */
#include <random>
/* parasoft-end-suppress ALL */

#include "ecs/IECSystem.h"
#include "components/AnimationComponents.h"  // SpawnerComponent, SpawnLocType

namespace GE::Systems {

    /**
     * @class SpawnerSystem
     * @brief ECS system that instantiates prefab entities at runtime.
     *        Spawners reference PrefabTemplates built at scene load time;
     *        EntityFactory::InstantiatePrefab() creates real ECS entities with all
     *        components when each scheduled spawn event fires.
     */
    class SpawnerSystem final : public GE::ECS::ICpuSystem {
    public:
        SpawnerSystem();
        ~SpawnerSystem() override = default;

        void       OnUpdate(float dt) override;
        ERROR_CODE Shutdown()         override;

        /** @brief Manually fire one spawn from the spawner, bypassing the time check. */
        void ForceSpawnOne(GE::Components::SpawnerComponent& sc);

    private:
        std::mt19937 m_rng;

        glm::vec3 randomInRange(const glm::vec3& lo, const glm::vec3& hi);
        glm::vec3 pickLocation (const GE::Components::SpawnerComponent& sc);
    };

} // namespace GE::Systems
