#include "render_module.hpp"

#include "../../core/engine_context.hpp"
#include "../platform/platform_module.hpp"

#include <algorithm>
#include <cstdlib>
#include <windows.h>

RenderModule::RenderModule() = default;

const char* RenderModule::GetName() const
{
    return "Render";
}

bool RenderModule::Initialize(EngineContext& context)
{
    (void)context;

    if (m_initialized)
    {
        return true;
    }

    m_initialized = true;
    return true;
}

void RenderModule::Startup(EngineContext& context)
{
    (void)context;

    if (!m_initialized || m_started)
    {
        return;
    }

    ResizeToClientArea();
    m_started = true;
}

void RenderModule::Update(EngineContext& context, float deltaTime)
{
    (void)context;
    (void)deltaTime;

    if (!m_started)
    {
        return;
    }

    ResizeToClientArea();
}

void RenderModule::Shutdown(EngineContext& context)
{
    (void)context;

    if (!m_initialized)
    {
        return;
    }

    m_backbuffer.clear();
    m_width = 0;
    m_height = 0;
    m_platform = nullptr;

    m_started = false;
    m_initialized = false;
}

void RenderModule::SetPlatformModule(PlatformModule* platform)
{
    m_platform = platform;
}

bool RenderModule::ResizeToClientArea()
{
    if (!m_platform)
    {
        return false;
    }

    const HWND hwnd = m_platform->GetWindowHandle();
    if (!hwnd)
    {
        return false;
    }

    RECT clientRect = {};
    if (!GetClientRect(hwnd, &clientRect))
    {
        return false;
    }

    const int width = clientRect.right - clientRect.left;
    const int height = clientRect.bottom - clientRect.top;

    if (width <= 0 || height <= 0)
    {
        return false;
    }

    return ResizeBackbuffer(width, height);
}

bool RenderModule::ResizeBackbuffer(int width, int height)
{
    if (width == m_width && height == m_height)
    {
        return true;
    }

    m_width = width;
    m_height = height;
    m_backbuffer.assign(
        static_cast<std::size_t>(m_width) * static_cast<std::size_t>(m_height),
        0x00000000u);
    return true;
}

void RenderModule::Clear(std::uint32_t color)
{
    if (m_backbuffer.empty())
    {
        return;
    }

    std::fill(m_backbuffer.begin(), m_backbuffer.end(), color);
}

void RenderModule::PutPixel(int x, int y, std::uint32_t color)
{
    if (x < 0 || y < 0 || x >= m_width || y >= m_height)
    {
        return;
    }

    const std::size_t index =
        static_cast<std::size_t>(y) * static_cast<std::size_t>(m_width) +
        static_cast<std::size_t>(x);
    m_backbuffer[index] = color;
}

void RenderModule::DrawLine(int x0, int y0, int x1, int y1, std::uint32_t color)
{
    int deltaX = std::abs(x1 - x0);
    const int stepX = (x0 < x1) ? 1 : -1;
    int deltaY = -std::abs(y1 - y0);
    const int stepY = (y0 < y1) ? 1 : -1;
    int error = deltaX + deltaY;

    while (true)
    {
        PutPixel(x0, y0, color);

        if (x0 == x1 && y0 == y1)
        {
            break;
        }

        const int doubledError = 2 * error;
        if (doubledError >= deltaY)
        {
            error += deltaY;
            x0 += stepX;
        }

        if (doubledError <= deltaX)
        {
            error += deltaX;
            y0 += stepY;
        }
    }
}

void RenderModule::DrawRect(int x, int y, int width, int height, std::uint32_t color)
{
    if (width <= 0 || height <= 0)
    {
        return;
    }

    const int right = x + width - 1;
    const int bottom = y + height - 1;

    DrawLine(x, y, right, y, color);
    DrawLine(x, bottom, right, bottom, color);
    DrawLine(x, y, x, bottom, color);
    DrawLine(right, y, right, bottom, color);
}

void RenderModule::FillRect(int x, int y, int width, int height, std::uint32_t color)
{
    if (width <= 0 || height <= 0)
    {
        return;
    }

    for (int row = 0; row < height; ++row)
    {
        const int drawY = y + row;
        for (int column = 0; column < width; ++column)
        {
            PutPixel(x + column, drawY, color);
        }
    }
}

void RenderModule::DrawGrid(int cellSize, std::uint32_t color)
{
    if (cellSize <= 0 || m_width <= 0 || m_height <= 0)
    {
        return;
    }

    for (int x = 0; x < m_width; x += cellSize)
    {
        DrawLine(x, 0, x, m_height - 1, color);
    }

    for (int y = 0; y < m_height; y += cellSize)
    {
        DrawLine(0, y, m_width - 1, y, color);
    }
}

void RenderModule::DrawCrosshair(int x, int y, int size, std::uint32_t color)
{
    if (size <= 0)
    {
        return;
    }

    DrawLine(x - size, y, x + size, y, color);
    DrawLine(x, y - size, x, y + size, color);
}

void RenderModule::Present()
{
    if (!m_platform || m_backbuffer.empty())
    {
        return;
    }

    const HWND hwnd = m_platform->GetWindowHandle();
    if (!hwnd)
    {
        return;
    }

    HDC dc = GetDC(hwnd);
    if (!dc)
    {
        return;
    }

    BITMAPINFO bitmapInfo = {};
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = m_width;
    bitmapInfo.bmiHeader.biHeight = -m_height;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    StretchDIBits(
        dc,
        0, 0, m_width, m_height,
        0, 0, m_width, m_height,
        m_backbuffer.data(),
        &bitmapInfo,
        DIB_RGB_COLORS,
        SRCCOPY);

    ReleaseDC(hwnd, dc);
}

int RenderModule::GetBackbufferWidth() const
{
    return m_width;
}

int RenderModule::GetBackbufferHeight() const
{
    return m_height;
}

double RenderModule::GetAspectRatio() const
{
    if (m_height <= 0)
    {
        return 0.0;
    }

    return static_cast<double>(m_width) / static_cast<double>(m_height);
}

bool RenderModule::IsInitialized() const
{
    return m_initialized;
}

bool RenderModule::IsStarted() const
{
    return m_started;
}
