#pragma once
#include "scripts/GameScriptComponent.h"

/* parasoft-begin-suppress ALL */
#include <glm/glm.hpp>
/* parasoft-end-suppress ALL */

namespace GE::Scripts {

/**
 * @class PlayerController
 * @brief Drives entity movement from arrow-key input for the locally owned player sphere.
 *
 * Template Method pattern: subclasses override ReadMovementInput(), ReadJumpInput(),
 * and OnPostUpdate() to change input source or add per-frame actions without touching
 * the core velocity-apply logic.
 *
 * Extension examples:
 *   ShooterPlayerController — override OnPostUpdate() to fire projectiles on a key
 *   RacerPlayerController   — override ReadMovementInput() for forward/steer axes,
 *                             override ReadJumpInput() to return false
 */
class PlayerController : public GameScriptComponent {
public:
    const char* GetScriptName() const override { return "PlayerController"; }

    void Update(float dt)  override;
    void OnDrawInspector() override;

protected:
    // --- Template Method hooks ---

    /** Returns desired (vx, vz) velocity this frame. Override for AI / touch / restricted axes. */
    virtual glm::vec2 ReadMovementInput() const;

    /** Returns true when a jump impulse should be applied. Override to disable (e.g. racing). */
    virtual bool ReadJumpInput() const;

    /** Called after movement is applied each frame. Override for shoot / boost / drift. */
    virtual void OnPostUpdate(float /*dt*/) {}

    // Tunables — children can change defaults in their own constructor.
    float m_moveSpeed { 5.0f  };
    float m_jumpImpulse { 6.0f  };
    float m_hDamping { 0.85f };

private:
    bool  m_prevJumpInput { false };
    float m_jumpCooldown { 0.0f  };
};

} // namespace GE::Scripts
