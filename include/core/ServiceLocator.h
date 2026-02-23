#pragma once

/* parasoft-begin-suppress ALL */
#include <stdexcept>
#include <memory>
/* parasoft-end-suppress ALL */

// Forward declarations
class EngineOrchestrator;
namespace GE::ECS { class EntityManager; }
class AssetManager;
class InputService;
class EngineServiceRegistry;
namespace GE::Graphics { struct VulkanContext; class GpuResourceManager; }
class TimeService;
namespace GE::Systems { class ParticleEmitterSystem; }

// NEW: Forward declaration for Scene
namespace GE::Scene { class Scene; }

class ServiceLocator final {
public:
    // --- Providers ---
    static void Provide(GE::Graphics::VulkanContext* context) { m_context = context; }
    static void Provide(GE::Graphics::GpuResourceManager* resources) { m_resources = resources; }
    static void Provide(InputService* input) { m_input = input; }
    static void Provide(GE::ECS::EntityManager* entityManager) { m_entityManager = entityManager; }
    static void Provide(AssetManager* assetManager) { m_assetManager = assetManager; }
    static void Provide(EngineServiceRegistry* factory) { m_factory = factory; }
    static void Provide(EngineOrchestrator* experience) { m_experience = experience; }
    static void Provide(GE::Scene::Scene* scene) { m_scene = scene; }
    static void Provide(TimeService* timeManager) { m_timeManager = timeManager; }
	static void Provide(GE::Systems::ParticleEmitterSystem* particleEmitterSystem) { m_particleEmitterSystem = particleEmitterSystem; }

    // --- Retrievers ---
    static GE::Graphics::VulkanContext* GetContext() {
        if (!m_context) throw std::runtime_error("ServiceLocator: VulkanContext not provided!");
        return m_context;
    }

    static GE::Graphics::GpuResourceManager* GetResourceManager() {
        if (!m_resources) throw std::runtime_error("ServiceLocator: GpuResourceManager not provided!");
        return m_resources;
    }

    static InputService* GetInput() {
        if (!m_input) throw std::runtime_error("ServiceLocator: InputService not provided!");
        return m_input;
    }

    static GE::ECS::EntityManager* GetEntityManager() {
        if (!m_entityManager) throw std::runtime_error("ServiceLocator: EntityManager not provided!");
        return m_entityManager;
    }

    static AssetManager* GetAssetManager() {
        if (!m_assetManager) throw std::runtime_error("ServiceLocator: AssetManager not provided!");
        return m_assetManager;
    }

    static EngineServiceRegistry* GetSystemFactory() {
        // Factory is optional in some contexts, but we'll keep it consistent
        return m_factory;
    }

    static GE::Scene::Scene* GetScene() {
        if (!m_scene) throw std::runtime_error("ServiceLocator: Scene not provided!");
        return m_scene;
    }

    static EngineOrchestrator* GetExperience() {
        if (!m_experience) throw std::runtime_error("ServiceLocator: EngineOrchestrator not provided!");
        return m_experience;
    }

    static TimeService* GetTimeManager() {
        if (!m_timeManager) throw std::runtime_error("ServiceLocator: TimeService not provided!");
        return m_timeManager;
	}

    static GE::Systems::ParticleEmitterSystem* GetParticleEmitterSystem() {
        if (!m_particleEmitterSystem) throw std::runtime_error("ServiceLocator: ParticleEmitterSystem not provided!");
        return m_particleEmitterSystem;
	}

private:
    static inline GE::Graphics::VulkanContext* m_context = nullptr;
    static inline GE::Graphics::GpuResourceManager* m_resources = nullptr;
    static inline InputService* m_input = nullptr;
    static inline EngineServiceRegistry* m_factory = nullptr;
    static inline GE::ECS::EntityManager* m_entityManager = nullptr;
    static inline AssetManager* m_assetManager = nullptr;
    static inline EngineOrchestrator* m_experience = nullptr;
    static inline GE::Scene::Scene* m_scene = nullptr;
	static inline TimeService* m_timeManager = nullptr;
	static inline GE::Systems::ParticleEmitterSystem* m_particleEmitterSystem = nullptr;
};