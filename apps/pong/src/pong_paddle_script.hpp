#pragma once

#include "engine/script/iscript_behavior.hpp"

#include <memory>

namespace pong
{
    class PongPaddleScript final : public engine::script::IScriptBehavior
    {
    public:
        void OnUpdate(engine::script::ScriptContext& ctx, float dt) override;
    };

    std::unique_ptr<engine::script::IScriptBehavior> CreatePongPaddleScript();
}
