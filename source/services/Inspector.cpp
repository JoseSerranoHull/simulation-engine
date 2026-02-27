#include "services/Inspector.h"

/**
 * @brief Draws the full inspector panel for the selected entity.
 */
void Inspector::Draw(const GE::ECS::EntityID entityID, GE::ECS::EntityManager* const em) const {
    using namespace GE::Components;

    // Entity name header
    auto* tag = em->TryGetTIComponent<Tag>(entityID);
    const char* name = (tag != nullptr) ? tag->m_name.c_str() : "Entity";
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.85f, 0.4f, 1.0f));
    ImGui::Text("%s  [ID: %u]", name, entityID);
    ImGui::PopStyleColor();
    ImGui::Separator();

    ImGui::PushItemWidth(200.0f);

    if (auto* t  = em->TryGetTIComponent<Transform>(entityID))       { DrawTransform(t); }
    if (auto* rb = em->TryGetTIComponent<RigidBody>(entityID))       { DrawRigidBody(rb); }
    if (auto* sc = em->TryGetTIComponent<SphereCollider>(entityID))  { DrawSphereCollider(sc); }
    if (auto* pc = em->TryGetTIComponent<PlaneCollider>(entityID))   { DrawPlaneCollider(pc); }

    ImGui::PopItemWidth();
}

void Inspector::DrawTransform(GE::Components::Transform* const t) const {
    if (!ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) return;

    if (ImGui::DragFloat3("Position", &t->m_position.x, 0.05f)) {
        t->m_state = GE::Components::Transform::TransformState::Dirty;
    }
    if (ImGui::DragFloat3("Rotation", &t->m_rotation.x, 0.5f)) {
        t->m_state = GE::Components::Transform::TransformState::Dirty;
    }
    if (ImGui::DragFloat3("Scale", &t->m_scale.x, 0.01f, 0.001f, 1000.0f)) {
        t->m_state = GE::Components::Transform::TransformState::Dirty;
    }
}

void Inspector::DrawRigidBody(GE::Components::RigidBody* const rb) const {
    if (!ImGui::CollapsingHeader("Rigid Body", ImGuiTreeNodeFlags_DefaultOpen)) return;

    ImGui::DragFloat3("Velocity",     &rb->velocity.x,     0.01f);
    ImGui::DragFloat3("Acceleration", &rb->acceleration.x, 0.01f);

    ImGui::Separator();

    ImGui::Checkbox("Is Static",   &rb->isStatic);
    ImGui::Checkbox("Use Gravity", &rb->useGravity);

    ImGui::Separator();

    if (ImGui::DragFloat("Mass", &rb->mass, 0.01f, 0.001f, 10000.0f)) {
        rb->inverseMass = (!rb->isStatic && rb->mass > 0.0f) ? (1.0f / rb->mass) : 0.0f;
    }
    ImGui::Text("Inverse Mass: %.5f", static_cast<double>(rb->inverseMass));

    ImGui::DragFloat("Restitution", &rb->restitution, 0.01f, 0.0f, 1.0f);
}

void Inspector::DrawSphereCollider(GE::Components::SphereCollider* const sc) const {
    if (!ImGui::CollapsingHeader("Sphere Collider", ImGuiTreeNodeFlags_DefaultOpen)) return;
    ImGui::DragFloat("Radius", &sc->radius, 0.01f, 0.001f, 1000.0f);
}

void Inspector::DrawPlaneCollider(GE::Components::PlaneCollider* const pc) const {
    if (!ImGui::CollapsingHeader("Plane Collider", ImGuiTreeNodeFlags_DefaultOpen)) return;
    ImGui::DragFloat3("Normal", &pc->normal.x, 0.01f, -1.0f, 1.0f);
    ImGui::DragFloat("Offset",  &pc->offset,   0.01f);
}
