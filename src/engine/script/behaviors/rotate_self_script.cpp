#include "engine/script/behaviors/rotate_self_script.hpp"

#include "engine/script/script_context.hpp"

namespace engine::script
{
    void RotateSelfScript::OnUpdate(ScriptContext& ctx, float dt)
    {
        ctx.Transform().RotateY(1.5f * dt);
    }

    std::unique_ptr<IScriptBehavior> CreateRotateSelfScript()
    {
        return std::make_unique<RotateSelfScript>();
    }
}
