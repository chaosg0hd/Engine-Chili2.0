#pragma once

#include "../../core/module.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>

class EngineContext;

enum class ResourceKind : std::uint8_t
{
    Unknown = 0,
    Texture,
    Mesh,
    Material,
    Shader
};

enum class ResourceState : std::uint8_t
{
    Unloaded = 0,
    Queued,
    Loading,
    Processing,
    Uploading,
    Ready,
    Failed
};

using ResourceHandle = std::uint32_t;

class ResourceModule : public IModule
{
public:
    const char* GetName() const override;
    bool Initialize(EngineContext& context) override;
    void Startup(EngineContext& context) override;
    void Update(EngineContext& context, float deltaTime) override;
    void Shutdown(EngineContext& context) override;

    ResourceHandle RequestResource(const std::string& assetId, ResourceKind kind);
    bool UnloadResource(ResourceHandle handle);
    bool SetResourceState(ResourceHandle handle, ResourceState state);
    ResourceState GetResourceState(ResourceHandle handle) const;
    ResourceKind GetResourceKind(ResourceHandle handle) const;
    std::string GetAssetId(ResourceHandle handle) const;
    bool IsResourceReady(ResourceHandle handle) const;
    std::size_t GetResourceCount() const;

    bool IsInitialized() const;
    bool IsStarted() const;

private:
    struct ResourceRecord
    {
        ResourceHandle handle = 0;
        ResourceKind kind = ResourceKind::Unknown;
        ResourceState state = ResourceState::Unloaded;
        std::string assetId;
    };

private:
    bool m_initialized = false;
    bool m_started = false;
    ResourceHandle m_nextHandle = 1U;
    std::unordered_map<ResourceHandle, ResourceRecord> m_resources;
    std::unordered_map<std::string, ResourceHandle> m_assetLookup;
};
