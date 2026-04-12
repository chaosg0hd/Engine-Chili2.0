#include "gpu_module.hpp"

#include "../../core/engine_context.hpp"
#include "../logger/logger_module.hpp"
#include "../platform/iplatform_service.hpp"
#include "../render/backend/irender_backend.hpp"
#include "../render/render_backend_factory.hpp"

GpuModule::GpuModule() = default;

GpuModule::~GpuModule()
{
    DestroyBackend();
}

const char* GpuModule::GetName() const
{
    return "Gpu";
}

bool GpuModule::Initialize(EngineContext& context)
{
    if (m_initialized)
    {
        return true;
    }

    if (!m_platform)
    {
        m_platform = context.Platform;
    }

    if (!CreateBackend(context))
    {
        if (context.Logger)
        {
            context.Logger->Error("GpuModule failed to create backend.");
        }

        return false;
    }

    m_initialized = true;

    if (context.Logger)
    {
        context.Logger->Info("GpuModule initialized.");
    }

    return true;
}

void GpuModule::Startup(EngineContext& context)
{
    if (!m_initialized || m_started)
    {
        return;
    }

    if (!m_platform)
    {
        m_platform = context.Platform;
    }

    m_started = true;

    if (context.Logger)
    {
        context.Logger->Info("GpuModule started.");
    }
}

void GpuModule::Update(EngineContext& context, float deltaTime)
{
    (void)deltaTime;

    if (!m_platform)
    {
        m_platform = context.Platform;
    }

    if (!m_initialized || !m_started || !m_backend)
    {
        return;
    }

    UpdateViewportFromPlatform();
}

void GpuModule::Shutdown(EngineContext& context)
{
    if (!m_initialized)
    {
        return;
    }

    DestroyBackend();
    m_viewport = RenderViewport{};
    m_frameIndex = 0;
    {
        std::lock_guard<std::mutex> lock(m_resourceMutex);
        m_resources.clear();
        m_nextResourceHandle = 1U;
    }

    m_started = false;
    m_initialized = false;

    if (context.Logger)
    {
        context.Logger->Info("GpuModule shutdown.");
    }
}

void GpuModule::SetBackendType(RenderBackendType type)
{
    m_backendType = type;
}

RenderBackendType GpuModule::GetBackendType() const
{
    return m_backendType;
}

const char* GpuModule::GetBackendName() const
{
    return m_backend ? m_backend->GetName() : "Unavailable";
}

void GpuModule::RenderFrame(const RenderFrameData& frame, const RenderClearColor& clearColor, float deltaTime)
{
    if (!m_initialized || !m_started || !m_backend)
    {
        return;
    }

    UpdateViewportFromPlatform();

    RenderFrameContext frameContext;
    frameContext.deltaTime = static_cast<double>(deltaTime);
    frameContext.frameIndex = m_frameIndex++;
    frameContext.viewport = m_viewport;
    frameContext.clearColor = clearColor;

    m_backend->BeginFrame(frameContext);
    m_backend->Render(frame);
    m_backend->EndFrame();
    m_backend->Present();
}

void GpuModule::Resize(std::uint32_t width, std::uint32_t height)
{
    m_viewport.width = width;
    m_viewport.height = height;

    if (m_backend)
    {
        m_backend->Resize(width, height);
    }
}

bool GpuModule::ResizeToSurface()
{
    const RenderViewport previousViewport = m_viewport;
    UpdateViewportFromPlatform();
    return previousViewport.width != m_viewport.width ||
        previousViewport.height != m_viewport.height;
}

int GpuModule::GetBackbufferWidth() const
{
    return static_cast<int>(m_viewport.width);
}

int GpuModule::GetBackbufferHeight() const
{
    return static_cast<int>(m_viewport.height);
}

double GpuModule::GetAspectRatio() const
{
    if (m_viewport.height == 0)
    {
        return 0.0;
    }

    return static_cast<double>(m_viewport.width) / static_cast<double>(m_viewport.height);
}

GpuResourceHandle GpuModule::CreateUploadedResource(const GpuUploadRequest& request)
{
    if (!m_initialized || !m_started || request.data == nullptr || request.size == 0)
    {
        return 0U;
    }

    std::lock_guard<std::mutex> lock(m_resourceMutex);

    const GpuResourceHandle handle = m_nextResourceHandle++;
    GpuResourceRecord record;
    record.handle = handle;
    record.kind = request.kind;
    record.size = request.size;
    record.debugName = request.debugName;
    m_resources.emplace(handle, std::move(record));
    return handle;
}

bool GpuModule::DestroyResource(GpuResourceHandle handle)
{
    std::lock_guard<std::mutex> lock(m_resourceMutex);
    return m_resources.erase(handle) > 0;
}

bool GpuModule::IsResourceValid(GpuResourceHandle handle) const
{
    std::lock_guard<std::mutex> lock(m_resourceMutex);
    return m_resources.find(handle) != m_resources.end();
}

GpuResourceKind GpuModule::GetResourceKind(GpuResourceHandle handle) const
{
    std::lock_guard<std::mutex> lock(m_resourceMutex);
    const auto it = m_resources.find(handle);
    return it != m_resources.end() ? it->second.kind : GpuResourceKind::Unknown;
}

std::size_t GpuModule::GetResourceSize(GpuResourceHandle handle) const
{
    std::lock_guard<std::mutex> lock(m_resourceMutex);
    const auto it = m_resources.find(handle);
    return it != m_resources.end() ? it->second.size : 0U;
}

std::size_t GpuModule::GetResourceCount() const
{
    std::lock_guard<std::mutex> lock(m_resourceMutex);
    return m_resources.size();
}

std::size_t GpuModule::GetTotalResourceBytes() const
{
    std::lock_guard<std::mutex> lock(m_resourceMutex);

    std::size_t totalBytes = 0;
    for (const auto& entry : m_resources)
    {
        totalBytes += entry.second.size;
    }

    return totalBytes;
}

bool GpuModule::SupportsGpuBuffers() const
{
    return m_initialized && m_started && m_backend && m_backend->SupportsComputeDispatch();
}

bool GpuModule::SupportsComputeDispatch() const
{
    return m_initialized && m_started && m_backend && m_backend->SupportsComputeDispatch();
}

bool GpuModule::SubmitGpuTask(const GpuTaskDesc& task)
{
    return m_initialized && m_started && m_backend && m_backend->SubmitGpuTask(task);
}

void GpuModule::WaitForGpuIdle()
{
    if (m_backend)
    {
        m_backend->WaitForGpuIdle();
    }
}

bool GpuModule::IsInitialized() const
{
    return m_initialized;
}

bool GpuModule::IsStarted() const
{
    return m_started;
}

bool GpuModule::CreateBackend(EngineContext& context)
{
    m_backend = RenderBackendFactory::Create(m_backendType);
    if (!m_backend)
    {
        return false;
    }

    if (!m_platform)
    {
        if (context.Logger)
        {
            context.Logger->Error("GpuModule requires PlatformModule.");
        }

        m_backend.reset();
        return false;
    }

    const RenderSurface surface = m_platform->GetRenderSurface();
    if (!surface.nativeHandle || surface.width == 0 || surface.height == 0)
    {
        if (context.Logger)
        {
            context.Logger->Error("GpuModule requires a valid RenderSurface.");
        }

        m_backend.reset();
        return false;
    }

    m_viewport.width = surface.width;
    m_viewport.height = surface.height;

    if (!m_backend->Initialize(context, surface.nativeHandle, m_viewport.width, m_viewport.height))
    {
        m_backend.reset();
        return false;
    }

    return true;
}

void GpuModule::DestroyBackend()
{
    if (m_backend)
    {
        m_backend->Shutdown();
        m_backend.reset();
    }
}

void GpuModule::UpdateViewportFromPlatform()
{
    if (!m_platform)
    {
        return;
    }

    const RenderSurface surface = m_platform->GetRenderSurface();
    if (!surface.nativeHandle)
    {
        return;
    }

    const std::uint32_t width = surface.width;
    const std::uint32_t height = surface.height;
    if (width == 0 || height == 0)
    {
        return;
    }

    if (width == m_viewport.width && height == m_viewport.height)
    {
        return;
    }

    Resize(width, height);
}
