#include "scripts/PlayerController.h"
#include "core/ServiceLocator.h"
#include "core/NetworkBridge.h"
#include "components/PhysicsComponents.h"
#include "ecs/EntityManager.h"
#include "services/InputService.h"

/* parasoft-begin-suppress ALL */
#include <glm/glm.hpp>
/* parasoft-end-suppress ALL */

namespace GE::Scripts {

void PlayerController::Update(float dt) {
    // Step 1: Ownership check — only control the entity owned by the local peer.
    GE::NetworkBridge* bridge = ServiceLocator::GetNetworkBridge();
    if (bridge == nullptr) { return; }

    const uint8_t localPeerId = bridge->GetLocalPeerId();
    if (localPeerId < 1U || localPeerId > 4U) { return; }
    const auto localOwner = static_cast<GE::Components::OwnerType>(localPeerId - 1U);

    GE::ECS::EntityManager* em = GetEntityManager();
    if (em == nullptr) { return; }

    auto* oc = em->TryGetTIComponent<GE::Components::OwnerComponent>(m_entityID);
    if (oc == nullptr || oc->owner != localOwner) { return; }

    auto* rb = em->TryGetTIComponent<GE::Components::RigidBody>(m_entityID);
    if (rb == nullptr || rb->isStatic) { return; }

    // Step 2: Read horizontal input (virtual — subclasses may override).
    const glm::vec2 move = ReadMovementInput();
    const bool anyInput  = (move.x != 0.0f || move.y != 0.0f);

    // Step 3: Apply horizontal velocity or damping (Y is left to gravity).
    if (anyInput) {
        rb->velocity.x = move.x;
        rb->velocity.z = move.y;
    } else {
        rb->velocity.x *= m_hDamping;
        rb->velocity.z *= m_hDamping;
    }

    // Step 4: Jump impulse (virtual — RacerPlayerController returns false).
    if (ReadJumpInput() && glm::abs(rb->velocity.y) < 0.5f) {
        rb->velocity.y = m_jumpImpulse;
    }

    // Step 5: Extension point for subclass per-frame actions (shoot, boost, etc.).
    OnPostUpdate(dt);
}

glm::vec2 PlayerController::ReadMovementInput() const {
    InputService* input = GetInput();
    if (input == nullptr) { return { 0.0f, 0.0f }; }

    glm::vec2 v{ 0.0f };
    if (input->IsKeyDown(GLFW_KEY_UP))    { v.y -= m_moveSpeed; }
    if (input->IsKeyDown(GLFW_KEY_DOWN))  { v.y += m_moveSpeed; }
    if (input->IsKeyDown(GLFW_KEY_LEFT))  { v.x -= m_moveSpeed; }
    if (input->IsKeyDown(GLFW_KEY_RIGHT)) { v.x += m_moveSpeed; }
    return v;
}

bool PlayerController::ReadJumpInput() const {
    InputService* input = GetInput();
    return (input != nullptr) && input->IsKeyDown(GLFW_KEY_SPACE);
}

void PlayerController::OnDrawInspector() {
    GameScriptComponent::OnDrawInspector();
    DrawField("Move Speed",   m_moveSpeed);
    DrawField("Jump Impulse", m_jumpImpulse);
    DrawField("H Damping",    m_hDamping);
}

} // namespace GE::Scripts
