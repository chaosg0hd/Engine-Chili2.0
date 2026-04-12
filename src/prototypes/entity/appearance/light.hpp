#pragma once

#include "../../iprototype.hpp"
#include "../../systems/light_ray.hpp"
#include "../../../core/engine_context.hpp"
#include "../../../modules/memory/imemory_service.hpp"
#include "../../../modules/memory/memory_types.hpp"

#include <new>

struct LightRuntime
{
    InstanceId instanceId = 0;
    LightRayEmitterPrototype emitter;
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
        SetRay(Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, -1.0f, 0.0f));
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
        runtime->emitter = emitter;
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
        emitter.origin = origin;
        emitter.SetDirection(direction);
    }

    bool IsValid() const
    {
        return emitter.IsValid();
    }

public:
    LightRayEmitterPrototype emitter;

private:
    PrototypeId m_id = 0U;
    const char* m_debugName = nullptr;
};
