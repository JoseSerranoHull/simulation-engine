#pragma once

/* parasoft-begin-suppress ALL */
#include <memory>
#include <string>
/* parasoft-end-suppress ALL */

namespace GE::Scripts { class GameScriptComponent; }

namespace GE::Scripts {

/**
 * @brief Creates a GameScriptComponent by type name string.
 *
 * Register new script types here. Returning nullptr is safe — FBSceneAdapter skips it.
 * Used by FBSceneAdapter::adaptObject() to attach scripts data-driven from the scene binary.
 */
std::shared_ptr<GameScriptComponent> CreateScript(const std::string& typeName);

} // namespace GE::Scripts
