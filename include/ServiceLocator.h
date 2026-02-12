#pragma once

/* parasoft-begin-suppress ALL */
#include <stdexcept>
#include <memory>

#include "../include/EntityManager.h"
/* parasoft-end-suppress ALL */

// Forward declarations
class SystemFactory;
struct VulkanContext;
class VulkanResourceManager;
class InputManager;

class ServiceLocator final {
public:
    // Provide services to the locator
    static void Provide(VulkanContext* context) { m_context = context; }
    static void Provide(VulkanResourceManager* resources) { m_resources = resources; }
    static void Provide(InputManager* input) { m_input = input; }
    static void Provide(GE::ECS::EntityManager* entityManager) { m_entityManager = entityManager; }

    // Retrieve services
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

    static void Provide(SystemFactory* factory) { m_factory = factory; }

    static SystemFactory* GetFactory() { return m_factory; }

private:
    static inline VulkanContext* m_context = nullptr;
    static inline VulkanResourceManager* m_resources = nullptr;
    static inline InputManager* m_input = nullptr;
    static inline SystemFactory* m_factory = nullptr;
	static inline GE::ECS::EntityManager* m_entityManager = nullptr;
};