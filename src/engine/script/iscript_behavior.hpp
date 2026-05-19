#pragma once

#include "engine/script/script_event.hpp"

namespace engine::script
{
    class ScriptContext;

    class IScriptBehavior
    {
    public:
        virtual ~IScriptBehavior() = default;

        virtual void OnConstruct(ScriptContext&) {}
        virtual void OnStart(ScriptContext&) {}
        virtual void OnUpdate(ScriptContext&, float) {}
        virtual void OnEvent(ScriptContext&, const ScriptEvent&) {}
        virtual void OnDestruct(ScriptContext&) {}
    };
}
