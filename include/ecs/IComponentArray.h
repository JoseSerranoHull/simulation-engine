#pragma once

/* parasoft-begin-suppress ALL */
#include <cstdint>
/* parasoft-end-suppress ALL */

#include "core/Common.h"

namespace GE::ECS
{
    /**
     * @struct RemovalInfo
     * @brief Carries the indices produced by a swap-and-pop removal in ComponentArray.
     * The caller uses these to patch the flat index map in EntityManager.
     */
    struct RemovalInfo
    {
        uint32_t movedSlot; // < Entity ID that was in the last packed slot before removal.
        uint32_t newPackedIdx; // < Packed index where that entity's data now lives after the swap.
    };

    /**
     * @struct IComponentArray
     * @brief Type-erased interface for all ComponentArray<T> specialisations.
     * Allows EntityManager to store and manage heterogeneous component arrays
     * without knowing the concrete component type T at the call site.
     */
    struct IComponentArray
    {
        virtual ~IComponentArray() = default;

        /** @brief Reserves storage for up to maxCount component instances. Call once before Add(). */
        virtual void Initialize(uint32_t maxCount) = 0;

        /** @brief Releases all storage. Safe to call multiple times. */
        virtual void Shutdown() = 0;

        /** @brief Adds a component copy for entityId; returns the entityId on success. */
        virtual uint32_t Add(uint32_t entityId, void const *componentData) = 0;

        /** @brief Removes the component at componentIdx via swap-and-pop; returns side-effect indices. */
        virtual RemovalInfo Remove(uint32_t componentIdx) = 0;

        /** @brief Returns true if componentIdx maps to a live component instance. */
        [[nodiscard]] virtual bool Has(uint32_t componentIdx) const = 0;

        /** @brief Returns the number of active component instances in the packed array. */
        [[nodiscard]] virtual uint32_t GetCount() const = 0;

        /** @brief Resets all data to default-constructed values and clears the arrays. */
        virtual void Clear() = 0;
    };
}
