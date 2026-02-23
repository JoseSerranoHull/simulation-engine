#pragma once
#include "scene/Scenario.h"

namespace GE {
    class GenericScenario final : public Scenario {
    public:
        explicit GenericScenario(std::string configPath) {
            m_configPath = std::move(configPath);
        }

        void OnLoad(GE::Graphics::GpuUploadContext& ctx) override;
        void OnUpdate(float dt, float totalTime) override;
        void OnUnload() override;
        void OnGUI() override;
    };
}