#pragma once

#include <cstdint>

namespace GE::ECS {

    /** @brief Unsigned integer handle uniquely identifying an entity. */
    using EntityID = uint32_t;

    /** @brief Sentinel value indicating an unassigned or invalid entity. */
    constexpr uint32_t INVALID_ENTITY_ID = UINT32_MAX;

    /**
     * @struct Entity
     * @brief Lightweight handle carrying an entity's numeric ID.
     * EntityManager is the authoritative source of truth for which IDs are live;
     * Entity itself performs no lifetime management.
     */
    struct Entity {
        Entity();
        ~Entity();

        /** @brief Assigns m_id; returns true on success. */
        bool Initialize(uint32_t id);

        /** @brief Resets m_id to INVALID_ENTITY_ID. */
        void Shutdown();

        uint32_t m_id{UINT32_MAX}; ///< Numeric entity handle; UINT32_MAX = unassigned.
    };
}
