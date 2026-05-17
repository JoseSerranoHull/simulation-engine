#pragma once
#include <atomic>
#include <cstdint>

namespace GE::ECS {

    namespace Internal {
        /** @brief Thread-safe monotonic counter; each call returns a unique uint32_t. */
        inline uint32_t FetchNextComponentID() {
            static std::atomic<uint32_t> nextID{0};
            return nextID.fetch_add(1, std::memory_order_relaxed);
        }
    }

    /**
     * @class ComponentType
     * @brief Provides a unique, stable compile-time-like ID for each component type T.
     *
     * The ID is generated lazily on first call to ID() and then cached as a static
     * local — safe across translation units because the atomic counter in
     * Internal::FetchNextComponentID() is process-global.
     *
     * @tparam T The component struct whose ID is being queried.
     */
    template <typename T>
    class ComponentType {
    public:
        /** @brief Returns the unique integer ID for component type T. Thread-safe. */
        static uint32_t ID() {
            static const uint32_t id = Internal::FetchNextComponentID();
            return id;
        }
    };
}
