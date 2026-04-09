#pragma once

#include "irender_backend.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>

struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain;
struct ID3D11RenderTargetView;
struct ID3D11Texture2D;
struct ID3D11DepthStencilView;
struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D11InputLayout;
struct ID3D11Buffer;
struct ID3D11RasterizerState;
struct ID3D11DepthStencilState;
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
    void Render(const RenderFrameData& frame) override;
    void EndFrame() override;
    void Present() override;

    void Resize(std::uint32_t width, std::uint32_t height) override;

private:
    struct MeshBuffers
    {
        ID3D11Buffer* vertexBuffer = nullptr;
        ID3D11Buffer* indexBuffer = nullptr;
        std::uint32_t indexCount = 0;
    };

private:
    bool CreateRenderResources();
    void ReleaseRenderResources();
    std::uint32_t ResolveMeshCacheKey(const RenderMeshData& mesh) const;
    bool EnsureMeshResources(const RenderMeshData& mesh);
    void ReleaseMeshResources();
    bool RenderSceneView(const RenderViewData& view);
    bool DrawObject(const RenderCameraData& camera, const RenderObjectData& object);
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
    ID3D11VertexShader* m_vertexShader = nullptr;
    ID3D11PixelShader* m_pixelShader = nullptr;
    ID3D11InputLayout* m_inputLayout = nullptr;
    ID3D11Buffer* m_vertexBuffer = nullptr;
    ID3D11Buffer* m_indexBuffer = nullptr;
    ID3D11Buffer* m_constantBuffer = nullptr;
    ID3D11RasterizerState* m_rasterizerState = nullptr;
    ID3D11DepthStencilState* m_depthStencilState = nullptr;
    std::uint32_t m_width = 0;
    std::uint32_t m_height = 0;
    RenderFrameContext m_frameContext{};
    std::unordered_map<std::uint32_t, MeshBuffers> m_meshBuffers;
    LoggerModule* m_logger = nullptr;
};
