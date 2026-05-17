#pragma once

/* parasoft-begin-suppress ALL */
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>
/* parasoft-end-suppress ALL */

#include "ecs/IComponentArray.h"
#include "core/Common.h"

namespace GE::ECS
{
    using ComponentArrayID = uint32_t;

    /**
     * @class ComponentArray
     * @brief Packed SoA storage for a single component type; enables O(1) add/remove/lookup.
     *
     * Uses swap-and-pop removal to keep data contiguous. An indirect mapping pair
     * (m_index, m_reverse) allows both forward (packed→entityID) and reverse
     * (entityID→packed) lookups in O(1). EntityManager owns the RemovalInfo returned
     * by Remove() and uses it to patch its flat index table.
     *
     * @tparam T Component struct type stored by this array.
     */
    template <typename T>
    class ComponentArray : public IComponentArray
    {
    public:
        ComponentArray() = default;
        ~ComponentArray() override;

        /** @brief Reserves storage for up to maxCount instances. Must be called before Add(). */
        void Initialize(uint32_t maxCount) override;

        /** @brief Clears all storage and resets state. Safe to call more than once. */
        void Shutdown() override;

        /** @brief Copies componentData into the packed array for entityID; returns entityID. */
        uint32_t Add(uint32_t entityID, const void* componentData) override;

        /** @brief Removes the component for entityID via swap-and-pop; returns side-effect info. */
        RemovalInfo Remove(const uint32_t entityID) override;

        /** @brief Returns true if entityID has a live component instance. */
        [[nodiscard]] bool Has(uint32_t slot) const override;

        /** @brief Returns the component instance for entityID; asserts on invalid access. */
        T& Get(uint32_t entityID);

        /** @brief Const overload of Get(). */
        const T& Get(uint32_t entityID) const;

        /** @brief Grows m_reverse if entityID exceeds its current capacity. */
        void EnsureReverseCapacity(uint32_t entityID);

        /** @brief Direct access to packed data vector for iteration (e.g., in system OnUpdate). */
        std::vector<T>& Data() { return m_data; }

        /** @brief Maps packed index → entity ID; parallel with m_data. */
        std::vector<uint32_t>& Index() { return m_index; }

        /** @brief Maps entity ID → packed index (UINT32_MAX = absent). */
        std::vector<uint32_t>& Reverse() { return m_reverse; }

        /** @brief Const Index() overload. */
        [[nodiscard]] const std::vector<uint32_t>& Index() const { return m_index; }

        /** @brief Returns the number of active component instances. */
        [[nodiscard]] uint32_t GetCount() const override { return m_size; }

        /** @brief Resets all instances to default-constructed T and clears the arrays. */
        void Clear() override;

    private:
        std::vector<T>        m_data;                         ///< Packed component data in insertion order.
        std::vector<uint32_t> m_index;                        ///< [packed index] → entity ID.
        std::vector<uint32_t> m_reverse;                      ///< [entity ID] → packed index; UINT32_MAX = absent.
        SystemState           m_state{SystemState::Uninitialized};
        uint32_t              m_size{0};                      ///< Active element count.
    };

    template <typename T>
    ComponentArray<T>::~ComponentArray() = default;

    template <typename T>
    void ComponentArray<T>::Initialize(uint32_t maxCount)
    {
        if (m_state != SystemState::Uninitialized) { GE_LOG_FATAL("ComponentArray is already initialized."); return; }
        m_state = SystemState::Initializing;
        m_data.reserve(maxCount);
        m_index.reserve(maxCount);
        m_reverse.assign(maxCount, UINT32_MAX); // pre-fill as unknown
        m_size = 0;

        m_state = SystemState::Running;
    }

    template <typename T>
    void ComponentArray<T>::Shutdown()
    {
        if (m_state == SystemState::Uninitialized || m_state == SystemState::ShuttingDown)
            return;

        m_state = SystemState::ShuttingDown;
        m_data.clear();
        m_index.clear();
        m_reverse.clear();
        m_size = 0;

        m_state = SystemState::Uninitialized;
    }

    template <typename T>
    uint32_t ComponentArray<T>::Add(uint32_t entityID, const void* componentData)
    {
        EnsureReverseCapacity(entityID);
        if (m_reverse[entityID] != UINT32_MAX) GE_LOG_FATAL("Entity already in reverse.");

        uint32_t packedIndex = m_size;
        m_data.push_back(*static_cast<const T*>(componentData));
        m_index.push_back(entityID);
        m_reverse[entityID] = packedIndex;
        m_size++;
        return entityID;
    }

    template <typename T>
    RemovalInfo ComponentArray<T>::Remove(const uint32_t entityID)
    {
        if (entityID >= m_reverse.size()) GE_LOG_FATAL("Entity does not exist.");

        uint32_t packed = m_reverse[entityID];

        if (packed == UINT32_MAX || packed >= m_size) GE_LOG_FATAL("Entity does not exist.");

        uint32_t lastPacked = m_size - 1;
        const uint32_t lastEntity = m_index[lastPacked];

        // move last into 'packed' if not removing last
        if (packed != lastPacked)
        {
            m_data[packed] = std::move(m_data[lastPacked]);
            m_index[packed] = lastEntity;
            m_reverse[lastEntity] = packed;
        }

        m_data.pop_back();
        m_index.pop_back();

        m_reverse[entityID] = UINT32_MAX;
        m_size--;

        return {lastEntity, packed}; // lastEntity now at packed (or returned even if same)
    }

    template <typename T>
    bool ComponentArray<T>::Has(uint32_t entityID) const
    {
        return entityID < m_reverse.size() && m_reverse[entityID] != UINT32_MAX && m_reverse[entityID] < m_size;
    }

    template <typename T>
    T& ComponentArray<T>::Get(uint32_t entityID)
    {
    	if (entityID >= m_reverse.size())
	    	GE_LOG_FATAL("Entity ID can't bigger than reverse vector size!");

    	uint32_t packed = m_reverse[entityID];
        assert(packed != UINT32_MAX);
        assert(packed < m_size);
        return m_data[packed];
    }

    template <typename T>
    const T& ComponentArray<T>::Get(uint32_t entityID) const
    {
        assert(entityID < m_reverse.size());
        uint32_t packed = m_reverse[entityID];
        assert(packed != UINT32_MAX);
        assert(packed < m_size);
        return m_data[packed];
    }

    template <typename T>
    void ComponentArray<T>::EnsureReverseCapacity(uint32_t entityID)
    {
        if (entityID >= m_reverse.size())
        {
            size_t old = m_reverse.size();
            m_reverse.resize(entityID + 1, UINT32_MAX);
            GE_LOG_INFO("[ComponentArray] Resized reverse from " + std::to_string(old) + " to " + std::to_string(m_reverse.size()));
        }
    }

    template <typename T>
    void ComponentArray<T>::Clear() {
    	m_size = 0;
    	std::fill(m_data.begin(), m_data.end(), T());
    	std::fill(m_index.begin(), m_index.end(), UINT32_MAX);
    	std::fill(m_reverse.begin(), m_reverse.end(), UINT32_MAX);

		m_data.clear();
    	m_index.clear();
    	m_reverse.clear();
    }
}
