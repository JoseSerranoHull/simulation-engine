#pragma once

/* parasoft-begin-suppress ALL */
#include <memory>
#include <string>
#include <map>
#include <functional>
/* parasoft-end-suppress ALL */

#include "ecs/ISystem.h"

/**
 * @class EngineServiceRegistry
 * @brief Registry-based factory that manages system creation logic.
 */
class EngineServiceRegistry final {
public:
    // A Creator is a function that returns a unique_ptr to an ISystem
    using Creator = std::function<std::unique_ptr<ISystem>()>;

    /**
     * @brief Registers a new system type with the factory.
     * Satisfies Open-Closed Principle: Register new systems without modifying this file.
     */
    void registerSystem(const std::string& name, Creator creator) {
        m_registry[name] = creator;
    }

    /**
     * @brief Instantiates a system by its registered name.
     */
    std::unique_ptr<ISystem> create(const std::string& name) const {
        auto it = m_registry.find(name);
        if (it != m_registry.end()) {
            return it->second();
        }
        return nullptr;
    }

private:
    std::map<std::string, Creator> m_registry;
};