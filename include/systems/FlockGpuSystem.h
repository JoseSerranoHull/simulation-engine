#pragma once

/* parasoft-begin-suppress ALL */
#include <atomic>
/* parasoft-end-suppress ALL */

#include "ecs/IECSystem.h"
#include "ecs/EntityManager.h"
#include "particles/FlockGpuBackend.h"
#include "components/FlockingComponent.h"

namespace GE::Systems {

/**
 * @class FlockGpuSystem
 * @brief Thin IGpuSystem wrapper that dispatches the FlockGpuBackend compute shader.
 *
 * Registered alongside FlockingSystem in FlatBuffersScenario::OnLoad.
 * When GPU mode is active (m_gpuMode == true), builds a FlockUBO from the first
 * FlockingComponent (all agents share params via the ImGui sliders) and calls
 * FlockGpuBackend::Dispatch(). When GPU mode is off, returns immediately so the
 * CPU FlockingSystem handles steering instead.
 *
 * Runs at ESystemStage::Particle (before the render pass, same as particle backends).
 */
class FlockGpuSystem : public GE::ECS::IGpuSystem {
public:
    /**
     * @param backend Non-owning pointer — lifetime managed by FlatBuffersScenario.
     * @param gpuMode Reference to the atomic flag on FlatBuffersScenario that
     * toggles GPU vs CPU mode from the ImGui combo.
     */
    explicit FlockGpuSystem(GE::Particles::FlockGpuBackend* backend, std::atomic<bool>& gpuMode);

    ~FlockGpuSystem() override = default;

    void OnUpdate(float dt, VkCommandBuffer cb) override;
    void Shutdown() override {}

    // Set by FlatBuffersScenario after creation — mirrors FlockingSystem::m_spawnOrigin/Radius.
    glm::vec3 m_spawnCenter { 0.0f };
    float m_spawnRadius { 10.0f };

    // Written by ImGui (main thread), read by OnUpdate (graphics thread via UpdateGpuStages).
    std::atomic<bool> m_frozen { false };

private:
    GE::Particles::FlockGpuBackend* m_backend; // < Non-owning
    std::atomic<bool>& m_gpuMode;
};

} // namespace GE::Systems
