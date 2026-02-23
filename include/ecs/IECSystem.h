#pragma once

#include <cstdint>
#include "core/Common.h"
#include "core/Logger.h"

namespace GE::ECS {
	enum class ESystemStage {
		EarlyUpdate = 0,
		Transform,
		Animation,
		Physics,
		SceneControl,
		DayNight,
		GameLogic, // For user scripts
		Camera,
		GUI,
		Particle,
		Render,
		LateUpdate,
		Count // Number of stages
	};

	using ISystemTypeID = uint32_t;

	ISystemTypeID GenerateISystemTypeID();

	struct IECSystem {
		virtual ~IECSystem();
		virtual void OnUpdate(float dt, VkCommandBuffer cb) = 0;
		virtual ERROR_CODE Shutdown() = 0;
		ISystemTypeID GetID() const;
		ESystemStage GetStage() const;
		template <typename TSystem>
		static ISystemTypeID GetUniqueISystemTypeID();

	protected:
		ISystemTypeID m_typeID = UINT32_MAX;
		ESystemStage m_stage = ESystemStage::Count;
		SystemState m_state = SystemState::Uninitialized;
	};

	inline ISystemTypeID IECSystem::GetID() const {
		if (m_typeID != UINT32_MAX)
			return m_typeID;

		GE_LOG_FATAL("System type ID is invalid.");
		return UINT32_MAX;
	}

	inline ESystemStage IECSystem::GetStage() const {
		if (m_stage != ESystemStage::Count)
			return m_stage;

		GE_LOG_FATAL("System stage is invalid");
		return ESystemStage::Count;
	}

	template <typename TISystem>
	ISystemTypeID IECSystem::GetUniqueISystemTypeID() {
		static_assert(std::is_base_of_v<IECSystem, TISystem>, "TISystem must inherit from IECSystem");
		static const uint32_t typeID = GenerateISystemTypeID();
		return typeID;
	}

	inline ISystemTypeID GenerateISystemTypeID() {
		static std::atomic<uint32_t> lastID{0};
		return lastID.fetch_add(1, std::memory_order_relaxed);
	}

	// Provide a definition for the virtual destructor to satisfy the linker.
	inline IECSystem::~IECSystem() = default;

	/**
	 * @struct ICpuSystem
	 * @brief ISP sub-interface for CPU-only ECS systems (no GPU work).
	 * Intercepts the 2-param dispatch from EntityManager and forwards to the
	 * clean 1-param OnUpdate(float dt), keeping VkCommandBuffer out of CPU code.
	 */
	struct ICpuSystem : IECSystem {
		/** @brief EntityManager calls this; strips the unused command buffer. */
		void OnUpdate(float dt, VkCommandBuffer /*cb*/) final { OnUpdate(dt); }
		/** @brief CPU systems implement this instead. */
		virtual void OnUpdate(float dt) = 0;
	};

	/**
	 * @struct IGpuSystem
	 * @brief ISP sub-interface for GPU-dispatch ECS systems.
	 * Subclasses override OnUpdate(float dt, VkCommandBuffer cb) directly,
	 * making it explicit that they perform GPU work each frame.
	 */
	struct IGpuSystem : IECSystem {};
}