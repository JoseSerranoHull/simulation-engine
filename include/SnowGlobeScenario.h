#pragma once
#include "Scenario.h"
#include "ClimateManager.h"
#include "Model.h"
#include <memory>
#include <vector>

namespace GE {
    /**
     * @class SnowGlobeScenario
     * @brief Encapsulates the specific logic for the Sandy Snow Globe simulation.
     */
    class SnowGlobeScenario final : public Scenario {
    public:
        SnowGlobeScenario() = default;
        ~SnowGlobeScenario() override = default;

        // --- Lifecycle Hooks ---
        void OnLoad(VkCommandBuffer cmd, std::vector<VkBuffer>& sb, std::vector<VkDeviceMemory>& sm) override;
        void OnUpdate(float dt, float totalTime) override;
        void OnUnload() override;

    private:
        /** @brief Handles the scaling of Cacti and the Oasis based on weather. */
        void updateDynamicEntityScaling();

        std::unique_ptr<ClimateService> m_climate;
        std::vector<std::unique_ptr<GE::Assets::Model>> m_ownedModels; // Local ownership of meshes
    };
}