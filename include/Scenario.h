#pragma once

/* parasoft-begin-suppress ALL */
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
/* parasoft-end-suppress ALL */

#include "Common.h"

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
        virtual void OnLoad(VkCommandBuffer cmd, std::vector<VkBuffer>& sb, std::vector<VkDeviceMemory>& sm) = 0;

        /** @brief Standard logic update for physics and simulation events. */
        virtual void OnUpdate(float dt, float totalTime) = 0;

        /** @brief Fulfills Requirement: Ability to unload/teardown scenarios. */
        virtual void OnUnload() = 0;

        // Lab Requirements: Simulation Controls
        bool  IsPaused()   const { return m_isPaused; }
        void  SetPaused(bool p) { m_isPaused = p; }
        float GetTimeScale() const { return m_timeScale; }
        void  SetTimeScale(float s) { m_timeScale = s; }

    protected:
        bool  m_isPaused = false;
        float m_timeScale = 1.0f;
    };
}