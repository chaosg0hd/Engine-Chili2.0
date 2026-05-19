#include "engine/script/script_registry.hpp"

namespace engine::script
{
    bool ScriptRegistry::Register(const ScriptPrototype& prototype)
    {
        if (!prototype.IsValid())
        {
            return false;
        }

        m_prototypes[prototype.GetName()] = prototype;
        return true;
    }

    const ScriptPrototype* ScriptRegistry::Find(std::string_view name) const
    {
        const auto found = m_prototypes.find(std::string(name));
        return found != m_prototypes.end() ? &found->second : nullptr;
    }
}
