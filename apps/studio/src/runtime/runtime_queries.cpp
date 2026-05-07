#include "runtime/runtime_queries.hpp"

namespace studio_runtime
{
    RuntimeQueries::RuntimeQueries(const RuntimeWorld& world)
        : m_world(world)
    {
    }

    bool RuntimeQueries::GetEntityInfo(EntityId id, EntityInfo& outInfo) const
    {
        return m_world.GetEntityInfo(id, outInfo);
    }

    std::vector<EntityId> RuntimeQueries::GetEntityList() const
    {
        return m_world.GetEntityList();
    }

    const TransformComponent* RuntimeQueries::GetTransform(EntityId id) const
    {
        return m_world.GetTransform(id);
    }

    const RenderableComponent* RuntimeQueries::GetRenderable(EntityId id) const
    {
        return m_world.GetRenderable(id);
    }
}
