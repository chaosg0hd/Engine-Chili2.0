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
        if (LightComponent* light = m_world.GetLight(id))
        {
            light->light.emitter.position = transform.translation;
        }
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

    bool RuntimeCommands::SetRenderable(EntityId id, const std::string& meshAssetId, float r, float g, float b, bool visible)
    {
        if (!m_world.Contains(id))
        {
            return false;
        }

        BuiltInMeshKind mesh = BuiltInMeshKind::Cube;
        if      (meshAssetId == "builtin:quad")       mesh = BuiltInMeshKind::Quad;
        else if (meshAssetId == "builtin:octahedron") mesh = BuiltInMeshKind::Octahedron;
        else if (meshAssetId == "builtin:diamond")    mesh = BuiltInMeshKind::Diamond;
        else if (meshAssetId == "builtin:triangle")   mesh = BuiltInMeshKind::Triangle;

        auto clamp01 = [](float f) -> std::uint8_t {
            return static_cast<std::uint8_t>(f < 0.0f ? 0 : f > 1.0f ? 255 : f * 255.0f);
        };

        RenderableComponent renderable;
        if (const RenderableComponent* existing = m_world.GetRenderable(id))
        {
            renderable = *existing;
        }
        renderable.mesh = mesh;
        renderable.material.baseLayer.albedo = ColorPrototype::FromBytes(clamp01(r), clamp01(g), clamp01(b));
        renderable.visible = visible;
        m_world.SetRenderable(id, renderable);
        return true;
    }

    bool RuntimeCommands::SetLight(EntityId id, float px, float py, float pz, float r, float g, float b, float intensity, float range, bool enabled)
    {
        if (!m_world.Contains(id))
        {
            return false;
        }

        auto clamp01 = [](float f) -> std::uint8_t {
            return static_cast<std::uint8_t>(f < 0.0f ? 0 : f > 1.0f ? 255 : f * 255.0f);
        };

        LightComponent light;
        if (const LightComponent* existing = m_world.GetLight(id))
        {
            light = *existing;
        }
        light.light.enabled = enabled;
        light.light.emitter.kind = LightEmitterKind::Point;
        light.light.emitter.position = Vector3(px, py, pz);
        light.light.emitter.color = ColorPrototype::FromBytes(clamp01(r), clamp01(g), clamp01(b));
        light.light.emitter.intensity = intensity;
        light.light.emitter.range = range;
        m_world.SetLight(id, light);
        return true;
    }
}
