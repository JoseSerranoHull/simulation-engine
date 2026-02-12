#pragma once
#include "../include/EntityManager.h"
#include "../include/AssetManager.h"
#include "../include/GeometryUtils.h"

namespace GE::Scene {

    class EntityFactory {
    public:
        EntityFactory() = delete;

        static ERROR_CODE Initialize(GE::ECS::EntityManager* entityManager);
        static void Shutdown();

        /**
         * @brief Creates an entity with a procedural primitive mesh.
         */
        static GE::ECS::EntityID CreatePrimitive(
            const std::string& shape,
            AssetManager* am,
            VkCommandBuffer cmd,
            std::vector<VkBuffer>& sb,
            std::vector<VkDeviceMemory>& sm
        );

    private:
        static inline GE::ECS::EntityManager* m_entityManager = nullptr;
        static inline bool m_isInitialized = false;
    };

}