#pragma once

#include "engine/script/iscript_behavior.hpp"

#include <memory>

namespace pong
{
    class PongBallScript final : public engine::script::IScriptBehavior
    {
    public:
        void OnStart(engine::script::ScriptContext& ctx) override;
        void OnUpdate(engine::script::ScriptContext& ctx, float dt) override;

    private:
        float m_vx = 0.0f;
        float m_vz = 0.0f;
    };

    std::unique_ptr<engine::script::IScriptBehavior> CreatePongBallScript();
}
