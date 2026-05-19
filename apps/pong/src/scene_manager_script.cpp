#include "scene_manager_script.hpp"

#include "engine/script/script_context.hpp"
#include "engine/script/script_event.hpp"

namespace pong
{
    namespace
    {
        IAppInput* s_input = nullptr;

        constexpr AppKey kEscape = static_cast<AppKey>(0x1B);
    }

    void SetSceneManagerInput(IAppInput* input)
    {
        s_input = input;
    }

    SceneManagerScript::SceneManagerScript(IAppInput* input)
        : m_input(input)
    {
    }

    void SceneManagerScript::OnStart(engine::script::ScriptContext& ctx)
    {
        ctx.LogInfo("SceneManager: scene active.");
    }

    void SceneManagerScript::OnUpdate(engine::script::ScriptContext& ctx, float)
    {
        if (!m_input)
            return;

        if (m_input->WasKeyPressed(AppKey::Space))
            ctx.EmitEvent(engine::script::ScriptEvent{"goto_scene:1"});

        if (m_input->WasKeyPressed(kEscape))
            ctx.EmitEvent(engine::script::ScriptEvent{"exit"});
    }

    void SceneManagerScript::OnDestruct(engine::script::ScriptContext& ctx)
    {
        ctx.LogInfo("SceneManager: scene unloading.");
    }

    std::unique_ptr<engine::script::IScriptBehavior> CreateSceneManagerScript()
    {
        return std::make_unique<SceneManagerScript>(s_input);
    }
}
