#pragma once

#include "engine/script/script_registry.hpp"

#include <string>

namespace engine::script
{
    class ScriptModule
    {
    public:
        ScriptModule();

        ScriptRegistry& Registry();
        const ScriptRegistry& Registry() const;
        void LoadFromProxyFolder(const std::string& proxyFolderPath);

    private:
        void RegisterBuiltIns();

    private:
        ScriptRegistry m_registry;
    };
}
