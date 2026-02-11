#pragma once
#include "EntityManager.h"
#include "AssetManager.h"

namespace GE::Scene {

class EntityFactory {
public:
	EntityFactory() = delete;

	static ERROR_CODE Initialize(ECS::EntityManager *entityManager);
	static void Shutdown();
	[[nodiscard]] static ECS::EntityID CreatePrimitive(Graphics::PrimitiveType type);

private:
	static inline ECS::EntityManager* m_entityManager = nullptr;
	static inline bool m_isInitialized = false;
};

}