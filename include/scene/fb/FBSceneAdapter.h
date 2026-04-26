#pragma once

/* parasoft-begin-suppress ALL */
#include <cstdint>
#include <string>
#include <vector>
/* parasoft-end-suppress ALL */

#include "scene/ISceneAdapter.h"
#include "scene/fb/FBSceneContext.h"

// Forward-declare FlatBuffers generated types (from Scene_generated.h)
namespace Simulation {
    struct Scene;
    struct Camera;
    struct Object;
    struct Material;
    struct MaterialInteraction;
}

namespace GE::ECS  { using EntityID = uint32_t; }

namespace GE::Scene::FB {

    /**
     * @class FBSceneAdapter
     * @brief Adapter (Object Adapter) that bridges the FlatBuffers scene representation
     *        (Adaptee: Simulation::Scene* from Scene_generated.h) to the engine's ECS
     *        population interface (Target: ISceneAdapter).
     *
     *        Owns the binary buffer. The FlatBuffers zero-copy guarantee is preserved
     *        throughout adaptToECS() — no intermediate data copies.
     *
     *        Pattern: Adapter (Object Adapter variant)
     *          - Adaptee:  Simulation::Scene* (FlatBuffers generated)
     *          - Target:   ISceneAdapter
     *          - Client:   FlatBuffersScenario::OnLoad()
     */
    class FBSceneAdapter final : public GE::Scene::ISceneAdapter {
    public:
        FBSceneAdapter() = default;

        // ISceneAdapter interface
        bool               load(const std::string& path) override;
        void               adaptToECS(FBSceneContext& ctx) override;
        const std::string& getSceneName() const override { return m_sceneName; }

    private:
        std::vector<uint8_t>     m_buffer;             // owns the binary data
        const Simulation::Scene* m_scene { nullptr };  // zero-copy view into m_buffer
        std::string              m_sceneName;

        // --- Per-element adapters ---
        void adaptPrefabs     (FBSceneContext& ctx) const;  // must run before adaptSpawners
        void adaptCameras     (FBSceneContext& ctx) const;
        void adaptMaterials   (FBSceneContext& ctx) const;
        void adaptObjects     (FBSceneContext& ctx) const;
        void adaptInteractions(FBSceneContext& ctx) const;
        void adaptSpawners    (FBSceneContext& ctx) const;
        void adaptParentLinks (FBSceneContext& ctx) const;

        void adaptObject     (const Simulation::Object*              obj, FBSceneContext& ctx) const;
        void adaptShape      (const Simulation::Object*              obj, GE::ECS::EntityID id,
                              const glm::vec3& color, bool isContainer,
                              FBSceneContext& ctx) const;
        void adaptBehaviour  (const Simulation::Object*              obj, GE::ECS::EntityID id,
                              FBSceneContext& ctx) const;
        void adaptCamera     (const Simulation::Camera*              cam, FBSceneContext& ctx) const;
        void adaptMaterial   (const Simulation::Material*            mat, FBSceneContext& ctx) const;
        void adaptInteraction(const Simulation::MaterialInteraction* mi,  FBSceneContext& ctx) const;

        glm::vec3 resolveColor(const Simulation::Object* obj,
                               const FBSceneContext& ctx) const;
    };

} // namespace GE::Scene::FB
