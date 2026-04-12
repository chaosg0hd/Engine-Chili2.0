#pragma once

#include "color.hpp"
#include "../geometry/line.hpp"
#include "../../iprototype.hpp"
#include "../../../core/engine_context.hpp"
#include "../../../modules/memory/imemory_service.hpp"
#include "../../../modules/memory/memory_types.hpp"

#include <new>

struct LightRuntime
{
    InstanceId instanceId = 0;
    LineRuntime ray;
    ColorPrototype color;
    float intensity = 1.0f;
    unsigned int raycastCount = 1U;
    bool enabled = true;
};

class LightPrototype final : public IPrototype
{
public:
    LightPrototype(
        PrototypeId inId = 0U,
        const char* inDebugName = "LightPrototype")
        : m_id(inId),
          m_debugName(inDebugName)
    {
        ray.SetRay(Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, -1.0f, 0.0f));
    }

    PrototypeId GetId() const override
    {
        return m_id;
    }

    const char* GetDebugName() const override
    {
        return m_debugName ? m_debugName : "LightPrototype";
    }

    void* Construct(EngineContext& context, InstanceId instanceId) const override
    {
        if (!context.Memory)
        {
            return nullptr;
        }

        void* memory = context.Memory->Allocate(
            sizeof(LightRuntime),
            MemoryClass::Persistent,
            alignof(LightRuntime),
            GetDebugName());
        if (!memory)
        {
            return nullptr;
        }

        auto* runtime = new (memory) LightRuntime();
        runtime->instanceId = instanceId;
        runtime->ray.instanceId = instanceId;
        runtime->ray.form = LineForm::Ray;
        runtime->ray.origin = ray.origin;
        runtime->ray.direction = ray.GetDirection();
        runtime->ray.minT = ray.minT;
        runtime->ray.maxT = ray.maxT;
        runtime->color = color;
        runtime->intensity = intensity;
        runtime->raycastCount = raycastCount;
        runtime->enabled = enabled;
        return runtime;
    }

    void Destruct(EngineContext& context, void* runtimeData) const override
    {
        auto* runtime = static_cast<LightRuntime*>(runtimeData);
        if (!runtime)
        {
            return;
        }

        runtime->~LightRuntime();
        if (context.Memory)
        {
            context.Memory->Free(runtime);
        }
    }

    void SetRay(const Vector3& origin, const Vector3& direction)
    {
        ray.SetRay(origin, direction);
    }

    bool IsValid() const
    {
        return !enabled || (ray.IsRay() && ray.IsValid() && intensity >= 0.0f && raycastCount > 0U);
    }

public:
    LinePrototype ray;
    ColorPrototype color;
    float intensity = 1.0f;
    unsigned int raycastCount = 1U;
    bool enabled = true;

private:
    PrototypeId m_id = 0U;
    const char* m_debugName = nullptr;
};
