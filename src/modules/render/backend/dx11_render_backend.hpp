#pragma once

#include "irender_backend.hpp"

#include <cstdint>
#include <string>

struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain;
struct ID3D11RenderTargetView;
struct ID3D11Texture2D;
struct ID3D11DepthStencilView;
struct HWND__;
using HWND = HWND__*;

class LoggerModule;

class Dx11RenderBackend : public IRenderBackend
{
public:
    Dx11RenderBackend();
    ~Dx11RenderBackend() override;

    const char* GetName() const override;

    bool Initialize(
        EngineContext& context,
        void* nativeWindowHandle,
        std::uint32_t width,
        std::uint32_t height) override;
    void Shutdown() override;

    void BeginFrame(const RenderFrameContext& frameContext) override;
    void Render(const RenderScene& scene) override;
    void EndFrame() override;
    void Present() override;

    void Resize(std::uint32_t width, std::uint32_t height) override;

private:
    bool CreateDeviceAndSwapChain(HWND nativeWindowHandle, std::uint32_t width, std::uint32_t height);
    bool CreateBackBufferResources(std::uint32_t width, std::uint32_t height);
    void ReleaseBackBufferResources();
    void BindDefaultTargets();
    void SetViewport(std::uint32_t width, std::uint32_t height);
    void LogInfo(const std::string& message) const;
    void LogError(const std::string& message) const;

private:
    bool m_initialized = false;
    HWND m_windowHandle = nullptr;
    ID3D11Device* m_device = nullptr;
    ID3D11DeviceContext* m_context = nullptr;
    IDXGISwapChain* m_swapChain = nullptr;
    ID3D11RenderTargetView* m_renderTargetView = nullptr;
    ID3D11Texture2D* m_depthStencilBuffer = nullptr;
    ID3D11DepthStencilView* m_depthStencilView = nullptr;
    std::uint32_t m_width = 0;
    std::uint32_t m_height = 0;
    LoggerModule* m_logger = nullptr;
};
