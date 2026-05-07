#pragma once

#include "runtime/runtime_world.hpp"

namespace studio_runtime
{
    class RuntimeCommands
    {
    public:
        explicit RuntimeCommands(RuntimeWorld& world);

        EntityId CreateEntity(const std::string& name);
        bool DeleteEntity(EntityId id);
        bool SetTransform(EntityId id, const TransformPrototype& transform);
        bool SetVisible(EntityId id, bool visible);

    private:
        RuntimeWorld& m_world;
    };
}
