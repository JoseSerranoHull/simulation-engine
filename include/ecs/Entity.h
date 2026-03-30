#pragma once

/* parasoft-begin-suppress ALL */
#include <cstdint>
/* parasoft-end-suppress ALL */

namespace GE::ECS {
	using EntityID = uint32_t;

	constexpr uint32_t INVALID_ENTITY_ID = UINT32_MAX;

	struct Entity {
		Entity();
		~Entity();

		bool Initialize(uint32_t);
		void Shutdown();

		uint32_t m_id;
	};
}