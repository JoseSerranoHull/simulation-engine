#pragma once

/* parasoft-begin-suppress ALL */
#include <string>
#include <vector>
/* parasoft-end-suppress ALL */

#include "scene/Scenario.h"
#include "scene/fb/FBSceneContext.h"   // for FBCameraRecord

namespace GE::Systems { class AnimationSystem; }

namespace GE {

    /**
     * @class FlatBuffersScenario
     * @brief Scenario that loads a scene from a FlatBuffers binary (.bin) file.
     *        Uses FBSceneAdapter (Adapter pattern) to bridge the FlatBuffers API to ECS.
     *        Supports named cameras, scene switching and owner-color display via ImGui.
     */
    class FlatBuffersScenario final : public Scenario {
    public:
        explicit FlatBuffersScenario(std::string binaryPath, bool useOwnerColors = true);
        ~FlatBuffersScenario() override = default;

        void OnLoad  (GE::Graphics::GpuUploadContext& ctx) override;
        void OnUpdate(float dt, float totalTime)           override;
        void OnUnload()                                    override;
        void OnGUI   ()                                    override;

        // Wire/checker pipeline accessors — not used by this scenario; return nullptr
        const GE::Graphics::GraphicsPipeline* GetWirePipeline() const override { return nullptr; }

    private:
        bool        m_useOwnerColors { true };
        std::string m_sceneName;

        // --- Per-scene camera state ---
        struct FBCameraState {
            std::string name;
            glm::vec3   position  { 0.0f };
            glm::vec3   direction { 0.0f, 0.0f, -1.0f };
            float       fov       { 45.0f };
            bool        isOrtho   { false };
            float       orthoSize { 5.0f };
        };
        std::vector<FBCameraState> m_cameras;
        int                        m_activeCameraIndex { 0 };

        // --- Scene switcher ---
        std::vector<std::string> m_availableScenes;  // .bin paths found in ./config/

        // --- Owned systems ---
        GE::Systems::AnimationSystem* m_animationSystem { nullptr };

        // --- Helpers ---
        void buildCamerasFromContext(const GE::Scene::FB::FBSceneContext& ctx);
        void applyActiveCamera() const;
        void scanSceneDirectory();
    };

} // namespace GE
