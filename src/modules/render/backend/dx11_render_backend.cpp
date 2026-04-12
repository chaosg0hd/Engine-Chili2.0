#include "dx11_render_backend.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "../../../../apps/sandbox/src/sandbox_builtin_meshes.hpp"
#include "../../../core/engine_context.hpp"
#include "../../gpu/gpu_compute_module.hpp"
#include "../../logger/logger_module.hpp"

#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#include <windows.h>

#include <array>
#include <cstring>
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

    using SimpleVertex = sandbox_builtin_meshes::Vertex;

    struct alignas(16) DrawConstants
    {
        float worldViewProjection[16];
        float baseColor[4];
    };

    constexpr const char* kVertexShaderSource = R"(
cbuffer DrawConstants : register(b0)
{
    row_major float4x4 worldViewProjection;
    float4 baseColor;
};

struct VertexInput
{
    float3 position : POSITION;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
};

VertexOutput VSMain(VertexInput input)
{
    VertexOutput output;
    output.position = mul(float4(input.position, 1.0f), worldViewProjection);
    return output;
}
)";

    constexpr const char* kPixelShaderSource = R"(
cbuffer DrawConstants : register(b0)
{
    row_major float4x4 worldViewProjection;
    float4 baseColor;
};

float4 PSMain() : SV_TARGET
{
    return baseColor;
}
)";

    bool CompileShader(
        const char* source,
        const char* entryPoint,
        const char* target,
        ID3DBlob** outBlob,
        std::string& outError)
    {
        if (!source || !entryPoint || !target || !outBlob)
        {
            return false;
        }

        *outBlob = nullptr;
        ID3DBlob* shaderBlob = nullptr;
        ID3DBlob* errorBlob = nullptr;
        const HRESULT result = D3DCompile(
            source,
            std::strlen(source),
            nullptr,
            nullptr,
            nullptr,
            entryPoint,
            target,
            0,
            0,
            &shaderBlob,
            &errorBlob);

        if (FAILED(result))
        {
            if (errorBlob)
            {
                outError.assign(
                    static_cast<const char*>(errorBlob->GetBufferPointer()),
                    errorBlob->GetBufferSize());
            }
            SafeRelease(errorBlob);
            SafeRelease(shaderBlob);
            return false;
        }

        SafeRelease(errorBlob);
        *outBlob = shaderBlob;
        return true;
    }

    void CopyMatrixToContiguousArray(const RenderMatrix4& matrix, float outValues[16])
    {
        for (int row = 0; row < 4; ++row)
        {
            for (int column = 0; column < 4; ++column)
            {
                outValues[(row * 4) + column] = matrix.m[row][column];
            }
        }
    }

    RenderCameraData BuildFallbackCamera()
    {
        return RenderCameraData{};
    }

    template<std::size_t VertexCount, std::size_t IndexCount>
    bool CreateMeshBuffers(
        ID3D11Device* device,
        const std::array<SimpleVertex, VertexCount>& vertices,
        const std::array<std::uint16_t, IndexCount>& indices,
        ID3D11Buffer** outVertexBuffer,
        ID3D11Buffer** outIndexBuffer)
    {
        if (!device || !outVertexBuffer || !outIndexBuffer)
        {
            return false;
        }

        *outVertexBuffer = nullptr;
        *outIndexBuffer = nullptr;

        D3D11_BUFFER_DESC vertexBufferDesc = {};
        vertexBufferDesc.ByteWidth = static_cast<UINT>(sizeof(vertices));
        vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
        vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA vertexData = {};
        vertexData.pSysMem = vertices.data();
        HRESULT result = device->CreateBuffer(&vertexBufferDesc, &vertexData, outVertexBuffer);
        if (FAILED(result))
        {
            return false;
        }

        D3D11_BUFFER_DESC indexBufferDesc = {};
        indexBufferDesc.ByteWidth = static_cast<UINT>(sizeof(indices));
        indexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
        indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

        D3D11_SUBRESOURCE_DATA indexData = {};
        indexData.pSysMem = indices.data();
        result = device->CreateBuffer(&indexBufferDesc, &indexData, outIndexBuffer);
        if (FAILED(result))
        {
            SafeRelease(*outVertexBuffer);
            return false;
        }

        return true;
    }

    bool IsRawBufferSizeValid(std::size_t byteSize)
    {
        return byteSize > 0U && (byteSize % 4U) == 0U && byteSize <= static_cast<std::size_t>(UINT32_MAX);
    }

    bool CreateRawBuffer(
        ID3D11Device* device,
        const void* data,
        std::uint32_t byteSize,
        std::uint32_t bindFlags,
        D3D11_USAGE usage,
        std::uint32_t cpuAccessFlags,
        ID3D11Buffer** outBuffer)
    {
        if (!device || !outBuffer || !IsRawBufferSizeValid(byteSize))
        {
            return false;
        }

        *outBuffer = nullptr;

        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = byteSize;
        desc.Usage = usage;
        desc.BindFlags = bindFlags;
        desc.CPUAccessFlags = cpuAccessFlags;
        desc.MiscFlags =
            (bindFlags & (D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS))
                ? static_cast<UINT>(D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS)
                : 0U;

        D3D11_SUBRESOURCE_DATA initialData = {};
        initialData.pSysMem = data;
        return SUCCEEDED(device->CreateBuffer(&desc, data ? &initialData : nullptr, outBuffer));
    }

    bool CreateRawShaderResourceView(
        ID3D11Device* device,
        ID3D11Buffer* buffer,
        std::uint32_t byteSize,
        ID3D11ShaderResourceView** outView)
    {
        if (!device || !buffer || !outView || !IsRawBufferSizeValid(byteSize))
        {
            return false;
        }

        *outView = nullptr;
        D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
        desc.Format = DXGI_FORMAT_R32_TYPELESS;
        desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
        desc.BufferEx.FirstElement = 0;
        desc.BufferEx.NumElements = byteSize / 4U;
        desc.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
        return SUCCEEDED(device->CreateShaderResourceView(buffer, &desc, outView));
    }

    bool CreateRawUnorderedAccessView(
        ID3D11Device* device,
        ID3D11Buffer* buffer,
        std::uint32_t byteSize,
        ID3D11UnorderedAccessView** outView)
    {
        if (!device || !buffer || !outView || !IsRawBufferSizeValid(byteSize))
        {
            return false;
        }

        *outView = nullptr;
        D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};
        desc.Format = DXGI_FORMAT_R32_TYPELESS;
        desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
        desc.Buffer.FirstElement = 0;
        desc.Buffer.NumElements = byteSize / 4U;
        desc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
        return SUCCEEDED(device->CreateUnorderedAccessView(buffer, &desc, outView));
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

    if (!CreateRenderResources())
    {
        Shutdown();
        return false;
    }

    m_width = width;
    m_height = height;
    m_supportsComputeDispatch = true;
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
        m_supportsComputeDispatch = false;
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

    ReleaseRenderResources();
    ReleaseMeshResources();
    ReleaseBackBufferResources();
    SafeRelease(m_swapChain);
    SafeRelease(m_context);
    SafeRelease(m_device);

    LogInfo("Render backend shutdown: Dx11RenderBackend");

    m_initialized = false;
    m_supportsComputeDispatch = false;
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

    m_frameContext = frameContext;
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

void Dx11RenderBackend::Render(const RenderFrameData& frame)
{
    if (!m_initialized || !m_context || !m_vertexShader || !m_pixelShader || !m_inputLayout)
    {
        return;
    }

    for (const RenderPassData& pass : frame.passes)
    {
        (void)pass.kind;
        for (const RenderViewData& view : pass.views)
        {
            if (view.kind == RenderViewDataKind::Scene3D)
            {
                RenderSceneView(view);
            }

            for (const RenderItemData& item : view.items)
            {
                if (item.kind == RenderItemDataKind::ScreenCell)
                {
                    DrawScreenCell(item.screenCell);
                }
            }
        }
    }
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

bool Dx11RenderBackend::SupportsComputeDispatch() const
{
    return m_initialized && m_supportsComputeDispatch && m_device && m_context;
}

bool Dx11RenderBackend::SubmitGpuTask(const GpuTaskDesc& task)
{
    if (!SupportsComputeDispatch() ||
        task.name.empty() ||
        task.shaderSource.empty() ||
        task.dispatchX == 0U ||
        task.dispatchY == 0U ||
        task.dispatchZ == 0U ||
        task.output.data == nullptr ||
        !IsRawBufferSizeValid(task.output.size) ||
        (task.input.size > 0U && (!task.input.data || !IsRawBufferSizeValid(task.input.size))))
    {
        return false;
    }

    std::string compileError;
    ID3DBlob* computeShaderBlob = nullptr;
    const char* entryPoint = task.entryPoint.empty() ? "CSMain" : task.entryPoint.c_str();
    if (!CompileShader(task.shaderSource.c_str(), entryPoint, "cs_5_0", &computeShaderBlob, compileError))
    {
        LogError("Dx11RenderBackend compute shader compile failed for " + task.name + ": " + compileError);
        return false;
    }

    ID3D11ComputeShader* computeShader = nullptr;
    HRESULT result = m_device->CreateComputeShader(
        computeShaderBlob->GetBufferPointer(),
        computeShaderBlob->GetBufferSize(),
        nullptr,
        &computeShader);
    SafeRelease(computeShaderBlob);
    if (FAILED(result))
    {
        LogError("Dx11RenderBackend failed to create compute shader for " + task.name + ".");
        return false;
    }

    ID3D11Buffer* inputBuffer = nullptr;
    ID3D11ShaderResourceView* inputView = nullptr;
    if (task.input.size > 0U)
    {
        const auto inputByteSize = static_cast<std::uint32_t>(task.input.size);
        if (!CreateRawBuffer(
                m_device,
                task.input.data,
                inputByteSize,
                D3D11_BIND_SHADER_RESOURCE,
                D3D11_USAGE_DEFAULT,
                0U,
                &inputBuffer) ||
            !CreateRawShaderResourceView(m_device, inputBuffer, inputByteSize, &inputView))
        {
            SafeRelease(inputView);
            SafeRelease(inputBuffer);
            SafeRelease(computeShader);
            LogError("Dx11RenderBackend failed to create compute input for " + task.name + ".");
            return false;
        }
    }

    const auto outputByteSize = static_cast<std::uint32_t>(task.output.size);
    ID3D11Buffer* outputBuffer = nullptr;
    ID3D11UnorderedAccessView* outputView = nullptr;
    ID3D11Buffer* stagingBuffer = nullptr;
    if (!CreateRawBuffer(
            m_device,
            nullptr,
            outputByteSize,
            D3D11_BIND_UNORDERED_ACCESS,
            D3D11_USAGE_DEFAULT,
            0U,
            &outputBuffer) ||
        !CreateRawUnorderedAccessView(m_device, outputBuffer, outputByteSize, &outputView) ||
        !CreateRawBuffer(
            m_device,
            nullptr,
            outputByteSize,
            0U,
            D3D11_USAGE_STAGING,
            D3D11_CPU_ACCESS_READ,
            &stagingBuffer))
    {
        SafeRelease(stagingBuffer);
        SafeRelease(outputView);
        SafeRelease(outputBuffer);
        SafeRelease(inputView);
        SafeRelease(inputBuffer);
        SafeRelease(computeShader);
        LogError("Dx11RenderBackend failed to create compute output for " + task.name + ".");
        return false;
    }

    ID3D11ShaderResourceView* shaderResourceViews[] = { inputView };
    ID3D11UnorderedAccessView* unorderedAccessViews[] = { outputView };
    m_context->CSSetShader(computeShader, nullptr, 0);
    m_context->CSSetShaderResources(0, 1, shaderResourceViews);
    m_context->CSSetUnorderedAccessViews(0, 1, unorderedAccessViews, nullptr);
    m_context->Dispatch(task.dispatchX, task.dispatchY, task.dispatchZ);

    ID3D11ShaderResourceView* nullShaderResourceViews[] = { nullptr };
    ID3D11UnorderedAccessView* nullUnorderedAccessViews[] = { nullptr };
    m_context->CSSetUnorderedAccessViews(0, 1, nullUnorderedAccessViews, nullptr);
    m_context->CSSetShaderResources(0, 1, nullShaderResourceViews);
    m_context->CSSetShader(nullptr, nullptr, 0);

    m_context->CopyResource(stagingBuffer, outputBuffer);
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    result = m_context->Map(stagingBuffer, 0, D3D11_MAP_READ, 0, &mapped);
    if (SUCCEEDED(result))
    {
        std::memcpy(task.output.data, mapped.pData, task.output.size);
        m_context->Unmap(stagingBuffer, 0);
    }

    SafeRelease(stagingBuffer);
    SafeRelease(outputView);
    SafeRelease(outputBuffer);
    SafeRelease(inputView);
    SafeRelease(inputBuffer);
    SafeRelease(computeShader);

    return SUCCEEDED(result);
}

void Dx11RenderBackend::WaitForGpuIdle()
{
    if (m_context)
    {
        m_context->Flush();
    }
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

bool Dx11RenderBackend::CreateRenderResources()
{
    if (!m_device)
    {
        return false;
    }

    std::string compileError;
    ID3DBlob* vertexShaderBlob = nullptr;
    if (!CompileShader(kVertexShaderSource, "VSMain", "vs_4_0", &vertexShaderBlob, compileError))
    {
        LogError(std::string("Dx11RenderBackend failed to compile vertex shader. ") + compileError);
        return false;
    }

    HRESULT result = m_device->CreateVertexShader(
        vertexShaderBlob->GetBufferPointer(),
        vertexShaderBlob->GetBufferSize(),
        nullptr,
        &m_vertexShader);
    if (FAILED(result))
    {
        SafeRelease(vertexShaderBlob);
        std::ostringstream message;
        message << "Dx11RenderBackend failed to create vertex shader. HRESULT=0x"
                << std::hex << static_cast<unsigned long>(result);
        LogError(message.str());
        return false;
    }

    const D3D11_INPUT_ELEMENT_DESC inputElements[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    result = m_device->CreateInputLayout(
        inputElements,
        static_cast<UINT>(sizeof(inputElements) / sizeof(inputElements[0])),
        vertexShaderBlob->GetBufferPointer(),
        vertexShaderBlob->GetBufferSize(),
        &m_inputLayout);
    SafeRelease(vertexShaderBlob);
    if (FAILED(result))
    {
        std::ostringstream message;
        message << "Dx11RenderBackend failed to create input layout. HRESULT=0x"
                << std::hex << static_cast<unsigned long>(result);
        LogError(message.str());
        return false;
    }

    ID3DBlob* pixelShaderBlob = nullptr;
    compileError.clear();
    if (!CompileShader(kPixelShaderSource, "PSMain", "ps_4_0", &pixelShaderBlob, compileError))
    {
        LogError(std::string("Dx11RenderBackend failed to compile pixel shader. ") + compileError);
        return false;
    }

    result = m_device->CreatePixelShader(
        pixelShaderBlob->GetBufferPointer(),
        pixelShaderBlob->GetBufferSize(),
        nullptr,
        &m_pixelShader);
    SafeRelease(pixelShaderBlob);
    if (FAILED(result))
    {
        std::ostringstream message;
        message << "Dx11RenderBackend failed to create pixel shader. HRESULT=0x"
                << std::hex << static_cast<unsigned long>(result);
        LogError(message.str());
        return false;
    }

    result = CreateMeshBuffers(
        m_device,
        sandbox_builtin_meshes::kTriangleVertices,
        sandbox_builtin_meshes::kTriangleIndices,
        &m_vertexBuffer,
        &m_indexBuffer) ? S_OK : E_FAIL;
    if (FAILED(result))
    {
        LogError("Dx11RenderBackend failed to create default mesh buffers.");
        return false;
    }

    D3D11_BUFFER_DESC constantBufferDesc = {};
    constantBufferDesc.ByteWidth = static_cast<UINT>(sizeof(DrawConstants));
    constantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    constantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    result = m_device->CreateBuffer(&constantBufferDesc, nullptr, &m_constantBuffer);
    if (FAILED(result))
    {
        std::ostringstream message;
        message << "Dx11RenderBackend failed to create constant buffer. HRESULT=0x"
                << std::hex << static_cast<unsigned long>(result);
        LogError(message.str());
        return false;
    }

    D3D11_RASTERIZER_DESC rasterizerDesc = {};
    rasterizerDesc.FillMode = D3D11_FILL_SOLID;
    rasterizerDesc.CullMode = D3D11_CULL_NONE;
    rasterizerDesc.FrontCounterClockwise = FALSE;
    rasterizerDesc.DepthClipEnable = TRUE;

    result = m_device->CreateRasterizerState(&rasterizerDesc, &m_rasterizerState);
    if (FAILED(result))
    {
        std::ostringstream message;
        message << "Dx11RenderBackend failed to create rasterizer state. HRESULT=0x"
                << std::hex << static_cast<unsigned long>(result);
        LogError(message.str());
        return false;
    }

    D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
    depthStencilDesc.DepthEnable = TRUE;
    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

    result = m_device->CreateDepthStencilState(&depthStencilDesc, &m_depthStencilState);
    if (FAILED(result))
    {
        std::ostringstream message;
        message << "Dx11RenderBackend failed to create depth stencil state. HRESULT=0x"
                << std::hex << static_cast<unsigned long>(result);
        LogError(message.str());
        return false;
    }

    return true;
}

void Dx11RenderBackend::ReleaseRenderResources()
{
    SafeRelease(m_depthStencilState);
    SafeRelease(m_rasterizerState);
    SafeRelease(m_constantBuffer);
    SafeRelease(m_indexBuffer);
    SafeRelease(m_vertexBuffer);
    SafeRelease(m_inputLayout);
    SafeRelease(m_pixelShader);
    SafeRelease(m_vertexShader);
}

std::uint32_t Dx11RenderBackend::ResolveMeshCacheKey(const RenderMeshData& mesh) const
{
    if (mesh.handle != 0U)
    {
        return mesh.handle;
    }

    return 0x80000000u | static_cast<std::uint32_t>(mesh.builtInKind);
}

bool Dx11RenderBackend::EnsureMeshResources(const RenderMeshData& mesh)
{
    const std::uint32_t meshKey = ResolveMeshCacheKey(mesh);
    if (meshKey == 0U || (mesh.builtInKind == RenderBuiltInMeshKind::None && mesh.handle == 0U))
    {
        return false;
    }

    if (m_meshBuffers.find(meshKey) != m_meshBuffers.end())
    {
        return true;
    }

    MeshBuffers meshBuffers;
    bool created = false;
    RenderBuiltInMeshKind builtInKind = mesh.builtInKind;
    if (builtInKind == RenderBuiltInMeshKind::None)
    {
        switch (mesh.handle)
        {
        case 1U:
            builtInKind = RenderBuiltInMeshKind::Triangle;
            break;
        case 2U:
            builtInKind = RenderBuiltInMeshKind::Diamond;
            break;
        case 3U:
            builtInKind = RenderBuiltInMeshKind::Cube;
            break;
        case 4U:
            builtInKind = RenderBuiltInMeshKind::Quad;
            break;
        case 5U:
            builtInKind = RenderBuiltInMeshKind::Octahedron;
            break;
        default:
            builtInKind = RenderBuiltInMeshKind::Triangle;
            break;
        }
    }

    switch (builtInKind)
    {
    case RenderBuiltInMeshKind::Triangle:
        created = CreateMeshBuffers(
            m_device,
            sandbox_builtin_meshes::kTriangleVertices,
            sandbox_builtin_meshes::kTriangleIndices,
            &meshBuffers.vertexBuffer,
            &meshBuffers.indexBuffer);
        meshBuffers.indexCount = static_cast<std::uint32_t>(sandbox_builtin_meshes::kTriangleIndices.size());
        break;
    case RenderBuiltInMeshKind::Diamond:
        created = CreateMeshBuffers(
            m_device,
            sandbox_builtin_meshes::kDiamondVertices,
            sandbox_builtin_meshes::kDiamondIndices,
            &meshBuffers.vertexBuffer,
            &meshBuffers.indexBuffer);
        meshBuffers.indexCount = static_cast<std::uint32_t>(sandbox_builtin_meshes::kDiamondIndices.size());
        break;
    case RenderBuiltInMeshKind::Cube:
        created = CreateMeshBuffers(
            m_device,
            sandbox_builtin_meshes::kCubeVertices,
            sandbox_builtin_meshes::kCubeIndices,
            &meshBuffers.vertexBuffer,
            &meshBuffers.indexBuffer);
        meshBuffers.indexCount = static_cast<std::uint32_t>(sandbox_builtin_meshes::kCubeIndices.size());
        break;
    case RenderBuiltInMeshKind::Quad:
        created = CreateMeshBuffers(
            m_device,
            sandbox_builtin_meshes::kQuadVertices,
            sandbox_builtin_meshes::kQuadIndices,
            &meshBuffers.vertexBuffer,
            &meshBuffers.indexBuffer);
        meshBuffers.indexCount = static_cast<std::uint32_t>(sandbox_builtin_meshes::kQuadIndices.size());
        break;
    case RenderBuiltInMeshKind::Octahedron:
        created = CreateMeshBuffers(
            m_device,
            sandbox_builtin_meshes::kOctahedronVertices,
            sandbox_builtin_meshes::kOctahedronIndices,
            &meshBuffers.vertexBuffer,
            &meshBuffers.indexBuffer);
        meshBuffers.indexCount = static_cast<std::uint32_t>(sandbox_builtin_meshes::kOctahedronIndices.size());
        break;
    default:
        created = CreateMeshBuffers(
            m_device,
            sandbox_builtin_meshes::kTriangleVertices,
            sandbox_builtin_meshes::kTriangleIndices,
            &meshBuffers.vertexBuffer,
            &meshBuffers.indexBuffer);
        meshBuffers.indexCount = static_cast<std::uint32_t>(sandbox_builtin_meshes::kTriangleIndices.size());
        break;
    }

    if (!created)
    {
        LogError("Dx11RenderBackend failed to create mesh resources for a submitted mesh handle.");
        return false;
    }

    m_meshBuffers.emplace(meshKey, meshBuffers);
    return true;
}

void Dx11RenderBackend::ReleaseMeshResources()
{
    for (auto& entry : m_meshBuffers)
    {
        SafeRelease(entry.second.indexBuffer);
        SafeRelease(entry.second.vertexBuffer);
    }

    m_meshBuffers.clear();
}

bool Dx11RenderBackend::RenderSceneView(const RenderViewData& view)
{
    const RenderCameraData camera = (view.kind == RenderViewDataKind::Scene3D) ? view.camera : BuildFallbackCamera();

    for (const RenderItemData& item : view.items)
    {
        if (item.kind != RenderItemDataKind::Object3D)
        {
            continue;
        }

        if (!DrawObject(camera, item.object3D))
        {
            return false;
        }
    }

    return true;
}

bool Dx11RenderBackend::DrawObject(const RenderCameraData& camera, const RenderObjectData& object)
{
    if (!m_context || !m_constantBuffer || !EnsureMeshResources(object.mesh))
    {
        return false;
    }

    const auto meshIt = m_meshBuffers.find(ResolveMeshCacheKey(object.mesh));
    if (meshIt == m_meshBuffers.end())
    {
        return false;
    }

    const MeshBuffers& meshBuffers = meshIt->second;
    const float aspectRatio =
        (m_frameContext.viewport.height > 0)
            ? (static_cast<float>(m_frameContext.viewport.width) / static_cast<float>(m_frameContext.viewport.height))
            : 1.0f;

    const RenderMatrix4 worldMatrix = object.transform.ToMatrix();
    const RenderMatrix4 viewMatrix = BuildRenderCameraViewMatrix(camera);
    const RenderMatrix4 projectionMatrix = BuildRenderCameraProjectionMatrix(camera, aspectRatio);
    const RenderMatrix4 worldViewMatrix = RenderMultiply(worldMatrix, viewMatrix);
    const RenderMatrix4 worldViewProjection = RenderMultiply(worldViewMatrix, projectionMatrix);

    DrawConstants constants = {};
    CopyMatrixToContiguousArray(worldViewProjection, constants.worldViewProjection);
    constants.baseColor[0] = static_cast<float>((object.material.baseColor >> 16) & 0xFFu) / 255.0f;
    constants.baseColor[1] = static_cast<float>((object.material.baseColor >> 8) & 0xFFu) / 255.0f;
    constants.baseColor[2] = static_cast<float>(object.material.baseColor & 0xFFu) / 255.0f;
    constants.baseColor[3] = static_cast<float>((object.material.baseColor >> 24) & 0xFFu) / 255.0f;

    D3D11_MAPPED_SUBRESOURCE mappedResource = {};
    HRESULT result = m_context->Map(m_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (FAILED(result))
    {
        std::ostringstream message;
        message << "Dx11RenderBackend failed to map constant buffer. HRESULT=0x"
                << std::hex << static_cast<unsigned long>(result);
        LogError(message.str());
        return false;
    }

    std::memcpy(mappedResource.pData, &constants, sizeof(constants));
    m_context->Unmap(m_constantBuffer, 0);

    const UINT stride = sizeof(SimpleVertex);
    const UINT offset = 0;
    m_context->IASetInputLayout(m_inputLayout);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_context->IASetVertexBuffers(0, 1, &meshBuffers.vertexBuffer, &stride, &offset);
    m_context->IASetIndexBuffer(meshBuffers.indexBuffer, DXGI_FORMAT_R16_UINT, 0);
    m_context->RSSetState(m_rasterizerState);
    m_context->OMSetDepthStencilState(m_depthStencilState, 0);
    m_context->VSSetShader(m_vertexShader, nullptr, 0);
    m_context->PSSetShader(m_pixelShader, nullptr, 0);
    m_context->VSSetConstantBuffers(0, 1, &m_constantBuffer);
    m_context->PSSetConstantBuffers(0, 1, &m_constantBuffer);
    m_context->DrawIndexed(meshBuffers.indexCount, 0, 0);
    return true;
}

bool Dx11RenderBackend::DrawScreenCell(const RenderScreenCellData& cell)
{
    RenderMeshData quadMesh;
    quadMesh.builtInKind = RenderBuiltInMeshKind::Quad;
    if (!m_context || !m_constantBuffer || !EnsureMeshResources(quadMesh))
    {
        return false;
    }

    const auto meshIt = m_meshBuffers.find(ResolveMeshCacheKey(quadMesh));
    if (meshIt == m_meshBuffers.end())
    {
        return false;
    }

    const MeshBuffers& meshBuffers = meshIt->second;
    DrawConstants constants = {};
    constants.worldViewProjection[0] = cell.halfWidth / 0.35f;
    constants.worldViewProjection[5] = cell.halfHeight / 0.35f;
    constants.worldViewProjection[10] = 1.0f;
    constants.worldViewProjection[12] = cell.centerX;
    constants.worldViewProjection[13] = cell.centerY;
    constants.worldViewProjection[15] = 1.0f;
    constants.baseColor[0] = static_cast<float>((cell.color >> 16) & 0xFFu) / 255.0f;
    constants.baseColor[1] = static_cast<float>((cell.color >> 8) & 0xFFu) / 255.0f;
    constants.baseColor[2] = static_cast<float>(cell.color & 0xFFu) / 255.0f;
    constants.baseColor[3] = static_cast<float>((cell.color >> 24) & 0xFFu) / 255.0f;

    D3D11_MAPPED_SUBRESOURCE mappedResource = {};
    HRESULT result = m_context->Map(m_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (FAILED(result))
    {
        std::ostringstream message;
        message << "Dx11RenderBackend failed to map screen cell constant buffer. HRESULT=0x"
                << std::hex << static_cast<unsigned long>(result);
        LogError(message.str());
        return false;
    }

    std::memcpy(mappedResource.pData, &constants, sizeof(constants));
    m_context->Unmap(m_constantBuffer, 0);

    const UINT stride = sizeof(SimpleVertex);
    const UINT offset = 0;
    m_context->IASetInputLayout(m_inputLayout);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_context->IASetVertexBuffers(0, 1, &meshBuffers.vertexBuffer, &stride, &offset);
    m_context->IASetIndexBuffer(meshBuffers.indexBuffer, DXGI_FORMAT_R16_UINT, 0);
    m_context->RSSetState(m_rasterizerState);
    m_context->OMSetDepthStencilState(m_depthStencilState, 0);
    m_context->VSSetShader(m_vertexShader, nullptr, 0);
    m_context->PSSetShader(m_pixelShader, nullptr, 0);
    m_context->VSSetConstantBuffers(0, 1, &m_constantBuffer);
    m_context->PSSetConstantBuffers(0, 1, &m_constantBuffer);
    m_context->DrawIndexed(meshBuffers.indexCount, 0, 0);
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
