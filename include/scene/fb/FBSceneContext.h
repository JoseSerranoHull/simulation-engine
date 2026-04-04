#pragma once

/* parasoft-begin-suppress ALL */
#include <array>
#include <memory>
#include <string>
#include <vector>
#include <glm/glm.hpp>
/* parasoft-end-suppress ALL */

#include "components/AnimationComponents.h"  // SpawnLocType, EntityID

// Forward declarations to avoid pulling in heavy headers
namespace GE::ECS    { class EntityManager; }
namespace GE::Scene  { class Scene; }
namespace GE::Graphics { struct GpuUploadContext; class GraphicsPipeline; }
namespace GE::Assets { class Model; }
class AssetManager;

namespace GE::Scene::FB {

    /** @brief Physics material record extracted from FlatBuffers Material table. */
    struct PhysicsMaterialRecord {
        std::string name;
        float       density { 1.0f };
    };

    /** @brief Interaction parameters for a material pair, extracted from FlatBuffers MaterialInteraction table. */
    struct MaterialInteractionRecord {
        std::string materialA;
        std::string materialB;
        float restitution     { 0.6f };
        float staticFriction  { 0.0f };
        float dynamicFriction { 0.0f };
    };

    /**
     * @brief Intermediate record for one spawner entry, populated by adaptSpawners().
     *        Entity IDs are pre-created at load time; SpawnerSystem activates them at runtime.
     */
    struct SpawnerRecord {
        std::string name;
        float    startTime  { 0.0f };
        bool     isBurst    { true };
        uint32_t maxCount   { 1 };
        float    interval   { 1.0f };

        // Location
        GE::Components::SpawnLocType locationType { GE::Components::SpawnLocType::FIXED };
        glm::vec3 fixedPos     { 0.0f };
        glm::vec3 boxMin       { -1.0f };
        glm::vec3 boxMax       {  1.0f };
        glm::vec3 sphereCenter {  0.0f };
        float     sphereRadius { 1.0f };

        // Velocity ranges
        glm::vec3 linVelMin { 0.0f }, linVelMax { 0.0f };
        glm::vec3 angVelMin { 0.0f }, angVelMax { 0.0f };

        // Pre-created entity IDs (in activation order, SEQUENTIAL cycling already applied)
        std::vector<GE::ECS::EntityID> entityIds;

        /// Peer ID (1-4) responsible for firing this spawner; derived from SpawnerOwnerType.
        uint8_t ownerPeerId { 1 };
    };

    /**
     * @brief Camera record extracted from a FlatBuffers Camera table.
     * Stored in FBSceneContext::cameras and consumed by FlatBuffersScenario.
     */
    struct FBCameraRecord {
        std::string name;
        glm::vec3   position  { 0.0f, 0.0f,  3.0f };
        glm::vec3   direction { 0.0f, 0.0f, -1.0f };
        float       fov       { 45.0f };
        bool        isOrtho   { false };
        float       orthoSize { 5.0f };
        float       nearPlane { 0.1f };
        float       farPlane  { 200.0f };
    };

    /**
     * @struct FBSceneContext
     * @brief Mutable output context passed through all adapt*() calls in FBSceneAdapter.
     *        Pattern: Adapter (Object Adapter) — this is the "target state" being populated.
     *        All service pointers are non-owning (lifetime managed by EngineOrchestrator).
     */
    struct FBSceneContext {
        // --- Input services (non-owning) ---
        GE::ECS::EntityManager*                                             em        { nullptr };
        AssetManager*                                                       am        { nullptr };
        GE::Scene::Scene*                                                   scene     { nullptr };
        GE::Graphics::GpuUploadContext*                                     uploadCtx { nullptr };
        const std::vector<std::unique_ptr<GE::Graphics::GraphicsPipeline>>* pipelines { nullptr };
        std::vector<std::unique_ptr<GE::Assets::Model>>*                    ownedModels { nullptr };

        // --- Config ---
        bool useOwnerColors { true };

        // --- Output collections (populated by adapt*() calls) ---
        std::vector<PhysicsMaterialRecord>    physicsMaterials;
        std::vector<FBCameraRecord>           cameras;
        std::vector<MaterialInteractionRecord> interactions;
        std::vector<SpawnerRecord>            spawners;

        // Owner color palette: ONE=red, TWO=green, THREE=blue, FOUR=yellow
        static constexpr std::array<glm::vec3, 4> ownerColors = {{
            { 1.0f, 0.2f, 0.2f },   // ONE   — red
            { 0.2f, 1.0f, 0.2f },   // TWO   — green
            { 0.2f, 0.4f, 1.0f },   // THREE — blue
            { 1.0f, 1.0f, 0.2f }    // FOUR  — yellow
        }};

        // Default grey for NONE/material-color mode
        static constexpr glm::vec3 defaultColor = { 0.7f, 0.7f, 0.7f };
    };

} // namespace GE::Scene::FB
