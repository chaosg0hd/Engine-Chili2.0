#include "runtime/runtime_world.hpp"

#include <algorithm>

namespace studio_runtime
{
    EntityId RuntimeWorld::CreateEntity(const std::string& name)
    {
        return CreateEntityWithId(AllocateId(), name);
    }

    EntityId RuntimeWorld::CreateEntityWithId(EntityId id, const std::string& name)
    {
        if (id == 0)
        {
            id = AllocateId();
        }

        if (Contains(id))
        {
            return 0;
        }

        EntityRecord record;
        record.id = id;
        record.name.name = name;
        record.alive = true;
        m_entities.push_back(record);
        m_nextId = std::max(m_nextId, id + 1);
        return id;
    }

    bool RuntimeWorld::DestroyEntity(EntityId id)
    {
        EntityRecord* record = Find(id);
        if (!record)
        {
            return false;
        }

        record->alive = false;
        return true;
    }

    void RuntimeWorld::Clear()
    {
        m_entities.clear();
        m_nextId = 1;
    }

    bool RuntimeWorld::Contains(EntityId id) const
    {
        return Find(id) != nullptr;
    }

    std::vector<EntityId> RuntimeWorld::GetEntityList() const
    {
        std::vector<EntityId> ids;
        for (const EntityRecord& record : m_entities)
        {
            if (record.alive)
            {
                ids.push_back(record.id);
            }
        }

        return ids;
    }

    bool RuntimeWorld::GetEntityInfo(EntityId id, EntityInfo& outInfo) const
    {
        const EntityRecord* record = Find(id);
        if (!record)
        {
            return false;
        }

        outInfo = EntityInfo{};
        outInfo.id = record->id;
        outInfo.name = record->name.name;
        outInfo.transform = record->transform.transform;
        outInfo.object = record->object;
        outInfo.renderable = record->renderable;
        outInfo.light = record->light;
        outInfo.hasObject = record->hasObject;
        outInfo.hasRenderable = record->hasRenderable;
        outInfo.hasLight = record->hasLight;
        return true;
    }

    NameComponent* RuntimeWorld::GetName(EntityId id)
    {
        EntityRecord* record = Find(id);
        return record ? &record->name : nullptr;
    }

    const NameComponent* RuntimeWorld::GetName(EntityId id) const
    {
        const EntityRecord* record = Find(id);
        return record ? &record->name : nullptr;
    }

    TransformComponent* RuntimeWorld::GetTransform(EntityId id)
    {
        EntityRecord* record = Find(id);
        return record ? &record->transform : nullptr;
    }

    const TransformComponent* RuntimeWorld::GetTransform(EntityId id) const
    {
        const EntityRecord* record = Find(id);
        return record ? &record->transform : nullptr;
    }

    RenderableComponent* RuntimeWorld::GetRenderable(EntityId id)
    {
        EntityRecord* record = Find(id);
        return record && record->hasRenderable ? &record->renderable : nullptr;
    }

    const RenderableComponent* RuntimeWorld::GetRenderable(EntityId id) const
    {
        const EntityRecord* record = Find(id);
        return record && record->hasRenderable ? &record->renderable : nullptr;
    }

    ObjectComponent* RuntimeWorld::GetObject(EntityId id)
    {
        EntityRecord* record = Find(id);
        return record && record->hasObject ? &record->object : nullptr;
    }

    const ObjectComponent* RuntimeWorld::GetObject(EntityId id) const
    {
        const EntityRecord* record = Find(id);
        return record && record->hasObject ? &record->object : nullptr;
    }

    LightComponent* RuntimeWorld::GetLight(EntityId id)
    {
        EntityRecord* record = Find(id);
        return record && record->hasLight ? &record->light : nullptr;
    }

    const LightComponent* RuntimeWorld::GetLight(EntityId id) const
    {
        const EntityRecord* record = Find(id);
        return record && record->hasLight ? &record->light : nullptr;
    }

    void RuntimeWorld::SetTransform(EntityId id, const TransformPrototype& transform)
    {
        if (EntityRecord* record = Find(id))
        {
            record->transform.transform = transform;
        }
    }

    void RuntimeWorld::SetObject(EntityId id, const ObjectComponent& object)
    {
        if (EntityRecord* record = Find(id))
        {
            record->object = object;
            record->hasObject = true;
        }
    }

    void RuntimeWorld::SetRenderable(EntityId id, const RenderableComponent& renderable)
    {
        if (EntityRecord* record = Find(id))
        {
            record->renderable = renderable;
            record->hasRenderable = true;
        }
    }

    void RuntimeWorld::SetLight(EntityId id, const LightComponent& light)
    {
        if (EntityRecord* record = Find(id))
        {
            record->light = light;
            record->hasLight = true;
        }
    }

    void RuntimeWorld::SetVisible(EntityId id, bool visible)
    {
        if (RenderableComponent* renderable = GetRenderable(id))
        {
            renderable->visible = visible;
        }
    }

    RuntimeWorld::EntityRecord* RuntimeWorld::Find(EntityId id)
    {
        for (EntityRecord& record : m_entities)
        {
            if (record.alive && record.id == id)
            {
                return &record;
            }
        }

        return nullptr;
    }

    const RuntimeWorld::EntityRecord* RuntimeWorld::Find(EntityId id) const
    {
        for (const EntityRecord& record : m_entities)
        {
            if (record.alive && record.id == id)
            {
                return &record;
            }
        }

        return nullptr;
    }

    EntityId RuntimeWorld::AllocateId()
    {
        while (Contains(m_nextId))
        {
            ++m_nextId;
        }

        return m_nextId++;
    }
}
