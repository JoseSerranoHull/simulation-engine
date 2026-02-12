#include "../include/EntityManager.h"

#include "../include/EntityFactory.h"
#include "../include/MemoryUtilities.h"

namespace GE::ECS
{
    EntityManager::EntityManager() {}
    EntityManager::~EntityManager() = default;
    ERROR_CODE EntityManager::Initialize(uint32_t maxEntities, uint32_t maxComponentTypes)
    {
    	GE_CHECK_STATE_INIT(m_state, "Entity manager is already initialized");
		m_state = SystemState::Initializing;

        m_maxEntities       = maxEntities;
        m_maxComponentTypes = maxComponentTypes;

        m_allComponentIndices.assign(maxEntities * maxComponentTypes, UINT32_MAX);
        m_componentArrays.clear();
        m_componentArrays.resize(maxComponentTypes);

        while (!m_freeEntities.empty())
            m_freeEntities.pop();
        for (EntityID i = 0; i < maxEntities; ++i)
            m_freeEntities.push(maxEntities - 1 - i);

    	ERROR_CODE result = ERROR_CODE::OK;
		GE_CHECK(result, Scene::EntityFactory::Initialize(this));

        m_state = SystemState::Running;
        return result;
    }

	// Currently not used
    // void EntityManager::Update(const float dt)
    // {
    //     for (auto const &stageVec : m_systems)
    //         for (auto &sys : stageVec)
    //             sys->OnUpdate(dt);
    // }

    ERROR_CODE EntityManager::Shutdown()
    {
    	if (m_state == SystemState::Uninitialized || m_state == SystemState::ShuttingDown) return ERROR_CODE::OK;
		m_state = SystemState::ShuttingDown;

        for (auto const& stageVec : m_systems)
            for (const auto &sys : stageVec)
                sys->Shutdown();

        m_allComponentIndices.clear();
        m_componentArrays.clear();
		Scene::EntityFactory::Shutdown();
        m_state = SystemState::Uninitialized;
        return ERROR_CODE::OK;
    }

    EntityID EntityManager::CreateEntity()
    {
        if (m_freeEntities.empty())
        {
            GE_LOG_ERROR("There isn't any free entity.");
            return UINT32_MAX;
        }

        const auto id = m_freeEntities.top();
        m_freeEntities.pop();


        for (uint32_t typeID = 0; typeID < m_maxComponentTypes; ++typeID)
            m_allComponentIndices[typeID * m_maxEntities + id] = UINT32_MAX;

        return id;
    }

    ERROR_CODE EntityManager::DestroyEntity(EntityID id)
    {
        if (id >= m_maxEntities)
        {
            GE_LOG_FATAL("Entity ID isn't correct.");
            return ERROR_CODE::WRONG_ENTITY_ID;
        }


        for (uint32_t typeID = 0; typeID < m_maxComponentTypes; ++typeID)
        {
            if (const uint32_t idx = m_allComponentIndices[typeID * m_maxEntities + id];
                idx != UINT32_MAX && m_componentArrays[typeID]->Has(idx))
            {
                m_componentArrays[typeID]->Remove(idx);
                m_allComponentIndices[typeID * m_maxEntities + id] = UINT32_MAX;
            }
        }

        m_freeEntities.push(id);
        return ERROR_CODE::OK;
    }

	void EntityManager::ClearAllEntities()
    {
    	GE_LOG_INFO("EntityManager: Clearing all entities and components...");

    	for (uint32_t typeID = 0; typeID < m_maxComponentTypes; ++typeID)
    	{
    		if (m_componentArrays[typeID])
    		{
    			m_componentArrays[typeID]->Clear();
    		}
    	}

    	std::fill(m_allComponentIndices.begin(), m_allComponentIndices.end(), UINT32_MAX);

    	while (!m_freeEntities.empty())
    		m_freeEntities.pop();

    	for (EntityID i = 0; i < m_maxEntities; ++i)
    	{
    		m_freeEntities.push(m_maxEntities - 1 - i);
    	}

    	GE_LOG_INFO("EntityManager: All entities cleared.");
    }

    ERROR_CODE EntityManager::RegisterSystem(IECSystem *system)
    {
        ESystemStage stage = system->GetStage();

        if (stage >= ESystemStage::Count)
        {
            GE_LOG_FATAL("Invalid system stage.");
            return ERROR_CODE::SYSTEM_INVALID_STAGE;
        }

        for (const auto &existing : m_systems[static_cast<size_t>(stage)])
        {
            if (existing->GetID() == system->GetID())
            {
                GE_LOG_FATAL("System already registered in this stage");
                return ERROR_CODE::SYSTEM_ALREADY_REGISTERED;
            }
        }
        m_systems[static_cast<size_t>(stage)].emplace_back(system);

        return ERROR_CODE::OK;
    }

    ERROR_CODE EntityManager::UnregisterSystem(const IECSystem *system)
    {
        ESystemStage stage = system->GetStage();

        if (stage >= ESystemStage::Count)
        {
            GE_LOG_FATAL("Invalid system stage.");
            return ERROR_CODE::SYSTEM_INVALID_STAGE;
        }

        for (auto it = m_systems[static_cast<size_t>(stage)].begin(); it != m_systems[static_cast<size_t>(stage)].end();
             ++it)
        {
            if ((*it)->GetID() == system->GetID())
            {
                m_systems[static_cast<size_t>(stage)].erase(it);
                return ERROR_CODE::OK;
            }
        }

        GE_LOG_FATAL("System not found in its stage");
        return ERROR_CODE::SYSTEM_NOT_REGISTERED;
    }
}
