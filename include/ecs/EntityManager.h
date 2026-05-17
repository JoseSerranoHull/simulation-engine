#pragma once

/* parasoft-begin-suppress ALL */
#include <array>
#include <cstdint>
#include <memory>
#include <stack>
#include <vector>
/* parasoft-end-suppress ALL */

#include "ecs/ComponentArray.h"
#include "ecs/ComponentType.h"
#include "ecs/Entity.h"
#include "ecs/IECSystem.h"

namespace GE::ECS {

    /**
     * @class EntityManager
     * @brief Central ECS authority: owns entity IDs, component arrays, and system dispatch.
     *
     * Layout: a flat index table [typeID * maxEntities + entityID] maps every
     * (component type, entity) pair to a packed-array slot index. This gives O(1)
     * lookup, add, and remove without hash maps.
     *
     * Threading: Initialize(), Shutdown(), and entity/component mutations are
     * NOT thread-safe. UpdateCpuStages() is called from the physics jthread;
     * UpdateGpuStages() is called from the main (graphics) thread. Do not mutate
     * the entity set while either is running.
     */
	class EntityManager {
	public:
		EntityManager();
		EntityManager(const EntityManager &) = delete;
		EntityManager &operator=(const EntityManager &) = delete;
		EntityManager(EntityManager &&) = delete;
		EntityManager &operator=(EntityManager &&) = delete;
		~EntityManager();

		/** @brief Allocates the entity pool and component array table. Call once at startup. */
		void Initialize(uint32_t maxEntities, uint32_t maxComponentTypes);

		/** @brief Dispatches all registered systems in stage order. Legacy full-frame entry point. */
		void Update(float dt, VkCommandBuffer cb);

		/** @brief Shuts down all systems and clears component arrays. */
		void Shutdown();

		/** @brief Allocates a recycled entity ID; returns INVALID_ENTITY_ID if the pool is exhausted. */
		EntityID CreateEntity();

		/** @brief Removes all components from the entity and returns its ID to the pool. */
		void DestroyEntity(EntityID id);

		/** @brief Resets the entity pool and clears all component arrays (scene reload). */
		void ClearAllEntities();

		/** @brief Registers a ComponentArray<TIComponent>; asserts if already registered. */
		template <typename TIComponent>
		void RegisterComponent(uint32_t componentCount = 0);

		/** @brief Shuts down and removes the ComponentArray<TIComponent>. */
		template <typename TIComponent>
		void UnregisterComponent();

		/** @brief Copies component into the array for entityID; asserts on duplicate or invalid ID. */
		template <typename TIComponent>
		void AddComponent(const EntityID entityID, const TIComponent &component);

		/** @brief Removes TIComponent from entityID via swap-and-pop. */
		template <typename TIComponent>
		void RemoveComponent(const EntityID entityID);

		/** @brief Returns true if entityID currently has a TIComponent instance. */
		template <typename TIComponent>
		[[nodiscard]] bool HasComponent(const EntityID entityID) const;

		/** @brief Returns a pointer to entityID's TIComponent; asserts if not present. */
		template <typename TIComponent>
		TIComponent *GetTIComponent(const EntityID entityID);

		/** @brief Returns a pointer to entityID's TIComponent, or nullptr if not present. */
		template <class TIComponent>
		TIComponent *TryGetTIComponent(EntityID entityID);

		/** @brief Convenience variadic getter; returns a tuple of pointers for each requested type. */
		template <typename... TIComponents>
		std::tuple<TIComponents *...> GetTIComponents(const EntityID entityID);

		/** @brief Appends system to its declared stage's dispatch list. Asserts on duplicate. */
		void RegisterSystem(IECSystem *system);

		/** @brief Removes system from its stage's dispatch list. Asserts if not found. */
		void UnregisterSystem(const IECSystem *system);

		/** @brief Removes and deletes the system with the given type ID (EntityManager owns it). */
		void UnregisterSystemByID(ISystemTypeID systemID);

		/** @brief Runs CPU stages (EarlyUpdate→Camera); safe to call from the physics jthread. */
		void UpdateCpuStages(float dt);

		/** @brief Runs GPU stages (Particle→LateUpdate); must be called from the main thread. */
		void UpdateGpuStages(float dt, VkCommandBuffer cb);

		/** @brief Returns the typed ComponentArray<T> for direct iteration in systems. */
		template <class T>
		ComponentArray<T> &GetCompArr();

	private:
		SystemState m_state{SystemState::Uninitialized};
		uint32_t    m_maxEntities{0};
		uint32_t    m_maxComponentTypes{0};

		std::stack<EntityID> m_freeEntities; ///< Recycled IDs available for CreateEntity().

		/// Flat index: [typeID * m_maxEntities + entityID] → packed array slot, or UINT32_MAX.
		std::vector<uint32_t>                         m_allComponentIndices;
		std::vector<std::unique_ptr<IComponentArray>> m_componentArrays; ///< One per registered component type.

		/// Systems grouped by stage; iterated in order 0..Count-1 each frame.
		std::array<std::vector<IECSystem *>, static_cast<size_t>(ESystemStage::Count)> m_systems;
	};

	// Template implementations
	template <typename T>
	ComponentArray<T> &EntityManager::GetCompArr() {
		const uint32_t typeID = ComponentType<T>::ID();
		return *static_cast<ComponentArray<T> *>(m_componentArrays[typeID].get());
	}

	template <typename TIComponent>
	void EntityManager::RegisterComponent(uint32_t componentCount) {
		const uint32_t typeID = ComponentType<TIComponent>::ID();
		if (typeID >= m_maxComponentTypes) {
			GE_LOG_ERROR("Too many component types. Consider increasing m_maxComponentTypes.");
			return;
		}

		if (m_componentArrays[typeID]) {
			GE_LOG_FATAL("Component is already registered!");
			return;
		}

		auto array = std::make_unique<ComponentArray<TIComponent>>();
		array->Initialize(componentCount == 0 ? m_maxEntities : componentCount);
		m_componentArrays[typeID] = std::move(array);
	}

	template <typename TIComponent>
	void EntityManager::UnregisterComponent() {
		const uint32_t typeID = ComponentType<TIComponent>::ID();
		if (typeID >= m_maxComponentTypes) {
			GE_LOG_FATAL("Invalid component type");
			return;
		}

		if (!m_componentArrays[typeID]) {
			GE_LOG_FATAL("Component not registered");
			return;
		}

		m_componentArrays[typeID]->Shutdown();
		m_componentArrays[typeID] = nullptr;
	}

	template <typename TIComponent>
	void EntityManager::AddComponent(EntityID entityID, const TIComponent &component) {
		if (entityID >= m_maxEntities) {
			GE_LOG_FATAL("Wrong entity ID.");
			return;
		}

		const uint32_t typeID = ComponentType<TIComponent>::ID();
		auto &         array  = static_cast<ComponentArray<TIComponent> &>(*m_componentArrays[typeID]);
		const uint32_t idx    = array.Add(entityID, &component);

		m_allComponentIndices[typeID * m_maxEntities + entityID] = idx;
	}

	template <typename TIComponent>
	void EntityManager::RemoveComponent(const EntityID entityID) {
		if (entityID >= m_maxEntities) {
			GE_LOG_FATAL("Wrong entity ID.");
			return;
		}

		const uint32_t typeID = ComponentType<TIComponent>::ID();
		const int32_t  idx    = m_allComponentIndices[typeID * m_maxEntities + entityID];

		if (idx == UINT32_MAX) {
			GE_LOG_WARN("Component ID is default value.");
			return;
		}

		const auto [movedSlot, newPackedIdx] = m_componentArrays[typeID]->Remove(idx);

		// Clear the removed entity's mapping
		m_allComponentIndices[typeID * m_maxEntities + entityID] = UINT32_MAX;

		// Update the mapping for the moved entity — only when a different entity was moved
		// (movedSlot == entityID when removing the last element; in that case there is
		// nothing to update and we must NOT overwrite the UINT32_MAX we just wrote above).
		// Store the moved entity's own ID so that Get(entityID) → m_reverse[entityID] works.
		if (movedSlot != entityID) {
			m_allComponentIndices[typeID * m_maxEntities + movedSlot] = movedSlot;
		}
	}

	template <typename TIComponent>
	TIComponent *EntityManager::GetTIComponent(const EntityID entityID) {
		if (entityID >= m_maxEntities) {
			GE_LOG_FATAL("Wrong entity ID.");
			return nullptr;
		}

		const uint32_t typeID = ComponentType<TIComponent>::ID();
		const uint32_t idx    = m_allComponentIndices[typeID * m_maxEntities + entityID];

		if (idx == UINT32_MAX) {
			GE_LOG_FATAL("Entity doesn't have the component.");
			return nullptr;
		}

		IComponentArray *arrBase = m_componentArrays[typeID].get();

		if (!arrBase) {
			GE_LOG_FATAL("Component array pointer is null, but index map was valid!");
			return nullptr;
		}

		if (!arrBase->Has(idx)) {
			GE_LOG_FATAL("Entity idx exists, but array::Has(idx) returned false.");
			return nullptr;
		}

		auto componentArray = static_cast<ComponentArray<TIComponent> *>(arrBase);
		return &(componentArray->Get(idx));
	}

	template <typename TIComponent>
	TIComponent *EntityManager::TryGetTIComponent(EntityID entityID) {
		if (HasComponent<TIComponent>(entityID))
			return GetTIComponent<TIComponent>(entityID);
		else {
			GE_LOG_TRACE("Entity does not have the component.");
			return nullptr;
		}
	}

	template <typename... TIComponents>
	std::tuple<TIComponents *...> EntityManager::GetTIComponents(EntityID entityID) {
		return std::tuple<TIComponents *...>{GetTIComponent<TIComponents>(entityID)...};
	}

	template <typename TIComponent>
	bool EntityManager::HasComponent(const EntityID entityID) const {
		if (entityID >= m_maxEntities) {
			GE_LOG_WARN("Wrong entity ID.");
			return false;
		}

		const uint32_t typeID = ComponentType<TIComponent>::ID();
		return m_allComponentIndices[typeID * m_maxEntities + entityID] != UINT32_MAX;
	}
}
