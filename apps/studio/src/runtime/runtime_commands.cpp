#include "runtime/runtime_commands.hpp"

namespace studio_runtime
{
    RuntimeCommands::RuntimeCommands(RuntimeWorld& world)
        : m_world(world)
    {
    }

    EntityId RuntimeCommands::CreateEntity(const std::string& name)
    {
        return m_world.CreateEntity(name);
    }

    bool RuntimeCommands::DeleteEntity(EntityId id)
    {
        return m_world.DestroyEntity(id);
    }

    bool RuntimeCommands::SetTransform(EntityId id, const TransformPrototype& transform)
    {
        if (!m_world.Contains(id))
        {
            return false;
        }
        m_world.SetTransform(id, transform);
        return true;
    }

    bool RuntimeCommands::SetVisible(EntityId id, bool visible)
    {
        if (!m_world.GetRenderable(id))
        {
            return false;
        }
        m_world.SetVisible(id, visible);
        return true;
    }
}
