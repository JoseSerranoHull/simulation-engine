#include "Entity.h"

namespace GE::ECS
{
    Entity::Entity() : m_id(UINT32_MAX) {}
    Entity::~Entity() {}

    bool Entity::Initialize(const uint32_t id)
    {
        m_id = id;
        return true;
    }

    void Entity::Shutdown()
    {
        m_id = UINT32_MAX; // UINT32_MAX is reserved id for null
    }
}
