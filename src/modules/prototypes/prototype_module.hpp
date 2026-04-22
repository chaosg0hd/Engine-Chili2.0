#pragma once

#include "../../core/module.hpp"
#include "../../prototypes/entity/appearance/material.hpp"
#include "../../prototypes/iprototype.hpp"
#include "iprototype_service.hpp"

#include <memory>
#include <mutex>
#include <unordered_map>

class EngineContext;

class PrototypeModule final : public IModule, public IPrototypeService
{
public:
    const char* GetName() const override;
    bool Initialize(EngineContext& context) override;
    void Startup(EngineContext& context) override;
    void Update(EngineContext& context, float deltaTime) override;
    void Shutdown(EngineContext& context) override;

    bool RegisterPrototype(std::unique_ptr<IPrototype> prototype) override;
    PrototypeId LoadPrototypeAsset(const std::string& assetId) override;
    bool UnregisterPrototype(PrototypeId prototypeId) override;
    const IPrototype* GetPrototype(PrototypeId prototypeId) const override;
    bool HasPrototype(PrototypeId prototypeId) const override;
    const MaterialPrototype* GetMaterialPrototype(const std::string& prototypeName) const override;
    bool HasMaterialPrototype(const std::string& prototypeName) const override;

    PrototypeInstanceHandle CreateInstance(PrototypeId prototypeId) override;
    bool DestroyInstance(PrototypeInstanceHandle instanceHandle) override;
    void* GetInstanceRuntimeData(PrototypeInstanceHandle instanceHandle) const override;
    PrototypeId GetInstancePrototypeId(PrototypeInstanceHandle instanceHandle) const override;

    std::size_t GetPrototypeCount() const override;
    std::size_t GetLiveInstanceCount() const override;

    bool IsInitialized() const;
    bool IsStarted() const;

private:
    struct PrototypeInstanceRecord
    {
        PrototypeInstanceHandle handle = 0;
        PrototypeId prototypeId = 0;
        void* runtimeData = nullptr;
    };

private:
    bool m_initialized = false;
    bool m_started = false;
    EngineContext* m_context = nullptr;
    PrototypeInstanceHandle m_nextInstanceHandle = 1U;
    mutable std::mutex m_mutex;
    std::unordered_map<PrototypeId, std::unique_ptr<IPrototype>> m_prototypes;
    std::unordered_map<std::string, MaterialPrototype> m_materialPrototypes;
    std::unordered_map<PrototypeInstanceHandle, PrototypeInstanceRecord> m_instances;
};
