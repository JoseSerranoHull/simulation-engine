#pragma once

/* parasoft-begin-suppress ALL */
#include <cstdint>
#include <glm/fwd.hpp>
#include <glm/vec3.hpp>
/* parasoft-end-suppress ALL */

namespace GE::Scene::Components {
	struct Transform {
		enum class TransformState : uint8_t {
			Clean,
			Dirty,
			Updated
		};

		glm::vec3 m_position{ 0.0f, 0.0f, 0.0f };
		glm::vec3 m_rotation{ 0.0f, 0.0f, 0.0f };
		glm::vec3 m_scale{ 1.0f, 1.0f, 1.0f };

		glm::mat4  m_localMatrix{ glm::mat4(1.0f) };
		glm::mat4  m_worldMatrix{ glm::mat4(1.0f) };

		uint32_t m_parentEntityID = UINT32_MAX;
		uint32_t m_parentPackedIndex = UINT32_MAX;

		TransformState m_state = TransformState::Clean;
	};
}