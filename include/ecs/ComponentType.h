#pragma once
#include <atomic>
#include <cstdint>

namespace GE::ECS {
// --------------------------------------------------------
// INTERNAL: Implementation Details (Do not use directly)
// --------------------------------------------------------
namespace Internal {
inline uint32_t FetchNextComponentID() {

	static std::atomic<uint32_t> nextID{0};
	return nextID.fetch_add(1, std::memory_order_relaxed);
}
}

// Usage: ComponentType<Transform>::ID()
template <typename T>
class ComponentType {
public:
	static uint32_t ID() {
		static const uint32_t id = Internal::FetchNextComponentID();
		return id;
	}
};
}