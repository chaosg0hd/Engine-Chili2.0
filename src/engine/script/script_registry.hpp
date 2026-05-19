#pragma once

#include "engine/script/script_prototype.hpp"

#include <string>
#include <string_view>
#include <unordered_map>

namespace engine::script
{
    class ScriptRegistry
    {
    public:
        bool Register(const ScriptPrototype& prototype);
        const ScriptPrototype* Find(std::string_view name) const;

    private:
        std::unordered_map<std::string, ScriptPrototype> m_prototypes;
    };
}
