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
    constexpr std::size_t MaxShaderIndirectProbes = 4U;
    constexpr std::size_t MaxTracePrimitives = 16U;
    constexpr std::size_t ShadowCubemapFaceCount = 6U;

    struct alignas(16) DrawConstants
    {
        float world[16];
        float worldViewProjection[16];
        float normalMatrix[12];
        float textureParams[4];
        float objectScale[4];
        float baseColor[4];
        float reflectionColor[4];
        float directBrdfParams[4];
        float reflectionParams[4];
        float emissiveParams[4];
        float cameraPosition[4];
        float cameraExposure[4];
        float nonDirectAmbientColor[4];
        float directLightSourcePositionRange[MaxShaderLights][4];
        float directLightSourceRadiance[MaxShaderLights][4];
        float directLightVisibilityParams[MaxShaderLights][4];
        float secondaryBounceSettings[4];
        float secondaryBounceEnvironmentTint[4];
        float indirectProbePositionRadius[MaxShaderIndirectProbes][4];
        float indirectProbeRadiance[MaxShaderIndirectProbes][4];
        float tracedIndirectSettings[4];
        float tracePrimitiveCenter[MaxTracePrimitives][4];
        float tracePrimitiveExtents[MaxTracePrimitives][4];
        float tracePrimitiveAlbedo[MaxTracePrimitives][4];
        float objectShadowParams[4];
        std::uint32_t lightCount = 0U;
        std::uint32_t indirectProbeCount = 0U;
        std::uint32_t tracePrimitiveCount = 0U;
        float _padding[1] = {};
    };

    struct alignas(16) ShadowDrawConstants
    {
        float world[16];
        float worldViewProjection[16];
        float lightPositionFar[4];
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
    float4 directBrdfParams;
    float4 reflectionParams;
    float4 emissiveParams;
    float4 cameraPosition;
    float4 cameraExposure;
    float4 nonDirectAmbientColor;
    float4 directLightSourcePositionRange[4];
    float4 directLightSourceRadiance[4];
    float4 directLightVisibilityParams[4];
    float4 secondaryBounceSettings;
    float4 secondaryBounceEnvironmentTint;
    float4 indirectProbePositionRadius[4];
    float4 indirectProbeRadiance[4];
    float4 tracedIndirectSettings;
    float4 tracePrimitiveCenter[16];
    float4 tracePrimitiveExtents[16];
    float4 tracePrimitiveAlbedo[16];
    float4 objectShadowParams;
    uint lightCount;
    uint indirectProbeCount;
    uint tracePrimitiveCount;
    float _padding;
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
    float4 directBrdfParams;
    float4 reflectionParams;
    float4 emissiveParams;
    float4 cameraPosition;
    float4 cameraExposure;
    float4 nonDirectAmbientColor;
    float4 directLightSourcePositionRange[4];
    float4 directLightSourceRadiance[4];
    float4 directLightVisibilityParams[4];
    float4 secondaryBounceSettings;
    float4 secondaryBounceEnvironmentTint;
    float4 indirectProbePositionRadius[4];
    float4 indirectProbeRadiance[4];
    float4 tracedIndirectSettings;
    float4 tracePrimitiveCenter[16];
    float4 tracePrimitiveExtents[16];
    float4 tracePrimitiveAlbedo[16];
    float4 objectShadowParams;
    uint lightCount;
    uint indirectProbeCount;
    uint tracePrimitiveCount;
    float _padding;
};

Texture2D albedoTexture : register(t0);
TextureCube shadowTexture : register(t1);
SamplerState linearSampler : register(s0);
SamplerState shadowSampler : register(s1);

struct VertexOutput
{
    float4 position : SV_POSITION;
    float3 worldPosition : TEXCOORD0;
    float3 worldNormal : TEXCOORD1;
    float3 localNormal : TEXCOORD2;
    float2 uv : TEXCOORD3;
};

struct SurfaceShadingContext
{
    float3 worldPosition;
    float3 normal;
    float3 viewDirection;
    float3 surfaceAlbedo;
    float3 reflectionColor;
    float3 emissiveColor;
    float ambientStrength;
    float diffuseStrength;
    float specularStrength;
    float specularPower;
    float reflectivity;
    float roughness;
    float emissiveIntensity;
    bool receivesShadows;
};

struct DirectLightSample
{
    float3 incidentDirection;
    float pathLength;
    float3 incidentRadiance;
    float visibility;
    float ndotl;
};

struct DirectLightContribution
{
    float3 radiance;
};

struct TraceHit
{
    bool hit;
    float distance;
    float3 position;
    float3 normal;
    float3 albedo;
};

float Hash13(float3 value)
{
    value = frac(value * 0.1031f);
    value += dot(value, value.zyx + 33.33f);
    return frac((value.x + value.y) * value.z);
}

float3 BuildHemisphereSample(float3 normal, float3 seedSource)
{
    const float u1 = Hash13(seedSource);
    const float u2 = Hash13(seedSource.yzx + float3(17.17f, 29.29f, 41.41f));
    const float phi = 6.28318530718f * u1;
    const float cosTheta = sqrt(1.0f - u2);
    const float sinTheta = sqrt(u2);
    const float3 tangentSeed = abs(normal.y) < 0.999f ? float3(0.0f, 1.0f, 0.0f) : float3(1.0f, 0.0f, 0.0f);
    const float3 tangent = normalize(cross(tangentSeed, normal));
    const float3 bitangent = cross(normal, tangent);
    return normalize(
        (tangent * (cos(phi) * sinTheta)) +
        (bitangent * (sin(phi) * sinTheta)) +
        (normal * cosTheta));
}

bool IntersectTracePrimitive(
    float3 rayOrigin,
    float3 rayDirection,
    uint primitiveIndex,
    out float outDistance,
    out float3 outNormal,
    out float3 outAlbedo)
{
    const float3 center = tracePrimitiveCenter[primitiveIndex].xyz;
    const float3 extents = max(tracePrimitiveExtents[primitiveIndex].xyz, float3(0.0001f, 0.0001f, 0.0001f));
    const float3 minBounds = center - extents;
    const float3 maxBounds = center + extents;
    const float3 invDirection = 1.0f / max(abs(rayDirection), float3(0.0001f, 0.0001f, 0.0001f)) * sign(rayDirection);
    const float3 t0 = (minBounds - rayOrigin) * invDirection;
    const float3 t1 = (maxBounds - rayOrigin) * invDirection;
    const float3 tMin3 = min(t0, t1);
    const float3 tMax3 = max(t0, t1);
    const float tNear = max(max(tMin3.x, tMin3.y), tMin3.z);
    const float tFar = min(min(tMax3.x, tMax3.y), tMax3.z);

    if (tFar < max(tNear, 0.001f))
    {
        outDistance = 0.0f;
        outNormal = float3(0.0f, 0.0f, 0.0f);
        outAlbedo = float3(0.0f, 0.0f, 0.0f);
        return false;
    }

    const float hitDistance = tNear > 0.001f ? tNear : tFar;
    const float3 hitPoint = rayOrigin + (rayDirection * hitDistance);
    const float3 localHit = hitPoint - center;
    const float3 absLocalHit = abs(localHit / extents);
    float3 normal = float3(0.0f, 1.0f, 0.0f);
    if (absLocalHit.x >= absLocalHit.y && absLocalHit.x >= absLocalHit.z)
    {
        normal = float3(localHit.x >= 0.0f ? 1.0f : -1.0f, 0.0f, 0.0f);
    }
    else if (absLocalHit.y >= absLocalHit.z)
    {
        normal = float3(0.0f, localHit.y >= 0.0f ? 1.0f : -1.0f, 0.0f);
    }
    else
    {
        normal = float3(0.0f, 0.0f, localHit.z >= 0.0f ? 1.0f : -1.0f);
    }

    outDistance = hitDistance;
    outNormal = normal;
    outAlbedo = tracePrimitiveAlbedo[primitiveIndex].rgb;
    return true;
}

float3 EvaluateDirectLightingAtTraceHit(float3 hitPosition, float3 hitNormal)
{
    float3 directLighting = float3(0.0f, 0.0f, 0.0f);
    [unroll]
    for (uint lightIndex = 0; lightIndex < 4; ++lightIndex)
    {
        if (lightIndex >= lightCount)
        {
            break;
        }

        const float3 toLight = directLightSourcePositionRange[lightIndex].xyz - hitPosition;
        const float lightDistance = max(length(toLight), 0.0001f);
        const float3 incidentDirection = toLight / lightDistance;
        const float attenuation = saturate(1.0f - (lightDistance / max(directLightSourcePositionRange[lightIndex].w, 0.0001f)));
        const float ndotl = saturate(dot(hitNormal, incidentDirection));
        directLighting += directLightSourceRadiance[lightIndex].rgb * attenuation * ndotl;
    }

    return directLighting;
}

float3 EvaluateTracedIndirectLighting(SurfaceShadingContext surface)
{
    if (tracedIndirectSettings.x <= 0.5f || tracePrimitiveCount == 0u)
    {
        return float3(0.0f, 0.0f, 0.0f);
    }

    const float bounceStrength = max(tracedIndirectSettings.y, 0.0f);
    const float maxTraceDistance = max(tracedIndirectSettings.z, 0.1f);
    const float3 bounceDirection = BuildHemisphereSample(surface.normal, surface.worldPosition + surface.normal);
    const float sampleWeight = saturate(dot(surface.normal, bounceDirection));
    if (sampleWeight <= 0.0f)
    {
        return float3(0.0f, 0.0f, 0.0f);
    }

    const float3 rayOrigin = surface.worldPosition + (surface.normal * 0.08f);
    TraceHit closestHit;
    closestHit.hit = false;
    closestHit.distance = maxTraceDistance;
    closestHit.position = float3(0.0f, 0.0f, 0.0f);
    closestHit.normal = float3(0.0f, 1.0f, 0.0f);
    closestHit.albedo = float3(0.0f, 0.0f, 0.0f);

    [unroll]
    for (uint primitiveIndex = 0; primitiveIndex < 16; ++primitiveIndex)
    {
        if (primitiveIndex >= tracePrimitiveCount)
        {
            break;
        }

        float hitDistance = 0.0f;
        float3 hitNormal = float3(0.0f, 1.0f, 0.0f);
        float3 hitAlbedo = float3(0.0f, 0.0f, 0.0f);
        if (!IntersectTracePrimitive(rayOrigin, bounceDirection, primitiveIndex, hitDistance, hitNormal, hitAlbedo))
        {
            continue;
        }

        if (hitDistance >= closestHit.distance || hitDistance > maxTraceDistance)
        {
            continue;
        }

        closestHit.hit = true;
        closestHit.distance = hitDistance;
        closestHit.position = rayOrigin + (bounceDirection * hitDistance);
        closestHit.normal = hitNormal;
        closestHit.albedo = hitAlbedo;
    }

    if (!closestHit.hit)
    {
        return float3(0.0f, 0.0f, 0.0f);
    }

    const float3 directLightingAtHit = EvaluateDirectLightingAtTraceHit(
        closestHit.position + (closestHit.normal * 0.05f),
        closestHit.normal);
    const float travelWeight = saturate(1.0f - (closestHit.distance / maxTraceDistance));
    const float3 bounceFromHit = directLightingAtHit * closestHit.albedo * bounceStrength;
    return surface.surfaceAlbedo * bounceFromHit * sampleWeight * travelWeight;
}

float3 EvaluateProbeIndirectLighting(SurfaceShadingContext surface)
{
    if (indirectProbeCount == 0u)
    {
        return float3(0.0f, 0.0f, 0.0f);
    }

    float3 weightedIndirectLight = float3(0.0f, 0.0f, 0.0f);
    float totalWeight = 0.0f;

    [unroll]
    for (uint probeIndex = 0; probeIndex < 4; ++probeIndex)
    {
        if (probeIndex >= indirectProbeCount)
        {
            break;
        }

        const float3 probePosition = indirectProbePositionRadius[probeIndex].xyz;
        const float probeRadius = max(indirectProbePositionRadius[probeIndex].w, 0.0001f);
        const float distanceToProbe = length(surface.worldPosition - probePosition);
        const float weight = saturate(1.0f - (distanceToProbe / probeRadius));
        if (weight <= 0.0f)
        {
            continue;
        }

        weightedIndirectLight += indirectProbeRadiance[probeIndex].rgb * weight;
        totalWeight += weight;
    }

    if (totalWeight <= 0.0001f)
    {
        return float3(0.0f, 0.0f, 0.0f);
    }

    const float3 indirectProbeLight = weightedIndirectLight / totalWeight;
    return surface.surfaceAlbedo * indirectProbeLight;
}

float3 EvaluateDerivedBounceFill(
    SurfaceShadingContext surface,
    DirectLightSample mainLightSample,
    float3 mainLightRadiance)
{
    if (secondaryBounceSettings.x <= 0.5f)
    {
        return float3(0.0f, 0.0f, 0.0f);
    }

    const float bounceStrength = max(secondaryBounceSettings.y, 0.0f);
    const float shadowAwareBounce = secondaryBounceSettings.z;
    const float3 worldUp = float3(0.0f, 1.0f, 0.0f);
    const float openness = 0.35f + (0.65f * saturate(dot(surface.normal, worldUp)));
    const float visibility = shadowAwareBounce > 0.5f ? mainLightSample.visibility : 1.0f;
    const float shadowBias = 0.6f + (0.4f * (1.0f - visibility));
    const float3 baseBounce = mainLightRadiance * bounceStrength;
    const float3 tintedBounce = baseBounce * secondaryBounceEnvironmentTint.rgb;
    return surface.surfaceAlbedo * tintedBounce * openness * shadowBias;
}

float EvaluateShadowVisibility(float3 lightToFragment, float pathLength, float shadowFarPlane, float shadowBias)
{
    const float normalizedDistance = saturate(pathLength / max(shadowFarPlane, 0.0001f));
    const float sampledShadowDepth = shadowTexture.Sample(shadowSampler, lightToFragment).r;
    return ((normalizedDistance - max(shadowBias, 0.0f)) <= sampledShadowDepth) ? 1.0f : 0.0f;
}

DirectLightSample SamplePointLight(uint lightIndex, SurfaceShadingContext surface)
{
    DirectLightSample sample;
    const float3 toLight = directLightSourcePositionRange[lightIndex].xyz - surface.worldPosition;
    sample.pathLength = max(length(toLight), 0.0001f);
    sample.incidentDirection = toLight / sample.pathLength;
    const float maxDistance = max(directLightSourcePositionRange[lightIndex].w, 0.0001f);
    const float attenuation = saturate(1.0f - (sample.pathLength / maxDistance));
    sample.incidentRadiance = directLightSourceRadiance[lightIndex].rgb * attenuation;
    sample.ndotl = saturate(dot(surface.normal, sample.incidentDirection));

    const bool shadowEnabled = surface.receivesShadows && (directLightVisibilityParams[lightIndex].x > 0.5f);
    if (!shadowEnabled)
    {
        sample.visibility = 1.0f;
        return sample;
    }

    const float3 lightToFragment = -sample.incidentDirection;
    sample.visibility = EvaluateShadowVisibility(
        lightToFragment,
        sample.pathLength,
        directLightVisibilityParams[lightIndex].z,
        directLightVisibilityParams[lightIndex].y);
    return sample;
}

// Direct BRDF response remains visibility-agnostic: visibility is applied only
// during contribution assembly so material response stays decoupled from shadows.
float3 EvaluateDirectBrdf(SurfaceShadingContext surface, float3 incidentDirection)
{
    const float3 halfVector = normalize(incidentDirection + surface.viewDirection);
    const float specularLobe = pow(max(dot(surface.normal, halfVector), 0.0f), max(surface.specularPower, 1.0f));
    const float NdotV = saturate(dot(surface.normal, surface.viewDirection));
    const float fresnel = surface.reflectivity + ((1.0f - surface.reflectivity) * pow(1.0f - NdotV, 5.0f));
    const float specularFresnelWeight = 0.15f + (0.85f * fresnel * (1.0f - (surface.roughness * 0.5f)));
    const float3 diffuseResponse = surface.surfaceAlbedo * surface.diffuseStrength;
    const float3 specularResponse =
        surface.reflectionColor *
        (surface.specularStrength * specularLobe * specularFresnelWeight);
    return diffuseResponse + specularResponse;
}

DirectLightContribution AssembleDirectLightContribution(
    SurfaceShadingContext surface,
    DirectLightSample sample)
{
    DirectLightContribution contribution;
    if (sample.ndotl <= 0.0f || sample.visibility <= 0.0f)
    {
        contribution.radiance = float3(0.0f, 0.0f, 0.0f);
        return contribution;
    }

    const float3 brdfResponse = EvaluateDirectBrdf(surface, sample.incidentDirection);
    contribution.radiance = brdfResponse * sample.incidentRadiance * sample.ndotl * sample.visibility;
    return contribution;
}

float4 PSMain(VertexOutput input) : SV_TARGET
{
    const float3 normal = normalize(input.worldNormal);
    const float3 viewDirection = normalize(cameraPosition.xyz - input.worldPosition);
    const float ambientStrength = directBrdfParams.x;
    const float diffuseStrength = directBrdfParams.y;
    const float specularStrength = directBrdfParams.z;
    const float specularPower = max(directBrdfParams.w, 1.0f);
    const float textureEnabled = textureParams.x;
    const float repeatsPerMeterU = max(textureParams.y, 0.0001f);
    const float repeatsPerMeterV = max(textureParams.z, 0.0001f);
    const float reflectivity = saturate(reflectionParams.x);
    const float roughness = saturate(reflectionParams.y);
    const float emissiveEnabled = emissiveParams.x;
    const float3 emissiveColor = emissiveParams.yzw;
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
    const float blendedTextureAmount = saturate(textureParams.w);
    const float3 tintedAlbedo = sampledAlbedo * baseColor.rgb;
    const float3 surfaceAlbedo = (textureEnabled > 0.5f)
        ? lerp(sampledAlbedo, tintedAlbedo, blendedTextureAmount)
        : baseColor.rgb;
    // Transitional non-direct ambient term. This remains outside direct-light accumulation.
    float3 litColor = surfaceAlbedo * nonDirectAmbientColor.rgb * ambientStrength;
    SurfaceShadingContext surface;
    surface.worldPosition = input.worldPosition;
    surface.normal = normal;
    surface.viewDirection = viewDirection;
    surface.surfaceAlbedo = surfaceAlbedo;
    surface.reflectionColor = reflectionColor.rgb;
    surface.emissiveColor = emissiveColor;
    surface.ambientStrength = ambientStrength;
    surface.diffuseStrength = diffuseStrength;
    surface.specularStrength = specularStrength;
    surface.specularPower = specularPower;
    surface.reflectivity = reflectivity;
    surface.roughness = roughness;
    surface.emissiveIntensity = emissiveEnabled > 0.5f ? max(reflectionParams.z, 0.0f) : 0.0f;
    surface.receivesShadows = objectShadowParams.x > 0.5f;

    [unroll]
    for (uint lightIndex = 0; lightIndex < 4; ++lightIndex)
    {
        if (lightIndex >= lightCount)
        {
            break;
        }

        const DirectLightSample lightSample = SamplePointLight(lightIndex, surface);
        const DirectLightContribution contribution = AssembleDirectLightContribution(surface, lightSample);
        litColor += contribution.radiance;
    }

    if (lightCount > 0)
    {
        const DirectLightSample mainLightSample = SamplePointLight(0, surface);
        litColor += EvaluateDerivedBounceFill(
            surface,
            mainLightSample,
            directLightSourceRadiance[0].rgb);
    }

    litColor += EvaluateProbeIndirectLighting(surface);
    litColor += EvaluateTracedIndirectLighting(surface);
    litColor += surface.emissiveColor * surface.emissiveIntensity;

    litColor *= max(cameraExposure.x, 0.0f);
    return float4(saturate(litColor), baseColor.a);
}
)";

    constexpr const char* kShadowVertexShaderSource = R"(
cbuffer ShadowDrawConstants : register(b0)
{
    row_major float4x4 world;
    row_major float4x4 worldViewProjection;
    float4 lightPositionFar;
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
};

VertexOutput VSMain(VertexInput input)
{
    VertexOutput output;
    const float4 worldPosition = mul(float4(input.position, 1.0f), world);
    output.position = mul(float4(input.position, 1.0f), worldViewProjection);
    output.worldPosition = worldPosition.xyz;
    return output;
}
)";

    constexpr const char* kShadowPixelShaderSource = R"(
cbuffer ShadowDrawConstants : register(b0)
{
    row_major float4x4 world;
    row_major float4x4 worldViewProjection;
    float4 lightPositionFar;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
    float3 worldPosition : TEXCOORD0;
};

float PSMain(VertexOutput input) : SV_TARGET
{
    const float farPlane = max(lightPositionFar.w, 0.0001f);
    const float depthDistance = length(input.worldPosition - lightPositionFar.xyz);
    return saturate(depthDistance / farPlane);
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
        switch (pass.kind)
        {
        case RenderPassDataKind::Shadow:
            RenderDirectLightVisibilityPass(frame, pass);
            break;
        case RenderPassDataKind::Scene:
        case RenderPassDataKind::Overlay:
        case RenderPassDataKind::Composite:
        default:
            BindDefaultTargets();
            SetViewport(m_frameContext.viewport.width, m_frameContext.viewport.height);
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
            break;
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

void Dx11RenderBackend::SetDerivedBounceFillSettings(const DerivedBounceFillSettings& settings)
{
    m_derivedBounceFillSettings = settings;
}

DerivedBounceFillSettings Dx11RenderBackend::GetDerivedBounceFillSettings() const
{
    return m_derivedBounceFillSettings;
}

void Dx11RenderBackend::SetTracedIndirectSettings(const TracedIndirectSettings& settings)
{
    m_tracedIndirectSettings = settings;
}

TracedIndirectSettings Dx11RenderBackend::GetTracedIndirectSettings() const
{
    return m_tracedIndirectSettings;
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

    ID3DBlob* shadowVertexShaderBlob = nullptr;
    compileError.clear();
    if (!CompileShader(kShadowVertexShaderSource, "VSMain", "vs_4_0", &shadowVertexShaderBlob, compileError))
    {
        LogError(std::string("Dx11RenderBackend failed to compile shadow vertex shader. ") + compileError);
        return false;
    }

    result = m_device->CreateVertexShader(
        shadowVertexShaderBlob->GetBufferPointer(),
        shadowVertexShaderBlob->GetBufferSize(),
        nullptr,
        &m_shadowVertexShader);
    SafeRelease(shadowVertexShaderBlob);
    if (FAILED(result))
    {
        std::ostringstream message;
        message << "Dx11RenderBackend failed to create shadow vertex shader. HRESULT=0x"
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

    ID3DBlob* shadowPixelShaderBlob = nullptr;
    compileError.clear();
    if (!CompileShader(kShadowPixelShaderSource, "PSMain", "ps_4_0", &shadowPixelShaderBlob, compileError))
    {
        LogError(std::string("Dx11RenderBackend failed to compile shadow pixel shader. ") + compileError);
        return false;
    }

    result = m_device->CreatePixelShader(
        shadowPixelShaderBlob->GetBufferPointer(),
        shadowPixelShaderBlob->GetBufferSize(),
        nullptr,
        &m_shadowPixelShader);
    SafeRelease(shadowPixelShaderBlob);
    if (FAILED(result))
    {
        std::ostringstream message;
        message << "Dx11RenderBackend failed to create shadow pixel shader. HRESULT=0x"
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

    D3D11_BUFFER_DESC shadowConstantBufferDesc = {};
    shadowConstantBufferDesc.ByteWidth = static_cast<UINT>(sizeof(ShadowDrawConstants));
    shadowConstantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    shadowConstantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    shadowConstantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    result = m_device->CreateBuffer(&shadowConstantBufferDesc, nullptr, &m_shadowConstantBuffer);
    if (FAILED(result))
    {
        std::ostringstream message;
        message << "Dx11RenderBackend failed to create shadow constant buffer. HRESULT=0x"
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

    D3D11_SAMPLER_DESC shadowSamplerDesc = {};
    shadowSamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    shadowSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    shadowSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    shadowSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    shadowSamplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    shadowSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    result = m_device->CreateSamplerState(&shadowSamplerDesc, &m_shadowSamplerState);
    if (FAILED(result))
    {
        std::ostringstream message;
        message << "Dx11RenderBackend failed to create shadow sampler state. HRESULT=0x"
                << std::hex << static_cast<unsigned long>(result);
        LogError(message.str());
        return false;
    }

    if (!CreateFallbackTexture())
    {
        LogError("Dx11RenderBackend failed to create fallback texture.");
        return false;
    }

    if (!CreateFallbackShadowTexture())
    {
        LogError("Dx11RenderBackend failed to create fallback shadow texture.");
        return false;
    }

    return true;
}

void Dx11RenderBackend::ReleaseRenderResources()
{
    ReleaseShadowMapResources();
    ReleaseTextureResources();
    SafeRelease(m_fallbackShadowTextureView);
    SafeRelease(m_fallbackTextureView);
    SafeRelease(m_shadowSamplerState);
    SafeRelease(m_samplerState);
    SafeRelease(m_depthStencilState);
    SafeRelease(m_rasterizerState);
    SafeRelease(m_shadowConstantBuffer);
    SafeRelease(m_constantBuffer);
    SafeRelease(m_indexBuffer);
    SafeRelease(m_vertexBuffer);
    SafeRelease(m_inputLayout);
    SafeRelease(m_shadowPixelShader);
    SafeRelease(m_shadowVertexShader);
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

bool Dx11RenderBackend::CreateFallbackShadowTexture()
{
    if (!m_device)
    {
        return false;
    }

    float clearDepth[ShadowCubemapFaceCount] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };

    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = 1;
    textureDesc.Height = 1;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = static_cast<UINT>(ShadowCubemapFaceCount);
    textureDesc.Format = DXGI_FORMAT_R32_FLOAT;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage = D3D11_USAGE_IMMUTABLE;
    textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    textureDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

    std::array<D3D11_SUBRESOURCE_DATA, ShadowCubemapFaceCount> initialData = {};
    for (std::size_t faceIndex = 0; faceIndex < ShadowCubemapFaceCount; ++faceIndex)
    {
        initialData[faceIndex].pSysMem = &clearDepth[faceIndex];
        initialData[faceIndex].SysMemPitch = sizeof(float);
    }

    ID3D11Texture2D* texture = nullptr;
    HRESULT result = m_device->CreateTexture2D(&textureDesc, initialData.data(), &texture);
    if (FAILED(result))
    {
        return false;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
    srvDesc.TextureCube.MostDetailedMip = 0;
    srvDesc.TextureCube.MipLevels = 1;

    result = m_device->CreateShaderResourceView(texture, &srvDesc, &m_fallbackShadowTextureView);
    SafeRelease(texture);
    return SUCCEEDED(result);
}

bool Dx11RenderBackend::EnsureShadowMapResources(std::uint32_t resolution)
{
    if (!m_device || resolution == 0U)
    {
        return false;
    }

    if (m_shadowMapResources.shaderResourceView && m_shadowMapResources.resolution == resolution)
    {
        return true;
    }

    ReleaseShadowMapResources();

    D3D11_TEXTURE2D_DESC shadowTextureDesc = {};
    shadowTextureDesc.Width = resolution;
    shadowTextureDesc.Height = resolution;
    shadowTextureDesc.MipLevels = 1;
    shadowTextureDesc.ArraySize = static_cast<UINT>(ShadowCubemapFaceCount);
    shadowTextureDesc.Format = DXGI_FORMAT_R32_FLOAT;
    shadowTextureDesc.SampleDesc.Count = 1;
    shadowTextureDesc.Usage = D3D11_USAGE_DEFAULT;
    shadowTextureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    shadowTextureDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

    HRESULT result = m_device->CreateTexture2D(&shadowTextureDesc, nullptr, &m_shadowMapResources.shadowTexture);
    if (FAILED(result))
    {
        return false;
    }

    D3D11_TEXTURE2D_DESC depthTextureDesc = {};
    depthTextureDesc.Width = resolution;
    depthTextureDesc.Height = resolution;
    depthTextureDesc.MipLevels = 1;
    depthTextureDesc.ArraySize = static_cast<UINT>(ShadowCubemapFaceCount);
    depthTextureDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    depthTextureDesc.SampleDesc.Count = 1;
    depthTextureDesc.Usage = D3D11_USAGE_DEFAULT;
    depthTextureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    depthTextureDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

    result = m_device->CreateTexture2D(&depthTextureDesc, nullptr, &m_shadowMapResources.depthTexture);
    if (FAILED(result))
    {
        ReleaseShadowMapResources();
        return false;
    }

    for (std::uint32_t faceIndex = 0; faceIndex < static_cast<std::uint32_t>(ShadowCubemapFaceCount); ++faceIndex)
    {
        D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = DXGI_FORMAT_R32_FLOAT;
        rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
        rtvDesc.Texture2DArray.MipSlice = 0;
        rtvDesc.Texture2DArray.FirstArraySlice = faceIndex;
        rtvDesc.Texture2DArray.ArraySize = 1;

        result = m_device->CreateRenderTargetView(
            m_shadowMapResources.shadowTexture,
            &rtvDesc,
            &m_shadowMapResources.renderTargetViews[faceIndex]);
        if (FAILED(result))
        {
            ReleaseShadowMapResources();
            return false;
        }

        D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
        dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
        dsvDesc.Texture2DArray.MipSlice = 0;
        dsvDesc.Texture2DArray.FirstArraySlice = faceIndex;
        dsvDesc.Texture2DArray.ArraySize = 1;

        result = m_device->CreateDepthStencilView(
            m_shadowMapResources.depthTexture,
            &dsvDesc,
            &m_shadowMapResources.depthViews[faceIndex]);
        if (FAILED(result))
        {
            ReleaseShadowMapResources();
            return false;
        }
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
    srvDesc.TextureCube.MostDetailedMip = 0;
    srvDesc.TextureCube.MipLevels = 1;

    result = m_device->CreateShaderResourceView(
        m_shadowMapResources.shadowTexture,
        &srvDesc,
        &m_shadowMapResources.shaderResourceView);
    if (FAILED(result))
    {
        ReleaseShadowMapResources();
        return false;
    }

    m_shadowMapResources.resolution = resolution;
    return true;
}

void Dx11RenderBackend::ReleaseShadowMapResources()
{
    for (ID3D11RenderTargetView*& renderTargetView : m_shadowMapResources.renderTargetViews)
    {
        SafeRelease(renderTargetView);
    }

    for (ID3D11DepthStencilView*& depthView : m_shadowMapResources.depthViews)
    {
        SafeRelease(depthView);
    }

    SafeRelease(m_shadowMapResources.shaderResourceView);
    SafeRelease(m_shadowMapResources.shadowTexture);
    SafeRelease(m_shadowMapResources.depthTexture);
    m_shadowMapResources.resolution = 0U;
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

ID3D11ShaderResourceView* Dx11RenderBackend::ResolveShadowTextureView() const
{
    if (m_shadowMapResources.shaderResourceView)
    {
        return m_shadowMapResources.shaderResourceView;
    }

    return m_fallbackShadowTextureView;
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

bool Dx11RenderBackend::RenderDirectLightVisibilityPass(const RenderFrameData& frame, const RenderPassData& pass)
{
    if (pass.compiledLightWorkIndex >= frame.compiledLightWork.size())
    {
        return false;
    }

    const CompiledLightWork& lightWork = frame.compiledLightWork[pass.compiledLightWorkIndex];
    if (!m_context ||
        !m_shadowVertexShader ||
        !m_shadowPixelShader ||
        !lightWork.requiresVisibilityPass ||
        lightWork.visibilityPassType != CompiledShadowPassType::RasterCubemap ||
        lightWork.visibilityPolicy.baseMethod != RenderShadowBaseMethod::RasterCubemap ||
        lightWork.visibilityPolicy.resolution == 0U)
    {
        return false;
    }

    if (!EnsureShadowMapResources(lightWork.visibilityPolicy.resolution))
    {
        LogError("Dx11RenderBackend failed to allocate omni shadow map resources.");
        return false;
    }

    for (const RenderViewData& view : pass.views)
    {
        if (view.kind != RenderViewDataKind::ShadowCubemapFace)
        {
            continue;
        }

        if (!RenderDirectLightVisibilityView(frame, pass, view))
        {
            return false;
        }
    }

    return true;
}

bool Dx11RenderBackend::RenderDirectLightVisibilityView(
    const RenderFrameData& frame,
    const RenderPassData& pass,
    const RenderViewData& view)
{
    if (!m_context ||
        pass.compiledLightWorkIndex >= frame.compiledLightWork.size() ||
        view.directLightVisibilityCubemapFaceIndex >= ShadowCubemapFaceCount)
    {
        return false;
    }

    const CompiledLightWork& lightWork = frame.compiledLightWork[pass.compiledLightWorkIndex];
    ID3D11RenderTargetView* renderTargetView =
        m_shadowMapResources.renderTargetViews[view.directLightVisibilityCubemapFaceIndex];
    ID3D11DepthStencilView* depthView =
        m_shadowMapResources.depthViews[view.directLightVisibilityCubemapFaceIndex];
    if (!renderTargetView || !depthView)
    {
        return false;
    }

    const float clearDepthValue[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    m_context->OMSetRenderTargets(1, &renderTargetView, depthView);
    m_context->ClearRenderTargetView(renderTargetView, clearDepthValue);
    m_context->ClearDepthStencilView(depthView, D3D11_CLEAR_DEPTH, 1.0f, 0);
    SetViewport(lightWork.visibilityPolicy.resolution, lightWork.visibilityPolicy.resolution);

    const float lightFarPlane =
        lightWork.visibilityPolicy.farPlane > lightWork.visibilityPolicy.nearPlane
            ? lightWork.visibilityPolicy.farPlane
            : view.camera.farPlane;

    for (const RenderItemData& item : view.items)
    {
        if (item.kind != RenderItemDataKind::Object3D)
        {
            continue;
        }

        if (!DrawShadowObject(
                view.camera,
                lightWork.visibilitySourcePosition,
                lightFarPlane,
                item.object3D))
        {
            return false;
        }
    }

    return true;
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

        if (!DrawObject(camera, view.compiledLights, view.indirectLightProbes, view.items, item.object3D))
        {
            return false;
        }
    }

    return true;
}

bool Dx11RenderBackend::DrawShadowObject(
    const RenderCameraData& camera,
    const RenderVector3& lightPosition,
    float lightFarPlane,
    const RenderObjectData& object)
{
    if (!object.shadowParticipation.casts)
    {
        return true;
    }

    if (!m_context || !m_shadowConstantBuffer || !EnsureMeshResources(object.mesh))
    {
        return false;
    }

    const auto meshIt = m_meshBuffers.find(ResolveMeshCacheKey(object.mesh));
    if (meshIt == m_meshBuffers.end())
    {
        return false;
    }

    const MeshBuffers& meshBuffers = meshIt->second;
    const RenderMatrix4 worldMatrix = object.transform.ToMatrix();
    const RenderMatrix4 viewMatrix = BuildRenderCameraViewMatrix(camera);
    const RenderMatrix4 projectionMatrix = BuildRenderCameraProjectionMatrix(camera, 1.0f);
    const RenderMatrix4 worldViewMatrix = RenderMultiply(worldMatrix, viewMatrix);
    const RenderMatrix4 worldViewProjection = RenderMultiply(worldViewMatrix, projectionMatrix);

    ShadowDrawConstants constants = {};
    CopyMatrixToContiguousArray(worldMatrix, constants.world);
    CopyMatrixToContiguousArray(worldViewProjection, constants.worldViewProjection);
    constants.lightPositionFar[0] = lightPosition.x;
    constants.lightPositionFar[1] = lightPosition.y;
    constants.lightPositionFar[2] = lightPosition.z;
    constants.lightPositionFar[3] = lightFarPlane;

    D3D11_MAPPED_SUBRESOURCE mappedResource = {};
    HRESULT result = m_context->Map(m_shadowConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (FAILED(result))
    {
        std::ostringstream message;
        message << "Dx11RenderBackend failed to map shadow constant buffer. HRESULT=0x"
                << std::hex << static_cast<unsigned long>(result);
        LogError(message.str());
        return false;
    }

    std::memcpy(mappedResource.pData, &constants, sizeof(constants));
    m_context->Unmap(m_shadowConstantBuffer, 0);

    const UINT stride = sizeof(SimpleVertex);
    const UINT offset = 0;
    m_context->IASetInputLayout(m_inputLayout);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_context->IASetVertexBuffers(0, 1, &meshBuffers.vertexBuffer, &stride, &offset);
    m_context->IASetIndexBuffer(meshBuffers.indexBuffer, DXGI_FORMAT_R16_UINT, 0);
    m_context->RSSetState(m_rasterizerState);
    m_context->OMSetDepthStencilState(m_depthStencilState, 0);
    m_context->VSSetShader(m_shadowVertexShader, nullptr, 0);
    m_context->PSSetShader(m_shadowPixelShader, nullptr, 0);
    m_context->VSSetConstantBuffers(0, 1, &m_shadowConstantBuffer);
    m_context->PSSetConstantBuffers(0, 1, &m_shadowConstantBuffer);
    m_context->DrawIndexed(meshBuffers.indexCount, 0, 0);
    return true;
}

bool Dx11RenderBackend::DrawObject(
    const RenderCameraData& camera,
    const std::vector<CompiledLightData>& compiledLights,
    const std::vector<RenderIndirectLightProbeData>& indirectLightProbes,
    const std::vector<RenderItemData>& sceneItems,
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
    constants.textureParams[3] = object.material.albedoBlend;
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
    constants.directBrdfParams[0] = object.material.ambientStrength;
    constants.directBrdfParams[1] = object.material.directBrdf.diffuseStrength;
    constants.directBrdfParams[2] = object.material.directBrdf.specularStrength;
    constants.directBrdfParams[3] = object.material.directBrdf.specularPower;
    constants.reflectionParams[0] = object.material.reflectivity;
    constants.reflectionParams[1] = object.material.roughness;
    constants.reflectionParams[2] = object.material.emissiveIntensity;
    constants.reflectionParams[3] = 0.0f;
    constants.emissiveParams[0] = object.material.emissiveEnabled ? 1.0f : 0.0f;
    constants.emissiveParams[1] = static_cast<float>((object.material.emissiveColor >> 16) & 0xFFu) / 255.0f;
    constants.emissiveParams[2] = static_cast<float>((object.material.emissiveColor >> 8) & 0xFFu) / 255.0f;
    constants.emissiveParams[3] = static_cast<float>(object.material.emissiveColor & 0xFFu) / 255.0f;
    constants.cameraPosition[0] = camera.worldPosition.x;
    constants.cameraPosition[1] = camera.worldPosition.y;
    constants.cameraPosition[2] = camera.worldPosition.z;
    constants.cameraPosition[3] = 1.0f;
    constants.cameraExposure[0] = camera.exposure;
    constants.cameraExposure[1] = 0.0f;
    constants.cameraExposure[2] = 0.0f;
    constants.cameraExposure[3] = 0.0f;
    constants.nonDirectAmbientColor[0] = 0.16f;
    constants.nonDirectAmbientColor[1] = 0.18f;
    constants.nonDirectAmbientColor[2] = 0.22f;
    constants.nonDirectAmbientColor[3] = 1.0f;
    constants.secondaryBounceSettings[0] = m_derivedBounceFillSettings.enabled ? 1.0f : 0.0f;
    constants.secondaryBounceSettings[1] = m_derivedBounceFillSettings.bounceStrength;
    constants.secondaryBounceSettings[2] = m_derivedBounceFillSettings.shadowAwareBounce ? 1.0f : 0.0f;
    constants.secondaryBounceSettings[3] = 0.0f;
    constants.secondaryBounceEnvironmentTint[0] = m_derivedBounceFillSettings.environmentTint[0];
    constants.secondaryBounceEnvironmentTint[1] = m_derivedBounceFillSettings.environmentTint[1];
    constants.secondaryBounceEnvironmentTint[2] = m_derivedBounceFillSettings.environmentTint[2];
    constants.secondaryBounceEnvironmentTint[3] = 1.0f;
    constants.tracedIndirectSettings[0] = m_tracedIndirectSettings.enabled ? 1.0f : 0.0f;
    constants.tracedIndirectSettings[1] = m_tracedIndirectSettings.bounceStrength;
    constants.tracedIndirectSettings[2] = m_tracedIndirectSettings.maxTraceDistance;
    constants.tracedIndirectSettings[3] = 0.0f;
    constants.indirectProbeCount = 0U;
    for (const RenderIndirectLightProbeData& probe : indirectLightProbes)
    {
        if (!probe.enabled || constants.indirectProbeCount >= MaxShaderIndirectProbes)
        {
            continue;
        }

        const std::size_t probeIndex = constants.indirectProbeCount;
        constants.indirectProbePositionRadius[probeIndex][0] = probe.position.x;
        constants.indirectProbePositionRadius[probeIndex][1] = probe.position.y;
        constants.indirectProbePositionRadius[probeIndex][2] = probe.position.z;
        constants.indirectProbePositionRadius[probeIndex][3] = probe.radius;
        constants.indirectProbeRadiance[probeIndex][0] =
            (static_cast<float>((probe.indirectColor >> 16U) & 0xFFU) / 255.0f) * probe.intensity;
        constants.indirectProbeRadiance[probeIndex][1] =
            (static_cast<float>((probe.indirectColor >> 8U) & 0xFFU) / 255.0f) * probe.intensity;
        constants.indirectProbeRadiance[probeIndex][2] =
            (static_cast<float>(probe.indirectColor & 0xFFU) / 255.0f) * probe.intensity;
        constants.indirectProbeRadiance[probeIndex][3] = 1.0f;
        ++constants.indirectProbeCount;
    }
    constants.tracePrimitiveCount = 0U;
    for (const RenderItemData& item : sceneItems)
    {
        if (item.kind != RenderItemDataKind::Object3D ||
            (&item.object3D == &object) ||
            constants.tracePrimitiveCount >= MaxTracePrimitives)
        {
            continue;
        }

        const RenderObjectData& traceObject = item.object3D;
        const RenderVector3 halfScale = traceObject.transform.scale * 0.5f;
        RenderVector3 extents(
            std::fabs(halfScale.x),
            std::fabs(halfScale.y),
            std::fabs(halfScale.z));
        const bool hasRotation =
            std::fabs(traceObject.transform.rotationRadians.x) > 0.0001f ||
            std::fabs(traceObject.transform.rotationRadians.y) > 0.0001f ||
            std::fabs(traceObject.transform.rotationRadians.z) > 0.0001f;
        if (hasRotation)
        {
            const float radius = std::sqrt(
                (extents.x * extents.x) +
                (extents.y * extents.y) +
                (extents.z * extents.z));
            extents = RenderVector3(radius, radius, radius);
        }

        const std::size_t primitiveIndex = constants.tracePrimitiveCount;
        constants.tracePrimitiveCenter[primitiveIndex][0] = traceObject.transform.translation.x;
        constants.tracePrimitiveCenter[primitiveIndex][1] = traceObject.transform.translation.y;
        constants.tracePrimitiveCenter[primitiveIndex][2] = traceObject.transform.translation.z;
        constants.tracePrimitiveCenter[primitiveIndex][3] = 1.0f;
        constants.tracePrimitiveExtents[primitiveIndex][0] = extents.x;
        constants.tracePrimitiveExtents[primitiveIndex][1] = extents.y;
        constants.tracePrimitiveExtents[primitiveIndex][2] = extents.z;
        constants.tracePrimitiveExtents[primitiveIndex][3] = 1.0f;
        constants.tracePrimitiveAlbedo[primitiveIndex][0] =
            static_cast<float>((traceObject.material.baseColor >> 16U) & 0xFFU) / 255.0f;
        constants.tracePrimitiveAlbedo[primitiveIndex][1] =
            static_cast<float>((traceObject.material.baseColor >> 8U) & 0xFFU) / 255.0f;
        constants.tracePrimitiveAlbedo[primitiveIndex][2] =
            static_cast<float>(traceObject.material.baseColor & 0xFFU) / 255.0f;
        constants.tracePrimitiveAlbedo[primitiveIndex][3] = 1.0f;
        ++constants.tracePrimitiveCount;
    }
    constants.objectShadowParams[0] = object.shadowParticipation.receives ? 1.0f : 0.0f;
    constants.objectShadowParams[1] = 0.0f;
    constants.objectShadowParams[2] = 0.0f;
    constants.objectShadowParams[3] = 0.0f;
    constants.lightCount = 0U;
    for (const CompiledLightData& light : compiledLights)
    {
        if (!light.enabled || !light.affectsDirectLighting || constants.lightCount >= MaxShaderLights)
        {
            continue;
        }

        const std::size_t lightIndex = constants.lightCount;
        constants.directLightSourcePositionRange[lightIndex][0] = light.source.position.x;
        constants.directLightSourcePositionRange[lightIndex][1] = light.source.position.y;
        constants.directLightSourcePositionRange[lightIndex][2] = light.source.position.z;
        constants.directLightSourcePositionRange[lightIndex][3] = light.source.range;
        constants.directLightSourceRadiance[lightIndex][0] =
            (static_cast<float>((light.source.color >> 16U) & 0xFFU) / 255.0f) * light.source.intensity;
        constants.directLightSourceRadiance[lightIndex][1] =
            (static_cast<float>((light.source.color >> 8U) & 0xFFU) / 255.0f) * light.source.intensity;
        constants.directLightSourceRadiance[lightIndex][2] =
            (static_cast<float>(light.source.color & 0xFFU) / 255.0f) * light.source.intensity;
        constants.directLightSourceRadiance[lightIndex][3] = 1.0f;
        constants.directLightVisibilityParams[lightIndex][0] =
            light.visibilityResourceSlot != kInvalidRenderShadowIndex ? 1.0f : 0.0f;
        constants.directLightVisibilityParams[lightIndex][1] = light.visibility.policy.depthBias;
        constants.directLightVisibilityParams[lightIndex][2] =
            light.visibility.policy.farPlane > light.visibility.policy.nearPlane
                ? light.visibility.policy.farPlane
                : light.source.range;
        constants.directLightVisibilityParams[lightIndex][3] =
            light.visibility.policy.refinementInfluence;
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
    ID3D11ShaderResourceView* shadowTextureView = ResolveShadowTextureView();
    ID3D11ShaderResourceView* shaderResources[2] = { albedoTextureView, shadowTextureView };
    ID3D11SamplerState* samplers[2] = { m_samplerState, m_shadowSamplerState };
    m_context->PSSetShaderResources(0, 2, shaderResources);
    m_context->PSSetSamplers(0, 2, samplers);
    m_context->DrawIndexed(meshBuffers.indexCount, 0, 0);
    ID3D11ShaderResourceView* nullResources[2] = { nullptr, nullptr };
    m_context->PSSetShaderResources(0, 2, nullResources);
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
    constants.directBrdfParams[0] = 1.0f;
    constants.directBrdfParams[1] = 1.0f;
    constants.directBrdfParams[2] = 0.0f;
    constants.directBrdfParams[3] = 1.0f;
    constants.reflectionParams[0] = 0.0f;
    constants.reflectionParams[1] = 1.0f;
    constants.reflectionParams[2] = 0.0f;
    constants.reflectionParams[3] = 0.0f;
    constants.cameraExposure[0] = 1.0f;
    constants.cameraExposure[1] = 0.0f;
    constants.cameraExposure[2] = 0.0f;
    constants.cameraExposure[3] = 0.0f;
    constants.nonDirectAmbientColor[0] = 1.0f;
    constants.nonDirectAmbientColor[1] = 1.0f;
    constants.nonDirectAmbientColor[2] = 1.0f;
    constants.nonDirectAmbientColor[3] = 1.0f;
    constants.objectShadowParams[0] = 0.0f;
    constants.objectShadowParams[1] = 0.0f;
    constants.objectShadowParams[2] = 0.0f;
    constants.objectShadowParams[3] = 0.0f;
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
    ID3D11ShaderResourceView* shaderResources[2] = { m_fallbackTextureView, ResolveShadowTextureView() };
    ID3D11SamplerState* samplers[2] = { m_samplerState, m_shadowSamplerState };
    m_context->PSSetShaderResources(0, 2, shaderResources);
    m_context->PSSetSamplers(0, 2, samplers);
    m_context->DrawIndexed(meshBuffers.indexCount, 0, 0);
    ID3D11ShaderResourceView* nullResources[2] = { nullptr, nullptr };
    m_context->PSSetShaderResources(0, 2, nullResources);
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
