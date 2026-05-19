#include "pong_paddle_script.hpp"
#include "pong_rules.hpp"
#include "engine/script/script_context.hpp"

#include <cmath>
#include <memory>

namespace pong
{
    void PongPaddleScript::OnUpdate(engine::script::ScriptContext& ctx, float dt)
    {
        const TransformPrototype* ball = ctx.FindEntity("Pong_Ball");
        if (!ball)
            return;

        auto& t = ctx.Transform();
        const float targetZ = ball->translation.z;
        const float diff = targetZ - t.translation.z;
        const float step = kAiSpeed * dt;

        if (std::abs(diff) > step)
            t.translation.z += (diff > 0.0f ? step : -step);
        else
            t.translation.z = targetZ;

        // Clamp within walls
        const float limit = kWallHalfZ - kPaddleHalfZ;
        if (t.translation.z > limit)
            t.translation.z = limit;
        else if (t.translation.z < -limit)
            t.translation.z = -limit;
    }

    std::unique_ptr<engine::script::IScriptBehavior> CreatePongPaddleScript()
    {
        return std::make_unique<PongPaddleScript>();
    }
}
