#pragma once

#include <cstdint>
#include "core/Common.h"

namespace GE::ECS {

    /**
     * @enum ESystemStage
     * @brief Ordered execution stages for ECS system dispatch within EntityManager::Update().
     * Systems registered at the same stage run in registration order.
     */
	enum class ESystemStage {
		EarlyUpdate = 0,
		Transform,
		Animation,
		Physics,
		SceneControl,
		DayNight,
		GameLogic, // < User scripts (ScriptSystem)
		Camera,
		GUI,
		Particle,
		Render,
		LateUpdate,
		Count      // < Sentinel — do not register systems at this stage.
	};

	using ISystemTypeID = uint32_t;

	/** @brief Internal counter; use GetUniqueISystemTypeID<T>() from user code. */
	ISystemTypeID GenerateISystemTypeID();

    /**
     * @struct IECSystem
     * @brief Abstract base for all ECS systems. Do NOT subclass this directly —
     * use ICpuSystem (CPU-only work) or IGpuSystem (GPU-dispatching work) instead.
     *
     * Each concrete system must set m_typeID, m_stage, and m_state in its constructor.
     * EntityManager dispatches OnUpdate() in stage order every frame.
     */
	struct IECSystem {
		virtual ~IECSystem();

		/** @brief Called every frame by EntityManager; GPU systems receive a live command buffer. */
		virtual void OnUpdate(float dt, VkCommandBuffer cb) = 0;

		/** @brief Releases system resources; called by EntityManager::Shutdown() or UnregisterSystem(). */
		virtual void Shutdown() = 0;

		/** @brief Returns the unique type ID assigned to this system instance. Asserts if unset. */
		ISystemTypeID GetID() const;

		/** @brief Returns the stage this system is registered in. Asserts if unset. */
		ESystemStage GetStage() const;

		/**
		 * @brief Returns a stable, per-type unique ID for TISystem (generated once via atomic counter).
		 * @tparam TISystem A concrete system type that inherits from IECSystem.
		 */
		template <typename TSystem>
		static ISystemTypeID GetUniqueISystemTypeID();

	protected:
		ISystemTypeID m_typeID{UINT32_MAX};          ///< Set by the concrete system constructor.
		ESystemStage m_stage{ESystemStage::Count};  ///< Stage slot in EntityManager dispatch order.
		SystemState m_state{SystemState::Uninitialized};
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