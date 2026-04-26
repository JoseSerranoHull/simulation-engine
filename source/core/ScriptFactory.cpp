#include "core/ScriptFactory.h"
#include "scripts/GameScriptComponent.h"

namespace GE::Scripts {

std::unordered_map<std::string, ScriptFactoryFn>& ScriptFactory::registry() {
    static std::unordered_map<std::string, ScriptFactoryFn> s_registry;
    return s_registry;
}

void ScriptFactory::Register(const std::string& typeName, ScriptFactoryFn fn) {
    registry()[typeName] = std::move(fn);
}

std::shared_ptr<GameScriptComponent> ScriptFactory::Create(const std::string& typeName) {
    const auto it = registry().find(typeName);
    if (it == registry().end()) { return nullptr; }
    return it->second();
}

void ScriptFactory::Clear() {
    registry().clear();
}

} // namespace GE::Scripts
