#pragma once

/* parasoft-begin-suppress ALL */
#include <array>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>
/* parasoft-end-suppress ALL */

#include "components/AnimationComponent.h"
#include "components/SpawnerComponent.h"

// Forward declarations to avoid pulling in heavy headers
namespace GE::ECS    { class EntityManager; }
namespace GE::Scene  { class Scene; }
namespace GE::Graphics { struct GpuUploadContext; class GraphicsPipeline; }
namespace GE::Assets { class Model; class Mesh; class Material; }
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

    // ---------------------------------------------------------------------------
    // Prefab system
    // ---------------------------------------------------------------------------

    /** @brief Shape kind stored in PrefabTemplate for collider selection at spawn time. */
    enum class PrefabShapeKind : uint8_t { Sphere, Plane, Capsule, Cylinder, Cuboid };

    /**
     * @brief Pre-computed entity template built at scene load time by adaptPrefabs().
     *        The shared GPU mesh is uploaded once and reused by all spawned instances.
     *        Lifetime: owned by FlatBuffersScenario::m_prefabRegistry (valid until OnUnload).
     */
    struct PrefabTemplate {
        std::string         name;
        PrefabShapeKind     shapeKind { PrefabShapeKind::Sphere };

        // Shape parameters (used to build the collider at spawn time)
        float     radius { 0.5f };   // sphere radius / capsule+cylinder radius
        float     height { 1.0f };   // capsule / cylinder height
        glm::vec3 size   { 1.0f };   // cuboid full extents (x, y, z)

        // Physics (computed from shape + material density at load time)
        float     density     { 1.0f };
        float     restitution { 0.6f };
        float     mass        { 0.0f };
        glm::mat3 invInertia  { glm::mat3(0.0f) };

        // Owner-colored meshes — indices 0–3 = peer 1–4 (red/green/blue/yellow).
        // Populated when useOwnerColors=true; all null otherwise.
        std::array<GE::Assets::Mesh*, 4> ownerMeshes { nullptr, nullptr, nullptr, nullptr };

        // Material-appearance mesh — populated when useOwnerColors=false.
        // Textured (Phong pipeline) when texture_path specified; tinted flat-color otherwise.
        GE::Assets::Mesh*                          materialMesh   { nullptr };
        std::shared_ptr<GE::Assets::Material>      materialMatPtr;  // keeps Phong material alive

        // Optional script type name (empty = no script attached on spawn)
        std::string scriptType;

        // Physics material name for spawned entities (enables MaterialInteractionRegistry lookups).
        // Empty = no PhysicsMaterialTag added.
        std::string physicsMaterialName;
    };

    /**
     * @brief Intermediate record for one spawner entry, populated by adaptSpawners().
     *        References a PrefabTemplate by name; SpawnerSystem instantiates at runtime.
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

        // Prefab reference (name in FBSceneContext::prefabRegistry)
        std::string prefabRef;

        /// Peer ID (1-4) responsible for firing this spawner; derived from SpawnerOwnerType.
        uint8_t ownerPeerId { 1 };

        /// True when SpawnerOwnerType::SEQUENTIAL — SpawnerSystem cycles color across peers.
        bool isSequential { false };

        // For spawners synthesised from radius_range (no prefab_ref): keys into prefabRegistry.
        // SpawnerSystem picks a random entry each spawn. Empty = single prefabTemplate.
        std::vector<std::string> prefabVariantRefs;
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
        std::vector<PhysicsMaterialRecord>              physicsMaterials;
        std::vector<FBCameraRecord>                     cameras;
        std::vector<MaterialInteractionRecord>          interactions;
        std::vector<SpawnerRecord>                      spawners;
        std::unordered_map<std::string, PrefabTemplate> prefabRegistry;

        // Whether the scene enables gravity (mirrors Scene::gravity_on). Applied to PhysicsSystem after load.
        bool gravityEnabled { true };

        // Material-name → base palette index. Populated by adaptMaterials() in registration order.
        // resolveColor() adds a per-material instance counter so shared-material objects each
        // get a distinct palette color (e.g. Newton's Cradle balls all using "BallMat").
        std::unordered_map<std::string, std::size_t> materialPaletteBaseIndex;

        // Per-material instance counter: incremented each time resolveColor() assigns a color
        // to an object with that material in Material Colors mode. Mutable so resolveColor()
        // (a const method) can update it.
        mutable std::unordered_map<std::string, uint32_t> materialInstanceCounters;

        // 16-entry vivid palette assigned in material-registration order.
        // Index 0 = first material in the scene's materials[] array, etc.
        static constexpr std::array<glm::vec3, 16> materialPalette = {{
            { 1.00f, 0.41f, 0.71f },  //  0: hot pink
            { 0.90f, 0.15f, 0.15f },  //  1: red
            { 0.12f, 0.80f, 0.20f },  //  2: green
            { 0.15f, 0.35f, 0.95f },  //  3: blue
            { 0.95f, 0.86f, 0.05f },  //  4: yellow
            { 0.70f, 0.10f, 0.90f },  //  5: violet
            { 0.05f, 0.84f, 0.84f },  //  6: cyan
            { 0.95f, 0.50f, 0.05f },  //  7: orange
            { 0.50f, 0.80f, 0.10f },  //  8: lime
            { 0.60f, 0.15f, 0.35f },  //  9: maroon
            { 0.10f, 0.55f, 0.55f },  // 10: teal
            { 0.90f, 0.65f, 0.30f },  // 11: peach
            { 0.25f, 0.60f, 0.85f },  // 12: sky blue
            { 0.85f, 0.30f, 0.10f },  // 13: burnt orange
            { 0.55f, 0.85f, 0.55f },  // 14: mint
            { 0.85f, 0.85f, 0.55f },  // 15: cream
        }};

        // Flock spawn configuration — populated by adaptBehaviour() when FlockAgent is present.
        // FlatBuffersScenario reads these after Adapt() to initialise FlockingSystem::Restart().
        glm::vec3 flockSpawnOrigin { 0.0f };
        float     flockSpawnRadius  { 5.0f };

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
