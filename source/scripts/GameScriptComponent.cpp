#include "scripts/GameScriptComponent.h"
#include "core/ServiceLocator.h"
#include "components/Transform.h"
#include "components/Tag.h"
#include "ecs/EntityManager.h"

/* parasoft-begin-suppress ALL */
#include "imgui.h"
#include <glm/glm.hpp>
/* parasoft-end-suppress ALL */

namespace GE::Scripts {

    // -------------------------------------------------------------------------
    // Inspector base implementation
    // -------------------------------------------------------------------------

    void GameScriptComponent::OnDrawInspector() {
        ImGui::Checkbox("Active", &m_active);
    }

    // -------------------------------------------------------------------------
    // DrawField overloads — each delegates to the appropriate ImGui widget
    // -------------------------------------------------------------------------

    void GameScriptComponent::DrawField(const char* label, bool& value) const {
        ImGui::Checkbox(label, &value);
    }

    void GameScriptComponent::DrawField(const char* label, int& value) const {
        ImGui::DragInt(label, &value);
    }

    void GameScriptComponent::DrawField(const char* label, float& value) const {
        ImGui::DragFloat(label, &value, 0.01f);
    }

    void GameScriptComponent::DrawField(const char* label, glm::vec2& value) const {
        ImGui::DragFloat2(label, &value.x, 0.01f);
    }

    void GameScriptComponent::DrawField(const char* label, glm::vec3& value) const {
        ImGui::DragFloat3(label, &value.x, 0.01f);
    }

    void GameScriptComponent::DrawField(const char* label, glm::vec4& value) const {
        ImGui::DragFloat4(label, &value.x, 0.01f);
    }

    void GameScriptComponent::DrawField(const char* label, const std::string& value) const {
        ImGui::Text("%s: %s", label, value.c_str());
    }

    // -------------------------------------------------------------------------
    // Convenience helpers
    // -------------------------------------------------------------------------

    GE::Components::Transform* GameScriptComponent::GetTransform() const {
        auto* em = ServiceLocator::GetEntityManager();
        auto* trans = em->TryGetTIComponent<GE::Components::Transform>(m_entityID);
        if (!trans) {
            GE_LOG_FATAL("GameScriptComponent::GetTransform() — entity "
                + std::to_string(m_entityID) + " has no Transform component.");
        }
        return trans;
    }

    InputService* GameScriptComponent::GetInput() const {
        return ServiceLocator::GetInput();
    }

    GE::ECS::EntityManager* GameScriptComponent::GetEntityManager() const {
        return ServiceLocator::GetEntityManager();
    }

    const std::string& GameScriptComponent::GetName() const {
        auto* em  = ServiceLocator::GetEntityManager();
        auto* tag = em->TryGetTIComponent<GE::Components::Tag>(m_entityID);
        if (tag) return tag->m_name;
        m_fallbackName = "(entity " + std::to_string(m_entityID) + ")";
        return m_fallbackName;
    }

} // namespace GE::Scripts
