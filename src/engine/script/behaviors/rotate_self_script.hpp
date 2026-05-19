#pragma once

#include "engine/script/iscript_behavior.hpp"

#include <memory>

namespace engine::script
{
    class RotateSelfScript final : public IScriptBehavior
    {
    public:
        void OnUpdate(ScriptContext& ctx, float dt) override;
    };

    std::unique_ptr<IScriptBehavior> CreateRotateSelfScript();
}
