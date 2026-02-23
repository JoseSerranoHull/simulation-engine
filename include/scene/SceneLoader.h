#pragma once

/* parasoft-begin-suppress ALL */
#include <string>
#include <map>
#include <unordered_map>
#include <functional>
#include <vector>
#include <memory>
#include <glm/glm.hpp>
/* parasoft-end-suppress ALL */

#include "ecs/EntityManager.h"
#include "assets/AssetManager.h"
#include "scene/Scene.h"
#include "components/Components.h"
#include "graphics/GpuUploadContext.h"

namespace GE::Scene {

    class SceneLoader {
    public:
        /** @brief Handler signature: receives the section id and parsed key-value props. */
        using SectionHandler = std::function<void(const std::string& id, const std::map<std::string, std::string>& props)>;

        SceneLoader() = default;

        /**
         * @brief Main entry point to load an .ini scene file.
         */
        void load(const std::string& path,
            GE::ECS::EntityManager* em,
            AssetManager* am,
            GE::Scene::Scene* scene,
            const std::vector<std::unique_ptr<GE::Graphics::GraphicsPipeline>>& pipelines,
            GE::Graphics::GpuUploadContext& ctx,
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
        void handleMeshRenderer(const std::map<std::string, std::string>& props, GE::ECS::EntityManager* em, AssetManager* am, GE::Graphics::GpuUploadContext& ctx, std::vector<std::unique_ptr<GE::Assets::Model>>& outOwnedModels);
        void handleTag(const std::map<std::string, std::string>& props, GE::ECS::EntityManager* em, GE::Scene::Scene* scene);
        void handleLightComponent(const std::map<std::string, std::string>& props, GE::ECS::EntityManager* em);
        void handleRigidBody(const std::map<std::string, std::string>& props, GE::ECS::EntityManager* em);
        void handleSphereCollider(const std::map<std::string, std::string>& props, GE::ECS::EntityManager* em);
        void handlePlaneCollider(const std::map<std::string, std::string>& props, GE::ECS::EntityManager* em);
        void handleParticleComponent(const std::map<std::string, std::string>& props, GE::ECS::EntityManager* em);
        void handleSkyboxComponent(const std::map<std::string, std::string>& props, GE::ECS::EntityManager* em);
    };
}