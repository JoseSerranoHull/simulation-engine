#include "scripts/ScriptFactory.h"
#include "scripts/PlayerController.h"
#include "core/Logger.h"

namespace GE::Scripts {

std::shared_ptr<GameScriptComponent> CreateScript(const std::string& typeName) {
    if (typeName == "PlayerController") { return std::make_shared<PlayerController>(); }
    // if (typeName == "ShooterPlayerController") { return std::make_shared<ShooterPlayerController>(); }
    // if (typeName == "RacerPlayerController")   { return std::make_shared<RacerPlayerController>();   }
    GE_LOG_WARN("ScriptFactory: unknown script type '" + typeName + "'");
    return nullptr;
}

} // namespace GE::Scripts
