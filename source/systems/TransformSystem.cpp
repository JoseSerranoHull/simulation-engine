#include "systems/TransformSystem.h"
#include "core/ServiceLocator.h"
#include "components/Transform.h"

/* parasoft-begin-suppress ALL */
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
/* parasoft-end-suppress ALL */

namespace GE::Systems {

void TransformSystem::OnUpdate(float dt) {
    auto* em = ServiceLocator::GetEntityManager();
    auto& transformArray = em->GetCompArr<GE::Components::Transform>();

    // Pass 1: Rebuild local matrices for Dirty entities (non-physics; physics sets Clean directly)
    for (uint32_t i = 0; i < transformArray.GetCount(); ++i) {
        auto& trans = transformArray.Data()[i];
        if (trans.m_state == GE::Components::Transform::TransformState::Dirty) {
            trans.m_localMatrix = calculateLocalMatrix(trans);
        }
    }

    // Pass 2: Hierarchical world matrix resolution
    for (uint32_t i = 0; i < transformArray.GetCount(); ++i) {
        auto id = transformArray.Index()[i];
        auto& trans = transformArray.Data()[i];
        resolveWorldMatrix(id, trans, em);
    }

    // Pass 3: Extract world-space decomposed values from worldMatrix
    for (uint32_t i = 0; i < transformArray.GetCount(); ++i) {
        auto& trans = transformArray.Data()[i];

        trans.m_worldPosition = glm::vec3(trans.m_worldMatrix[3]);

        trans.m_worldScale = {
            glm::length(glm::vec3(trans.m_worldMatrix[0])),
            glm::length(glm::vec3(trans.m_worldMatrix[1])),
            glm::length(glm::vec3(trans.m_worldMatrix[2]))
        };

        // YXZ Euler extraction matching calculateLocalMatrix rotation order (display-only)
        const glm::vec3 invScale = 1.0f / glm::max(trans.m_worldScale, glm::vec3(1e-6f));
        const glm::mat3 rotMat {
            glm::vec3(trans.m_worldMatrix[0]) * invScale.x,
            glm::vec3(trans.m_worldMatrix[1]) * invScale.y,
            glm::vec3(trans.m_worldMatrix[2]) * invScale.z
        };
        trans.m_worldRotation.x = glm::degrees(std::asin(-rotMat[1][2]));
        trans.m_worldRotation.y = glm::degrees(std::atan2(rotMat[0][2], rotMat[2][2]));
        trans.m_worldRotation.z = glm::degrees(std::atan2(rotMat[1][0], rotMat[1][1]));
    }
}

glm::mat4 TransformSystem::calculateLocalMatrix(const GE::Components::Transform& trans) {
    glm::mat4 m = glm::translate(glm::mat4(1.0f), trans.m_localPosition);

    // Rotation YXZ order
    m = glm::rotate(m, glm::radians(trans.m_localRotation.y), { 0, 1, 0 });
    m = glm::rotate(m, glm::radians(trans.m_localRotation.x), { 1, 0, 0 });
    m = glm::rotate(m, glm::radians(trans.m_localRotation.z), { 0, 0, 1 });

    m = glm::scale(m, trans.m_localScale);
    return m;
}

void TransformSystem::resolveWorldMatrix(uint32_t id, GE::Components::Transform& trans, GE::ECS::EntityManager* em) {
    if (trans.m_parentEntityID == UINT32_MAX) {
        trans.m_worldMatrix = trans.m_localMatrix;
    } else {
        auto* parentTrans = em->GetTIComponent<GE::Components::Transform>(trans.m_parentEntityID);
        if (parentTrans) {
            trans.m_worldMatrix = parentTrans->m_worldMatrix * trans.m_localMatrix;
        } else {
            trans.m_worldMatrix = trans.m_localMatrix;
        }
    }

    trans.m_state = GE::Components::Transform::TransformState::Clean;
}

} // namespace GE::Systems
