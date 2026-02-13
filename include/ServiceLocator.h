#pragma once

/* parasoft-begin-suppress ALL */
#include <stdexcept>
#include <memory>
/* parasoft-end-suppress ALL */

#include "../include/EntityManager.h"
#include "../include/AssetManager.h"
#include "../include/Scene.h"
#include "../include/InputManager.h" // <-- Add this include to fix incomplete type error

// Forward declarations
class SystemFactory;
struct VulkanContext;
class VulkanResourceManager;

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

    // NEW: Provide the Scene Registry
    static void Provide(GE::Scene::Scene* scene) { m_scene = scene; }

    // --- Retrievers ---
    static VulkanContext* GetContext() {
        if (!m_context) throw std::runtime_error("ServiceLocator: VulkanContext not provided!");
        return m_context;
    }

    static VulkanResourceManager* GetResources() {
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

    static SystemFactory* GetFactory() {
        // Factory is optional in some contexts, but we'll keep it consistent
        return m_factory;
    }

    // NEW: Retrieve the Scene Registry
    static GE::Scene::Scene* GetScene() {
        if (!m_scene) throw std::runtime_error("ServiceLocator: Scene not provided!");
        return m_scene;
    }

private:
    static inline VulkanContext* m_context = nullptr;
    static inline VulkanResourceManager* m_resources = nullptr;
    static inline InputManager* m_input = nullptr;
    static inline SystemFactory* m_factory = nullptr;
    static inline GE::ECS::EntityManager* m_entityManager = nullptr;
    static inline AssetManager* m_assetManager = nullptr;

    // NEW: Static pointer for the scene
    static inline GE::Scene::Scene* m_scene = nullptr;
};