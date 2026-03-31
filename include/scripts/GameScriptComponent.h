#pragma once

/* parasoft-begin-suppress ALL */
#include <string>
#include <glm/glm.hpp>
/* parasoft-end-suppress ALL */

#include "ecs/Entity.h"
#include "scripts/CollisionInfo.h"

// Forward declarations — keeps heavy Vulkan / ECS headers out of user script files.
namespace GE::ECS    { class EntityManager; }
namespace GE::Components { struct Transform; }
class InputService;

namespace GE::Scripts {

/**
 * @class GameScriptComponent
 * @brief Abstract base class for user-authored C++ scripts. Inspired by Unity MonoBehaviour.
 *
 * ## Lifecycle (in order of execution)
 *
 *   Awake()       — called once on the first frame the script becomes active (even if
 *                   the scenario started with the script inactive). Immediately followed by:
 *   Start()       — called once on the same first-active frame, before the first Update.
 *   FixedUpdate() — called at a fixed rate (ScriptSystem accumulator) while active.
 *   Update()      — called every frame while active.
 *   LateUpdate()  — called after ALL Update() calls in the same frame while active.
 *   OnCollisionEnter/Exit()  — fired by ScriptSystem when solid contacts begin/end.
 *   OnTriggerEnter/Exit()    — fired by ScriptSystem when trigger overlaps begin/end.
 *   OnDestroy()   — called once when the host entity is removed from the scene.
 *
 * ## Activation semantics
 *   - While m_active == false, NOTHING fires (not even Awake).
 *   - Awake+Start fire the first time m_active becomes true.
 *   - If deactivated after starting, only Update/FixedUpdate/LateUpdate pause; they
 *     resume when re-activated (Awake/Start do NOT re-fire).
 *
 * ## Usage
 * @code
 *   class PlayerScript : public GE::Scripts::GameScriptComponent {
 *   public:
 *       float moveSpeed { 5.0f };
 *
 *       void Update(float dt) override {
 *           if (GetInput()->IsKeyDown(GLFW_KEY_RIGHT))
 *               GetTransform()->m_position.x += moveSpeed * dt;
 *       }
 *
 *       void OnDrawInspector() override {
 *           GameScriptComponent::OnDrawInspector();   // draws m_active
 *           DrawField("Move Speed", moveSpeed);
 *       }
 *   };
 *
 *   // Attach to entity:
 *   em->AddComponent<ScriptComponent>(id, ScriptComponent{ std::make_shared<PlayerScript>() });
 * @endcode
 */
class GameScriptComponent {
public:
    virtual ~GameScriptComponent() = default;

    // -----------------------------------------------------------------------
    // Lifecycle callbacks — override as needed; all have empty default bodies.
    // -----------------------------------------------------------------------

    /** Called once on the first frame m_active becomes true. */
    virtual void Awake() {}

    /** Called once on the first active frame, immediately after Awake, before Update. */
    virtual void Start() {}

    /** Called every frame while m_active is true. */
    virtual void Update(float deltaTime) {}

    /** Called at a fixed interval (ScriptSystem accumulator) while m_active is true. */
    virtual void FixedUpdate(float fixedDeltaTime) {}

    /** Called after all Update() calls in the same frame while m_active is true. */
    virtual void LateUpdate(float deltaTime) {}

    /** Called when this entity first enters a solid collision this frame. */
    virtual void OnCollisionEnter(const CollisionInfo& info) {}

    /** Called when a previously-tracked solid collision ends. */
    virtual void OnCollisionExit(const CollisionInfo& info) {}

    /** Called when this entity enters a trigger volume overlap. */
    virtual void OnTriggerEnter(GE::ECS::EntityID otherEntity) {}

    /** Called when a previously-tracked trigger overlap ends. */
    virtual void OnTriggerExit(GE::ECS::EntityID otherEntity) {}

    /** Called by ScriptSystem when DestroyEntity() is issued for this entity. */
    virtual void OnDestroy() {}

    // -----------------------------------------------------------------------
    // ImGui Inspector
    // -----------------------------------------------------------------------

    /**
     * @brief Override to expose public variables in the ImGui inspector.
     *
     * The base implementation draws the m_active checkbox.
     * Subclasses chain the base call, then use DrawField() for their own fields:
     *
     * @code
     *   void OnDrawInspector() override {
     *       GameScriptComponent::OnDrawInspector();  // m_active
     *       DrawField("Speed",  speed);
     *       DrawField("Health", health);
     *   }
     * @endcode
     */
    virtual void OnDrawInspector();

    // -----------------------------------------------------------------------
    // Engine-visible state (read/written by ScriptSystem)
    // -----------------------------------------------------------------------

    bool m_active  { true };    ///< While false, no lifecycle callbacks fire.
    bool m_started { false };   ///< Set to true after Awake+Start have fired.

    void              SetEntityID(GE::ECS::EntityID id) { m_entityID = id; }
    GE::ECS::EntityID GetEntityID() const               { return m_entityID; }

protected:
    // -----------------------------------------------------------------------
    // Convenience helpers — use in lifecycle overrides.
    // All access global services via ServiceLocator; never null during callbacks.
    // -----------------------------------------------------------------------

    /** Returns this entity's Transform component. Fatal-logs if missing. */
    GE::Components::Transform*  GetTransform()      const;

    /** Returns the engine's InputService for key/mouse queries. */
    InputService*               GetInput()          const;

    /** Returns the global EntityManager for component access and entity queries. */
    GE::ECS::EntityManager*     GetEntityManager()  const;

    /** Returns this entity's Tag name string (empty if no Tag component). */
    const std::string&          GetName()           const;

    // -----------------------------------------------------------------------
    // Inspector field helpers — call from OnDrawInspector()
    // -----------------------------------------------------------------------
    void DrawField(const char* label, bool&              value) const;
    void DrawField(const char* label, int&               value) const;
    void DrawField(const char* label, float&             value) const;
    void DrawField(const char* label, glm::vec2&         value) const;
    void DrawField(const char* label, glm::vec3&         value) const;
    void DrawField(const char* label, glm::vec4&         value) const;
    void DrawField(const char* label, const std::string& value) const;  ///< Read-only text

    GE::ECS::EntityID m_entityID { GE::ECS::INVALID_ENTITY_ID };

private:
    // Backing storage for GetName() when no Tag component is present.
    mutable std::string m_fallbackName;
};

} // namespace GE::Scripts
