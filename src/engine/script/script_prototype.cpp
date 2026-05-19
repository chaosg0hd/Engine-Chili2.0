#include "engine/script/script_prototype.hpp"

#include <utility>

namespace engine::script
{
    ScriptPrototype& ScriptPrototype::SetName(std::string name)
    {
        m_name = std::move(name);
        return *this;
    }

    ScriptPrototype& ScriptPrototype::SetFactory(Factory factory)
    {
        m_factory = factory;
        return *this;
    }

    const std::string& ScriptPrototype::GetName() const
    {
        return m_name;
    }

    ScriptPrototype::Factory ScriptPrototype::GetFactory() const
    {
        return m_factory;
    }

    bool ScriptPrototype::IsValid() const
    {
        return !m_name.empty() && m_factory != nullptr;
    }

    std::unique_ptr<IScriptBehavior> ScriptPrototype::CreateInstanceBehavior() const
    {
        if (m_factory == nullptr)
        {
            return nullptr;
        }

        return m_factory();
    }
}
