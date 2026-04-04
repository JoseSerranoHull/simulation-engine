#pragma once

/* parasoft-begin-suppress ALL */
#include <random>
/* parasoft-end-suppress ALL */

#include "ecs/IECSystem.h"
#include "components/AnimationComponents.h"  // SpawnerComponent, SpawnLocType

namespace GE::Systems {

    /**
     * @class SpawnerSystem
     * @brief ECS system that activates pre-created entity pools at runtime.
     *        Entities are created with all components at scene load time by
     *        FBSceneAdapter::adaptSpawners(); this system only positions them
     *        and enables physics when their scheduled time arrives.
     */
    class SpawnerSystem final : public GE::ECS::ICpuSystem {
    public:
        SpawnerSystem();
        ~SpawnerSystem() override = default;

        void       OnUpdate(float dt) override;
        ERROR_CODE Shutdown()         override;

    private:
        std::mt19937 m_rng;

        glm::vec3 randomInRange(const glm::vec3& lo, const glm::vec3& hi);
        glm::vec3 pickLocation (const GE::Components::SpawnerComponent& sc);
    };

} // namespace GE::Systems
