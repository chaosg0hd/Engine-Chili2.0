#pragma once

#include "engine/script/iscript_behavior.hpp"

#include <memory>
#include <string>

namespace engine::script
{
    class ScriptPrototype
    {
    public:
        using Factory = std::unique_ptr<IScriptBehavior>(*)();

        ScriptPrototype& SetName(std::string name);
        ScriptPrototype& SetFactory(Factory factory);

        const std::string& GetName() const;
        Factory GetFactory() const;
        bool IsValid() const;
        std::unique_ptr<IScriptBehavior> CreateInstanceBehavior() const;

    private:
        std::string m_name;
        Factory m_factory = nullptr;
    };
}
