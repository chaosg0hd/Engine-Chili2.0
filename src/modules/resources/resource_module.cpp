#include "resource_module.hpp"

#include "../../core/engine_context.hpp"
#include "../file/ifile_service.hpp"
#include "../gpu/igpu_service.hpp"
#include "../jobs/ijob_service.hpp"

#include <vector>

const char* ResourceModule::GetName() const
{
    return "Resource";
}

bool ResourceModule::Initialize(EngineContext& context)
{
    if (m_initialized)
    {
        return true;
    }

    m_jobs = context.Jobs;
    m_files = context.Files;
    m_gpu = context.Gpu;
    m_resources.clear();
    m_assetLookup.clear();
    m_nextHandle = 1U;
    m_initialized = true;
    return true;
}

void ResourceModule::Startup(EngineContext& context)
{
    if (!m_initialized || m_started)
    {
        return;
    }

    if (!m_jobs)
    {
        m_jobs = context.Jobs;
    }

    if (!m_files)
    {
        m_files = context.Files;
    }

    if (!m_gpu)
    {
        m_gpu = context.Gpu;
    }

    m_started = true;
}

void ResourceModule::Update(EngineContext& context, float deltaTime)
{
    (void)context;
    (void)deltaTime;

    if (!m_jobs)
    {
        m_jobs = context.Jobs;
    }

    if (!m_files)
    {
        m_files = context.Files;
    }

    if (!m_gpu)
    {
        m_gpu = context.Gpu;
    }
}

void ResourceModule::Shutdown(EngineContext& context)
{
    (void)context;

    if (!m_initialized)
    {
        return;
    }

    m_resources.clear();
    m_assetLookup.clear();
    m_nextHandle = 1U;
    m_jobs = nullptr;
    m_files = nullptr;
    m_gpu = nullptr;
    m_started = false;
    m_initialized = false;
}

ResourceHandle ResourceModule::RequestResource(const std::string& assetId, ResourceKind kind)
{
    if (!m_started || assetId.empty())
    {
        return 0U;
    }

    ResourceHandle handle = 0U;

    {
        std::lock_guard<std::mutex> lock(m_mutex);

        const auto existing = m_assetLookup.find(assetId);
        if (existing != m_assetLookup.end())
        {
            return existing->second;
        }

        handle = m_nextHandle++;

        ResourceRecord record;
        record.handle = handle;
        record.kind = kind;
        record.state = ResourceState::Queued;
        record.assetId = assetId;
        record.resolvedPath = m_files ? m_files->GetAbsolutePath(assetId) : assetId;
        record.lastError.clear();
        record.sourceByteSize = 0;
        record.sourceText.clear();
        record.gpuHandle = 0;
        record.uploadedByteSize = 0;

        m_resources.emplace(handle, record);
        m_assetLookup.emplace(assetId, handle);
    }

    QueueLoadWork(handle);
    return handle;
}

bool ResourceModule::UnloadResource(ResourceHandle handle)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    const auto it = m_resources.find(handle);
    if (it == m_resources.end())
    {
        return false;
    }

    if (m_gpu && it->second.gpuHandle != 0U)
    {
        m_gpu->DestroyResource(it->second.gpuHandle);
        it->second.gpuHandle = 0U;
        it->second.uploadedByteSize = 0U;
    }

    m_assetLookup.erase(it->second.assetId);

    if (IsLoadInFlight(it->second.state))
    {
        it->second.unloadRequested = true;
        it->second.state = ResourceState::Unloaded;
        it->second.lastError.clear();
        return true;
    }

    m_resources.erase(it);
    return true;
}

bool ResourceModule::SetResourceState(ResourceHandle handle, ResourceState state)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    const auto it = m_resources.find(handle);
    if (it == m_resources.end())
    {
        return false;
    }

    it->second.state = state;
    return true;
}

ResourceState ResourceModule::GetResourceState(ResourceHandle handle) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    const auto it = m_resources.find(handle);
    return it != m_resources.end() ? it->second.state : ResourceState::Unloaded;
}

ResourceKind ResourceModule::GetResourceKind(ResourceHandle handle) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    const auto it = m_resources.find(handle);
    return it != m_resources.end() ? it->second.kind : ResourceKind::Unknown;
}

std::string ResourceModule::GetAssetId(ResourceHandle handle) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    const auto it = m_resources.find(handle);
    return it != m_resources.end() ? it->second.assetId : std::string();
}

std::string ResourceModule::GetResolvedPath(ResourceHandle handle) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    const auto it = m_resources.find(handle);
    return it != m_resources.end() ? it->second.resolvedPath : std::string();
}

std::string ResourceModule::GetLastError(ResourceHandle handle) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    const auto it = m_resources.find(handle);
    return it != m_resources.end() ? it->second.lastError : std::string();
}

std::size_t ResourceModule::GetSourceByteSize(ResourceHandle handle) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    const auto it = m_resources.find(handle);
    return it != m_resources.end() ? it->second.sourceByteSize : 0U;
}

std::string ResourceModule::GetSourceText(ResourceHandle handle) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    const auto it = m_resources.find(handle);
    return it != m_resources.end() ? it->second.sourceText : std::string();
}

GpuResourceHandle ResourceModule::GetGpuResourceHandle(ResourceHandle handle) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    const auto it = m_resources.find(handle);
    return it != m_resources.end() ? it->second.gpuHandle : 0U;
}

std::size_t ResourceModule::GetUploadedByteSize(ResourceHandle handle) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    const auto it = m_resources.find(handle);
    return it != m_resources.end() ? it->second.uploadedByteSize : 0U;
}

bool ResourceModule::IsResourceReady(ResourceHandle handle) const
{
    return GetResourceState(handle) == ResourceState::Ready;
}

std::size_t ResourceModule::GetResourceCountByState(ResourceState state) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    std::size_t count = 0;
    for (const auto& [handle, record] : m_resources)
    {
        (void)handle;
        if (record.state == state)
        {
            ++count;
        }
    }

    return count;
}

std::size_t ResourceModule::GetResourceCount() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_resources.size();
}

bool ResourceModule::IsInitialized() const
{
    return m_initialized;
}

bool ResourceModule::IsStarted() const
{
    return m_started;
}

bool ResourceModule::SetResourceError(ResourceHandle handle, const std::string& error)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    const auto it = m_resources.find(handle);
    if (it == m_resources.end())
    {
        return false;
    }

    it->second.lastError = error;
    it->second.sourceText.clear();
    it->second.gpuHandle = 0;
    it->second.uploadedByteSize = 0;
    return true;
}

bool ResourceModule::SetResourceUploadData(
    ResourceHandle handle,
    GpuResourceHandle gpuHandle,
    std::size_t uploadedByteSize)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    const auto it = m_resources.find(handle);
    if (it == m_resources.end())
    {
        return false;
    }

    it->second.gpuHandle = gpuHandle;
    it->second.uploadedByteSize = uploadedByteSize;
    it->second.sourceText.clear();
    it->second.lastError.clear();
    return true;
}

bool ResourceModule::ResolvePrototypeJson(ResourceHandle handle, const std::string& resolvedPath)
{
    if (!m_files)
    {
        SetResourceError(handle, "Prototype JSON requires FileModule.");
        return SetResourceState(handle, ResourceState::Failed);
    }

    std::string sourceText;
    if (!m_files->ReadJsonFile(resolvedPath, sourceText))
    {
        SetResourceError(handle, std::string("Failed to read prototype JSON resource: ") + resolvedPath);
        if (IsUnloadRequested(handle))
        {
            return FinalizePendingUnload(handle);
        }

        return SetResourceState(handle, ResourceState::Failed);
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        const auto it = m_resources.find(handle);
        if (it == m_resources.end())
        {
            return false;
        }

        it->second.sourceByteSize = sourceText.size();
        it->second.sourceText = std::move(sourceText);
        it->second.lastError.clear();
        it->second.gpuHandle = 0;
        it->second.uploadedByteSize = 0;
    }

    if (IsUnloadRequested(handle))
    {
        return FinalizePendingUnload(handle);
    }

    return SetResourceState(handle, ResourceState::Ready);
}

bool ResourceModule::ResolveAndUpload(ResourceHandle handle, const std::string& resolvedPath)
{
    if (IsUnloadRequested(handle))
    {
        return FinalizePendingUnload(handle);
    }

    if (m_files && !m_files->FileExists(resolvedPath))
    {
        SetResourceError(handle, std::string("Resource file does not exist: ") + resolvedPath);
        if (IsUnloadRequested(handle))
        {
            return FinalizePendingUnload(handle);
        }

        return SetResourceState(handle, ResourceState::Failed);
    }

    SetResourceState(handle, ResourceState::Processing);

    if (GetResourceKind(handle) == ResourceKind::PrototypeJson)
    {
        return ResolvePrototypeJson(handle, resolvedPath);
    }

    std::vector<std::byte> fileBytes;
    if (m_files)
    {
        if (!m_files->ReadBinaryFile(resolvedPath, fileBytes))
        {
            SetResourceError(handle, std::string("Failed to read resource file: ") + resolvedPath);
            if (IsUnloadRequested(handle))
            {
                return FinalizePendingUnload(handle);
            }

            return SetResourceState(handle, ResourceState::Failed);
        }
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        const auto it = m_resources.find(handle);
        if (it == m_resources.end())
        {
            return false;
        }

        it->second.sourceByteSize = fileBytes.size();
        it->second.sourceText.clear();
        it->second.lastError.clear();
        it->second.gpuHandle = 0;
        it->second.uploadedByteSize = 0;
    }

    if (IsUnloadRequested(handle))
    {
        return FinalizePendingUnload(handle);
    }

    if (!m_gpu)
    {
        if (IsUnloadRequested(handle))
        {
            return FinalizePendingUnload(handle);
        }

        return SetResourceState(handle, ResourceState::Ready);
    }

    SetResourceState(handle, ResourceState::Uploading);

    GpuUploadRequest request;
    request.kind = ToGpuResourceKind(GetResourceKind(handle));
    request.data = fileBytes.empty() ? nullptr : fileBytes.data();
    request.size = fileBytes.size();
    request.debugName = GetAssetId(handle);

    const GpuResourceHandle gpuHandle = m_gpu->CreateUploadedResource(request);
    if (gpuHandle == 0U)
    {
        SetResourceError(handle, std::string("Gpu upload failed for resource: ") + resolvedPath);
        if (IsUnloadRequested(handle))
        {
            return FinalizePendingUnload(handle);
        }

        return SetResourceState(handle, ResourceState::Failed);
    }

    if (IsUnloadRequested(handle))
    {
        return FinalizePendingUnload(handle, gpuHandle);
    }

    SetResourceUploadData(handle, gpuHandle, m_gpu->GetResourceSize(gpuHandle));
    return SetResourceState(handle, ResourceState::Ready);
}

bool ResourceModule::QueueLoadWork(ResourceHandle handle)
{
    if (GetResourceKind(handle) == ResourceKind::PrototypeJson)
    {
        std::string assetId;
        std::string resolvedPath;
        if (!TryGetLoadRequest(handle, assetId, resolvedPath) || assetId.empty())
        {
            SetResourceError(handle, "Prototype JSON resource request has an empty asset id.");
            return SetResourceState(handle, ResourceState::Failed);
        }

        SetResourceState(handle, ResourceState::Loading);
        return ResolveAndUpload(handle, resolvedPath);
    }

    if (!m_jobs || !m_jobs->IsStarted())
    {
        std::string assetId;
        std::string resolvedPath;
        if (!TryGetLoadRequest(handle, assetId, resolvedPath) || assetId.empty())
        {
            SetResourceError(handle, "Resource request has an empty asset id.");
            return SetResourceState(handle, ResourceState::Failed);
        }

        SetResourceState(handle, ResourceState::Loading);
        return ResolveAndUpload(handle, resolvedPath);
    }

    m_jobs->Submit([this, handle]()
    {
        std::string assetId;
        std::string resolvedPath;
        if (!TryGetLoadRequest(handle, assetId, resolvedPath) || assetId.empty())
        {
            FinalizePendingUnload(handle);
            return;
        }

        SetResourceState(handle, ResourceState::Loading);

        ResolveAndUpload(handle, resolvedPath);
    });

    return true;
}

bool ResourceModule::IsLoadInFlight(ResourceState state) const
{
    return state == ResourceState::Queued ||
        state == ResourceState::Loading ||
        state == ResourceState::Processing ||
        state == ResourceState::Uploading;
}

bool ResourceModule::IsUnloadRequested(ResourceHandle handle) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    const auto it = m_resources.find(handle);
    return it == m_resources.end() || it->second.unloadRequested;
}

bool ResourceModule::TryGetLoadRequest(
    ResourceHandle handle,
    std::string& outAssetId,
    std::string& outResolvedPath) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    const auto it = m_resources.find(handle);
    if (it == m_resources.end() || it->second.unloadRequested)
    {
        outAssetId.clear();
        outResolvedPath.clear();
        return false;
    }

    outAssetId = it->second.assetId;
    outResolvedPath = it->second.resolvedPath;
    return true;
}

bool ResourceModule::FinalizePendingUnload(ResourceHandle handle, GpuResourceHandle uploadedHandle)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    const auto it = m_resources.find(handle);
    if (it == m_resources.end())
    {
        if (uploadedHandle != 0U && m_gpu)
        {
            m_gpu->DestroyResource(uploadedHandle);
        }

        return false;
    }

    if (uploadedHandle != 0U && m_gpu)
    {
        m_gpu->DestroyResource(uploadedHandle);
    }

    if (it->second.unloadRequested)
    {
        m_resources.erase(it);
        return true;
    }

    return false;
}

GpuResourceKind ResourceModule::ToGpuResourceKind(ResourceKind kind)
{
    switch (kind)
    {
    case ResourceKind::Texture:
        return GpuResourceKind::Texture;
    case ResourceKind::Mesh:
        return GpuResourceKind::MeshData;
    case ResourceKind::Material:
        return GpuResourceKind::MaterialData;
    case ResourceKind::Shader:
        return GpuResourceKind::ShaderBlob;
    default:
        return GpuResourceKind::Unknown;
    }
}
