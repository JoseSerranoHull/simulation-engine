#pragma once

/* parasoft-begin-suppress ALL */
#include <map>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>
/* parasoft-end-suppress ALL */

#include "Entity.h"
#include "Components.h"

// Forward declarations
class Pipeline;

namespace GE::Scene
{
    /**
     * @class Scene
     * @brief Orchestrates ECS entities by name and manages procedural animations.
     */
    class Scene final {
    public:
        // --- Model Identification Keys ---
        inline static const std::string KEY_MAGIC_CIRCLE = "MagicCircle";
        inline static const std::string KEY_DESERT_QUEEN = "DesertQueen";

        // --- Animation Constants ---
        static constexpr float CIRCLE_ROT_SPEED = 60.0f;
        static constexpr float QUEEN_HOVER_FREQ = 2.0f;
        static constexpr float QUEEN_HOVER_AMP = 0.05f;
        static constexpr float QUEEN_BASE_HEIGHT = 0.5f;

        explicit Scene();
        ~Scene() = default;

        Scene(const Scene&) = delete;
        Scene& operator=(const Scene&) = delete;

        /** @brief Updates procedural animations by manipulating Transform components. */
        void update(const float deltaTime);

        // --- Entity Registry Management ---

        /** @brief Associates a string name with an ECS Entity. */
        void addEntity(const std::string& name, GE::ECS::EntityID id) { entities[name] = id; }

        /** @brief Checks if a named entity exists. */
        bool hasEntity(const std::string& key) const { return entities.find(key) != entities.end(); }

        /** @brief Gets the ID for a named entity. */
        GE::ECS::EntityID getEntityID(const std::string& key) const;

        /** @brief Returns the full name-to-ID map for the Renderer. */
        const std::map<std::string, GE::ECS::EntityID>& getEntityMap() const { return entities; }

    private:
        // This map now just tracks IDs for easy lookups by name.
        std::map<std::string, GE::ECS::EntityID> entities;
    };
}