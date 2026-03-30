#pragma once
#include "scene/Scenario.h"
#include "systems/PhysicsSystem.h"
#include "systems/ColliderVisualizerSystem.h"
#include <glm/glm.hpp>

namespace GE {

    /**
     * @struct CheckerboardPushConstants
     * @brief Per-frame push constant data for the procedural checkerboard pipeline.
     * Sent via vkCmdPushConstants before each checkerboard mesh draw call.
     */
    struct CheckerboardPushConstants {
        glm::vec4 colorA{ 1.0f, 1.0f, 1.0f, 1.0f }; // Light squares  (offset 0, 16 bytes)
        glm::vec4 colorB{ 0.1f, 0.1f, 0.1f, 1.0f }; // Dark squares   (offset 16, 16 bytes)
        float scale{ 2.0f };                           // Tiling density (offset 32, 4 bytes)
    };
    // Total = 36 bytes — matches the pipeline layout's checker push constant range.
    // glm::vec4 is already 16-byte aligned; alignas(16) was causing struct padding to 48.
    static_assert(sizeof(CheckerboardPushConstants) == 36U,
        "CheckerboardPushConstants must be exactly 36 bytes to match the pipeline layout");

    class GenericScenario final : public Scenario {
    public:
        explicit GenericScenario(std::string configPath) {
            m_configPath = std::move(configPath);
        }

        void OnLoad(GE::Graphics::GpuUploadContext& ctx) override;
        void OnUpdate(float dt, float totalTime) override;
        void OnUnload() override;
        void OnGUI() override;

        // --- Scenario Checkerboard Interface ---
        const GE::Graphics::GraphicsPipeline* GetCheckerboardPipeline() const override {
            // Pipeline index 6 is the checkerboard pipeline (added after the standard 6)
            return (m_pipelines.size() > 6U) ? m_pipelines[6U].get() : nullptr;
        }
        const void* GetCheckerboardPushData() const override { return &m_checkerConstants; }
        uint32_t    GetCheckerboardPushDataSize() const override {
            return static_cast<uint32_t>(sizeof(m_checkerConstants));
        }

        // --- Collider Wireframe Visualizer Interface ---
        const GE::Graphics::GraphicsPipeline* GetWirePipeline() const override {
            return (m_pipelines.size() > 7U) ? m_pipelines[7U].get() : nullptr;
        }
        GE::Systems::ColliderVisualizerSystem* GetVisualizerSystem() const override {
            return m_visualizerSystem;
        }

    private:
        /** @brief Non-owning pointer to the registered PhysicsSystem; used to expose
         *  integration method selection in OnGUI(). EntityManager owns the lifetime. */
        Systems::PhysicsSystem* m_physicsSystem{ nullptr };

        /** @brief Non-owning pointer to the registered ColliderVisualizerSystem.
         *  EntityManager owns the lifetime; pointer is nulled on OnUnload(). */
        Systems::ColliderVisualizerSystem* m_visualizerSystem{ nullptr };

        /** @brief Per-scenario checkerboard material colours, editable via ImGui. */
        CheckerboardPushConstants m_checkerConstants{};
    };
}