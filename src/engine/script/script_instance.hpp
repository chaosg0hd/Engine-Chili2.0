#pragma once

#include "engine/script/iscript_behavior.hpp"
#include "engine/script/script_context.hpp"

#include <memory>

namespace engine::script
{
    class ScriptInstance
    {
    public:
        ScriptInstance() = default;
        explicit ScriptInstance(std::unique_ptr<IScriptBehavior> behavior);

        void SetBehavior(std::unique_ptr<IScriptBehavior> behavior);
        bool HasBehavior() const;
        void Construct(ScriptContext& ctx);
        void Start(ScriptContext& ctx);
        void Update(ScriptContext& ctx, float dt);
        void SendEvent(ScriptContext& ctx, const ScriptEvent& event);
        void Destruct(ScriptContext& ctx);

    private:
        std::unique_ptr<IScriptBehavior> m_behavior;
        bool m_constructed = false;
        bool m_started = false;
    };
}
