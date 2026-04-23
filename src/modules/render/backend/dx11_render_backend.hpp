#pragma once

#include "irender_backend.hpp"

#include <cstdint>
#include <array>
#include <string>
#include <unordered_map>
#include <vector>

struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain;
struct ID3D11RenderTargetView;
struct ID3D11Texture2D;
struct ID3D11DepthStencilView;
struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D11ComputeShader;
struct ID3D11InputLayout;
struct ID3D11Buffer;
struct ID3D11RasterizerState;
struct ID3D11DepthStencilState;
struct ID3D11SamplerState;
struct ID3D11ShaderResourceView;
struct ID3D11UnorderedAccessView;
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
    bool CreateResource(GpuResourceHandle handle, const GpuUploadRequest& request) override;
    void DestroyResource(GpuResourceHandle handle) override;
    void SetDerivedBounceFillSettings(const DerivedBounceFillSettings& settings) override;
    DerivedBounceFillSettings GetDerivedBounceFillSettings() const override;
    void SetTracedIndirectSettings(const TracedIndirectSettings& settings) override;
    TracedIndirectSettings GetTracedIndirectSettings() const override;
    bool SupportsComputeDispatch() const override;
    bool SubmitGpuTask(const GpuTaskDesc& task) override;
    void WaitForGpuIdle() override;

private:
    struct MeshBuffers
    {
        ID3D11Buffer* vertexBuffer = nullptr;
        ID3D11Buffer* indexBuffer = nullptr;
        std::uint32_t indexCount = 0;
    };

    struct TextureResource
    {
        ID3D11Texture2D* texture = nullptr;
        ID3D11ShaderResourceView* shaderResourceView = nullptr;
        std::uint32_t width = 0U;
        std::uint32_t height = 0U;
    };

    struct ShadowMapResources
    {
        ID3D11Texture2D* depthTexture = nullptr;
        ID3D11Texture2D* shadowTexture = nullptr;
        ID3D11ShaderResourceView* shaderResourceView = nullptr;
        std::array<ID3D11DepthStencilView*, 6> depthViews = {};
        std::array<ID3D11RenderTargetView*, 6> renderTargetViews = {};
        std::uint32_t resolution = 0U;
    };

private:
    bool CreateRenderResources();
    void ReleaseRenderResources();
    bool CreateFallbackTexture();
    bool CreateFallbackShadowTexture();
    bool CreateTextureResource(const GpuTextureUploadPayload& texturePayload, TextureResource& outTexture);
    ID3D11ShaderResourceView* ResolveAlbedoTextureView(const RenderMaterialData& material, float& outRepeatsPerMeter);
    ID3D11ShaderResourceView* ResolveShadowTextureView() const;
    void ReleaseTextureResources();
    bool EnsureShadowMapResources(std::uint32_t resolution);
    void ReleaseShadowMapResources();
    std::uint32_t ResolveMeshCacheKey(const RenderMeshData& mesh) const;
    bool EnsureMeshResources(const RenderMeshData& mesh);
    void ReleaseMeshResources();
    bool RenderDirectLightVisibilityPass(const RenderFrameData& frame, const RenderPassData& pass);
    bool RenderDirectLightVisibilityView(const RenderFrameData& frame, const RenderPassData& pass, const RenderViewData& view);
    bool RenderSceneView(const RenderViewData& view);
    bool DrawShadowObject(
        const RenderCameraData& camera,
        const RenderVector3& lightPosition,
        float lightFarPlane,
        const RenderObjectData& object);
    bool DrawObject(
        const RenderCameraData& camera,
        const std::vector<CompiledLightData>& compiledLights,
        const std::vector<RenderIndirectLightProbeData>& indirectLightProbes,
        const std::vector<RenderItemData>& sceneItems,
        const RenderObjectData& object);
    bool DrawScreenPatch(const RenderScreenPatchData& patch);
    bool DrawScreenHexPatch(const RenderScreenPatchData& patch);
    bool DrawScreenMeshPatch(const RenderScreenPatchData& patch, RenderBuiltInMeshKind meshKind);
    bool CreateDeviceAndSwapChain(HWND nativeWindowHandle, std::uint32_t width, std::uint32_t height);
    bool CreateBackBufferResources(std::uint32_t width, std::uint32_t height);
    void ReleaseBackBufferResources();
    void BindDefaultTargets();
    void SetViewport(std::uint32_t width, std::uint32_t height);
    void LogInfo(const std::string& message) const;
    void LogError(const std::string& message) const;

private:
    bool m_initialized = false;
    bool m_supportsComputeDispatch = false;
    HWND m_windowHandle = nullptr;
    ID3D11Device* m_device = nullptr;
    ID3D11DeviceContext* m_context = nullptr;
    IDXGISwapChain* m_swapChain = nullptr;
    ID3D11RenderTargetView* m_renderTargetView = nullptr;
    ID3D11Texture2D* m_depthStencilBuffer = nullptr;
    ID3D11DepthStencilView* m_depthStencilView = nullptr;
    ID3D11VertexShader* m_vertexShader = nullptr;
    ID3D11PixelShader* m_pixelShader = nullptr;
    ID3D11VertexShader* m_shadowVertexShader = nullptr;
    ID3D11PixelShader* m_shadowPixelShader = nullptr;
    ID3D11InputLayout* m_inputLayout = nullptr;
    ID3D11Buffer* m_vertexBuffer = nullptr;
    ID3D11Buffer* m_indexBuffer = nullptr;
    ID3D11Buffer* m_constantBuffer = nullptr;
    ID3D11Buffer* m_shadowConstantBuffer = nullptr;
    ID3D11RasterizerState* m_rasterizerState = nullptr;
    ID3D11DepthStencilState* m_depthStencilState = nullptr;
    ID3D11SamplerState* m_samplerState = nullptr;
    ID3D11SamplerState* m_shadowSamplerState = nullptr;
    ID3D11ShaderResourceView* m_fallbackTextureView = nullptr;
    ID3D11ShaderResourceView* m_fallbackShadowTextureView = nullptr;
    std::uint32_t m_width = 0;
    std::uint32_t m_height = 0;
    RenderFrameContext m_frameContext{};
    std::unordered_map<std::uint32_t, MeshBuffers> m_meshBuffers;
    std::unordered_map<GpuResourceHandle, TextureResource> m_textureResources;
    ShadowMapResources m_shadowMapResources;
    DerivedBounceFillSettings m_derivedBounceFillSettings;
    TracedIndirectSettings m_tracedIndirectSettings;
    LoggerModule* m_logger = nullptr;
};
