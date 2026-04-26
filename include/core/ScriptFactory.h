#pragma once

/* parasoft-begin-suppress ALL */
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
/* parasoft-end-suppress ALL */

namespace GE::Scripts { class GameScriptComponent; }

namespace GE::Scripts {

    using ScriptFactoryFn = std::function<std::shared_ptr<GameScriptComponent>()>;

    /**
     * @class ScriptFactory
     * @brief Global registry mapping script type name strings to factory lambdas.
     *        Register concrete GameScriptComponent subclasses in FlatBuffersScenario::OnLoad().
     *        EntityFactory calls Create() when instantiating prefabs with a script_type field.
     */
    class ScriptFactory {
    public:
        static void Register(const std::string& typeName, ScriptFactoryFn fn);
        static std::shared_ptr<GameScriptComponent> Create(const std::string& typeName);
        static void Clear();

    private:
        static std::unordered_map<std::string, ScriptFactoryFn>& registry();
    };

} // namespace GE::Scripts
