#include "systems/TransformSystem.h"
#include "core/ServiceLocator.h"
#include "components/Transform.h"
#include <glm/gtc/matrix_transform.hpp>

namespace GE::Systems {

    void TransformSystem::OnUpdate(float dt) {
        auto* em = ServiceLocator::GetEntityManager();
        auto& transformArray = em->GetCompArr<GE::Components::Transform>();

        // 1. First Pass: Update all Local Matrices
        for (uint32_t i = 0; i < transformArray.GetCount(); ++i) {
            auto& trans = transformArray.Data()[i];

            // Only recalculate local matrix if the entity itself changed (Agnostic Logic)
            if (trans.m_state == GE::Components::Transform::TransformState::Dirty) {
                trans.m_localMatrix = calculateLocalMatrix(trans);
            }
        }

        // 2. Second Pass: Hierarchical World Matrix Resolution
        // In a production engine, this would use a sorted list or tree traversal.
        // For simplicity, we ensure parents are resolved before children.
        for (uint32_t i = 0; i < transformArray.GetCount(); ++i) {
            auto id = transformArray.Index()[i];
            auto& trans = transformArray.Data()[i];

            resolveWorldMatrix(id, trans, em);
        }
    }

    glm::mat4 TransformSystem::calculateLocalMatrix(const GE::Components::Transform& trans) {
        glm::mat4 m = glm::translate(glm::mat4(1.0f), trans.m_position);

        // Rotation (Y * X * Z order)
        m = glm::rotate(m, glm::radians(trans.m_rotation.y), { 0, 1, 0 });
        m = glm::rotate(m, glm::radians(trans.m_rotation.x), { 1, 0, 0 });
        m = glm::rotate(m, glm::radians(trans.m_rotation.z), { 0, 0, 1 });

        m = glm::scale(m, trans.m_scale);
        return m;
    }

    void TransformSystem::resolveWorldMatrix(uint32_t id, GE::Components::Transform& trans, GE::ECS::EntityManager* em) {
        // If it's a root entity (no parent)
        if (trans.m_parentEntityID == UINT32_MAX) {
            trans.m_worldMatrix = trans.m_localMatrix;
        }
        else {
            // Retrieve parent's transform
            auto* parentTrans = em->GetTIComponent<GE::Components::Transform>(trans.m_parentEntityID);
            if (parentTrans) {
                // GameObject Logic: Child is relative to Parent
                trans.m_worldMatrix = parentTrans->m_worldMatrix * trans.m_localMatrix;
            }
            else {
                trans.m_worldMatrix = trans.m_localMatrix;
            }
        }

        // Mark as "Clean" or "Updated" so systems like Physics or Renderer can use it
        trans.m_state = GE::Components::Transform::TransformState::Clean;
    }
}