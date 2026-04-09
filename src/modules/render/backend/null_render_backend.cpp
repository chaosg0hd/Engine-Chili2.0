#include "null_render_backend.hpp"

#include "../../../core/engine_context.hpp"
#include "../../logger/logger_module.hpp"

#include <string>

NullRenderBackend::NullRenderBackend() = default;

const char* NullRenderBackend::GetName() const
{
    return "NullRenderBackend";
}

bool NullRenderBackend::Initialize(
    EngineContext& context,
    void* nativeWindowHandle,
    std::uint32_t width,
    std::uint32_t height)
{
    (void)nativeWindowHandle;

    if (m_initialized)
    {
        return true;
    }

    m_logger = context.Logger;
    m_width = width;
    m_height = height;
    m_initialized = true;

    if (m_logger)
    {
        m_logger->Info("Render backend initialized: NullRenderBackend");
    }

    return true;
}

void NullRenderBackend::Shutdown()
{
    if (!m_initialized)
    {
        return;
    }

    if (m_logger)
    {
        m_logger->Info("Render backend shutdown: NullRenderBackend");
    }

    m_initialized = false;
    m_width = 0;
    m_height = 0;
    m_logger = nullptr;
}

void NullRenderBackend::BeginFrame(const RenderFrameContext& frameContext)
{
    (void)frameContext;
}

void NullRenderBackend::Render(const RenderFrameData& frame)
{
    (void)frame;
}

void NullRenderBackend::EndFrame()
{
}

void NullRenderBackend::Present()
{
}

void NullRenderBackend::Resize(std::uint32_t width, std::uint32_t height)
{
    m_width = width;
    m_height = height;

    if (m_logger)
    {
        m_logger->Info(
            "Render backend resized: NullRenderBackend | width=" +
            std::to_string(m_width) +
            " | height=" +
            std::to_string(m_height));
    }
}
