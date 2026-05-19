#pragma once

#include "engine/script/iscript_behavior.hpp"
#include "app/app_capabilities.hpp"

#include <memory>

namespace pong
{
    // Set once at game startup so the non-capturing factory can forward it.
    // Single-instance pattern matching the raw Factory pointer constraint in ScriptPrototype.
    void SetSceneManagerInput(IAppInput* input);

    class SceneManagerScript : public engine::script::IScriptBehavior
    {
    public:
        explicit SceneManagerScript(IAppInput* input);

        void OnStart(engine::script::ScriptContext& ctx) override;
        void OnUpdate(engine::script::ScriptContext& ctx, float dt) override;
        void OnDestruct(engine::script::ScriptContext& ctx) override;

    private:
        IAppInput* m_input = nullptr;
    };

    // Non-capturing factory — reads input from the injection slot set by SetSceneManagerInput().
    std::unique_ptr<engine::script::IScriptBehavior> CreateSceneManagerScript();
}
