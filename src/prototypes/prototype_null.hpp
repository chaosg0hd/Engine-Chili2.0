#pragma once

#include "iprototype.hpp"
#include "../core/engine_context.hpp"
#include "../modules/memory/imemory_service.hpp"
#include "../modules/memory/memory_types.hpp"
#include "../modules/resources/iresource_service.hpp"
#include "../modules/resources/resource_types.hpp"

#include <new>
#include <string>

// Runtime state created by PrototypeNull.
// The prototype defines how this state is created, changed, and destroyed;
// Engine modules provide the actual memory and resource services.
struct PrototypeNullRuntime
{
    InstanceId instanceId = 0;
    int value = 0;

    bool hasResource = false;
    ResourceHandle resource = 0;
    std::string resourceAssetId;
};

// Prototype 0 is the baseline recipe for module-backed prototype behavior.
// It is not the runtime memory itself. It gives the engine instructions for
// constructing and destructing runtime state using the services in EngineContext.
class PrototypeNull final : public IPrototype
{
public:
    PrototypeNull(
        PrototypeId id,
        int defaultValue = 0,
        const char* debugName = "PrototypeNull")
        : m_id(id),
          m_defaultValue(defaultValue),
          m_debugName(debugName)
    {
    }

    PrototypeId GetId() const override
    {
        return m_id;
    }

    const char* GetDebugName() const override
    {
        return m_debugName ? m_debugName : "PrototypeNull";
    }

    void* Construct(EngineContext& context, InstanceId instanceId) const override
    {
        PrototypeNullRuntime* runtime = AllocateRuntime(context);
        if (!runtime)
        {
            return nullptr;
        }

        runtime->instanceId = instanceId;
        runtime->value = m_defaultValue;

        if (!m_resourceAssetId.empty() && context.Resources)
        {
            runtime->resource = context.Resources->RequestResource(m_resourceAssetId, ResourceKind::Mesh);
            runtime->hasResource = runtime->resource != 0U;
            runtime->resourceAssetId = m_resourceAssetId;
        }

        return runtime;
    }

    void Destruct(EngineContext& context, void* runtimeData) const override
    {
        auto* runtime = static_cast<PrototypeNullRuntime*>(runtimeData);
        if (!runtime)
        {
            return;
        }

        if (runtime->hasResource && runtime->resource != 0U && context.Resources)
        {
            context.Resources->UnloadResource(runtime->resource);
        }

        FreeRuntime(context, runtime);
    }

    void SetDefaultValue(int value)
    {
        m_defaultValue = value;
    }

    int GetDefaultValue() const
    {
        return m_defaultValue;
    }

    void SetValue(void* runtimeData, int value) const
    {
        if (auto* runtime = static_cast<PrototypeNullRuntime*>(runtimeData))
        {
            runtime->value = value;
        }
    }

    void AddValue(void* runtimeData, int delta) const
    {
        if (auto* runtime = static_cast<PrototypeNullRuntime*>(runtimeData))
        {
            runtime->value += delta;
        }
    }

    int GetValue(const void* runtimeData) const
    {
        const auto* runtime = static_cast<const PrototypeNullRuntime*>(runtimeData);
        return runtime ? runtime->value : 0;
    }

    void SetOptionalResourceAsset(const std::string& assetId)
    {
        m_resourceAssetId = assetId;
    }

    const std::string& GetOptionalResourceAsset() const
    {
        return m_resourceAssetId;
    }

private:
    PrototypeNullRuntime* AllocateRuntime(EngineContext& context) const
    {
        if (!context.Memory)
        {
            return nullptr;
        }

        void* memory = context.Memory->Allocate(
            sizeof(PrototypeNullRuntime),
            MemoryClass::Persistent,
            alignof(PrototypeNullRuntime),
            GetDebugName());
        if (!memory)
        {
            return nullptr;
        }

        return new (memory) PrototypeNullRuntime();
    }

    void FreeRuntime(EngineContext& context, PrototypeNullRuntime* runtime) const
    {
        if (!runtime)
        {
            return;
        }

        runtime->~PrototypeNullRuntime();

        if (context.Memory)
        {
            context.Memory->Free(runtime);
        }
    }

private:
    PrototypeId m_id = 0;
    int m_defaultValue = 0;
    const char* m_debugName = nullptr;
    std::string m_resourceAssetId;
};
