#include "dx11_render_backend.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "../../../core/engine_context.hpp"
#include "../../gpu/gpu_compute_module.hpp"
#include "../../logger/logger_module.hpp"
#include "../render_builtin_meshes.hpp"

#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#include <windows.h>

#include <array>
#include <cmath>
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

    using SimpleVertex = render_builtin_meshes::Vertex;
    constexpr std::size_t MaxShaderLights = 4U;

    struct alignas(16) DrawConstants
    {
        float world[16];
        float worldViewProjection[16];
        float normalMatrix[12];
        float textureParams[4];
        float objectScale[4];
        float baseColor[4];
        float reflectionColor[4];
        float materialParams[4];
        float reflectionParams[4];
        float cameraPosition[4];
        float cameraExposure[4];
        float ambientColor[4];
        float lightPosition[MaxShaderLights][4];
        float lightColor[MaxShaderLights][4];
        std::uint32_t lightCount = 0U;
        float _padding[3] = {};
    };

    constexpr const char* kVertexShaderSource = R"(
cbuffer DrawConstants : register(b0)
{
    row_major float4x4 world;
    row_major float4x4 worldViewProjection;
    row_major float3x4 normalMatrix;
    float4 textureParams;
    float4 objectScale;
    float4 baseColor;
    float4 reflectionColor;
    float4 materialParams;
    float4 reflectionParams;
    float4 cameraPosition;
    float4 cameraExposure;
    float4 ambientColor;
    float4 lightPosition[4];
    float4 lightColor[4];
    uint lightCount;
    float3 _padding;
};

struct VertexInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
    float3 worldPosition : TEXCOORD0;
    float3 worldNormal : TEXCOORD1;
    float3 localNormal : TEXCOORD2;
    float2 uv : TEXCOORD3;
};

VertexOutput VSMain(VertexInput input)
{
    VertexOutput output;
    const float4 worldPosition = mul(float4(input.position, 1.0f), world);
    output.position = mul(float4(input.position, 1.0f), worldViewProjection);
    output.worldPosition = worldPosition.xyz;
    output.worldNormal = normalize(mul(input.normal, (float3x3)normalMatrix));
    output.localNormal = input.normal;
    output.uv = input.uv;
    return output;
}
)";

    constexpr const char* kPixelShaderSource = R"(
cbuffer DrawConstants : register(b0)
{
    row_major float4x4 world;
    row_major float4x4 worldViewProjection;
    row_major float3x4 normalMatrix;
    float4 textureParams;
    float4 objectScale;
    float4 baseColor;
    float4 reflectionColor;
    float4 materialParams;
    float4 reflectionParams;
    float4 cameraPosition;
    float4 cameraExposure;
    float4 ambientColor;
    float4 lightPosition[4];
    float4 lightColor[4];
    uint lightCount;
    float3 _padding;
};

Texture2D albedoTexture : register(t0);
SamplerState linearSampler : register(s0);

struct VertexOutput
{
    float4 position : SV_POSITION;
    float3 worldPosition : TEXCOORD0;
    float3 worldNormal : TEXCOORD1;
    float3 localNormal : TEXCOORD2;
    float2 uv : TEXCOORD3;
};

float4 PSMain(VertexOutput input) : SV_TARGET
{
    const float3 normal = normalize(input.worldNormal);
    const float3 viewDirection = normalize(cameraPosition.xyz - input.worldPosition);
    float ambientStrength = materialParams.x;
    float diffuseStrength = materialParams.y;
    float specularStrength = materialParams.z;
    float specularPower = max(materialParams.w, 1.0f);
    const float textureEnabled = textureParams.x;
    const float repeatsPerMeterU = max(textureParams.y, 0.0001f);
    const float repeatsPerMeterV = max(textureParams.z, 0.0001f);
    float reflectivity = saturate(reflectionParams.x);
    float roughness = saturate(reflectionParams.y);
    const float3 absLocalNormal = abs(input.localNormal);
    float2 faceDimensions = float2(objectScale.x, objectScale.y);
    if (absLocalNormal.x > absLocalNormal.y && absLocalNormal.x > absLocalNormal.z)
    {
        faceDimensions = float2(objectScale.z, objectScale.y);
    }
    else if (absLocalNormal.y > absLocalNormal.z)
    {
        faceDimensions = float2(objectScale.x, objectScale.z);
    }
    const float2 tiledUv = input.uv * float2(faceDimensions.x * repeatsPerMeterU, faceDimensions.y * repeatsPerMeterV);
    const float3 sampledAlbedo = albedoTexture.Sample(linearSampler, tiledUv).rgb;
    const float3 surfaceAlbedo = (textureEnabled > 0.5f) ? sampledAlbedo : baseColor.rgb;
    float3 litColor = surfaceAlbedo * ambientColor.rgb * ambientStrength;
    const float NdotV = saturate(dot(normal, viewDirection));
    const float fresnel = reflectivity + ((1.0f - reflectivity) * pow(1.0f - NdotV, 5.0f));

    [unroll]
    for (uint lightIndex = 0; lightIndex < 4; ++lightIndex)
    {
        if (lightIndex >= lightCount)
        {
            break;
        }

        const float3 toLight = lightPosition[lightIndex].xyz - input.worldPosition;
        const float distanceToLight = max(length(toLight), 0.0001f);
        const float3 lightDirection = toLight / distanceToLight;
        const float diffuse = max(dot(normal, lightDirection), 0.0f);
        const float maxDistance = max(lightPosition[lightIndex].w, 0.0001f);
        const float attenuation = saturate(1.0f - (distanceToLight / maxDistance));
        const float3 halfVector = normalize(lightDirection + viewDirection);
        const float specular = pow(max(dot(normal, halfVector), 0.0f), specularPower) * attenuation;
        const float3 reflectedLightColor = lightColor[lightIndex].rgb * reflectionColor.rgb;
        litColor += (surfaceAlbedo * lightColor[lightIndex].rgb * diffuse * attenuation * diffuseStrength);
        litColor += reflectedLightColor * specular * specularStrength * (0.15f + (0.85f * fresnel * (1.0f - (roughness * 0.5f))));
    }

    litColor *= max(cameraExposure.x, 0.0f);
    return float4(saturate(litColor), baseColor.a);
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

    void CopyNormalMatrixToContiguousArray(const RenderMatrix4& worldMatrix, float outValues[12])
    {
        const float a00 = worldMatrix.m[0][0];
        const float a01 = worldMatrix.m[0][1];
        const float a02 = worldMatrix.m[0][2];
        const float a10 = worldMatrix.m[1][0];
        const float a11 = worldMatrix.m[1][1];
        const float a12 = worldMatrix.m[1][2];
        const float a20 = worldMatrix.m[2][0];
        const float a21 = worldMatrix.m[2][1];
        const float a22 = worldMatrix.m[2][2];

        const float determinant =
            (a00 * ((a11 * a22) - (a12 * a21))) -
            (a01 * ((a10 * a22) - (a12 * a20))) +
            (a02 * ((a10 * a21) - (a11 * a20)));

        if (std::fabs(determinant) <= 0.000001f)
        {
            outValues[0] = 1.0f; outValues[1] = 0.0f; outValues[2] = 0.0f; outValues[3] = 0.0f;
            outValues[4] = 0.0f; outValues[5] = 1.0f; outValues[6] = 0.0f; outValues[7] = 0.0f;
            outValues[8] = 0.0f; outValues[9] = 0.0f; outValues[10] = 1.0f; outValues[11] = 0.0f;
            return;
        }

        const float inverseDeterminant = 1.0f / determinant;

        outValues[0]  = ((a11 * a22) - (a12 * a21)) * inverseDeterminant;
        outValues[1]  = ((a12 * a20) - (a10 * a22)) * inverseDeterminant;
        outValues[2]  = ((a10 * a21) - (a11 * a20)) * inverseDeterminant;
        outValues[3]  = 0.0f;

        outValues[4]  = ((a02 * a21) - (a01 * a22)) * inverseDeterminant;
        outValues[5]  = ((a00 * a22) - (a02 * a20)) * inverseDeterminant;
        outValues[6]  = ((a01 * a20) - (a00 * a21)) * inverseDeterminant;
        outValues[7]  = 0.0f;

        outValues[8]  = ((a01 * a12) - (a02 * a11)) * inverseDeterminant;
        outValues[9]  = ((a02 * a10) - (a00 * a12)) * inverseDeterminant;
        outValues[10] = ((a00 * a11) - (a01 * a10)) * inverseDeterminant;
        outValues[11] = 0.0f;
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
                if (item.kind == RenderItemDataKind::ScreenPatch)
                {
                    DrawScreenPatch(item.screenPatch);
                }
                else if (item.kind == RenderItemDataKind::ScreenHexPatch)
                {
                    DrawScreenHexPatch(item.screenPatch);
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
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 }
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
        render_builtin_meshes::kTriangleVertices,
        render_builtin_meshes::kTriangleIndices,
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

    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    result = m_device->CreateSamplerState(&samplerDesc, &m_samplerState);
    if (FAILED(result))
    {
        std::ostringstream message;
        message << "Dx11RenderBackend failed to create sampler state. HRESULT=0x"
                << std::hex << static_cast<unsigned long>(result);
        LogError(message.str());
        return false;
    }

    if (!CreateFallbackTexture())
    {
        LogError("Dx11RenderBackend failed to create fallback texture.");
        return false;
    }

    return true;
}

void Dx11RenderBackend::ReleaseRenderResources()
{
    ReleaseTextureResources();
    SafeRelease(m_fallbackTextureView);
    SafeRelease(m_samplerState);
    SafeRelease(m_depthStencilState);
    SafeRelease(m_rasterizerState);
    SafeRelease(m_constantBuffer);
    SafeRelease(m_indexBuffer);
    SafeRelease(m_vertexBuffer);
    SafeRelease(m_inputLayout);
    SafeRelease(m_pixelShader);
    SafeRelease(m_vertexShader);
}

bool Dx11RenderBackend::CreateFallbackTexture()
{
    if (!m_device)
    {
        return false;
    }

    const std::uint32_t whitePixel = 0xFFFFFFFFu;

    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = 1;
    textureDesc.Height = 1;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage = D3D11_USAGE_IMMUTABLE;
    textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA initialData = {};
    initialData.pSysMem = &whitePixel;
    initialData.SysMemPitch = sizeof(whitePixel);

    ID3D11Texture2D* texture = nullptr;
    HRESULT result = m_device->CreateTexture2D(&textureDesc, &initialData, &texture);
    if (FAILED(result))
    {
        return false;
    }

    result = m_device->CreateShaderResourceView(texture, nullptr, &m_fallbackTextureView);
    SafeRelease(texture);
    return SUCCEEDED(result);
}

bool Dx11RenderBackend::CreateTextureResource(
    const GpuTextureUploadPayload& texturePayload,
    TextureResource& outTexture)
{
    if (!m_device)
    {
        return false;
    }

    if (texturePayload.format != GpuTextureFormat::Rgba8Unorm ||
        texturePayload.pixels == nullptr ||
        texturePayload.pixelBytes == 0U ||
        texturePayload.width == 0U ||
        texturePayload.height == 0U ||
        texturePayload.rowPitch == 0U)
    {
        LogError("Dx11RenderBackend received invalid texture payload.");
        return false;
    }

    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = texturePayload.width;
    textureDesc.Height = texturePayload.height;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage = D3D11_USAGE_IMMUTABLE;
    textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA initialData = {};
    initialData.pSysMem = texturePayload.pixels;
    initialData.SysMemPitch = texturePayload.rowPitch;

    ID3D11Texture2D* texture = nullptr;
    HRESULT result = m_device->CreateTexture2D(&textureDesc, &initialData, &texture);
    if (FAILED(result))
    {
        std::ostringstream message;
        message << "Dx11RenderBackend failed to create texture from upload payload"
                << ". HRESULT=0x" << std::hex << static_cast<unsigned long>(result);
        LogError(message.str());
        return false;
    }

    result = m_device->CreateShaderResourceView(texture, nullptr, &outTexture.shaderResourceView);
    SafeRelease(texture);
    if (FAILED(result))
    {
        std::ostringstream message;
        message << "Dx11RenderBackend failed to create texture SRV from upload payload"
                << ". HRESULT=0x" << std::hex << static_cast<unsigned long>(result);
        LogError(message.str());
        SafeRelease(texture);
        return false;
    }

    outTexture.texture = texture;
    outTexture.width = texturePayload.width;
    outTexture.height = texturePayload.height;
    return true;
}

bool Dx11RenderBackend::CreateResource(GpuResourceHandle handle, const GpuUploadRequest& request)
{
    if (!m_initialized || handle == 0U)
    {
        return false;
    }

    DestroyResource(handle);

    switch (request.kind)
    {
    case GpuResourceKind::Texture:
    {
        if (request.texture == nullptr)
        {
            return false;
        }

        TextureResource texture;
        if (!CreateTextureResource(*request.texture, texture))
        {
            return false;
        }

        m_textureResources.emplace(handle, std::move(texture));
        return true;
    }
    default:
        return false;
    }
}

void Dx11RenderBackend::DestroyResource(GpuResourceHandle handle)
{
    const auto textureIt = m_textureResources.find(handle);
    if (textureIt != m_textureResources.end())
    {
        SafeRelease(textureIt->second.shaderResourceView);
        SafeRelease(textureIt->second.texture);
        m_textureResources.erase(textureIt);
    }
}

ID3D11ShaderResourceView* Dx11RenderBackend::ResolveAlbedoTextureView(
    const RenderMaterialData& material,
    float& outRepeatsPerMeter)
{
    outRepeatsPerMeter = 1.0f;

    if (material.albedoTextureHandle == 0U)
    {
        return m_fallbackTextureView;
    }

    const auto existing = m_textureResources.find(material.albedoTextureHandle);
    if (existing == m_textureResources.end())
    {
        return m_fallbackTextureView;
    }

    outRepeatsPerMeter = existing->second.width > 0U
        ? (1000.0f / static_cast<float>(existing->second.width))
        : 1.0f;
    return existing->second.shaderResourceView ? existing->second.shaderResourceView : m_fallbackTextureView;
}

void Dx11RenderBackend::ReleaseTextureResources()
{
    for (auto& entry : m_textureResources)
    {
        SafeRelease(entry.second.shaderResourceView);
        SafeRelease(entry.second.texture);
    }

    m_textureResources.clear();
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
            render_builtin_meshes::kTriangleVertices,
            render_builtin_meshes::kTriangleIndices,
            &meshBuffers.vertexBuffer,
            &meshBuffers.indexBuffer);
        meshBuffers.indexCount = static_cast<std::uint32_t>(render_builtin_meshes::kTriangleIndices.size());
        break;
    case RenderBuiltInMeshKind::Diamond:
        created = CreateMeshBuffers(
            m_device,
            render_builtin_meshes::kDiamondVertices,
            render_builtin_meshes::kDiamondIndices,
            &meshBuffers.vertexBuffer,
            &meshBuffers.indexBuffer);
        meshBuffers.indexCount = static_cast<std::uint32_t>(render_builtin_meshes::kDiamondIndices.size());
        break;
    case RenderBuiltInMeshKind::Cube:
        created = CreateMeshBuffers(
            m_device,
            render_builtin_meshes::kCubeVertices,
            render_builtin_meshes::kCubeIndices,
            &meshBuffers.vertexBuffer,
            &meshBuffers.indexBuffer);
        meshBuffers.indexCount = static_cast<std::uint32_t>(render_builtin_meshes::kCubeIndices.size());
        break;
    case RenderBuiltInMeshKind::Quad:
        created = CreateMeshBuffers(
            m_device,
            render_builtin_meshes::kQuadVertices,
            render_builtin_meshes::kQuadIndices,
            &meshBuffers.vertexBuffer,
            &meshBuffers.indexBuffer);
        meshBuffers.indexCount = static_cast<std::uint32_t>(render_builtin_meshes::kQuadIndices.size());
        break;
    case RenderBuiltInMeshKind::Hex:
        created = CreateMeshBuffers(
            m_device,
            render_builtin_meshes::kHexVertices,
            render_builtin_meshes::kHexIndices,
            &meshBuffers.vertexBuffer,
            &meshBuffers.indexBuffer);
        meshBuffers.indexCount = static_cast<std::uint32_t>(render_builtin_meshes::kHexIndices.size());
        break;
    case RenderBuiltInMeshKind::Octahedron:
        created = CreateMeshBuffers(
            m_device,
            render_builtin_meshes::kOctahedronVertices,
            render_builtin_meshes::kOctahedronIndices,
            &meshBuffers.vertexBuffer,
            &meshBuffers.indexBuffer);
        meshBuffers.indexCount = static_cast<std::uint32_t>(render_builtin_meshes::kOctahedronIndices.size());
        break;
    default:
        created = CreateMeshBuffers(
            m_device,
            render_builtin_meshes::kTriangleVertices,
            render_builtin_meshes::kTriangleIndices,
            &meshBuffers.vertexBuffer,
            &meshBuffers.indexBuffer);
        meshBuffers.indexCount = static_cast<std::uint32_t>(render_builtin_meshes::kTriangleIndices.size());
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

        if (!DrawObject(camera, view.lights, item.object3D))
        {
            return false;
        }
    }

    return true;
}

bool Dx11RenderBackend::DrawObject(
    const RenderCameraData& camera,
    const std::vector<RenderSceneLightData>& lights,
    const RenderObjectData& object)
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
    CopyMatrixToContiguousArray(worldMatrix, constants.world);
    CopyMatrixToContiguousArray(worldViewProjection, constants.worldViewProjection);
    CopyNormalMatrixToContiguousArray(worldMatrix, constants.normalMatrix);
    float repeatsPerMeter = 1.0f;
    ID3D11ShaderResourceView* albedoTextureView = ResolveAlbedoTextureView(object.material, repeatsPerMeter);
    constants.textureParams[0] = object.material.albedoTextureHandle == 0U ? 0.0f : 1.0f;
    constants.textureParams[1] = repeatsPerMeter;
    constants.textureParams[2] = repeatsPerMeter;
    constants.textureParams[3] = 0.0f;
    constants.objectScale[0] = std::fabs(object.transform.scale.x);
    constants.objectScale[1] = std::fabs(object.transform.scale.y);
    constants.objectScale[2] = std::fabs(object.transform.scale.z);
    constants.objectScale[3] = 0.0f;
    constants.baseColor[0] = static_cast<float>((object.material.baseColor >> 16) & 0xFFu) / 255.0f;
    constants.baseColor[1] = static_cast<float>((object.material.baseColor >> 8) & 0xFFu) / 255.0f;
    constants.baseColor[2] = static_cast<float>(object.material.baseColor & 0xFFu) / 255.0f;
    constants.baseColor[3] = static_cast<float>((object.material.baseColor >> 24) & 0xFFu) / 255.0f;
    constants.reflectionColor[0] = static_cast<float>((object.material.reflectionColor >> 16) & 0xFFu) / 255.0f;
    constants.reflectionColor[1] = static_cast<float>((object.material.reflectionColor >> 8) & 0xFFu) / 255.0f;
    constants.reflectionColor[2] = static_cast<float>(object.material.reflectionColor & 0xFFu) / 255.0f;
    constants.reflectionColor[3] = static_cast<float>((object.material.reflectionColor >> 24) & 0xFFu) / 255.0f;
    constants.materialParams[0] = object.material.ambientStrength;
    constants.materialParams[1] = object.material.diffuseStrength;
    constants.materialParams[2] = object.material.specularStrength;
    constants.materialParams[3] = object.material.specularPower;
    constants.reflectionParams[0] = object.material.reflectivity;
    constants.reflectionParams[1] = object.material.roughness;
    constants.reflectionParams[2] = 0.0f;
    constants.reflectionParams[3] = 0.0f;
    constants.cameraPosition[0] = camera.position.x;
    constants.cameraPosition[1] = camera.position.y;
    constants.cameraPosition[2] = camera.position.z;
    constants.cameraPosition[3] = 1.0f;
    constants.cameraExposure[0] = camera.exposure;
    constants.cameraExposure[1] = 0.0f;
    constants.cameraExposure[2] = 0.0f;
    constants.cameraExposure[3] = 0.0f;
    constants.ambientColor[0] = 0.16f;
    constants.ambientColor[1] = 0.18f;
    constants.ambientColor[2] = 0.22f;
    constants.ambientColor[3] = 1.0f;
    constants.lightCount = 0U;
    for (const RenderSceneLightData& light : lights)
    {
        if (!light.enabled || constants.lightCount >= MaxShaderLights)
        {
            continue;
        }

        const std::size_t lightIndex = constants.lightCount;
        constants.lightPosition[lightIndex][0] = light.position.x;
        constants.lightPosition[lightIndex][1] = light.position.y;
        constants.lightPosition[lightIndex][2] = light.position.z;
        constants.lightPosition[lightIndex][3] = light.range;
        constants.lightColor[lightIndex][0] = (static_cast<float>((light.color >> 16U) & 0xFFU) / 255.0f) * light.intensity;
        constants.lightColor[lightIndex][1] = (static_cast<float>((light.color >> 8U) & 0xFFU) / 255.0f) * light.intensity;
        constants.lightColor[lightIndex][2] = (static_cast<float>(light.color & 0xFFU) / 255.0f) * light.intensity;
        constants.lightColor[lightIndex][3] = 1.0f;
        ++constants.lightCount;
    }

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
    m_context->PSSetShaderResources(0, 1, &albedoTextureView);
    m_context->PSSetSamplers(0, 1, &m_samplerState);
    m_context->DrawIndexed(meshBuffers.indexCount, 0, 0);
    return true;
}

bool Dx11RenderBackend::DrawScreenPatch(const RenderScreenPatchData& patch)
{
    return DrawScreenMeshPatch(patch, RenderBuiltInMeshKind::Quad);
}

bool Dx11RenderBackend::DrawScreenHexPatch(const RenderScreenPatchData& patch)
{
    return DrawScreenMeshPatch(patch, RenderBuiltInMeshKind::Hex);
}

bool Dx11RenderBackend::DrawScreenMeshPatch(
    const RenderScreenPatchData& patch,
    RenderBuiltInMeshKind meshKind)
{
    RenderMeshData patchMesh;
    patchMesh.builtInKind = meshKind;
    if (!m_context || !m_constantBuffer || !EnsureMeshResources(patchMesh))
    {
        return false;
    }

    const auto meshIt = m_meshBuffers.find(ResolveMeshCacheKey(patchMesh));
    if (meshIt == m_meshBuffers.end())
    {
        return false;
    }

    const MeshBuffers& meshBuffers = meshIt->second;
    DrawConstants constants = {};
    CopyMatrixToContiguousArray(RenderMatrix4::Identity(), constants.world);
    CopyNormalMatrixToContiguousArray(RenderMatrix4::Identity(), constants.normalMatrix);
    constants.textureParams[0] = 0.0f;
    constants.textureParams[1] = 1.0f;
    constants.textureParams[2] = 1.0f;
    constants.textureParams[3] = 0.0f;
    constants.objectScale[0] = 1.0f;
    constants.objectScale[1] = 1.0f;
    constants.objectScale[2] = 1.0f;
    constants.objectScale[3] = 0.0f;
    const float scaleX = patch.halfWidth / 0.35f;
    const float scaleY = patch.halfHeight / 0.35f;
    const float c = std::cos(patch.rotationRadians);
    const float s = std::sin(patch.rotationRadians);
    constants.worldViewProjection[0] = scaleX * c;
    constants.worldViewProjection[1] = scaleX * s;
    constants.worldViewProjection[4] = -scaleY * s;
    constants.worldViewProjection[5] = scaleY * c;
    constants.worldViewProjection[10] = 1.0f;
    constants.worldViewProjection[12] = patch.centerX;
    constants.worldViewProjection[13] = patch.centerY;
    constants.worldViewProjection[15] = 1.0f;
    constants.baseColor[0] = static_cast<float>((patch.color >> 16) & 0xFFu) / 255.0f;
    constants.baseColor[1] = static_cast<float>((patch.color >> 8) & 0xFFu) / 255.0f;
    constants.baseColor[2] = static_cast<float>(patch.color & 0xFFu) / 255.0f;
    constants.baseColor[3] = static_cast<float>((patch.color >> 24) & 0xFFu) / 255.0f;
    constants.reflectionColor[0] = constants.baseColor[0];
    constants.reflectionColor[1] = constants.baseColor[1];
    constants.reflectionColor[2] = constants.baseColor[2];
    constants.reflectionColor[3] = constants.baseColor[3];
    constants.materialParams[0] = 1.0f;
    constants.materialParams[1] = 1.0f;
    constants.materialParams[2] = 0.0f;
    constants.materialParams[3] = 1.0f;
    constants.reflectionParams[0] = 0.0f;
    constants.reflectionParams[1] = 1.0f;
    constants.reflectionParams[2] = 0.0f;
    constants.reflectionParams[3] = 0.0f;
    constants.cameraExposure[0] = 1.0f;
    constants.cameraExposure[1] = 0.0f;
    constants.cameraExposure[2] = 0.0f;
    constants.cameraExposure[3] = 0.0f;
    constants.ambientColor[0] = 1.0f;
    constants.ambientColor[1] = 1.0f;
    constants.ambientColor[2] = 1.0f;
    constants.ambientColor[3] = 1.0f;
    constants.lightCount = 0U;

    D3D11_MAPPED_SUBRESOURCE mappedResource = {};
    HRESULT result = m_context->Map(m_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (FAILED(result))
    {
        std::ostringstream message;
        message << "Dx11RenderBackend failed to map screen patch constant buffer. HRESULT=0x"
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
    ID3D11ShaderResourceView* fallbackTexture = m_fallbackTextureView;
    m_context->PSSetShaderResources(0, 1, &fallbackTexture);
    m_context->PSSetSamplers(0, 1, &m_samplerState);
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
