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
        bool SetRenderable(EntityId id, const std::string& meshAssetId, float r, float g, float b, bool visible);
        bool SetLight(EntityId id, float px, float py, float pz, float r, float g, float b, float intensity, float range, bool enabled);

    private:
        RuntimeWorld& m_world;
    };
}
