#pragma once

#include "prototypes/entity/appearance/light.hpp"
#include "prototypes/entity/object/mesh.hpp"
#include "prototypes/entity/scene/camera.hpp"
#include "prototypes/math/math.hpp"
#include "modules/render/render_types.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace studio_runtime
{
    enum class RenderConfigurationPreset : unsigned char
    {
        Low = 0,
        Balanced,
        High
    };

    using EntityId = std::uint64_t;

    struct NameComponent
    {
        std::string name;
    };

    struct TransformComponent
    {
        TransformPrototype transform;
    };

    struct RenderableComponent
    {
        BuiltInMeshKind mesh = BuiltInMeshKind::Cube;
        MaterialPrototype material;
        bool visible = true;
    };

    struct ObjectComponent
    {
        std::string kind = "Object";
        std::string prototypeId;
        std::string behaviorPrototypeId;
        bool selectable = true;
    };

    struct LightComponent
    {
        LightPrototype light;
    };

    struct CameraComponent
    {
        CameraPrototype camera;
    };

    struct SceneRenderSettings
    {
        DerivedBounceFillSettings derivedBounce;
        TracedIndirectSettings tracedIndirect;
    };

    SceneRenderSettings MakeSceneRenderSettings(RenderConfigurationPreset preset);

    struct EntityInfo
    {
        EntityId id = 0;
        std::string name;
        TransformPrototype transform;
        ObjectComponent object;
        RenderableComponent renderable;
        LightComponent light;
        CameraComponent camera;
        bool hasObject = false;
        bool hasRenderable = false;
        bool hasLight = false;
        bool hasCamera = false;
    };

    class RuntimeWorld
    {
    public:
        EntityId CreateEntity(const std::string& name);
        EntityId CreateEntityWithId(EntityId id, const std::string& name);
        bool DestroyEntity(EntityId id);
        void Clear();

        bool Contains(EntityId id) const;
        std::vector<EntityId> GetEntityList() const;
        bool GetEntityInfo(EntityId id, EntityInfo& outInfo) const;

        NameComponent* GetName(EntityId id);
        const NameComponent* GetName(EntityId id) const;
        TransformComponent* GetTransform(EntityId id);
        const TransformComponent* GetTransform(EntityId id) const;
        RenderableComponent* GetRenderable(EntityId id);
        const RenderableComponent* GetRenderable(EntityId id) const;
        ObjectComponent* GetObject(EntityId id);
        const ObjectComponent* GetObject(EntityId id) const;
        LightComponent* GetLight(EntityId id);
        const LightComponent* GetLight(EntityId id) const;
        CameraComponent* GetCamera(EntityId id);
        const CameraComponent* GetCamera(EntityId id) const;

        void SetName(EntityId id, const std::string& name);
        void SetTransform(EntityId id, const TransformPrototype& transform);
        void SetObject(EntityId id, const ObjectComponent& object);
        void SetRenderable(EntityId id, const RenderableComponent& renderable);
        void SetLight(EntityId id, const LightComponent& light);
        void SetCamera(EntityId id, const CameraComponent& camera);
        void SetVisible(EntityId id, bool visible);
        void SetSceneRenderSettings(const SceneRenderSettings& settings);
        const SceneRenderSettings& GetSceneRenderSettings() const;

    private:
        struct EntityRecord
        {
            EntityId id = 0;
            NameComponent name;
            TransformComponent transform;
            ObjectComponent object;
            RenderableComponent renderable;
            LightComponent light;
            CameraComponent camera;
            bool alive = false;
            bool hasObject = false;
            bool hasRenderable = false;
            bool hasLight = false;
            bool hasCamera = false;
        };

        EntityRecord* Find(EntityId id);
        const EntityRecord* Find(EntityId id) const;
        EntityId AllocateId();

    private:
        std::vector<EntityRecord> m_entities;
        EntityId m_nextId = 1;
        SceneRenderSettings m_sceneRenderSettings;
    };
}
