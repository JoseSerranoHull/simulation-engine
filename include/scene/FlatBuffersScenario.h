#pragma once

/* parasoft-begin-suppress ALL */
#include <string>
#include <unordered_map>
#include <vector>
/* parasoft-end-suppress ALL */

#include "scene/Scenario.h"
#include "scene/fb/FBSceneContext.h"      // for FBCameraRecord, PrefabTemplate
#include "physics/MaterialInteractionRegistry.h"

namespace GE::Systems { class AnimationSystem; class PhysicsSystem; class SpawnerSystem; class ClothSystem; class FlockingSystem; class ScriptSystem; class ColliderVisualizerSystem; }

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

        const GE::Graphics::GraphicsPipeline* GetWirePipeline() const override {
            return (m_pipelines.size() > 7U) ? m_pipelines[7U].get() : nullptr;
        }
        GE::Systems::ColliderVisualizerSystem* GetVisualizerSystem() const override {
            return m_visualizerSystem;
        }

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
        GE::Systems::PhysicsSystem*   m_physicsSystem   { nullptr };
        GE::Systems::SpawnerSystem*   m_spawnerSystem   { nullptr };
        GE::Systems::ClothSystem*     m_clothSystem     { nullptr };
        GE::Systems::FlockingSystem*  m_flockingSystem  { nullptr };
        GE::Systems::ScriptSystem*              m_scriptSystem      { nullptr };
        GE::Systems::ColliderVisualizerSystem*  m_visualizerSystem  { nullptr };

        // --- Cloth sphere spawner (scene 05 only; UINT32_MAX = not present) ---
        uint32_t  m_clothSpawnerEntityID { UINT32_MAX };

        // --- Material interaction registry (populated at load, passed to PhysicsSystem) ---
        GE::Physics::MaterialInteractionRegistry m_interactionRegistry;

        // --- Prefab registry (populated at load; SpawnerComponents hold raw pointers into this) ---
        std::unordered_map<std::string, GE::Scene::FB::PrefabTemplate> m_prefabRegistry;

        // --- Networking UI state (ImGui "Network" menu) ---
        struct PeerUIEntry {
            char ip[64]    { "127.0.0.1" };
            int  port      { 7001 };
            int  peerId    { 2 };      ///< Explicit remote peer ID (1-4, != local)
            bool connected { false };
        };
        int          m_localPeerId    { 1 };
        int          m_localPort      { 7000 };
        bool         m_netInitialised { false };
        PeerUIEntry  m_peerEntries[3] {};   ///< Entries for peers 2, 3, 4 relative to local

        enum class ConnectionMethod { None, Auto, Manual };
        ConnectionMethod m_connectionMethod { ConnectionMethod::None };

        char m_autoConnectHostIP[64] {};  ///< Leave empty to broadcast (host); fill with host's IP to unicast (joiner)

        // --- Helpers ---
        void buildCamerasFromContext(const GE::Scene::FB::FBSceneContext& ctx);
        void applyActiveCamera() const;
        void scanSceneDirectory();
        void disconnectNetwork();
    };

} // namespace GE
