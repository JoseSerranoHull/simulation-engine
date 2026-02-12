#include "../include/EntityFactory.h"
#include "../include/Components.h"

namespace GE::Scene {

    ERROR_CODE EntityFactory::Initialize(GE::ECS::EntityManager* entityManager) {
        if (m_isInitialized) return ERROR_CODE::ALREADY_INITIALIZED;
        m_entityManager = entityManager;
        m_isInitialized = true;
        return ERROR_CODE::OK;
    }

    void EntityFactory::Shutdown() {
        m_isInitialized = false;
        m_entityManager = nullptr;
    }

    GE::ECS::EntityID EntityFactory::CreatePrimitive(
        const std::string& shape,
        AssetManager* am,
        VkCommandBuffer cmd,
        std::vector<VkBuffer>& sb,
        std::vector<VkDeviceMemory>& sm
    ) {
        if (!m_isInitialized) return GE::ECS::INVALID_ENTITY_ID;

        // 1. Create the base entity
        const GE::ECS::EntityID entityID = m_entityManager->CreateEntity();

        // 2. Generate the requested geometry data
        OBJLoader::MeshData data;
        if (shape == "Plane") {
            data = GeometryUtils::generatePlane(1.0f, 1.0f); // Default 1x1 plane
        }
        else if (shape == "Capsule") {
            data = GeometryUtils::generateCapsule(0.5f, 1.0f, 32, 16); // Default capsule
        }

        // 3. Process data into a GPU Mesh using your AssetManager
        // We use a null material initially; the SceneLoader will assign the correct one.
        auto meshPtr = am->processMeshData(data, nullptr, cmd, sb, sm);

        // 4. Assemble the MeshRenderer component
        GE::Components::MeshRenderer mr;
        mr.subMeshes.push_back({ meshPtr.get(), nullptr });

        // 5. Attach Components
        m_entityManager->AddComponent<GE::Scene::Components::Transform>(entityID, GE::Scene::Components::Transform());
        m_entityManager->AddComponent<GE::Components::MeshRenderer>(entityID, mr);

        // Note: To keep the Mesh* valid, the unique_ptr must be stored. 
        // In your architecture, this is usually handled by Experience::ownedModels.
        // We assume the caller handles the model ownership.

        return entityID;
    }

}