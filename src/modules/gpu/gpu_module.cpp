#include "gpu_module.hpp"

#include "../../core/engine_context.hpp"
#include "../logger/logger_module.hpp"
#include "../platform/platform_module.hpp"
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

void GpuModule::RenderFrame(const RenderScene& scene, const RenderClearColor& clearColor, float deltaTime)
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
    m_backend->Render(scene);
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
