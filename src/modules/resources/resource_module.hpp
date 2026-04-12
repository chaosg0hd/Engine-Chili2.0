#pragma once

#include "../../core/module.hpp"
#include "iresource_service.hpp"
#include "resource_types.hpp"

#include <mutex>
#include <string>
#include <unordered_map>

class EngineContext;
enum class GpuResourceKind : std::uint8_t;
class IJobService;
class IFileService;

class FileModule;
class IGpuService;

class ResourceModule : public IModule, public IResourceService
{
public:
    const char* GetName() const override;
    bool Initialize(EngineContext& context) override;
    void Startup(EngineContext& context) override;
    void Update(EngineContext& context, float deltaTime) override;
    void Shutdown(EngineContext& context) override;

    ResourceHandle RequestResource(const std::string& assetId, ResourceKind kind) override;
    bool UnloadResource(ResourceHandle handle) override;
    bool SetResourceState(ResourceHandle handle, ResourceState state);
    ResourceState GetResourceState(ResourceHandle handle) const override;
    ResourceKind GetResourceKind(ResourceHandle handle) const override;
    std::string GetAssetId(ResourceHandle handle) const override;
    std::string GetResolvedPath(ResourceHandle handle) const override;
    std::string GetLastError(ResourceHandle handle) const override;
    std::size_t GetSourceByteSize(ResourceHandle handle) const override;
    std::string GetSourceText(ResourceHandle handle) const override;
    GpuResourceHandle GetGpuResourceHandle(ResourceHandle handle) const override;
    std::size_t GetUploadedByteSize(ResourceHandle handle) const override;
    bool IsResourceReady(ResourceHandle handle) const override;
    std::size_t GetResourceCountByState(ResourceState state) const override;
    std::size_t GetResourceCount() const override;

    bool IsInitialized() const;
    bool IsStarted() const;

private:
    bool IsLoadInFlight(ResourceState state) const;
    bool IsUnloadRequested(ResourceHandle handle) const;
    bool TryGetLoadRequest(ResourceHandle handle, std::string& outAssetId, std::string& outResolvedPath) const;
    bool FinalizePendingUnload(ResourceHandle handle, GpuResourceHandle uploadedHandle = 0U);
    bool SetResourceError(ResourceHandle handle, const std::string& error);
    bool SetResourceUploadData(ResourceHandle handle, GpuResourceHandle gpuHandle, std::size_t uploadedByteSize);
    bool ResolvePrototypeJson(ResourceHandle handle, const std::string& resolvedPath);
    bool ResolveAndUpload(ResourceHandle handle, const std::string& resolvedPath);
    bool QueueLoadWork(ResourceHandle handle);
    static GpuResourceKind ToGpuResourceKind(ResourceKind kind);

private:
    struct ResourceRecord
    {
        ResourceHandle handle = 0;
        ResourceKind kind = ResourceKind::Unknown;
        ResourceState state = ResourceState::Unloaded;
        std::string assetId;
        std::string resolvedPath;
        std::string lastError;
        std::size_t sourceByteSize = 0;
        std::string sourceText;
        GpuResourceHandle gpuHandle = 0;
        std::size_t uploadedByteSize = 0;
        bool unloadRequested = false;
    };

private:
    bool m_initialized = false;
    bool m_started = false;
    ResourceHandle m_nextHandle = 1U;
    IJobService* m_jobs = nullptr;
    IFileService* m_files = nullptr;
    IGpuService* m_gpu = nullptr;
    mutable std::mutex m_mutex;
    std::unordered_map<ResourceHandle, ResourceRecord> m_resources;
    std::unordered_map<std::string, ResourceHandle> m_assetLookup;
};
