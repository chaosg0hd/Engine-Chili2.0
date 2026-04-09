#include "dx11_render_backend.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "../../../core/engine_context.hpp"
#include "../../logger/logger_module.hpp"

#include <d3d11.h>
#include <dxgi.h>
#include <windows.h>

#include <sstream>
#include <string>

namespace
{
    template<typename T>
    void SafeRelease(T*& value)
    {
        if (value)
        {
            value->Release();
            value = nullptr;
        }
    }
}

Dx11RenderBackend::Dx11RenderBackend() = default;

Dx11RenderBackend::~Dx11RenderBackend()
{
    Shutdown();
}

const char* Dx11RenderBackend::GetName() const
{
    return "Dx11RenderBackend";
}

bool Dx11RenderBackend::Initialize(
    EngineContext& context,
    void* nativeWindowHandle,
    std::uint32_t width,
    std::uint32_t height)
{
    if (m_initialized)
    {
        return true;
    }

    if (!nativeWindowHandle || width == 0 || height == 0)
    {
        return false;
    }

    m_logger = context.Logger;
    m_windowHandle = static_cast<HWND>(nativeWindowHandle);

    if (!CreateDeviceAndSwapChain(m_windowHandle, width, height))
    {
        Shutdown();
        return false;
    }

    if (!CreateBackBufferResources(width, height))
    {
        Shutdown();
        return false;
    }

    m_width = width;
    m_height = height;
    m_initialized = true;

    std::ostringstream message;
    message << "Render backend initialized: Dx11RenderBackend | width=" << m_width
            << " | height=" << m_height;
    LogInfo(message.str());
    return true;
}

void Dx11RenderBackend::Shutdown()
{
    if (!m_device && !m_context && !m_swapChain && !m_renderTargetView && !m_depthStencilView)
    {
        m_initialized = false;
        m_windowHandle = nullptr;
        m_width = 0;
        m_height = 0;
        m_logger = nullptr;
        return;
    }

    if (m_context)
    {
        m_context->OMSetRenderTargets(0, nullptr, nullptr);
        m_context->ClearState();
    }

    ReleaseBackBufferResources();
    SafeRelease(m_swapChain);
    SafeRelease(m_context);
    SafeRelease(m_device);

    LogInfo("Render backend shutdown: Dx11RenderBackend");

    m_initialized = false;
    m_windowHandle = nullptr;
    m_width = 0;
    m_height = 0;
    m_logger = nullptr;
}

void Dx11RenderBackend::BeginFrame(const RenderFrameContext& frameContext)
{
    if (!m_initialized || !m_context || !m_renderTargetView)
    {
        return;
    }

    BindDefaultTargets();
    SetViewport(frameContext.viewport.width, frameContext.viewport.height);

    const float clearColor[4] =
    {
        frameContext.clearColor.r,
        frameContext.clearColor.g,
        frameContext.clearColor.b,
        frameContext.clearColor.a
    };

    m_context->ClearRenderTargetView(m_renderTargetView, clearColor);
    if (m_depthStencilView)
    {
        m_context->ClearDepthStencilView(m_depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
    }
}

void Dx11RenderBackend::Render(const RenderFramePrototype& frame)
{
    (void)frame;
}

void Dx11RenderBackend::EndFrame()
{
}

void Dx11RenderBackend::Present()
{
    if (!m_initialized || !m_swapChain)
    {
        return;
    }

    const HRESULT result = m_swapChain->Present(0, 0);
    if (FAILED(result))
    {
        std::ostringstream message;
        message << "Dx11RenderBackend present failed. HRESULT=0x" << std::hex << static_cast<unsigned long>(result);
        LogError(message.str());
    }
}

void Dx11RenderBackend::Resize(std::uint32_t width, std::uint32_t height)
{
    if (!m_initialized || !m_swapChain || width == 0 || height == 0)
    {
        return;
    }

    if (width == m_width && height == m_height)
    {
        return;
    }

    if (m_context)
    {
        m_context->OMSetRenderTargets(0, nullptr, nullptr);
    }

    ReleaseBackBufferResources();

    const HRESULT resizeResult = m_swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    if (FAILED(resizeResult))
    {
        std::ostringstream message;
        message << "Dx11RenderBackend resize failed. HRESULT=0x" << std::hex << static_cast<unsigned long>(resizeResult);
        LogError(message.str());
        return;
    }

    if (!CreateBackBufferResources(width, height))
    {
        LogError("Dx11RenderBackend failed to recreate backbuffer resources after resize.");
        return;
    }

    m_width = width;
    m_height = height;

    std::ostringstream message;
    message << "Render backend resized: Dx11RenderBackend | width=" << m_width
            << " | height=" << m_height;
    LogInfo(message.str());
}

bool Dx11RenderBackend::CreateDeviceAndSwapChain(HWND nativeWindowHandle, std::uint32_t width, std::uint32_t height)
{
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferDesc.Width = width;
    swapChainDesc.BufferDesc.Height = height;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.OutputWindow = nativeWindowHandle;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    swapChainDesc.Flags = 0;

    UINT creationFlags = 0;
#ifdef _DEBUG
    creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    const D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
    };
    const D3D_FEATURE_LEVEL fallbackFeatureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
    };

    D3D_FEATURE_LEVEL selectedFeatureLevel = D3D_FEATURE_LEVEL_11_0;
    HRESULT result = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        creationFlags,
        featureLevels,
        static_cast<UINT>(sizeof(featureLevels) / sizeof(featureLevels[0])),
        D3D11_SDK_VERSION,
        &swapChainDesc,
        &m_swapChain,
        &m_device,
        &selectedFeatureLevel,
        &m_context);

    if (result == E_INVALIDARG)
    {
        result = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            creationFlags,
            fallbackFeatureLevels,
            static_cast<UINT>(sizeof(fallbackFeatureLevels) / sizeof(fallbackFeatureLevels[0])),
            D3D11_SDK_VERSION,
            &swapChainDesc,
            &m_swapChain,
            &m_device,
            &selectedFeatureLevel,
            &m_context);
    }

#ifdef _DEBUG
    if (FAILED(result) && (creationFlags & D3D11_CREATE_DEVICE_DEBUG) != 0U)
    {
        creationFlags &= ~D3D11_CREATE_DEVICE_DEBUG;
        result = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            creationFlags,
            featureLevels,
            static_cast<UINT>(sizeof(featureLevels) / sizeof(featureLevels[0])),
            D3D11_SDK_VERSION,
            &swapChainDesc,
            &m_swapChain,
            &m_device,
            &selectedFeatureLevel,
            &m_context);

        if (result == E_INVALIDARG)
        {
            result = D3D11CreateDeviceAndSwapChain(
                nullptr,
                D3D_DRIVER_TYPE_HARDWARE,
                nullptr,
                creationFlags,
                fallbackFeatureLevels,
                static_cast<UINT>(sizeof(fallbackFeatureLevels) / sizeof(fallbackFeatureLevels[0])),
                D3D11_SDK_VERSION,
                &swapChainDesc,
                &m_swapChain,
                &m_device,
                &selectedFeatureLevel,
                &m_context);
        }
    }
#endif

    if (FAILED(result))
    {
        std::ostringstream message;
        message << "Dx11RenderBackend failed to create device and swap chain. HRESULT=0x"
                << std::hex << static_cast<unsigned long>(result);
        LogError(message.str());
        return false;
    }

    return true;
}

bool Dx11RenderBackend::CreateBackBufferResources(std::uint32_t width, std::uint32_t height)
{
    if (!m_device || !m_context || !m_swapChain || width == 0 || height == 0)
    {
        return false;
    }

    ID3D11Texture2D* backBuffer = nullptr;
    HRESULT result = m_swapChain->GetBuffer(0, IID_ID3D11Texture2D, reinterpret_cast<void**>(&backBuffer));
    if (FAILED(result))
    {
        std::ostringstream message;
        message << "Dx11RenderBackend failed to acquire swap-chain backbuffer. HRESULT=0x"
                << std::hex << static_cast<unsigned long>(result);
        LogError(message.str());
        return false;
    }

    result = m_device->CreateRenderTargetView(backBuffer, nullptr, &m_renderTargetView);
    SafeRelease(backBuffer);
    if (FAILED(result))
    {
        std::ostringstream message;
        message << "Dx11RenderBackend failed to create render target view. HRESULT=0x"
                << std::hex << static_cast<unsigned long>(result);
        LogError(message.str());
        return false;
    }

    D3D11_TEXTURE2D_DESC depthDesc = {};
    depthDesc.Width = width;
    depthDesc.Height = height;
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1;
    depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.SampleDesc.Quality = 0;
    depthDesc.Usage = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    result = m_device->CreateTexture2D(&depthDesc, nullptr, &m_depthStencilBuffer);
    if (FAILED(result))
    {
        std::ostringstream message;
        message << "Dx11RenderBackend failed to create depth stencil buffer. HRESULT=0x"
                << std::hex << static_cast<unsigned long>(result);
        LogError(message.str());
        return false;
    }

    result = m_device->CreateDepthStencilView(m_depthStencilBuffer, nullptr, &m_depthStencilView);
    if (FAILED(result))
    {
        std::ostringstream message;
        message << "Dx11RenderBackend failed to create depth stencil view. HRESULT=0x"
                << std::hex << static_cast<unsigned long>(result);
        LogError(message.str());
        return false;
    }

    BindDefaultTargets();
    SetViewport(width, height);
    return true;
}

void Dx11RenderBackend::ReleaseBackBufferResources()
{
    SafeRelease(m_depthStencilView);
    SafeRelease(m_depthStencilBuffer);
    SafeRelease(m_renderTargetView);
}

void Dx11RenderBackend::BindDefaultTargets()
{
    if (!m_context || !m_renderTargetView)
    {
        return;
    }

    m_context->OMSetRenderTargets(
        1,
        &m_renderTargetView,
        m_depthStencilView);
}

void Dx11RenderBackend::SetViewport(std::uint32_t width, std::uint32_t height)
{
    if (!m_context || width == 0 || height == 0)
    {
        return;
    }

    D3D11_VIEWPORT viewport = {};
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width = static_cast<float>(width);
    viewport.Height = static_cast<float>(height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    m_context->RSSetViewports(1, &viewport);
}

void Dx11RenderBackend::LogInfo(const std::string& message) const
{
    if (m_logger)
    {
        m_logger->Info(message);
    }
}

void Dx11RenderBackend::LogError(const std::string& message) const
{
    if (m_logger)
    {
        m_logger->Error(message);
    }
}
