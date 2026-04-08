#include "render_module.hpp"

#include "../../core/engine_context.hpp"
#include "../gpu/igpu_service.hpp"
#include "../logger/logger_module.hpp"

RenderModule::RenderModule() = default;
RenderModule::~RenderModule() = default;

const char* RenderModule::GetName() const
{
    return "Render";
}

bool RenderModule::Initialize(EngineContext& context)
{
    if (m_initialized)
    {
        return true;
    }

    if (!m_gpu)
    {
        m_gpu = context.Gpu;
    }

    m_initialized = true;

    if (context.Logger)
    {
        context.Logger->Info("RenderModule initialized.");
    }

    return true;
}

void RenderModule::Startup(EngineContext& context)
{
    if (!m_initialized || m_started)
    {
        return;
    }

    if (!m_gpu)
    {
        m_gpu = context.Gpu;
    }

    m_started = true;

    if (context.Logger)
    {
        context.Logger->Info("RenderModule started.");
    }
}

void RenderModule::Update(EngineContext& context, float deltaTime)
{
    if (!m_gpu)
    {
        m_gpu = context.Gpu;
    }

    if (!m_initialized || !m_started || !m_gpu)
    {
        return;
    }

    m_gpu->RenderFrame(m_scene, m_clearColor, deltaTime);
}

void RenderModule::Shutdown(EngineContext& context)
{
    if (!m_initialized)
    {
        return;
    }

    m_scene = RenderScene{};
    m_clearColor = RenderClearColor{};
    m_legacyCompatibilityCommandCount = 0;
    m_gpu = nullptr;

    m_started = false;
    m_initialized = false;

    if (context.Logger)
    {
        context.Logger->Info("RenderModule shutdown.");
    }
}

void RenderModule::SubmitScene(const RenderScene& scene)
{
    m_scene = scene;
}

void RenderModule::Resize(std::uint32_t width, std::uint32_t height)
{
    if (m_gpu)
    {
        m_gpu->Resize(width, height);
    }
}

bool RenderModule::ResizeToClientArea()
{
    return m_gpu ? m_gpu->ResizeToSurface() : false;
}

RenderClearColor RenderModule::ToClearColor(std::uint32_t color)
{
    RenderClearColor clearColor;
    clearColor.a = static_cast<float>((color >> 24) & 0xFFu) / 255.0f;
    clearColor.r = static_cast<float>((color >> 16) & 0xFFu) / 255.0f;
    clearColor.g = static_cast<float>((color >> 8) & 0xFFu) / 255.0f;
    clearColor.b = static_cast<float>(color & 0xFFu) / 255.0f;
    return clearColor;
}

void RenderModule::Clear(std::uint32_t color)
{
    m_clearColor = ToClearColor(color);
}

void RenderModule::PutPixel(int x, int y, std::uint32_t color)
{
    (void)x;
    (void)y;
    (void)color;
    ++m_legacyCompatibilityCommandCount;
}

void RenderModule::DrawLine(int x0, int y0, int x1, int y1, std::uint32_t color)
{
    (void)x0;
    (void)y0;
    (void)x1;
    (void)y1;
    (void)color;
    ++m_legacyCompatibilityCommandCount;
}

void RenderModule::DrawRect(int x, int y, int width, int height, std::uint32_t color)
{
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)color;
    ++m_legacyCompatibilityCommandCount;
}

void RenderModule::FillRect(int x, int y, int width, int height, std::uint32_t color)
{
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)color;
    ++m_legacyCompatibilityCommandCount;
}

void RenderModule::DrawGrid(int cellSize, std::uint32_t color)
{
    (void)cellSize;
    (void)color;
    ++m_legacyCompatibilityCommandCount;
}

void RenderModule::DrawCrosshair(int x, int y, int size, std::uint32_t color)
{
    (void)x;
    (void)y;
    (void)size;
    (void)color;
    ++m_legacyCompatibilityCommandCount;
}

void RenderModule::Present()
{
    // Presentation belongs to the GPU-owned frame flow now.
    // Keep this as a no-op only for older engine-facing compatibility calls.
}

int RenderModule::GetBackbufferWidth() const
{
    return m_gpu ? m_gpu->GetBackbufferWidth() : 0;
}

int RenderModule::GetBackbufferHeight() const
{
    return m_gpu ? m_gpu->GetBackbufferHeight() : 0;
}

double RenderModule::GetAspectRatio() const
{
    return m_gpu ? m_gpu->GetAspectRatio() : 0.0;
}

std::size_t RenderModule::GetSubmittedItemCount() const
{
    return m_scene.objects.size();
}

std::size_t RenderModule::GetLegacyCompatibilityCommandCount() const
{
    return m_legacyCompatibilityCommandCount;
}

bool RenderModule::IsInitialized() const
{
    return m_initialized;
}

bool RenderModule::IsStarted() const
{
    return m_started;
}
