#pragma once

/* parasoft-begin-suppress ALL */
#include <vector>
#include <memory>
#include <string>
/* parasoft-end-suppress ALL */

#include "core/Common.h"
#include "assets/Model.h"
#include "graphics/ShaderModule.h"
#include "graphics/GraphicsPipeline.h"

// Forward declaration — keeps Vulkan types out of the scenario interface header.
namespace GE::Graphics { struct GpuUploadContext; }

namespace GE {
    /**
     * @class Scenario
     * @brief Abstract base class for different simulation states.
     * Fulfills State Pattern requirements for the Physics Lab.
     */
    class Scenario {
    public:
        virtual ~Scenario() = default;

        /** @brief Fulfills Requirement: Ability to load/initialize scenarios. */
        virtual void OnLoad(GE::Graphics::GpuUploadContext& ctx) = 0;

        /** @brief Standard logic update for physics and simulation events. */
        virtual void OnUpdate(float dt, float totalTime) = 0;

        /** @brief Fulfills Requirement: Ability to unload/teardown scenarios. */
        virtual void OnUnload() = 0;

        /** @brief NEW: Hook for scenario-specific ImGui menus. */
        virtual void OnGUI() {}

        // Lab Requirements: Simulation Controls
        bool  IsPaused()   const { return m_isPaused; }
        void  SetPaused(bool p) { m_isPaused = p; }
        float GetTimeScale() const { return m_timeScale; }
        void  SetTimeScale(float s) { m_timeScale = s; }

        /** @brief Returns the path to the .ini file used to build this scenario. */
        const std::string& GetConfigPath() const { return m_configPath; }
        /** @brief Sets the data source path for this scenario instance. */
        void SetConfigPath(const std::string& path) { m_configPath = path; }

        /** @brief Accessor for the scenario's material pipeline set, used by the renderer. */
        const std::vector<std::unique_ptr<GE::Graphics::GraphicsPipeline>>& GetPipelines() const {
            return m_pipelines;
        }

    protected:
        /** @brief Builds the fixed set of material pipelines for this scenario. */
        void createMaterialPipelines();
        bool  m_isPaused = false;
        float m_timeScale = 1.0f;

        /** @brief Registry of models unique to this scenario for cleanup. */
        std::vector<std::unique_ptr<GE::Assets::Model>> m_ownedModels;

        /** @brief Scenario-scoped shader modules; lifetime tied to this scenario. */
        std::vector<std::unique_ptr<GE::Graphics::ShaderModule>> m_shaderModules;

        /** @brief Scenario-scoped material pipelines; destroyed on scenario unload. */
        std::vector<std::unique_ptr<GE::Graphics::GraphicsPipeline>> m_pipelines;

        /** @brief Path to the .ini file defining this scenario's entities. */
        std::string m_configPath;
    };
}