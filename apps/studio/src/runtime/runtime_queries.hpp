#pragma once

#include "runtime/runtime_world.hpp"

namespace studio_runtime
{
    class RuntimeQueries
    {
    public:
        explicit RuntimeQueries(const RuntimeWorld& world);

        bool GetEntityInfo(EntityId id, EntityInfo& outInfo) const;
        std::vector<EntityId> GetEntityList() const;
        const TransformComponent* GetTransform(EntityId id) const;
        const RenderableComponent* GetRenderable(EntityId id) const;
        const CameraComponent* GetCamera(EntityId id) const;

    private:
        const RuntimeWorld& m_world;
    };
}
