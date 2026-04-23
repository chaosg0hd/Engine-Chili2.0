#pragma once

#include "math/render_math.hpp"

#include <cstdint>
#include <string>
#include <vector>

enum class RenderPassDataKind : unsigned char
{
    Unknown = 0,
    Shadow,
    Scene,
    Overlay,
    Composite
};

enum class RenderViewDataKind : unsigned char
{
    Unknown = 0,
    ShadowCubemapFace,
    Scene3D,
    Overlay2D
};

enum class RenderShadowBaseMethod : unsigned char
{
    None = 0,
    RasterCubemap
};

enum class RenderShadowRefinementMethod : unsigned char
{
    None = 0
};

enum class RenderShadowUpdatePolicy : unsigned char
{
    Continuous = 0
};

constexpr std::uint32_t kInvalidRenderShadowIndex = 0xFFFFFFFFu;

enum class RenderItemDataKind : unsigned char
{
    Unknown = 0,
    Object3D,
    Overlay2D,
    ScreenPatch,
    ScreenHexPatch
};

enum class RenderBuiltInMeshKind : unsigned char
{
    None = 0,
    Triangle,
    Diamond,
    Cube,
    Quad,
    Octahedron,
    Hex
};

enum class RenderProjectionMode : unsigned char
{
    Perspective = 0,
    Orthographic
};

struct RenderCameraData
{
    RenderVector3 worldPosition = RenderVector3(0.0f, 0.0f, -5.0f);
    RenderVector3 target = RenderVector3(0.0f, 0.0f, 0.0f);
    RenderVector3 up = RenderVector3(0.0f, 1.0f, 0.0f);
    RenderVector3 forward = RenderVector3(0.0f, 0.0f, 1.0f);
    RenderVector3 right = RenderVector3(1.0f, 0.0f, 0.0f);
    RenderProjectionMode projectionMode = RenderProjectionMode::Perspective;
    float fieldOfViewDegrees = 60.0f;
    float orthographicHeight = 10.0f;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
    float aspectRatioOverride = 0.0f;
    float exposure = 1.0f;
};

inline RenderMatrix4 BuildRenderCameraViewMatrix(const RenderCameraData& camera)
{
    return RenderCreateLookAtMatrix(camera.worldPosition, camera.target, camera.up);
}

inline RenderMatrix4 BuildRenderCameraProjectionMatrix(const RenderCameraData& camera, float aspectRatio)
{
    const float resolvedAspectRatio =
        camera.aspectRatioOverride > 0.000001f
            ? camera.aspectRatioOverride
            : aspectRatio;

    if (camera.projectionMode == RenderProjectionMode::Orthographic)
    {
        return RenderCreateOrthographicMatrix(
            camera.orthographicHeight,
            resolvedAspectRatio,
            camera.nearPlane,
            camera.farPlane);
    }

    return RenderCreatePerspectiveMatrix(
        camera.fieldOfViewDegrees,
        resolvedAspectRatio,
        camera.nearPlane,
        camera.farPlane);
}

struct RenderMeshData
{
    std::uint32_t handle = 0;
    RenderBuiltInMeshKind builtInKind = RenderBuiltInMeshKind::None;
};

struct RenderMaterialData
{
    struct RenderDirectBrdfData
    {
        float diffuseStrength = 1.0f;
        float specularStrength = 0.35f;
        float specularPower = 24.0f;
    };

    std::uint32_t handle = 0;
    std::uint32_t baseColor = 0xFFFFFFFFu;
    std::string albedoAssetId;
    std::uint32_t albedoTextureHandle = 0U;
    float albedoBlend = 0.0f;
    std::uint32_t reflectionColor = 0xFFFFFFFFu;
    float reflectivity = 0.0f;
    float roughness = 1.0f;
    bool emissiveEnabled = false;
    std::uint32_t emissiveColor = 0xFFFFFFFFu;
    float emissiveIntensity = 0.0f;
    // Transitional non-direct term. This is intentionally outside the direct-light law.
    float ambientStrength = 0.18f;
    RenderDirectBrdfData directBrdf;
};

struct RenderShadowPolicyData
{
    bool enabled = false;
    RenderShadowBaseMethod baseMethod = RenderShadowBaseMethod::None;
    RenderShadowRefinementMethod refinementMethod = RenderShadowRefinementMethod::None;
    RenderShadowUpdatePolicy updatePolicy = RenderShadowUpdatePolicy::Continuous;
    std::uint32_t resolution = 0U;
    float depthBias = 0.0f;
    float normalBias = 0.0f;
    float nearPlane = 0.1f;
    float farPlane = 0.0f;
    float refinementInfluence = 0.0f;
};

struct RenderShadowParticipationData
{
    bool casts = true;
    bool receives = true;
};

struct RenderObjectData
{
    RenderTransform transform;
    RenderMeshData mesh;
    RenderMaterialData material;
    RenderShadowParticipationData shadowParticipation;
};

struct RenderScreenPatchData
{
    float centerX = 0.0f;
    float centerY = 0.0f;
    float halfWidth = 0.1f;
    float halfHeight = 0.1f;
    float rotationRadians = 0.0f;
    std::uint32_t color = 0xFFFFFFFFu;
};

struct RenderItemData
{
    RenderItemDataKind kind = RenderItemDataKind::Unknown;
    RenderObjectData object3D;
    RenderScreenPatchData screenPatch;
};

struct RenderDirectLightSourceData
{
    // First supported direct-light source reduction: point/omni emitter terms.
    // Future directional/spot paths should lower into the same contract shape.
    RenderVector3 position = RenderVector3(0.0f, 0.0f, 0.0f);
    std::uint32_t color = 0xFFFFFFFFu;
    float intensity = 1.0f;
    float range = 10.0f;
};

struct RenderDirectLightVisibilityData
{
    // Transitional visibility payload for the current raster cubemap path.
    // Future visibility strategies should still reduce to a scalar Vis term.
    RenderShadowPolicyData policy;
    std::uint32_t activeShadowIndex = kInvalidRenderShadowIndex;
};

enum class CompiledLightType : unsigned char
{
    Point = 0,
    Directional,
    Spot
};

enum class CompiledVisibilityMode : unsigned char
{
    None = 0,
    RasterCubemap,
    RasterShadowMap,
    Traced
};

enum class CompiledShadowPassType : unsigned char
{
    None = 0,
    RasterCubemap
};

struct CompiledLightData
{
    // Render-owned executable light record used by shading/runtime evaluation.
    CompiledLightType type = CompiledLightType::Point;
    bool enabled = true;
    bool affectsDirectLighting = true;
    RenderDirectLightSourceData source;
    CompiledVisibilityMode visibilityMode = CompiledVisibilityMode::None;
    std::uint32_t visibilityResourceSlot = kInvalidRenderShadowIndex;
    RenderDirectLightVisibilityData visibility;
};

struct CompiledLightWork
{
    // Render-owned work record describing pass requirements before shading.
    std::uint32_t compiledLightIndex = kInvalidRenderShadowIndex;
    std::uint32_t sourceViewIndex = kInvalidRenderShadowIndex;
    std::uint32_t sourceLightIndex = kInvalidRenderShadowIndex;
    bool requiresVisibilityPass = false;
    CompiledShadowPassType visibilityPassType = CompiledShadowPassType::None;
    std::uint32_t visibilityViewCount = 0U;
    std::uint32_t visibilityResourceSlot = kInvalidRenderShadowIndex;
    RenderShadowPolicyData visibilityPolicy;
    RenderVector3 visibilitySourcePosition = RenderVector3(0.0f, 0.0f, 0.0f);
    bool optionalRefresh = false;
    bool refreshRequiredThisFrame = false;
};

struct RenderLightRayData
{
    RenderVector3 origin = RenderVector3(0.0f, 0.0f, 0.0f);
    RenderVector3 direction = RenderVector3(0.0f, -1.0f, 0.0f);
    std::uint32_t color = 0xFFFFFFFFu;
    float intensity = 1.0f;
    std::uint32_t rayCount = 1U;
    std::uint32_t maxBounceCount = 0U;
    std::uint32_t randomSeed = 0U;
    float spreadAngleRadians = 0.0f;
    float maxDistance = 100.0f;
    bool enabled = true;
};

struct RenderIndirectLightProbeData
{
    RenderVector3 position = RenderVector3(0.0f, 0.0f, 0.0f);
    std::uint32_t indirectColor = 0xFFFFFFFFu;
    float intensity = 0.0f;
    float radius = 1.0f;
    bool enabled = true;
};

struct RenderViewData
{
    RenderViewDataKind kind = RenderViewDataKind::Unknown;
    RenderCameraData camera;
    std::vector<CompiledLightData> compiledLights;
    std::vector<RenderIndirectLightProbeData> indirectLightProbes;
    std::vector<RenderLightRayData> lightRayEmitters;
    std::vector<RenderItemData> items;
    std::uint32_t compiledLightWorkIndex = kInvalidRenderShadowIndex;
    std::uint32_t directLightVisibilityCubemapFaceIndex = 0U;
};

struct RenderPassData
{
    RenderPassDataKind kind = RenderPassDataKind::Unknown;
    std::vector<RenderViewData> views;
    std::uint32_t compiledLightWorkIndex = kInvalidRenderShadowIndex;
};

struct RenderFrameData
{
    std::vector<RenderPassData> passes;
    std::vector<CompiledLightWork> compiledLightWork;
};
