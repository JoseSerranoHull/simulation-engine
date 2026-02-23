#pragma once

/* parasoft-begin-suppress ALL */
#include <string>
#include <map>
#include <vector>
#include <memory>
#include <glm/glm.hpp>
/* parasoft-end-suppress ALL */

#include "../include/EntityManager.h"
#include "../include/AssetManager.h"
#include "../include/Scene.h"
#include "../include/Components.h"

namespace GE::Scene {

    class SceneLoader {
    public:
        SceneLoader() = default;

        /**
         * @brief Main entry point to load an .ini scene file.
         */
        void load(const std::string& path,
            GE::ECS::EntityManager* em,
            AssetManager* am,
            GE::Scene::Scene* scene,
            const std::vector<std::unique_ptr<GE::Graphics::GraphicsPipeline>>& pipelines,
            VkCommandBuffer setupCmd,
            std::vector<VkBuffer>& stagingBufs,
            std::vector<VkDeviceMemory>& stagingMems,
            std::vector<std::unique_ptr<GE::Assets::Model>>& outOwnedModels
        );

    private:
        // --- Resource Registries (Alises used in .ini) ---
        std::map<std::string, std::shared_ptr<GE::Graphics::Texture>> m_textures;
        std::map<std::string, std::string> m_texturePaths;
        std::map<std::string, std::shared_ptr<GE::Assets::Material>> m_materials;

        // Tracking the current entity being built
        GE::ECS::EntityID m_currentEntity = GE::ECS::INVALID_ENTITY_ID;

        // --- Parsing Helpers ---
        std::vector<std::string> splitString(const std::string& str);
        glm::vec3 parseVec3(const std::string& val);
        float parseFloat(const std::string& val);

        // --- Section Handlers ---
        void handleTexture(const std::string& id, const std::map<std::string, std::string>& properties, AssetManager* am);
        void handleMaterial(const std::string& id, const std::map<std::string, std::string>& properties, AssetManager* am, const std::vector<std::unique_ptr<GE::Graphics::GraphicsPipeline>>& pipelines);
        void handleEntity(GE::ECS::EntityManager* em);
        void handleTransform(const std::map<std::string, std::string>& props, GE::ECS::EntityManager* em);
        void handleMeshRenderer(const std::map<std::string, std::string>& props, GE::ECS::EntityManager* em, AssetManager* am, VkCommandBuffer cmd, std::vector<VkBuffer>& sb, std::vector<VkDeviceMemory>& sm, std::vector<std::unique_ptr<GE::Assets::Model>>& outOwnedModels);
        void handleTag(const std::map<std::string, std::string>& props, GE::ECS::EntityManager* em, GE::Scene::Scene* scene);
        void handleLightComponent(const std::map<std::string, std::string>& props, GE::ECS::EntityManager* em);
        void handleRigidBody(const std::map<std::string, std::string>& props, GE::ECS::EntityManager* em);
        void handleSphereCollider(const std::map<std::string, std::string>& props, GE::ECS::EntityManager* em);
        void handlePlaneCollider(const std::map<std::string, std::string>& props, GE::ECS::EntityManager* em);
        void handleParticleComponent(const std::map<std::string, std::string>& props, GE::ECS::EntityManager* em);
        void handleSkyboxComponent(const std::map<std::string, std::string>& props, GE::ECS::EntityManager* em);
    };
}