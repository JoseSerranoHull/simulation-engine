#include "systems/FlockGpuSystem.h"
#include "core/ServiceLocator.h"

namespace GE::Systems {

FlockGpuSystem::FlockGpuSystem(GE::Particles::FlockGpuBackend* backend,
                               std::atomic<bool>& gpuMode)
    : m_backend(backend)
    , m_gpuMode(gpuMode)
{
    m_typeID = ECS::IECSystem::GetUniqueISystemTypeID<FlockGpuSystem>();
    m_stage = ECS::ESystemStage::Particle;
    m_state = SystemState::Running;
}

void FlockGpuSystem::OnUpdate(float dt, VkCommandBuffer cb) {
    if (!m_gpuMode.load(std::memory_order_relaxed)) { return; }
    if (m_frozen.load(std::memory_order_relaxed)) { return; }
    if (m_backend == nullptr || m_backend->GetBoidCount() == 0U) { return; }

    GE::ECS::EntityManager* em = ServiceLocator::GetEntityManager();
    if (em == nullptr) { return; }

    auto& fcarr = em->GetCompArr<GE::Components::FlockingComponent>();
    if (fcarr.GetCount() == 0U) { return; }

    // All agents share the same steering parameters (ImGui sliders update all at once).
    const GE::Components::FlockingComponent& fc = fcarr.Data()[0];

    GE::Particles::FlockUBO ubo{};
    ubo.deltaTime = dt;
    ubo.boidCount = m_backend->GetBoidCount(); // set by Dispatch internally
    ubo.pingPong = 0U; // set by Dispatch internally
    ubo.pad0 = 0.0f;
    ubo.separationRadius = fc.separationRadius;
    ubo.alignmentRadius = fc.alignmentRadius;
    ubo.cohesionRadius = fc.cohesionRadius;
    ubo.wSeparation = fc.wSeparation;
    ubo.wAlignment = fc.wAlignment;
    ubo.wCohesion = fc.wCohesion;
    ubo.maxSpeed = fc.maxSpeed;
    ubo.maxForce = fc.maxForce;
    ubo.spawnCenterRadius = glm::vec4(m_spawnCenter, m_spawnRadius);
    ubo.containmentK = fc.maxForce * 2.0f; // proportional to agent strength
    ubo.pad1 = ubo.pad2 = ubo.pad3 = 0.0f;

    m_backend->Dispatch(cb, ubo);
}

} // namespace GE::Systems
