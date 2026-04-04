#pragma once

#include "ecs/IECSystem.h"
#include "ecs/EntityManager.h"

namespace GE::Systems {

/**
 * @class ClothSystem
 * @brief Verlet-integration cloth simulator with structural, shear, and flexion springs.
 *
 * Each frame:
 *  1. Accumulates gravity + wind forces on free particles.
 *  2. Computes spring forces (structural / shear / flexion) via Hooke + damping.
 *  3. Integrates using position Verlet.
 *  4. Resolves sphere-cloth penetration by pushing particles to the sphere surface.
 *
 * Runs at ESystemStage::Physics (before PhysicsSystem so RigidBodies receive impulse
 * from the already-resolved cloth frame).
 */
class ClothSystem : public ECS::ICpuSystem {
public:
    ClothSystem();
    ~ClothSystem() override = default;

    void       OnUpdate(float dt) override;
    ERROR_CODE Shutdown()         override { return ERROR_CODE::OK; }
};

} // namespace GE::Systems
