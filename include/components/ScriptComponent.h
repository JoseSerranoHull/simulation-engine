#pragma once

/* parasoft-begin-suppress ALL */
#include <memory>
/* parasoft-end-suppress ALL */

#include "scripts/GameScriptComponent.h"

namespace GE::Components {

    /**
     * @struct ScriptComponent
     * @brief ECS component that attaches a GameScriptComponent instance to an entity.
     *
     * Uses shared_ptr rather than unique_ptr because ComponentArray::Add copies the
     * struct via push_back(). The copy simply increments the reference count so both
     * the caller's local variable and the packed-array slot share the same script object.
     *
     * ## Usage
     * @code
     *   em->AddComponent<ScriptComponent>(
     *       entityID,
     *       ScriptComponent{ std::make_shared<PlayerScript>() }
     *   );
     * @endcode
     *
     * @note A null script pointer is valid (no-op component), but ScriptSystem will skip it.
     */
    struct ScriptComponent {
        std::shared_ptr<GE::Scripts::GameScriptComponent> script;
    };

} // namespace GE::Components
