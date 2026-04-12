#pragma once

#include <cstdint>

struct EngineContext;

using PrototypeId = std::uint32_t;
using InstanceId = std::uint32_t;

class IPrototype
{
public:
    virtual ~IPrototype() = default;

    virtual PrototypeId GetId() const = 0;
    virtual const char* GetDebugName() const = 0;

    virtual void* Construct(EngineContext& context, InstanceId instanceId) const = 0;
    virtual void Destruct(EngineContext& context, void* runtimeData) const = 0;
};
