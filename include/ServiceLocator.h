#pragma once

/* parasoft-begin-suppress ALL */
#include <stdexcept>
#include <memory>
/* parasoft-end-suppress ALL */

// Forward declarations
class Experience;
namespace GE::ECS { class EntityManager; }
class AssetManager;
class InputManager;
class SystemFactory;
struct VulkanContext;
class VulkanResourceManager;
class TimeManager;
class ParticleEmitterSystem;

// NEW: Forward declaration for Scene
namespace GE::Scene { class Scene; }

class ServiceLocator final {
public:
    // --- Providers ---
    static void Provide(VulkanContext* context) { m_context = context; }
    static void Provide(VulkanResourceManager* resources) { m_resources = resources; }
    static void Provide(InputManager* input) { m_input = input; }
    static void Provide(GE::ECS::EntityManager* entityManager) { m_entityManager = entityManager; }
    static void Provide(AssetManager* assetManager) { m_assetManager = assetManager; }
    static void Provide(SystemFactory* factory) { m_factory = factory; }
    static void Provide(Experience* experience) { m_experience = experience; }
    static void Provide(GE::Scene::Scene* scene) { m_scene = scene; }
    static void Provide(TimeManager* timeManager) { m_timeManager = timeManager; }
	static void Provide(ParticleEmitterSystem* particleEmitterSystem) { m_particleEmitterSystem = particleEmitterSystem; }

    // --- Retrievers ---
    static VulkanContext* GetContext() {
        if (!m_context) throw std::runtime_error("ServiceLocator: VulkanContext not provided!");
        return m_context;
    }

    static VulkanResourceManager* GetResourceManager() {
        if (!m_resources) throw std::runtime_error("ServiceLocator: VulkanResourceManager not provided!");
        return m_resources;
    }

    static InputManager* GetInput() {
        if (!m_input) throw std::runtime_error("ServiceLocator: InputManager not provided!");
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

    static SystemFactory* GetSystemFactory() {
        // Factory is optional in some contexts, but we'll keep it consistent
        return m_factory;
    }

    static GE::Scene::Scene* GetScene() {
        if (!m_scene) throw std::runtime_error("ServiceLocator: Scene not provided!");
        return m_scene;
    }

    static Experience* GetExperience() {
        if (!m_experience) throw std::runtime_error("ServiceLocator: Experience not provided!");
        return m_experience;
    }

    static TimeManager* GetTimeManager() {
        if (!m_timeManager) throw std::runtime_error("ServiceLocator: TimeManager not provided!");
        return m_timeManager;
	}

    static ParticleEmitterSystem* GetParticleEmitterSystem() {
        if (!m_particleEmitterSystem) throw std::runtime_error("ServiceLocator: ParticleEmitterSystem not provided!");
        return m_particleEmitterSystem;
	}

private:
    static inline VulkanContext* m_context = nullptr;
    static inline VulkanResourceManager* m_resources = nullptr;
    static inline InputManager* m_input = nullptr;
    static inline SystemFactory* m_factory = nullptr;
    static inline GE::ECS::EntityManager* m_entityManager = nullptr;
    static inline AssetManager* m_assetManager = nullptr;
    static inline Experience* m_experience = nullptr;
    static inline GE::Scene::Scene* m_scene = nullptr;
	static inline TimeManager* m_timeManager = nullptr;
	static inline ParticleEmitterSystem* m_particleEmitterSystem = nullptr;
};