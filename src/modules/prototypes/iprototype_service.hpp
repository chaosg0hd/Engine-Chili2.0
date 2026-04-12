#pragma once

#include "../../prototypes/iprototype.hpp"

#include <cstddef>
#include <memory>
#include <string>

using PrototypeInstanceHandle = InstanceId;

class IPrototypeService
{
public:
    virtual ~IPrototypeService() = default;

    virtual bool RegisterPrototype(std::unique_ptr<IPrototype> prototype) = 0;
    virtual PrototypeId LoadPrototypeAsset(const std::string& assetId) = 0;
    virtual bool UnregisterPrototype(PrototypeId prototypeId) = 0;
    virtual const IPrototype* GetPrototype(PrototypeId prototypeId) const = 0;
    virtual bool HasPrototype(PrototypeId prototypeId) const = 0;

    virtual PrototypeInstanceHandle CreateInstance(PrototypeId prototypeId) = 0;
    virtual bool DestroyInstance(PrototypeInstanceHandle instanceHandle) = 0;
    virtual void* GetInstanceRuntimeData(PrototypeInstanceHandle instanceHandle) const = 0;
    virtual PrototypeId GetInstancePrototypeId(PrototypeInstanceHandle instanceHandle) const = 0;

    virtual std::size_t GetPrototypeCount() const = 0;
    virtual std::size_t GetLiveInstanceCount() const = 0;
};
