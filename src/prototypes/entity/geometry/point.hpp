#pragma once

#include "../../iprototype.hpp"
#include "../../math/math.hpp"
#include "../../../core/engine_context.hpp"
#include "../../../modules/memory/imemory_service.hpp"
#include "../../../modules/memory/memory_types.hpp"

#include <new>

struct PointRuntime
{
    InstanceId instanceId = 0;
    Vector3 position = Vector3(0.0f, 0.0f, 0.0f);
};

class PointPrototype final : public IPrototype
{
public:
    PointPrototype() = default;

    PointPrototype(
        const Vector3& inPosition,
        PrototypeId inId = 0U,
        const char* inDebugName = "PointPrototype")
        : position(inPosition),
          m_id(inId),
          m_debugName(inDebugName)
    {
    }

    PrototypeId GetId() const override
    {
        return m_id;
    }

    const char* GetDebugName() const override
    {
        return m_debugName ? m_debugName : "PointPrototype";
    }

    void* Construct(EngineContext& context, InstanceId instanceId) const override
    {
        PointRuntime* runtime = AllocateRuntime(context);
        if (!runtime)
        {
            return nullptr;
        }

        runtime->instanceId = instanceId;
        runtime->position = position;
        return runtime;
    }

    void Destruct(EngineContext& context, void* runtimeData) const override
    {
        auto* runtime = static_cast<PointRuntime*>(runtimeData);
        if (!runtime)
        {
            return;
        }

        FreeRuntime(context, runtime);
    }

    void SetPosition(const Vector3& value)
    {
        position = value;
    }

    const Vector3& GetPosition() const
    {
        return position;
    }

    bool IsValid() const
    {
        return true;
    }

public:
    Vector3 position = Vector3(0.0f, 0.0f, 0.0f);

private:
    PointRuntime* AllocateRuntime(EngineContext& context) const
    {
        if (!context.Memory)
        {
            return nullptr;
        }

        void* memory = context.Memory->Allocate(
            sizeof(PointRuntime),
            MemoryClass::Persistent,
            alignof(PointRuntime),
            GetDebugName());
        if (!memory)
        {
            return nullptr;
        }

        return new (memory) PointRuntime();
    }

    void FreeRuntime(EngineContext& context, PointRuntime* runtime) const
    {
        if (!runtime)
        {
            return;
        }

        runtime->~PointRuntime();

        if (context.Memory)
        {
            context.Memory->Free(runtime);
        }
    }

private:
    PrototypeId m_id = 0U;
    const char* m_debugName = "PointPrototype";
};
