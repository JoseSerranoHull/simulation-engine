#pragma once
#include <string>

namespace GE::Scene::FB { struct FBSceneContext; }

namespace GE::Scene {

    /**
     * @interface ISceneAdapter
     * @brief Target interface in the Adapter pattern. Defines what FlatBuffersScenario
     *        needs from any scene loader, regardless of binary format.
     *        Pattern: Adapter (Object Adapter variant)
     *        Enables future formats: IniSceneAdapter, JsonSceneAdapter, etc.
     */
    class ISceneAdapter {
    public:
        virtual ~ISceneAdapter() = default;

        /** @brief Loads the scene binary from disk. Returns false on failure. */
        virtual bool load(const std::string& path) = 0;

        /** @brief Adapts the loaded scene to ECS by populating ctx with entities and cameras. */
        virtual void adaptToECS(FB::FBSceneContext& ctx) = 0;

        /** @brief Returns the scene's display name as declared in the binary. */
        virtual const std::string& getSceneName() const = 0;
    };

} // namespace GE::Scene
