#include "resource_module.hpp"

#include "../../core/engine_context.hpp"

const char* ResourceModule::GetName() const
{
    return "Resource";
}

bool ResourceModule::Initialize(EngineContext& context)
{
    (void)context;

    if (m_initialized)
    {
        return true;
    }

    m_resources.clear();
    m_assetLookup.clear();
    m_nextHandle = 1U;
    m_initialized = true;
    return true;
}

void ResourceModule::Startup(EngineContext& context)
{
    (void)context;

    if (!m_initialized || m_started)
    {
        return;
    }

    m_started = true;
}

void ResourceModule::Update(EngineContext& context, float deltaTime)
{
    (void)context;
    (void)deltaTime;
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
    m_started = false;
    m_initialized = false;
}

ResourceHandle ResourceModule::RequestResource(const std::string& assetId, ResourceKind kind)
{
    if (!m_started || assetId.empty())
    {
        return 0U;
    }

    const auto existing = m_assetLookup.find(assetId);
    if (existing != m_assetLookup.end())
    {
        return existing->second;
    }

    const ResourceHandle handle = m_nextHandle++;

    ResourceRecord record;
    record.handle = handle;
    record.kind = kind;
    record.state = ResourceState::Queued;
    record.assetId = assetId;

    m_resources.emplace(handle, record);
    m_assetLookup.emplace(assetId, handle);
    return handle;
}

bool ResourceModule::UnloadResource(ResourceHandle handle)
{
    const auto it = m_resources.find(handle);
    if (it == m_resources.end())
    {
        return false;
    }

    m_assetLookup.erase(it->second.assetId);
    m_resources.erase(it);
    return true;
}

bool ResourceModule::SetResourceState(ResourceHandle handle, ResourceState state)
{
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
    const auto it = m_resources.find(handle);
    return it != m_resources.end() ? it->second.state : ResourceState::Unloaded;
}

ResourceKind ResourceModule::GetResourceKind(ResourceHandle handle) const
{
    const auto it = m_resources.find(handle);
    return it != m_resources.end() ? it->second.kind : ResourceKind::Unknown;
}

std::string ResourceModule::GetAssetId(ResourceHandle handle) const
{
    const auto it = m_resources.find(handle);
    return it != m_resources.end() ? it->second.assetId : std::string();
}

bool ResourceModule::IsResourceReady(ResourceHandle handle) const
{
    return GetResourceState(handle) == ResourceState::Ready;
}

std::size_t ResourceModule::GetResourceCount() const
{
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
